//
//  gl-stream-file.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/02/2017.
//
//

#include "gl-stream-file.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static void gl_stream_file_free(gl_object *obj_obj);

static struct gl_stream_file_funcs gl_stream_file_funcs_global = {

};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_stream_error gl_stream_file_open(gl_stream *obj_stream)
{
	gl_stream_file *obj = (gl_stream_file *)obj_stream;
	
	gl_url *url = obj_stream->data._URL;
	
	if (!url) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	if (url->data.host || url->data.port || url->data.username || url->data.password) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	obj->data.f = fopen(url->data.path, "r");
	if (!obj->data.f) {
		switch (errno) {
			case ENOMEM:
			default:
				return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
			case EACCES:
			case EISDIR:
			case ELOOP:
			case ENAMETOOLONG:
			case ENOENT:
			case ENOTDIR:
			case EBADF:
				return obj_stream->f->return_error(obj_stream, gl_stream_error_notfound);
		}
	}
	int flags = fcntl(fileno(obj->data.f), F_GETFD);
	fcntl(fileno(obj->data.f), F_SETFD, flags | O_CLOEXEC);
	
	return gl_stream_error_ok;
}

static gl_stream_error gl_stream_file_close(gl_stream *obj_stream)
{
	gl_stream_file *obj = (gl_stream_file *)obj_stream;
	
	if (!obj->data.f) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	if (fclose(obj->data.f)) {
		obj->data.f = NULL;
		switch (errno) {
			case EBADF:
				return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
			default:
				return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
		}
	}
	obj->data.f = NULL;
	
	return gl_stream_error_ok;
}

static size_t gl_stream_file_read (gl_stream *obj_stream, void *buffer, size_t size)
{
	gl_stream_file *obj = (gl_stream_file *)obj_stream;
	
	if (!buffer) {
		// the intention is to skip bytes
		return obj_stream->f->skip(obj_stream, size);
	}
	
	size_t cursor = 0;
	size_t num_read;
	
	if (!obj->data.f) {
		obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
		return 0;
	}
	
	while (cursor < size) {
		errno = 0;
		num_read = fread(buffer + cursor, 1, size - cursor, obj->data.f);

		if (num_read < (size - cursor)) {
			switch (errno) {
				case EAGAIN:
					break;
				case 0:
					if (feof(obj->data.f)) {
						if (!num_read && !cursor) {
							obj_stream->f->return_error(obj_stream, gl_stream_error_end_of_file);
						}
						return cursor + num_read;
					}
					break;
				case EBADF:
				case EISDIR:
				case EINVAL:
					obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
					return 0;
				case ENXIO:
				case EIO:
				default:
					obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
					return 0;
			}
		}
		cursor += num_read;
	}
	
	return cursor;
}

void gl_stream_file_setup()
{
	gl_stream *parent = gl_stream_new();
	memcpy(&gl_stream_file_funcs_global.p, parent->f, sizeof(gl_stream_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_stream_file_funcs_global.p.open = &gl_stream_file_open;
	gl_stream_file_funcs_global.p.close = &gl_stream_file_close;
	gl_stream_file_funcs_global.p.read = &gl_stream_file_read;

	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_stream_file_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_stream_file_free;
}

gl_stream_file *gl_stream_file_init(gl_stream_file *obj)
{
	gl_stream_init((gl_stream *)obj);
	
	obj->f = &gl_stream_file_funcs_global;
	
	return obj;
}

gl_stream_file *gl_stream_file_new()
{
	gl_stream_file *ret = calloc(1, sizeof(gl_stream_file));
	
	return gl_stream_file_init(ret);
}

static void gl_stream_file_free(gl_object *obj_obj)
{
	gl_stream_file *obj = (gl_stream_file *)obj_obj;
	
	if (obj->data.f) {
		fclose(obj->data.f);
		obj->data.f = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}
