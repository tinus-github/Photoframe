//
//  gl-tree-cache-directory.h
//  Photoframe
//
//  Created by Martijn Vernooij on 20/03/2017.
//
//

#ifndef gl_tree_cache_directory_h
#define gl_tree_cache_directory_h

#include <stdio.h>

#include "infrastructure/gl-object.h"

typedef struct gl_tree_cache_directory_leaf gl_tree_cache_directory_leaf;

struct gl_tree_cache_directory_leaf {
	gl_tree_cache_directory_leaf *next_leaf;
	char *name;
};

typedef struct gl_tree_cache_directory gl_tree_cache_directory;

typedef struct gl_tree_cache_directory_funcs {
	gl_object_funcs p;
	void (*load) (gl_tree_cache_directory *obj, const char *url);
	void (*prepend_branch) (gl_tree_cache_directory *obj, gl_tree_cache_directory *child);
	void (*prepend_leaf) (gl_tree_cache_directory *obj, char *name);
	void (*update_count) (gl_tree_cache_directory *obj, int isBranch, int difference);
	unsigned int (*get_num_child_leafs) (gl_tree_cache_directory *obj, int isRecursive);
	unsigned int (*get_num_branches) (gl_tree_cache_directory *obj);
	char * (*get_url) (gl_tree_cache_directory *obj);
	gl_tree_cache_directory *(*new_branch) (gl_tree_cache_directory *obj);
	
	gl_tree_cache_directory *(*get_nth_branch) (gl_tree_cache_directory *obj, unsigned int offset);
	char *(*get_nth_child_url) (gl_tree_cache_directory *obj, unsigned int offset);
} gl_tree_cache_directory_funcs;

typedef struct gl_tree_cache_directory_data {
	gl_object_data p;
	
	char *name;
	gl_tree_cache_directory *firstBranch;
	gl_tree_cache_directory *nextSibling;
	gl_tree_cache_directory_leaf *firstLeaf;
	const gl_tree_cache_directory *parent;
	
	unsigned int level;
	unsigned int _numChildLeafsRecursive;
	unsigned int _numChildLeafs;
	unsigned int _numChildBranchesRecursive;
	char *_url;
} gl_tree_cache_directory_data;

struct gl_tree_cache_directory {
	gl_tree_cache_directory_funcs *f;
	gl_tree_cache_directory_data data;
};

void gl_tree_cache_directory_setup();
gl_tree_cache_directory *gl_tree_cache_directory_init(gl_tree_cache_directory *obj);
gl_tree_cache_directory *gl_tree_cache_directory_new();

#endif /* gl_tree_cache_directory_h */
