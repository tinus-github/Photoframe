//
//  gl-directory.c
//  Photoframe
//
//  Created by Martijn Vernooij on 15/03/2017.
//
//

#include "fs/gl-directory.h"
#include "fs/gl-directory-file.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

static gl_stream_error gl_directory_set_url(gl_directory *obj, const char *URLstring);
static void gl_directory_free(gl_object *obj_obj);
static gl_stream_error gl_directory_open(gl_directory *obj);
static const gl_directory_read_entry *gl_directory_read(gl_directory *obj);
static gl_stream_error gl_directory_close(gl_directory *obj);
static gl_stream_error gl_directory_return_error(gl_directory *obj, gl_stream_error error);
static gl_directory_list_entry *gl_directory_get_list(gl_directory *obj);
static void gl_directory_free_list(gl_directory *obj, gl_directory_list_entry * list);

static struct gl_directory_funcs gl_directory_funcs_global = {
	.set_url = &gl_directory_set_url,
	.open = &gl_directory_open,
	.read = &gl_directory_read,
	.close = &gl_directory_close,
	.return_error = &gl_directory_return_error,
	
	.get_list = &gl_directory_get_list,
	.free_list = &gl_directory_free_list,
};

static void (*gl_object_free_org_global) (gl_object *obj);


static gl_stream_error gl_directory_set_url(gl_directory *obj, const char *URLstring)
{
	int ret;
	
	if (obj->data._URL) {
		return gl_stream_error_invalid_operation;
	}
	obj->data._URL = gl_url_new();
	ret = obj->data._URL->f->decode(obj->data._URL, URLstring);
	
	switch (ret) {
		case 0:
			return gl_stream_error_ok;
		case EINVAL:
		case EIO:
		case ENOENT:
			return gl_stream_error_invalid_operation;
		case ENOMEM:
		default:
			return gl_stream_error_unspecified_error;
	}
}

static gl_stream_error gl_directory_open(gl_directory *obj)
{
	printf("%s\n", "gl_directory_open is an abstract function");
	abort();
}

static const gl_directory_read_entry *gl_directory_read(gl_directory *obj)
{
	printf("%s\n", "gl_directory_read is an abstract function");
	abort();
}

static gl_stream_error gl_directory_close(gl_directory *obj)
{
	printf("%s\n", "gl_directory_close is an abstract function");
	abort();
}

typedef struct gl_directory_entry_list_head gl_directory_entry_list_head;

struct gl_directory_entry_list_head {
	gl_directory_entry_type entry_type;
	char *name;
	gl_directory_entry_list_head *next;
};

static void gl_directory_free_list(gl_directory *obj, gl_directory_list_entry * list)
{
	gl_directory_list_entry *entry_index = list;
	while (entry_index->name) {
		free(entry_index->name);
		entry_index++;
	}
	
	free (list);
}

static gl_directory_list_entry *gl_directory_get_list(gl_directory *obj)
{
	gl_stream_error ret;
	
	ret = obj->f->open(obj);
	if (ret != gl_stream_error_ok) {
		obj->data.lastError = ret;
		return NULL;
	}
	
	const gl_directory_read_entry *entry;
	
	gl_directory_entry_list_head *first_entry = calloc(1, sizeof (gl_directory_entry_list_head));
	gl_directory_entry_list_head *last_entry = first_entry;
	gl_directory_entry_list_head *current_entry;
	size_t num_entries = 0;
	
	if (!first_entry) {
		goto ERROR_EXIT1;
	}
	
	while ((entry = obj->f->read(obj))) {
		current_entry = calloc(1, sizeof(gl_directory_entry_list_head));
		if (!current_entry) {
			obj->data.lastError = gl_stream_error_unspecified_error;
			
			goto ERROR_EXIT1;
		}
		
		current_entry->entry_type = entry->type;
		current_entry->name = strdup(entry->name);
		
		if (!current_entry->name) {
			free(current_entry);
			obj->data.lastError = gl_stream_error_unspecified_error;
			
			goto ERROR_EXIT1;
		}
		
		last_entry->next = current_entry;
		last_entry = current_entry;
		num_entries++;
	}
	
	if (obj->data.lastError != gl_stream_error_end_of_file) {
		obj->f->close(obj);
		goto ERROR_EXIT1;
	}
	obj->f->close(obj);
	
	if (num_entries == SIZE_MAX) { // who knows...
		obj->data.lastError = gl_stream_error_unspecified_error;
		
		goto ERROR_EXIT1;
	}
	
	gl_directory_list_entry *entries = calloc(num_entries + 1, sizeof(gl_directory_list_entry));
	gl_directory_list_entry *entry_index;
	
	if (!entries) {
		obj->data.lastError = gl_stream_error_unspecified_error;
		
		goto ERROR_EXIT1;
	}

	entry_index = entries;
	current_entry = first_entry->next;
	while (current_entry) {
		entry_index->name = current_entry->name;
		entry_index->type = current_entry->entry_type;
		current_entry->name = NULL;
		current_entry = current_entry->next;
		entry_index++;
	}
	
	current_entry = first_entry;
	while (current_entry) {
		last_entry = current_entry->next;
		free (current_entry);
		current_entry = last_entry;
	}
	
	return entries;
	
ERROR_EXIT1:
	ret = obj->data.lastError;
	current_entry = first_entry;
	while (current_entry) {
		free (current_entry->name);
		current_entry->name = NULL;
		last_entry = current_entry->next;
		free (current_entry);
		current_entry = last_entry;
	}
	
	obj->f->close(obj);
	obj->data.lastError = ret;
	return NULL;
}

static gl_stream_error gl_directory_return_error(gl_directory *obj, gl_stream_error error)
{
	return obj->data.lastError = error;
}

void gl_directory_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_directory_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_directory_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_directory_free;
}

gl_directory *gl_directory_init(gl_directory *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_directory_funcs_global;
	
	return obj;
}

gl_directory *gl_directory_new()
{
	gl_directory *ret = calloc(1, sizeof(gl_directory));
	
	return gl_directory_init(ret);
}

static void gl_directory_free(gl_object *obj_obj)
{
	gl_directory *obj = (gl_directory *)obj_obj;
	
	if (obj->data._URL) {
		((gl_object *)obj->data._URL)->f->unref((gl_object *)obj->data._URL);
		obj->data._URL = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}

gl_directory *gl_directory_new_for_url(const char *URLstring)
{
	gl_url *url = gl_url_new();
	gl_directory *ret = NULL;
	
	int decodeRet = url->f->decode(url, URLstring);
	if (decodeRet) {
		((gl_object *)url)->f->unref((gl_object *)url);
		return NULL;
	}
	
	if (!strcmp(url->data.scheme, "file")) {
		ret = (gl_directory *)gl_directory_file_new();
	}
	
	if (!ret) {
		return ret;
	}
	
	ret->data._URL = url;
	
	return ret;
}
