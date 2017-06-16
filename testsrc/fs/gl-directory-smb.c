//
//  gl-directory-smb.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/06/2017.
//
//

#define _GNU_SOURCE
// for asprintf

#include "gl-directory-smb.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

static void gl_directory_smb_free(gl_object *obj_obj);
static gl_stream_error gl_directory_smb_open(gl_directory *obj_dir);

static struct gl_directory_smb_funcs gl_directory_smb_funcs_global = {
	
};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_stream_error gl_directory_smb_open(gl_directory *obj_dir)
{
	gl_directory_smb *obj = (gl_directory_smb *)obj_dir;
	
	gl_url *url = obj_dir->data._URL;
	
	if (!url) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	if ((url->data.username || url->data.password) ||
	    (!(url->data.host && url->data.path))){
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	if (obj->data.dirFd != -1) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	errno = 0;

	const char *responsePacket;
	size_t responsePacketSize;
	smb_rpc_command_argument args[3];
	smb_rpc_decode_result run_ret;
	
	char *urlString;
 
	asprintf(&urlString, "smb://%s/%s", url->data.host, url->data.path);
	if (!urlString) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
	}
	
	run_ret = obj->data.connection->f->run_command_sync(obj->data.connection, &responsePacket, &responsePacketSize,
							    args, &smb_rpc_arguments_dopen,
							    urlString);
	
	free (urlString);
	
	switch (run_ret) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_nomatch:
		case smb_rpc_decode_result_tooshort:
		default:
			return obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
		case smb_rpc_decode_result_ok:
			break;
	}
	
	switch (args[0].value.int_value) {
		case EMFILE:
		case ENFILE:
		case ENOMEM:
		default:
			return obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
		case ENOENT:
		case EACCES:
			return obj_dir->f->return_error(obj_dir, gl_stream_error_notfound);
			
		case ENOTDIR:
			return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
		case 0:
			break;
	}
	
	obj->data.dirFd = args[1].value.int_value;
	return gl_stream_error_ok;
}

static const gl_directory_read_entry *gl_directory_smb_read(gl_directory *obj_dir)
{
	gl_directory_smb *obj = (gl_directory_smb *)obj_dir;
	
	errno = 0;
	
	const char *responsePacket;
	size_t responsePacketSize;
	smb_rpc_command_argument args[3];
	smb_rpc_decode_result run_ret;
	
	if (obj->data._current_entry.name) {
		free((char *)obj->data._current_entry.name);
		obj->data._current_entry.name = NULL;
	}
	
	run_ret = obj->data.connection->f->run_command_sync(obj->data.connection, &responsePacket, &responsePacketSize,
							    args, &smb_rpc_arguments_dread,
							    obj->data.dirFd);
	switch (run_ret) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_nomatch:
		case smb_rpc_decode_result_tooshort:
		default:
			obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
			return NULL;
		case smb_rpc_decode_result_ok:
			break;
	}
	
	switch (args[0].value.int_value) {
		case 0:
			break;
		case EBADF:
			obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
			return NULL;
		default:
			obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
			return NULL;
	}

	obj->data._current_entry.type = args[1].value.int_value;
	
	if (obj->data._current_entry.type == gl_directory_entry_type_none) {
		obj_dir->f->return_error(obj_dir, gl_stream_error_end_of_file);
		return NULL;
	}
	
	obj->data._current_entry.name = strndup(args[2].value.string_value.string, args[2].value.string_value.length);
	
	return &obj->data._current_entry;
}

static gl_stream_error gl_directory_smb_close(gl_directory *obj_dir)
{
	gl_directory_smb *obj = (gl_directory_smb *)obj_dir;
	
	if (obj->data.dirFd == -1) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	const char *responsePacket;
	size_t responsePacketSize;
	smb_rpc_command_argument args[1];
	smb_rpc_decode_result run_ret;
	
	run_ret = obj->data.connection->f->run_command_sync(obj->data.connection, &responsePacket, &responsePacketSize,
							    args, &smb_rpc_arguments_dclose,
							    obj->data.dirFd);
	switch (run_ret) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_nomatch:
		case smb_rpc_decode_result_tooshort:
		default:
			return obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
		case smb_rpc_decode_result_ok:
			break;
	}

	obj->data.dirFd = -1;
	
	return gl_stream_error_ok;
}

void gl_directory_smb_setup()
{
	gl_directory *parent = gl_directory_new();
	memcpy(&gl_directory_smb_funcs_global.p, parent->f, sizeof(gl_directory_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_directory_smb_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_directory_smb_free;
	
	gl_directory_smb_funcs_global.p.open = &gl_directory_smb_open;
	gl_directory_smb_funcs_global.p.read = &gl_directory_smb_read;
	gl_directory_smb_funcs_global.p.close = &gl_directory_smb_close;
}

gl_directory_smb *gl_directory_smb_init(gl_directory_smb *obj)
{
	gl_directory_init((gl_directory *)obj);
	
	obj->f = &gl_directory_smb_funcs_global;
	
	obj->data.connection = gl_smb_util_connection_get_global_connection();
	
	obj->data.dirFd = -1;
	
	return obj;
}

gl_directory_smb *gl_directory_smb_new()
{
	gl_directory_smb *ret = calloc(1, sizeof(gl_directory_smb));
	
	return gl_directory_smb_init(ret);
}

static void gl_directory_smb_free(gl_object *obj_obj)
{
	gl_directory_smb *obj = (gl_directory_smb *)obj_obj;
	
	if (obj->data.dirFd != -1) {
		((gl_directory *)obj)->f->close((gl_directory *)obj);
	}
	
	if (obj->data._current_entry.name) {
		obj->data._current_entry.name = NULL;
	}
	
	return gl_object_free_org_global(obj_obj);
}
