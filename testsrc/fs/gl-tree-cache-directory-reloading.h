//
//  gl-tree-cache-reloading.h
//  Photoframe
//
//  Created by Martijn Vernooij on 23/04/2017.
//
//

#ifndef gl_tree_cache_directory_reloading_h
#define gl_tree_cache_directory_reloading_h

#include <stdio.h>

#include "fs/gl-tree-cache-directory-ordered.h"
#include "infrastructure/gl-notice.h"

typedef enum {
	gl_tree_cache_directory_reloading_loadstate_unloaded = 0,
	gl_tree_cache_directory_reloading_loadstate_outdated,
	gl_tree_cache_directory_reloading_loadstate_loading,
	gl_tree_cache_directory_reloading_loadstate_loaded,
	gl_tree_cache_directory_reloading_loadstate_reloading,
} gl_tree_cache_directory_reloading_loadstate;

typedef struct gl_tree_cache_directory_reloading gl_tree_cache_directory_reloading;

typedef struct gl_tree_cache_directory_reloading_funcs {
	gl_tree_cache_directory_ordered_funcs p;
	int (*trigger_reload) (gl_tree_cache_directory_reloading *obj);
	void (*set_loadstate) (gl_tree_cache_directory_reloading *obj, gl_tree_cache_directory_reloading_loadstate state);
} gl_tree_cache_directory_reloading_funcs;

typedef struct gl_tree_cache_directory_reloading_data {
	gl_tree_cache_directory_ordered_data p;

	gl_tree_cache_directory_reloading_loadstate _loadstate;
	size_t branchesToReload;
	gl_notice *loadedNotice;
} gl_tree_cache_directory_reloading_data;

struct gl_tree_cache_directory_reloading {
	gl_tree_cache_directory_reloading_funcs *f;
	gl_tree_cache_directory_reloading_data data;
};

void gl_tree_cache_directory_reloading_setup();
gl_tree_cache_directory_reloading *gl_tree_cache_directory_reloading_init(gl_tree_cache_directory_reloading *obj);
gl_tree_cache_directory_reloading *gl_tree_cache_directory_reloading_new();

#endif /* gl_tree_cache_directory_reloading_h */
