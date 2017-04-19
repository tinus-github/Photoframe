//
//  gl-directory-file.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/03/2017.
//
//

#ifndef gl_directory_file_h
#define gl_directory_file_h

#include <stdio.h>
#include <dirent.h>

#include "gl-directory.h"

typedef struct gl_directory_file gl_directory_file;

typedef struct gl_directory_file_funcs {
	gl_directory_funcs p;
} gl_directory_file_funcs;

typedef struct gl_directory_file_data {
	gl_directory_data p;
	
	DIR *_dirp;
	gl_directory_entry _current_entry;
} gl_directory_file_data;

struct gl_directory_file {
	gl_directory_file_funcs *f;
	gl_directory_file_data data;
};

void gl_directory_file_setup();
gl_directory_file *gl_directory_file_init(gl_directory_file *obj);
gl_directory_file *gl_directory_file_new();


#endif /* gl_directory_file_h */
