//
//  gl-stream.c
//  Photoframe
//
//  Created by Martijn Vernooij on 14/02/2017.
//
// Abstract class for reading files

#include "fs/gl-stream.h"
#include "fs/gl-stream-file.h"
#include "fs/gl-stream-smb.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static gl_stream_error gl_stream_open(gl_stream *stream);
static gl_stream_error gl_stream_close(gl_stream *stream);
static gl_stream_error gl_stream_return_error(gl_stream *obj, gl_stream_error error);
static size_t gl_stream_read(gl_stream *stream, void *buffer, size_t size);
static size_t gl_stream_skip(gl_stream *obj, size_t size);
static gl_stream_error gl_stream_set_url(gl_stream *obj, const char *URLstring);
static void gl_stream_free(gl_object *obj);

static struct gl_stream_funcs gl_stream_funcs_global = {
	.open = &gl_stream_open,
	.close = &gl_stream_close,
	.read = &gl_stream_read,
	.skip = &gl_stream_skip,
	.set_url = &gl_stream_set_url,
	.return_error = &gl_stream_return_error
};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_stream_error gl_stream_open(gl_stream *obj)
{
	printf("%s\n", "gl_stream_open is an abstract function");
	abort();
}

static gl_stream_error gl_stream_close(gl_stream *obj)
{
	printf("%s\n", "gl_stream_close is an abstract function");
	abort();
}

static size_t gl_stream_read(gl_stream *obj, void *buffer, size_t size)
{
	printf("%s\n", "gl_stream_read is an abstract function");
	abort();
}

static size_t gl_stream_skip(gl_stream *obj, size_t size)
{
	unsigned char buf[1024];
	
	size_t num_read;
	size_t to_read;
	size_t num_skipped = 0;
	
	while (size) {
		if (size > 1024) {
			to_read = 1024;
		} else {
			to_read = size;
		}
		
		num_read = obj->f->read(obj, buf, to_read);
		num_skipped += num_read;
		if (!num_read) {
			return num_skipped;
		}
		size -= num_read;
	}
	
	return num_skipped;
}

static gl_stream_error gl_stream_set_url(gl_stream *obj, const char *URLstring)
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

static gl_stream_error gl_stream_return_error(gl_stream *obj, gl_stream_error error)
{
	return obj->data.lastError = error;
}

void gl_stream_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_stream_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_stream_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_stream_free;

}

gl_stream *gl_stream_init(gl_stream *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_stream_funcs_global;
	
	obj->data.lastError = gl_stream_error_ok;
	
	return obj;
}

gl_stream *gl_stream_new()
{
	gl_stream *ret = calloc(1, sizeof(gl_stream));
	
	return gl_stream_init(ret);
}

static void gl_stream_free(gl_object *obj_obj)
{
	gl_stream *obj = (gl_stream *)obj_obj;
	
	if (obj->data._URL) {
		((gl_object *)obj->data._URL)->f->unref((gl_object *)obj->data._URL);
		obj->data._URL = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}

gl_stream *gl_stream_new_for_url(const char *URLstring)
{
	gl_url *url = gl_url_new();
	gl_stream *ret = NULL;
	
	int decodeRet = url->f->decode(url, URLstring);
	if (decodeRet) {
		((gl_object *)url)->f->unref((gl_object *)url);
		return NULL;
	}
	
	if (!strcmp(url->data.scheme, "file")) {
		ret = (gl_stream *)gl_stream_file_new();
	} else if (!strcmp(url->data.scheme, "smb")) {
		ret = (gl_stream *)gl_stream_smb_new();
	}
	
	if (!ret) {
		return ret;
	}

	ret->data._URL = url;
	
	return ret;
}
