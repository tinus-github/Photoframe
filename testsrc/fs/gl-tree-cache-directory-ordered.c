//
//  gl-tree-cache-directory-ordered.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/2017.
//
//

#include "gl-tree-cache-directory-ordered.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static gl_tree_cache_directory *gl_tree_cache_directory_ordered_new_branch(gl_tree_cache_directory *obj);
void gl_tree_cache_directory_ordered_load(gl_tree_cache_directory *obj_obj, const char *url);

static struct gl_tree_cache_directory_ordered_funcs gl_tree_cache_directory_ordered_funcs_global = {
};

static	void (*gl_tree_cache_directory_load_global) (gl_tree_cache_directory *obj, const char *url);


void gl_tree_cache_directory_ordered_setup()
{
	gl_tree_cache_directory *parent = gl_tree_cache_directory_new();
	memcpy(&gl_tree_cache_directory_ordered_funcs_global.p, parent->f, sizeof(gl_tree_cache_directory_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_tree_cache_directory_load_global = gl_tree_cache_directory_ordered_funcs_global.p.load;
	
	gl_tree_cache_directory_ordered_funcs_global.p.new_branch = &gl_tree_cache_directory_ordered_new_branch;
	gl_tree_cache_directory_ordered_funcs_global.p.load = &gl_tree_cache_directory_ordered_load;
	
}

static gl_tree_cache_directory *gl_tree_cache_directory_ordered_new_branch(gl_tree_cache_directory *obj_obj)
{
	return (gl_tree_cache_directory *)gl_tree_cache_directory_ordered_new();
}

static int leaf_compare(const void *left_leaf_obj, const void *right_leaf_obj)
{
	gl_tree_cache_directory_leaf **left_leaf_p = (gl_tree_cache_directory_leaf **)left_leaf_obj;
	gl_tree_cache_directory_leaf **right_leaf_p = (gl_tree_cache_directory_leaf **)right_leaf_obj;
	gl_tree_cache_directory_leaf *left_leaf = left_leaf_p[0];
	gl_tree_cache_directory_leaf *right_leaf = right_leaf_p[0];
	
	return strcoll(left_leaf->name, right_leaf->name);
}

static int branch_compare(const void *left_branch_obj, const void *right_branch_obj)
{
	gl_tree_cache_directory **left_branch_p = (gl_tree_cache_directory **)left_branch_obj;
	gl_tree_cache_directory **right_branch_p = (gl_tree_cache_directory **)right_branch_obj;
	gl_tree_cache_directory *left_branch = left_branch_p[0];
	gl_tree_cache_directory *right_branch = right_branch_p[0];
	
	return strcoll(left_branch->data.name, right_branch->data.name);
}

void gl_tree_cache_directory_ordered_load(gl_tree_cache_directory *obj_obj, const char *url)
{
	gl_tree_cache_directory_load_global(obj_obj, url);
	size_t leaf_count = obj_obj->f->get_num_child_leafs(obj_obj, 0);
	size_t counter;
	if (leaf_count) {
		gl_tree_cache_directory_leaf **leafs = calloc(sizeof(gl_tree_cache_directory_leaf *), leaf_count);
		
		if (!leafs) {
			return;
		}
		gl_tree_cache_directory_leaf *currentLeaf = obj_obj->data.firstLeaf;
		counter = 0;
		while (currentLeaf) {
			assert (currentLeaf->name);
			leafs[counter++] = currentLeaf;
			currentLeaf = currentLeaf->next_leaf;
		}
		assert (counter == leaf_count);
		
		qsort(leafs, leaf_count, sizeof(gl_tree_cache_directory_leaf *), &leaf_compare);
		
		for (counter = 0; counter < leaf_count; counter++) {
			if (counter == 0) {
				obj_obj->data.firstLeaf = leafs[0];
			} else {
				leafs[counter - 1]->next_leaf = leafs[counter];
			}
			leafs[leaf_count - 1]->next_leaf = NULL;
		}
		
		free (leafs);
	}
	
	size_t branch_count = obj_obj->f->get_num_branches(obj_obj, 0);
	if (branch_count) {
		gl_tree_cache_directory **branches = calloc(sizeof(gl_tree_cache_directory *), branch_count);
		
		if (!branches) {
			return;
		}
		
		gl_tree_cache_directory *currentBranch = obj_obj->data.firstBranch;
		counter = 0;
		while (currentBranch) {
			branches[counter++] = currentBranch;
			currentBranch = currentBranch->data.nextSibling;
		}
		
		qsort(branches, branch_count, sizeof(gl_tree_cache_directory *), &branch_compare);
		
		for (counter = 0; counter < branch_count; counter++) {
			if (counter == 0) {
				obj_obj->data.firstBranch = branches[0];
			} else {
				branches[counter - 1]->data.nextSibling = branches[counter];
			}
			branches[branch_count - 1]->data.nextSibling = NULL;
		}
		
		free (branches);
	}
	
}

gl_tree_cache_directory_ordered *gl_tree_cache_directory_ordered_init(gl_tree_cache_directory_ordered *obj)
{
	gl_tree_cache_directory_init((gl_tree_cache_directory *)obj);
	
	obj->f = &gl_tree_cache_directory_ordered_funcs_global;
	
	return obj;
}

gl_tree_cache_directory_ordered *gl_tree_cache_directory_ordered_new()
{
	gl_tree_cache_directory_ordered *ret = calloc(1, sizeof(gl_tree_cache_directory_ordered));
	
	return gl_tree_cache_directory_ordered_init(ret);
}
