//
//  gl-tree-cache-directory.c
//  Photoframe
//
//  Created by Martijn Vernooij on 20/03/2017.
//
//

#define _GNU_SOURCE
// for asprintf

#include "gl-tree-cache-directory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "fs/gl-directory.h"
#include "fs/gl-url.h"

#define TRUE 1
#define FALSE 0

#define GL_TREE_CACHE_MAX_DEPTH 8

static void gl_tree_cache_directory_free(gl_object *obj_obj);
static void gl_tree_cache_directory_load(gl_tree_cache_directory *obj, const char *url);
static void gl_tree_cache_directory_prepend_branch(gl_tree_cache_directory *obj, gl_tree_cache_directory *branch);
static void gl_tree_cache_directory_prepend_leaf(gl_tree_cache_directory *obj, char *name);
static void gl_tree_cache_directory_update_count(gl_tree_cache_directory *obj, int isBranch, int difference);
static unsigned int gl_tree_cache_directory_get_num_child_leafs(gl_tree_cache_directory *obj, int isRecursive);
static unsigned int gl_tree_cache_directory_get_num_branches(gl_tree_cache_directory *obj, int isRecursive);
static gl_tree_cache_directory *gl_tree_cache_directory_new_branch(gl_tree_cache_directory *obj);
static char * gl_tree_cache_directory_get_url(gl_tree_cache_directory *obj);
static gl_tree_cache_directory *gl_tree_cache_directory_get_nth_branch(gl_tree_cache_directory *obj, unsigned int offset);
static char *gl_tree_cache_directory_get_nth_child_url(gl_tree_cache_directory *obj, unsigned int offset);

static gl_url *global_urlobj = NULL;

static struct gl_tree_cache_directory_funcs gl_tree_cache_directory_funcs_global = {
	.load = &gl_tree_cache_directory_load,
	.prepend_branch = &gl_tree_cache_directory_prepend_branch,
	.prepend_leaf = &gl_tree_cache_directory_prepend_leaf,
	.new_branch = &gl_tree_cache_directory_new_branch,
	.update_count = &gl_tree_cache_directory_update_count,
	.get_num_child_leafs = &gl_tree_cache_directory_get_num_child_leafs,
	.get_num_branches = &gl_tree_cache_directory_get_num_branches,
	.get_url = &gl_tree_cache_directory_get_url,
	.get_nth_branch = &gl_tree_cache_directory_get_nth_branch,
	.get_nth_child_url = &gl_tree_cache_directory_get_nth_child_url
};

static void (*gl_object_free_org_global) (gl_object *obj);

static char *combine_url_name(const char *url, const char *name)
{
	char *name_encoded;
	char *ret;
	global_urlobj->f->url_escape(global_urlobj, name, &name_encoded);
	
	if (!name_encoded) {
		return NULL;
	}
	asprintf(&ret, "%s/%s", url, name_encoded);
	free (name_encoded);
	return ret;
}

static gl_tree_cache_directory *gl_tree_cache_directory_get_nth_branch(gl_tree_cache_directory *obj, unsigned int offset)
{
	while (offset > (obj->data._numChildBranchesRecursive + 1)) {
		offset -= (obj->data._numChildBranchesRecursive + 1);
	}
	
	if (!offset) {
		return obj;
	}
	
	if (!obj->data.firstBranch) {
		return NULL;
	}
	
	gl_tree_cache_directory *current_branch = obj->data.firstBranch;
	
	while (offset > 0) {
		size_t current_branch_count_inc = current_branch->data._numChildBranchesRecursive + 1;
		if (current_branch_count_inc >= offset) {
			return current_branch->f->get_nth_branch(current_branch, offset - 1);
		}
		offset -= current_branch_count_inc;
		current_branch = current_branch->data.nextSibling;
		
		if (!current_branch) { // Shouldn't happen
			return obj;
		}
	}
	return current_branch;
}

// the children in this directory come first so if you don't want to recurse, just use an offset < num_children
static char *gl_tree_cache_directory_get_nth_child_url(gl_tree_cache_directory *obj, unsigned int offset)
{
	do {
		char *ret;
		unsigned int org_offset = offset;
		unsigned int childCount;
		
		childCount = obj->f->get_num_child_leafs(obj, FALSE);
		if (offset < childCount) {
			gl_tree_cache_directory_leaf *currentLeaf = obj->data.firstLeaf;
			while (currentLeaf && offset) {
				offset--;
				currentLeaf = currentLeaf->next_leaf;
			}
			if (!currentLeaf) {
				return NULL;
			}
			char *my_url = obj->f->get_url(obj);
			
			if (!my_url) {
				return NULL;
			}
			
			ret = combine_url_name(my_url, currentLeaf->name);
			free(my_url);
			return ret;
		}
		offset -= childCount;
		
		gl_tree_cache_directory *currentBranch = obj->data.firstBranch;
		while (currentBranch) {
			childCount = currentBranch->f->get_num_child_leafs(currentBranch, TRUE);
			if (offset < childCount) {
				return currentBranch->f->get_nth_child_url(currentBranch, offset);
			}
			offset -= childCount;
			currentBranch = currentBranch->data.nextSibling;
		}
		
		if (offset == org_offset) {
			return NULL;
		}
	} while (1);
}

static char * gl_tree_cache_directory_get_url(gl_tree_cache_directory *obj)
{
	char *my_url;
	
	if (!obj->data.parent) {
		return strdup(obj->data._url);
	}
	
	char *parent_url = obj->data.parent->f->get_url((gl_tree_cache_directory *)obj->data.parent);
	if (!parent_url) {
		return parent_url;
	}
	
	my_url = combine_url_name(parent_url, obj->data.name);
	free (parent_url);
	return my_url;
}

static gl_tree_cache_directory *gl_tree_cache_directory_new_branch(gl_tree_cache_directory *obj)
{
	return gl_tree_cache_directory_new();
}

static void gl_tree_cache_directory_prepend_branch(gl_tree_cache_directory *obj, gl_tree_cache_directory *branch)
{
	branch->data.nextSibling = obj->data.firstBranch;
	branch->data.parent = obj;
	obj->data.firstBranch = branch;
	obj->f->update_count(obj, TRUE, 1 + branch->data._numChildBranchesRecursive);
	obj->f->update_count(obj, FALSE, branch->data._numChildLeafsRecursive);
	obj->data._numChildBranches++;
}

static void gl_tree_cache_directory_prepend_leaf(gl_tree_cache_directory *obj, char *name)
{
	gl_tree_cache_directory_leaf *leaf = calloc(1, sizeof(gl_tree_cache_directory_leaf));
	
	leaf->name = name;
	leaf->next_leaf = obj->data.firstLeaf;
	obj->data.firstLeaf = leaf;
	obj->f->update_count(obj, FALSE, 1);
	obj->data._numChildLeafs++;
}

static void gl_tree_cache_directory_update_count(gl_tree_cache_directory *obj, int isBranch, int difference)
{
	if (difference < 0) {
		if (isBranch) {
			assert ((-difference) <= obj->data._numChildBranchesRecursive);
		} else {
			assert ((-difference) <= obj->data._numChildLeafsRecursive);
		}
	}
	
	if (isBranch) {
		obj->data._numChildBranchesRecursive += difference;
	} else {
		obj->data._numChildLeafsRecursive += difference;
	}
	
	if (obj->data.parent) {
		gl_tree_cache_directory *parent = (gl_tree_cache_directory *)obj->data.parent;
		parent->f->update_count(parent, isBranch, difference);
	}
}

static void gl_tree_cache_directory_load(gl_tree_cache_directory *obj, const char *url)
{
	if (obj->data.level > GL_TREE_CACHE_MAX_DEPTH) {
		return;
	}
	gl_directory *dirObj = gl_directory_new_for_url(url);
	if (!dirObj) {
		return;
	}
	
	gl_stream_error err = dirObj->f->open(dirObj);
	if (err != gl_stream_error_ok) {
		((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
		return;
	}
	const gl_directory_read_entry *entry;
	char *nameCopy;
	
	gl_tree_cache_directory *branch = NULL;
	
	
	while ((entry = dirObj->f->read(dirObj))) {
		switch (entry->type) {
			case gl_directory_entry_type_file:
				nameCopy = strdup(entry->name);
				if (!nameCopy) {
					// out of memory...
					dirObj->f->close(dirObj);
					((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
					return;
				}
				obj->f->prepend_leaf(obj, nameCopy);
				nameCopy = NULL;
				break;
			case gl_directory_entry_type_directory:
				if ((strcmp(entry->name, ".")) && (strcmp(entry->name, ".."))) {
					nameCopy = combine_url_name(url, entry->name);
					if (!nameCopy) {
						// out of memory...
						dirObj->f->close(dirObj);
						((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
						return;
					}
					
					branch = obj->f->new_branch(obj);
					branch->data.level = obj->data.level + 1;
					branch->data.name = strdup(entry->name);
					branch->f->load(branch, nameCopy);
					free (nameCopy); nameCopy = NULL;
					
					obj->f->prepend_branch(obj, branch);
				}
				break;
			case gl_directory_entry_type_none:
				break;
		}
	}
	
	dirObj->f->close(dirObj);
	((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
	
}

static unsigned int gl_tree_cache_directory_get_num_child_leafs(gl_tree_cache_directory *obj, int isRecursive)
{
	if (isRecursive) {
		return obj->data._numChildLeafsRecursive;
	}
	return obj->data._numChildLeafs;
}

static unsigned int gl_tree_cache_directory_get_num_branches(gl_tree_cache_directory *obj, int isRecursive)
{
	if (isRecursive) {
		return obj->data._numChildBranchesRecursive;
	} else {
		return obj->data._numChildBranches;
	}
}

void gl_tree_cache_directory_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_tree_cache_directory_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_tree_cache_directory_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_tree_cache_directory_free;
	
	global_urlobj = gl_url_new();
}

gl_tree_cache_directory *gl_tree_cache_directory_init(gl_tree_cache_directory *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_tree_cache_directory_funcs_global;
	
	return obj;
}

gl_tree_cache_directory *gl_tree_cache_directory_new()
{
	gl_tree_cache_directory *ret = calloc(1, sizeof(gl_tree_cache_directory));
	
	return gl_tree_cache_directory_init(ret);
}

static void gl_tree_cache_directory_free(gl_object *obj_obj)
{
	gl_tree_cache_directory *obj = (gl_tree_cache_directory *)obj_obj;
	gl_tree_cache_directory *branch = obj->data.firstBranch;
	gl_tree_cache_directory *next_branch;
	
	while (branch) {
		next_branch = branch->data.nextSibling;
		if (branch->data.parent == obj) {
			branch->data.parent = NULL;
		}
		((gl_object *)branch)->f->unref((gl_object *)branch);
		branch = next_branch;
	}
	obj->data.firstBranch = NULL;
	
	gl_tree_cache_directory_leaf *leaf = obj->data.firstLeaf;
	gl_tree_cache_directory_leaf *next_leaf;
	
	while (leaf) {
		next_leaf = leaf->next_leaf;
		free(leaf->name);
		free(leaf);
		leaf = next_leaf;
	}
	obj->data.firstLeaf = NULL;
	
	return gl_object_free_org_global(obj_obj);
}
