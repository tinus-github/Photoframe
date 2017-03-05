//
//  gl-stream-rewindable.c
//  Photoframe
//
//  Created by Martijn Vernooij on 22/02/2017.
//
//

#include "gl-stream-rewindable.h"
#include <stdlib.h>
#include <string.h>

static void gl_stream_rewindable_free(gl_object *obj_obj);

static void gl_stream_rewindable_set_stream(gl_stream_rewindable *obj, gl_stream *stream)
{
	if (obj->data._orgStream) {
		// Shouldn't happen, but whatever
		((gl_object *)obj->data._orgStream)->f->unref((gl_object *)obj->data._orgStream);
	}
	obj->data._orgStream = stream;
}

static gl_stream_error gl_stream_rewindable_open(gl_stream *obj_obj)
{
	gl_stream_rewindable *obj = (gl_stream_rewindable *)obj_obj;

	obj->data._open_count++;
	if (obj->data._open_count > 1) {
		return obj_obj->f->return_error(obj_obj, gl_stream_error_ok);
	}
	
	gl_stream_error ret = obj->data._orgStream->f->open(obj->data._orgStream);
	obj->data._cursor = 0;
	
	return obj_obj->f->return_error(obj_obj, ret);
}

static gl_stream_error gl_stream_rewindable_close(gl_stream *obj_obj)
{
	gl_stream_rewindable *obj = (gl_stream_rewindable *)obj_obj;
	
	obj->data._open_count--;
	if (obj->data._open_count > 0) {
		return obj_obj->f->return_error(obj_obj, gl_stream_error_ok);
	}
	
	gl_stream_error ret = obj->data._orgStream->f->close(obj->data._orgStream);
	
	if (obj->data._buffer) {
		free (obj->data._buffer);
		obj->data._buffer = NULL;
	}
	
	return obj_obj->f->return_error(obj_obj, ret);
}

static size_t gl_stream_rewindable_read(gl_stream *obj_obj, void *vbuffer, size_t size)
{
	unsigned char *buffer = (unsigned char*)vbuffer;
	
	gl_stream_rewindable *obj = (gl_stream_rewindable *)obj_obj;
	gl_stream_error ret = gl_stream_error_ok;
	size_t output_cursor = 0;
	size_t bytes_copied = 0;
	
	if (obj->data._cursor < obj->data._bufferSize) {
		size_t bytes_to_copy = obj->data._bufferSize - obj->data._cursor;
		if (bytes_to_copy > size) {
			bytes_to_copy = size;
		}
		
		if (buffer) {
			memcpy(buffer, obj->data._buffer + obj->data._cursor, bytes_to_copy);
		}
		obj->data._cursor += bytes_to_copy;
		output_cursor += bytes_to_copy;
		size -= bytes_to_copy;
	}
	
	if (size > 0) {
		if (buffer) {
			bytes_copied = obj->data._orgStream->f->read(obj->data._orgStream, buffer + output_cursor, size);
		} else {
			bytes_copied = obj->data._orgStream->f->read(obj->data._orgStream, NULL, size);
		}
		obj->data._cursor += bytes_copied;
		output_cursor += bytes_copied;
		
		ret = obj->data._orgStream->data.lastError;
	}
	
	obj_obj->data.lastError = ret;
	
	return output_cursor;
}

static size_t gl_stream_rewindable_skip(gl_stream *obj_obj, size_t size)
{
	return obj_obj->f->read(obj_obj, NULL, size);
}

static gl_stream_error gl_stream_rewindable_rewind(gl_stream_rewindable *obj, const unsigned char* buffer, size_t size)
{
	if (size != obj->data._cursor) {
		return ((gl_stream *)obj)->f->return_error((gl_stream *)obj, gl_stream_error_invalid_operation);
	}
	
	if (obj->data._buffer) {
		if (obj->data._bufferSize >= size) {
			memcpy(obj->data._buffer, buffer, size);
			obj->data._cursor = 0;
			return gl_stream_error_ok;
		} else {
			free (obj->data._buffer);
			obj->data._buffer = NULL;
		}
	}
	
	obj->data._buffer = malloc(size);
	if (!obj->data._buffer) {
		return ((gl_stream *)obj)->f->return_error((gl_stream *)obj, gl_stream_error_unspecified_error);
	}
	obj->data._bufferSize = size;
	
	memcpy(obj->data._buffer, buffer, size);
	obj->data._cursor = 0;
	return gl_stream_error_ok;
}

static struct gl_stream_rewindable_funcs gl_stream_rewindable_funcs_global = {
	.set_stream = &gl_stream_rewindable_set_stream,
	.rewind = &gl_stream_rewindable_rewind
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_stream_rewindable_setup()
{
	gl_stream *parent = gl_stream_new();
	memcpy(&gl_stream_rewindable_funcs_global.p, parent->f, sizeof(gl_stream_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_stream_rewindable_funcs_global.p.open = &gl_stream_rewindable_open;
	gl_stream_rewindable_funcs_global.p.close = &gl_stream_rewindable_close;
	gl_stream_rewindable_funcs_global.p.read = &gl_stream_rewindable_read;
	gl_stream_rewindable_funcs_global.p.skip = &gl_stream_rewindable_skip;
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_stream_rewindable_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_stream_rewindable_free;
}

gl_stream_rewindable *gl_stream_rewindable_init(gl_stream_rewindable *obj)
{
	gl_stream_init((gl_stream *)obj);
	
	obj->f = &gl_stream_rewindable_funcs_global;
	
	return obj;
}

gl_stream_rewindable *gl_stream_rewindable_new()
{
	gl_stream_rewindable *ret = calloc(1, sizeof(gl_stream_rewindable));
	
	return gl_stream_rewindable_init(ret);
}

static void gl_stream_rewindable_free(gl_object *obj_obj)
{
	gl_stream_rewindable *obj = (gl_stream_rewindable *)obj_obj;
	
	if (obj->data._orgStream) {
		((gl_object *)obj->data._orgStream)->f->unref((gl_object *)obj->data._orgStream);
		obj->data._orgStream = NULL;
	}
	
	free (obj->data._buffer);
	obj->data._buffer = NULL;
	
	gl_object_free_org_global(obj_obj);
}
