//
//  gl-directory-file.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/03/2017.
//
//
#define _GNU_SOURCE
// for asprintf

#include "gl-directory-file.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

static void gl_directory_file_free(gl_object *obj_obj);
static gl_stream_error gl_directory_file_open(gl_directory *obj_dir);

static struct gl_directory_file_funcs gl_directory_file_funcs_global = {
	
};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_stream_error gl_directory_file_open(gl_directory *obj_dir)
{
	gl_directory_file *obj = (gl_directory_file *)obj_dir;
	
	gl_url *url = obj_dir->data._URL;
	
	if (!url) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	if (url->data.host || url->data.port || url->data.username || url->data.password) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}

	errno = 0;
	obj->data._dirp = opendir(url->data.path);
	if (!obj->data._dirp) {
		switch (errno) {
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
				
		}
	}
	
	return gl_stream_error_ok;
}

static const gl_directory_entry *gl_directory_file_read(gl_directory *obj_dir)
{
	gl_directory_file *obj = (gl_directory_file *)obj_dir;
	char *filename;
	struct stat statbuf;
	mode_t statmode;
	
	errno = 0;
	
	struct dirent *dirent = readdir(obj->data._dirp);
	if (!dirent) {
		switch (errno) {
			case 0:
				obj_dir->f->return_error(obj_dir, gl_stream_error_end_of_file);
				return NULL;
			case EBADF:
				obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
				return NULL;
			default:
				obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
				return NULL;
		}
	}
	obj->data._current_entry.name = dirent->d_name;
	switch (dirent->d_type) {
		case DT_DIR:
			obj->data._current_entry.type = gl_directory_entry_type_directory;
			break;
		case DT_REG:
			obj->data._current_entry.type = gl_directory_entry_type_file;
			break;
		default:
			// Some weird kind of non-file
			obj->data._current_entry.type = gl_directory_entry_type_none;
			break;
		case DT_LNK:
		case DT_UNKNOWN:
			asprintf(&filename, "%s/%s", obj_dir->data._URL->data.path, dirent->d_name);
			if (!filename) {
				obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
				return NULL;
			}
			errno = 0;
			if (stat(filename, &statbuf)) {
				free(filename);
				obj_dir->f->return_error(obj_dir, gl_stream_error_unspecified_error);
				return NULL;
			}
			free(filename);
			statmode = statbuf.st_mode & S_IFMT;
			switch (statmode) {
				case S_IFDIR:
					obj->data._current_entry.type = gl_directory_entry_type_directory;
					break;
				case S_IFREG:
					obj->data._current_entry.type = gl_directory_entry_type_file;
					break;
				default:
					// Some weird kind of non-file
					obj->data._current_entry.type = gl_directory_entry_type_none;
					break;
			}
	}
	
	return &obj->data._current_entry;
}

static gl_stream_error gl_directory_file_close(gl_directory *obj_dir)
{
	gl_directory_file *obj = (gl_directory_file *)obj_dir;

	if (!obj->data._dirp) {
		return obj_dir->f->return_error(obj_dir, gl_stream_error_invalid_operation);
	}
	
	closedir(obj->data._dirp);
	obj->data._dirp = NULL;
	
	return gl_stream_error_ok;
}

void gl_directory_file_setup()
{
	gl_directory *parent = gl_directory_new();
	memcpy(&gl_directory_file_funcs_global.p, parent->f, sizeof(gl_directory_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_directory_file_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_directory_file_free;
	
	gl_directory_file_funcs_global.p.open = &gl_directory_file_open;
	gl_directory_file_funcs_global.p.read = &gl_directory_file_read;
	gl_directory_file_funcs_global.p.close = &gl_directory_file_close;
}

gl_directory_file *gl_directory_file_init(gl_directory_file *obj)
{
	gl_directory_init((gl_directory *)obj);
	
	obj->f = &gl_directory_file_funcs_global;
	
	return obj;
}

gl_directory_file *gl_directory_file_new()
{
	gl_directory_file *ret = calloc(1, sizeof(gl_directory_file));
	
	return gl_directory_file_init(ret);
}

static void gl_directory_file_free(gl_object *obj_obj)
{
	gl_directory_file *obj = (gl_directory_file *)obj_obj;
	
	if (obj->data._dirp) {
		((gl_directory *)obj)->f->close((gl_directory *)obj);
	}
	
	if (obj->data._current_entry.name) {
		obj->data._current_entry.name = NULL;
	}
	
	return gl_object_free_org_global(obj_obj);
}
