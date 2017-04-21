//
//  gl-directory.h
//  Photoframe
//
//  Created by Martijn Vernooij on 15/03/2017.
//
//

#ifndef gl_directory_h
#define gl_directory_h

#include <stdio.h>
#include "infrastructure/gl-object.h"
#include "fs/gl-stream.h"

typedef enum {
	gl_directory_entry_type_none = 0,
	gl_directory_entry_type_file,
	gl_directory_entry_type_directory
} gl_directory_entry_type;

typedef struct gl_directory_read_entry {
	gl_directory_entry_type type;
	const char *name;
} gl_directory_read_entry;

typedef struct gl_directory_list_entry {
	gl_directory_entry_type type;
	char *name;
} gl_directory_list_entry;

typedef struct gl_directory gl_directory;

typedef struct gl_directory_funcs {
	gl_object_funcs p;
	gl_stream_error (*set_url) (gl_directory *obj, const char *URLstring);
	gl_stream_error (*open) (gl_directory *obj);
	const gl_directory_read_entry *(*read) (gl_directory *obj);
	gl_stream_error (*close) (gl_directory *obj);
	
	gl_directory_list_entry *(*get_list) (gl_directory *obj);
	void (*free_list) (gl_directory *obj, gl_directory_list_entry *list);
	
	gl_stream_error (*return_error) (gl_directory *obj, gl_stream_error error);
} gl_directory_funcs;

typedef struct gl_directory_data {
	gl_object_data p;
	
	gl_url *_URL;
	gl_stream_error lastError;
} gl_directory_data;

struct gl_directory {
	gl_directory_funcs *f;
	gl_directory_data data;
};

void gl_directory_setup();
gl_directory *gl_directory_init(gl_directory *obj);
gl_directory *gl_directory_new();
gl_directory *gl_directory_new_for_url(const char *URLstring);


#endif /* gl_directory_h */
