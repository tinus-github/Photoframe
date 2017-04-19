//
//  gl-tree-cache-directory-ordered.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/2017.
//
//

#ifndef gl_tree_cache_directory_ordered_h
#define gl_tree_cache_directory_ordered_h

#include <stdio.h>

#include "fs/gl-tree-cache-directory.h"

typedef struct gl_tree_cache_directory_ordered gl_tree_cache_directory_ordered;

typedef struct gl_tree_cache_directory_ordered_funcs {
	gl_tree_cache_directory_funcs p;
} gl_tree_cache_directory_ordered_funcs;

typedef struct gl_tree_cache_directory_ordered_data {
	gl_tree_cache_directory_data p;
	
	gl_tree_cache_directory *first_child;
} gl_tree_cache_directory_ordered_data;

struct gl_tree_cache_directory_ordered {
	gl_tree_cache_directory_ordered_funcs *f;
	gl_tree_cache_directory_ordered_data data;
};

void gl_tree_cache_directory_ordered_setup();
gl_tree_cache_directory_ordered *gl_tree_cache_directory_ordered_init(gl_tree_cache_directory_ordered *obj);
gl_tree_cache_directory_ordered *gl_tree_cache_directory_ordered_new();

#endif /* gl_tree_cache_directory_ordered_h */
