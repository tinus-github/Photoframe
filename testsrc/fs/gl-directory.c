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

static gl_stream_error gl_directory_set_url(gl_directory *obj, const char *URLstring);
static void gl_directory_free(gl_object *obj_obj);
static gl_stream_error gl_directory_open(gl_directory *obj);
static const gl_directory_read_entry *gl_directory_read(gl_directory *obj);
static gl_stream_error gl_directory_close(gl_directory *obj);
static gl_stream_error gl_directory_return_error(gl_directory *obj, gl_stream_error error);

static struct gl_directory_funcs gl_directory_funcs_global = {
	.set_url = &gl_directory_set_url,
	.open = &gl_directory_open,
	.read = &gl_directory_read,
	.close = &gl_directory_close,
	.return_error = &gl_directory_return_error
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
