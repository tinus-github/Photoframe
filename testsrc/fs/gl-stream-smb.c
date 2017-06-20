//
//  gl-stream-smb.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/06/2017.
//
//

#define _GNU_SOURCE
// for asprintf
#include "gl-stream-smb.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

static void gl_stream_smb_free(gl_object *obj_obj);

static struct gl_stream_smb_funcs gl_stream_smb_funcs_global = {
	
};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_stream_error gl_stream_smb_open(gl_stream *obj_stream)
{
	gl_stream_smb *obj = (gl_stream_smb *)obj_stream;
	
	gl_url *url = obj_stream->data._URL;
	
	assert (obj->data.fd == -1);
	
	if (!url) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	if ((!url->data.host) || url->data.port || url->data.username || url->data.password || (! url->data.path)) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	char *urlString;
	asprintf(&urlString, "smb://%s/%s", url->data.host, url->data.path);
	
	smb_rpc_command_argument args[3];
	smb_rpc_decode_result run_ret;
	
	run_ret = obj->data.connection->f->run_command_async(obj->data.connection, NULL, NULL,
							     args, &smb_rpc_arguments_fopen,
							     urlString);
	
	free (urlString);
	
	switch (run_ret) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_nomatch:
		case smb_rpc_decode_result_tooshort:
		default:
			obj->data.connection->f->run_command_async_completed(obj->data.connection);
			return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
		case smb_rpc_decode_result_ok:
			break;
	}
	int arg0 = args[0].value.int_value;
	int arg1 = args[1].value.int_value;
	obj->data.connection->f->run_command_async_completed(obj->data.connection);
	
	switch (arg0) {
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
		case 0:
			break;
	}
	
	obj->data.fd = arg1;
	
	return gl_stream_error_ok;
}

static gl_stream_error gl_stream_smb_close(gl_stream *obj_stream)
{
	gl_stream_smb *obj = (gl_stream_smb *)obj_stream;
	
	if (obj->data.fd == -1) {
		return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
	}
	
	smb_rpc_command_argument args[3];
	smb_rpc_decode_result run_ret;
	
	run_ret = obj->data.connection->f->run_command_async(obj->data.connection, NULL, NULL,
							     args, &smb_rpc_arguments_fclose,
							     obj->data.fd);
	switch (run_ret) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_nomatch:
		case smb_rpc_decode_result_tooshort:
		default:
			obj->data.connection->f->run_command_async_completed(obj->data.connection);
			return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
		case smb_rpc_decode_result_ok:
			break;
	}
	int arg1 = args[1].value.int_value;
	obj->data.connection->f->run_command_async_completed(obj->data.connection);

	switch (arg1) {
		case 0:
			break;
		case EBADF:
			return obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
		default:
			return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
	}
	
	obj->data.fd = -1;
	
	return gl_stream_error_ok;
}

static size_t gl_stream_smb_read (gl_stream *obj_stream, void *buffer, size_t size)
{
	gl_stream_smb *obj = (gl_stream_smb *)obj_stream;
	
	if (!buffer) {
		// the intention is to skip bytes
		return obj_stream->f->skip(obj_stream, size);
	}
	
	size_t cursor = 0;
	size_t num_read;
	
	if (obj->data.fd == -1) {
		obj_stream->f->return_error(obj_stream, gl_stream_error_invalid_operation);
		return 0;
	}
	
	smb_rpc_command_argument args[3];
	smb_rpc_decode_result run_ret;
	
	while (cursor < size) {
		run_ret = obj->data.connection->f->run_command_async(obj->data.connection, NULL, NULL,
								    args, &smb_rpc_arguments_fread,
								    obj->data.fd, size - cursor);

		switch (run_ret) {
			case smb_rpc_decode_result_invalid:
			case smb_rpc_decode_result_nomatch:
			case smb_rpc_decode_result_tooshort:
			default:
				obj->data.connection->f->run_command_async_completed(obj->data.connection);
				return obj_stream->f->return_error(obj_stream, gl_stream_error_unspecified_error);
			case smb_rpc_decode_result_ok:
				break;
		}
		int arg0 = args[0].value.int_value;
		
		num_read = args[1].value.string_value.length;
		if (num_read > 0) {
			memcpy(buffer + cursor, args[1].value.string_value.string, num_read);
		}
		obj->data.connection->f->run_command_async_completed(obj->data.connection);
		
		if (num_read < (size - cursor)) {
			switch (arg0) {
				case EAGAIN:
					break;
				case 0:
					if (!num_read) {
						if (!cursor) {
							obj_stream->f->return_error(obj_stream, gl_stream_error_end_of_file);
						}
						return cursor;
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

void gl_stream_smb_setup()
{
	gl_stream *parent = gl_stream_new();
	memcpy(&gl_stream_smb_funcs_global.p, parent->f, sizeof(gl_stream_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_stream_smb_funcs_global.p.open = &gl_stream_smb_open;
	gl_stream_smb_funcs_global.p.close = &gl_stream_smb_close;
	gl_stream_smb_funcs_global.p.read = &gl_stream_smb_read;
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_stream_smb_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_stream_smb_free;
}

gl_stream_smb *gl_stream_smb_init(gl_stream_smb *obj)
{
	gl_stream_init((gl_stream *)obj);
	
	obj->f = &gl_stream_smb_funcs_global;
	
	obj->data.fd = -1;
	obj->data.connection = gl_smb_util_connection_get_global_connection();
	obj->data.connection->f->set_async(obj->data.connection);
	
	return obj;
}

gl_stream_smb *gl_stream_smb_new()
{
	gl_stream_smb *ret = calloc(1, sizeof(gl_stream_smb));
	
	return gl_stream_smb_init(ret);
}

static void gl_stream_smb_free(gl_object *obj_obj)
{
	gl_stream_smb *obj = (gl_stream_smb *)obj_obj;
	
	if (obj->data.fd) {
		((gl_stream *)obj)->f->close((gl_stream *)obj);
	}
	
	gl_object_free_org_global(obj_obj);
}
