//
//  gl-url.c
//  Photoframe
//
//  Created by Martijn Vernooij on 14/02/2017.
//
//

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "fs/gl-url.h"
#include "infrastructure/gl-object.h"

static int gl_url_decode(gl_url *obj, const char *urlString);
static int gl_url_decode_scheme(gl_url *obj, const char *urlString);
static void gl_url_free(gl_object *obj);
static void gl_url_free_results(gl_url *obj);
static int url_escape(gl_url *obj, const char *input, char **output);
static int url_unescape(gl_url *obj, const char *input, char **output);

static struct gl_url_funcs gl_url_funcs_global = {
	.decode = &gl_url_decode,
	.decode_scheme = &gl_url_decode_scheme,
	.url_escape = &url_escape,
	.url_unescape = &url_unescape
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_url_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_url_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_url_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_url_free;
}

static int decode_scheme(gl_url *obj, const char *urlString, size_t *cursor)
{
	size_t part_start = *cursor;
	
	free(obj->data.scheme);
	obj->data.scheme = NULL;
	
	while (urlString[*cursor] != ':') {
		if (urlString[*cursor] == '\0') {
			return E2BIG;
		}
		(*cursor)++;
	}
	
	obj->data.scheme = calloc(1, 1 + (*cursor) - part_start);
	if (!obj->data.scheme) {
		return ENOMEM;
	}
	memcpy(obj->data.scheme, urlString+part_start, (*cursor) - part_start);

	return 0;
}

static int ignore_character(gl_url *obj, const char *urlString, size_t *cursor, char character)
{
	if (urlString[*cursor] == '\0') {
		return E2BIG;
	}
	if (urlString[*cursor] == character) {
		(*cursor)++;
		return 0;
	}
	return ENOENT;
}

static int decode_hex(gl_url *obj, char hex)
{
	switch (hex) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': case 'A': return 10;
		case 'b': case 'B': return 11;
		case 'c': case 'C': return 12;
		case 'd': case 'D': return 13;
		case 'e': case 'E': return 14;
		case 'f': case 'F': return 15;
		
		default: return -1;
	}
}

static int url_unescape(gl_url *obj, const char* input, char** output)
{
	size_t cursor = 0;
	size_t outputCursor = 0;
	size_t outputLength = 0;
	
	while (input[cursor]) {
		if (input[cursor] == '%') {
			if (!input[++cursor]) {
				return EINVAL;
			}
			if (!input[++cursor]) {
				return EINVAL;
			}
		}
		outputLength++;
		cursor++;
	}
	
	char *ret = calloc(1, outputLength + 1);
	if (!ret) {
		return ENOMEM;
	}
	
	cursor = 0;
	char currentChar;
	char hex1;
	char hex2;
	
	
	while (input[cursor]) {
		currentChar = input[cursor];
		if (currentChar == '%') {
			hex1 = input[++cursor];
			hex2 = input[++cursor];
			
			currentChar = decode_hex(obj, hex1) * 16 + decode_hex(obj, hex2);
			
			if (currentChar == 0) { // Let's not go there
				free(ret);
				return EINVAL;
			}
		}
		ret[outputCursor] = currentChar;
		outputCursor++;
		cursor++;
	}
	
	*output = ret;
	return 0;
}

static int character_is_unsafe(char character)
{
	if ((character >= '0') && (character <= '9')) {
		return 0;
	}
	if ((character >= 'a') && (character <= 'z')) {
		return 0;
	}
	if ((character >= 'A') && (character <= 'Z')) {
		return 0;
	}
	switch (character) {
		case '$':
		case '-':
		case '_':
		case '@':
		case '.':
		case '!':
		case '*':
		case '"':
		case '\'':
		case '(':
		case ')':
		case ',':
		case '/':
			return 0;
		default:
			return 1;
	}
}

static char hex_char(unsigned int value)
{
	static char *characters = "0123456789abcdef";
	
	if (value < 16) {
		return characters[value];
	}
	return '*';
}

static int url_escape(gl_url *obj, const char *input, char **output)
{
	size_t outputLength = 0;
	size_t cursor = 0;
	size_t outputCursor = 0;
	char *outputStr;
	
	while (input[cursor]) {
		if (character_is_unsafe(input[cursor])) {
			outputLength+=2;
		}
		outputLength++;
		cursor++;
	}
	
	if (!outputLength) {
		return EINVAL;
	}
	
	*output = calloc(1, outputLength);
	if (!*output) {
		return ENOMEM;
	}
	outputStr = *output;
	
	cursor = 0;
	while (input[cursor]) {
		if (character_is_unsafe(input[cursor])) {
			outputStr[outputCursor++] = '%';
			outputStr[outputCursor++] = hex_char(input[cursor] >> 4);
			outputStr[outputCursor++] = hex_char(input[cursor] & 15);
		} else {
			outputStr[outputCursor++] = input[cursor];
		}
		cursor++;
	}
	
	return 0;
}

static int decode_hostpart(gl_url *obj, const char *urlString, size_t *cursor)
{
	size_t part_start = *cursor;
	
	int colon_found = 0;
	int at_found = 0;
	int has_username = 0;
	int has_password = 0;
	
	int ret;
	
	while (urlString[*cursor] && (urlString[*cursor] != '/')) {
		if (urlString[*cursor] == ':') {
			if (!colon_found) {
				colon_found = 1;
			} else if (colon_found == 1) {
				if (at_found == 1) {
					colon_found = 2;
				} else {
					return EINVAL;
				}
			} else {
				return EINVAL;
			}
		} else if (urlString[*cursor] == '@') {
			if (!at_found) {
				at_found = 1;
				if (colon_found) {
					has_password = 1;
				}
				has_username = 1;
			} else {
				return EINVAL;
			}
		}
		(*cursor)++;
	}
	
	*cursor = part_start;
	
	if (at_found) {
		if (has_username) {
			while ((urlString[*cursor] != ':') && (urlString[*cursor] != '@')) {
				if (!urlString[*cursor]) {
					return EIO; // This can't happen
				}
				(*cursor)++;
			}
			obj->data.username = calloc(1, 1 + (*cursor) - part_start);
			if (!obj->data.username) {
				ret = ENOMEM; goto handle_err;
			}
			memcpy(obj->data.username, urlString + part_start, (*cursor) - part_start);
			
			if (has_password) {
				if (urlString[*cursor] != ':') {
					return EIO;
				}
				(*cursor)++;
				
				part_start = *cursor;
				
				while (urlString[*cursor] != '@') {
					if (!urlString[*cursor]) {
						return EIO;
					}
					(*cursor)++;
				}
				
				obj->data.password = calloc(1, 1 + (*cursor) - part_start);
				if (!obj->data.password) {
					ret = ENOMEM; goto handle_err;
				}
				memcpy(obj->data.password, urlString + part_start, (*cursor) - part_start);
			}
			
			if (urlString[*cursor] != '@') {
				ret = EIO; goto handle_err;
			}
			(*cursor)++;
		}
	}
	
	part_start = *cursor;
	
	while ((urlString[*cursor]) && (urlString[*cursor] != ':') && (urlString[*cursor] != '/')) {
		(*cursor)++;
	}
	
	if ((*cursor) != part_start) {
		obj->data.host = calloc(1, 1 + (*cursor) - part_start);
		if (!obj->data.host) {
			ret = ENOMEM; goto handle_err;
		}
		memcpy(obj->data.host, urlString + part_start, (*cursor) - part_start);
	}
	
	if (urlString[*cursor] == ':') {
		(*cursor)++;
		
		int maxNums = 5;
		int done = 0;
		while ((maxNums > 0) && !done) {
			switch (urlString[*cursor]) {
				case '\0':
				case '/':
					done = 1;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					obj->data.port = (10 * obj->data.port) + (urlString[*cursor] - '0');
					break;
				default:
					ret = EINVAL;
					goto handle_err;
					
			}
			(*cursor)++;
			maxNums--;
		}
		if ((urlString[*cursor]) && (urlString[*cursor] != '/')) {
			ret = EINVAL; goto handle_err;
		}
	}
	return 0;

handle_err:
	free(obj->data.username);
	obj->data.username = NULL;
	free(obj->data.password);
	obj->data.password = NULL;
	free (obj->data.host);
	obj->data.host = NULL;
	
	return ret;
}

static int decode_path(gl_url *obj, const char *urlString, size_t *cursor)
{
	size_t part_start = *cursor;
	int done = 0;
	
	while (!done) {
		switch (urlString[*cursor]) {
			case '\0':
			case '?':
			case '#':
				done = 1;
				break;
		}
		(*cursor)++;
	}
	(*cursor)--;
	
	if ((*cursor) > part_start) {
		char *str;
		str = calloc(1, 1 + (*cursor) - part_start);
		if (!str) {
			return ENOMEM;
		}
		memcpy(str, urlString + part_start, *cursor - part_start);
		
		int ret = url_unescape(obj, str, &obj->data.path);
		free (str);
		return ret;
	}
	
	return ENOENT;
}

static int decode_arguments(gl_url *obj, const char *urlString, size_t *cursor)
{
	size_t part_start = *cursor;
	int done = 0;
	
	while (!done) {
		switch (urlString[*cursor]) {
			case '\0':
			case '#':
				done = 1;
				break;
		}
		(*cursor)++;
	}
	
	(*cursor)--;
	
	if ((*cursor) > part_start) {
		char *str;
		str = calloc(1, 1 + (*cursor) - part_start);
		if (!str) {
			return ENOMEM;
		}
		memcpy(str, urlString + part_start, *cursor - part_start);
		
		int ret = url_unescape(obj, str, &obj->data.arguments);
		free (str);
		return ret;
	}
	
	return ENOENT;
}

static int decode_fragment(gl_url *obj, const char *urlString, size_t *cursor)
{
	size_t part_start = *cursor;
	int done = 0;
	
	while (!done) {
		switch (urlString[*cursor]) {
			case '\0':
				done = 1;
				break;
		}
		(*cursor)++;
	}
	
	if ((*cursor) != part_start) {
		char *str;
		str = calloc(1, 1 + (*cursor) - part_start);
		if (!str) {
			return ENOMEM;
		}
		memcpy(str, urlString + part_start, *cursor - part_start);
		
		int ret = url_unescape(obj, str, &obj->data.fragment);
		free (str);
		return ret;
	}
	
	return ENOENT;
}

static int gl_url_decode(gl_url *obj, const char *urlString)
{
	int ret;
	size_t cursor = 0;
	
	if ((ret = decode_scheme(obj, urlString, &cursor))) {
		goto handle_err;
	}
	if (ignore_character(obj, urlString, &cursor, ':')) {
		ret = EINVAL;
		goto handle_err;
	}
	
	ignore_character(obj, urlString, &cursor, '/');
	ignore_character(obj, urlString, &cursor, '/');
	
	if ((ret = decode_hostpart(obj, urlString, &cursor))) {
		goto handle_err;
	}
	
	ret = decode_path (obj, urlString, &cursor);
	if ((ret != 0) && (ret != ENOENT)) {
		goto handle_err;
	}
	
	if (urlString[cursor] == '?') {
		cursor++;
		if ((ret = decode_arguments(obj, urlString, &cursor))) {
			goto handle_err;
		}
	}
	if (urlString[cursor] == '#') {
		cursor++;
		ret = decode_fragment(obj, urlString, &cursor);
		if ((ret != 0) && (ret != ENOENT)) {
			goto handle_err;
		}
	}
	
	return 0;
	
handle_err:
	gl_url_free_results(obj);
	return ret;
}

static int gl_url_decode_scheme(gl_url *obj, const char *urlString)
{
	int ret;
	size_t cursor = 0;
	
	ret = decode_scheme(obj, urlString, &cursor);
	
	return ret;
}

static void gl_url_free_results(gl_url *obj)
{
	free (obj->data.scheme);
	free (obj->data.username);
	free (obj->data.password);
	free (obj->data.host);
	free (obj->data.path);
	free (obj->data.arguments);
	free (obj->data.fragment);

	obj->data.scheme = NULL;
	obj->data.username = NULL;
	obj->data.password = NULL;
	obj->data.host = NULL;
	obj->data.path = NULL;
	obj->data.arguments = NULL;
	obj->data.fragment = NULL;
}

static void gl_url_free(gl_object *obj_obj)
{
	gl_url *obj = (gl_url *)obj_obj;
	
	gl_url_free_results(obj);
	
	gl_object_free_org_global(obj_obj);
}

gl_url *gl_url_init(gl_url *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_url_funcs_global;
	
	return obj;
}

gl_url *gl_url_new()
{
	gl_url *ret = calloc(1, sizeof(gl_url));
	
	return gl_url_init(ret);
}
