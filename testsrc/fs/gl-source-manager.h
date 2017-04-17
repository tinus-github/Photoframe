//
//  gl-source-manager.h
//  Photoframe
//
//  Created by Martijn Vernooij on 18/04/2017.
//
//

#ifndef gl_source_manager_h
#define gl_source_manager_h

#include <stdio.h>

#include "infrastructure/gl-object.h"
#include "fs/gl-tree-cache-directory.h"

typedef struct gl_source_manager_entry gl_source_manager_entry;

typedef struct gl_source_manager_entry {
	char *name;
	gl_tree_cache_directory *source;
	gl_source_manager_entry *next;
} gl_source_manager_entry;

typedef struct gl_source_manager gl_source_manager;

typedef struct gl_source_manager_funcs {
	gl_object_funcs p;
	gl_tree_cache_directory *(*get_source) (gl_source_manager *obj, const char *name);
} gl_source_manager_funcs;

typedef struct gl_source_manager_data {
	gl_object_data p;
	
	gl_source_manager_entry *first_entry;
} gl_source_manager_data;

struct gl_source_manager {
	gl_source_manager_funcs *f;
	gl_source_manager_data data;
};

void gl_source_manager_setup();
gl_source_manager *gl_source_manager_init(gl_source_manager *obj);
gl_source_manager *gl_source_manager_new();
gl_source_manager *gl_source_manager_get_global_manager();

#endif /* gl_source_manager_h */
