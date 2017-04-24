//
//  gl-tree-cache-directory-reloading.c
//  Photoframe
//
//  Created by Martijn Vernooij on 23/04/2017.
//
//

#include "fs/gl-tree-cache-directory-reloading.h"
#include "fs/gl-directory.h"
#include "infrastructure/gl-workqueue-job.h"
#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-notice-subscription.h"
#include "infrastructure/gl-notice.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static gl_workqueue *loading_workqueue;

static void gl_tree_cache_directory_reloading_reload_done(void *target, gl_notice_subscription *notice_subscription, void *action_data);
static void *gl_tree_cache_directory_reloading_reload_work(void *target, void *extra_data);
static void load_branch_completed(void *target, gl_notice_subscription *subscription, void *extra_data);
static void gl_tree_cache_directory_reloading_free(gl_object *obj_obj);
static int gl_tree_cache_directory_reloading_trigger_reload(gl_tree_cache_directory_reloading *obj);
static void gl_tree_cache_directory_reloading_set_loadstate(gl_tree_cache_directory_reloading *obj, gl_tree_cache_directory_reloading_loadstate loadstate);
static gl_tree_cache_directory *gl_tree_cache_directory_reloading_new_branch(gl_tree_cache_directory *obj);
static void gl_tree_cache_directory_reloading_load(gl_tree_cache_directory *obj_d, const char *url);

static struct gl_tree_cache_directory_reloading_funcs gl_tree_cache_directory_reloading_funcs_global = {
	.trigger_reload = &gl_tree_cache_directory_reloading_trigger_reload,
	.set_loadstate = &gl_tree_cache_directory_reloading_set_loadstate,
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_tree_cache_directory_reloading_setup()
{
	gl_tree_cache_directory_ordered *parent = gl_tree_cache_directory_ordered_new();
	memcpy(&gl_tree_cache_directory_reloading_funcs_global.p, parent->f, sizeof(gl_tree_cache_directory_ordered_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_tree_cache_directory_reloading_funcs_global.p.p.new_branch = &gl_tree_cache_directory_reloading_new_branch;
	gl_tree_cache_directory_reloading_funcs_global.p.p.load = &gl_tree_cache_directory_reloading_load;
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_tree_cache_directory_reloading_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_tree_cache_directory_reloading_free;
	
	loading_workqueue = gl_workqueue_new();
	loading_workqueue->f->start(loading_workqueue);
}

// Trigger_reload:

// If the loadstate is unloaded or outdated, create a job for loading the directory
// then as that completes:

// * Remove subdirs that are gone
// * Add new subdirs that are gone and add to reloading queue
// * Add subdirs that are still there to reloading queue, mark as out of date
// Run queue
// After queue is done, set loadstate to loaded

struct reload_result_data {
	gl_directory_list_entry *dirList;
	gl_directory *dirObj;
};

static gl_tree_cache_directory *gl_tree_cache_directory_reloading_new_branch(gl_tree_cache_directory *obj)
{
	return (gl_tree_cache_directory *)gl_tree_cache_directory_reloading_new();
}

static void gl_tree_cache_directory_reloading_load(gl_tree_cache_directory *obj_d, const char *url)
{
	gl_tree_cache_directory_reloading *obj = (gl_tree_cache_directory_reloading *)obj_d;
	
	obj_d->data._url = strdup(url);
	obj->f->trigger_reload(obj);
}

// 1 if it actually is going to do something
static int gl_tree_cache_directory_reloading_trigger_reload(gl_tree_cache_directory_reloading *obj)
{
	switch (obj->data._loadstate) {
		case gl_tree_cache_directory_reloading_loadstate_unloaded:
			obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_loading);
			break;
		case gl_tree_cache_directory_reloading_loadstate_loaded:
		case gl_tree_cache_directory_reloading_loadstate_outdated:
			obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_reloading);
			break;
		default:
			return 0;
	}
	
	((gl_object *)obj)->f->ref((gl_object *)obj);
	
	gl_workqueue_job *job = gl_workqueue_job_new();
	job->data.target = obj;
	job->data.action = &gl_tree_cache_directory_reloading_reload_work;
	
	((gl_object *)obj)->f->ref((gl_object *)obj);
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action = gl_tree_cache_directory_reloading_reload_done;
	sub->data.action_data = job;
	
	job->data.doneNotice->f->subscribe(job->data.doneNotice, sub);
	loading_workqueue->f->append_job(loading_workqueue, job);
	
	return 1;
}

static void *gl_tree_cache_directory_reloading_reload_work(void *target, void *extra_data)
{
	gl_tree_cache_directory_reloading *obj = (gl_tree_cache_directory_reloading *)target;
	
	char *url = ((gl_tree_cache_directory *)obj)->f->get_url((gl_tree_cache_directory *)obj);
	
	gl_directory *dirObj = gl_directory_new_for_url(url);
	free (url);
	
	if (!dirObj) {
		return NULL;
	}
	
	gl_directory_list_entry *dirList = dirObj->f->get_list(dirObj);
	
	struct reload_result_data *result = calloc(1, sizeof(struct reload_result_data));
	if (!result) {
		dirObj->f->free_list(dirObj, dirList);
		((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
		return NULL;
	}
	
	result->dirList = dirList;
	result->dirObj = dirObj;
	
	return result;
}

typedef struct gl_directory_list_entry_head gl_directory_list_entry_head;

struct gl_directory_list_entry_head {
	gl_directory_list_entry *entry;
	gl_directory_list_entry_head *next;
};

static void compare_leafs(void *obj, gl_directory_list_entry *dirList)
{ // TODO counting
	gl_directory_list_entry *currentEntry = dirList;
	gl_tree_cache_directory_leaf *currentLeaf = ((gl_tree_cache_directory *)obj)->data.firstLeaf;
	size_t leafCount;
	
	int matched = 1;
	while (currentEntry->name) {
		if (currentEntry->type != gl_directory_entry_type_file) {
			currentEntry++;
			continue;
		}
		
		if ((!currentLeaf) || strcmp(currentEntry->name, currentLeaf->name)) {
			matched = 0;
			break;
		}
		currentEntry++;
		currentLeaf = currentLeaf->next_leaf;
	}
	
	if (currentLeaf) {
		matched = 0;
	}
	
	if (matched) {
		return;
	}
	
	// Clean out leafs
	currentLeaf = ((gl_tree_cache_directory *)obj)->data.firstLeaf;
	gl_tree_cache_directory_leaf *nextLeaf;
	
	leafCount = 0;
	while (currentLeaf) {
		nextLeaf = currentLeaf->next_leaf;
		free(currentLeaf->name);
		free(currentLeaf);
		currentLeaf = nextLeaf;
		leafCount++;
	}
	((gl_tree_cache_directory *)obj)->data.firstLeaf = NULL;
	((gl_tree_cache_directory *)obj)->f->update_count((gl_tree_cache_directory *)obj, 0, -leafCount);
	((gl_tree_cache_directory *)obj)->data._numChildLeafs -= leafCount;
	
	
	currentEntry = dirList;
	currentLeaf = NULL;
	leafCount = 0;
	while (currentEntry->name) {
		if (currentEntry->type != gl_directory_entry_type_file) {
			currentEntry++;
			continue;
		}
		
		nextLeaf = calloc(1, sizeof(gl_tree_cache_directory_leaf));
		if (!nextLeaf) {
			//TODO Handle this somehow?
			return;
		}
		nextLeaf->name = strdup(currentEntry->name);
		if (!nextLeaf->name) {
			//TODO Handle this somehow?
			free (nextLeaf);
			return;
		}
		
		if (!currentLeaf) {
			((gl_tree_cache_directory *)obj)->data.firstLeaf = nextLeaf;
		} else {
			currentLeaf->next_leaf = nextLeaf;
		}
		currentLeaf = nextLeaf;
		currentEntry++;
		leafCount++;
	}
	((gl_tree_cache_directory *)obj)->f->update_count((gl_tree_cache_directory *)obj, 0, leafCount);
	((gl_tree_cache_directory *)obj)->data._numChildLeafs += leafCount;
}

static int gl_directory_list_entry_compare(const void *l, const void *r)
{
	const gl_directory_list_entry *entryL = (const gl_directory_list_entry *)l;
	const gl_directory_list_entry *entryR = (const gl_directory_list_entry *)r;
	
	return strcoll(entryL->name, entryR->name);
}

static int is_ignored_dirname(const char* name)
{
	if (!strcmp(name, ".")) return 1;
	if (!strcmp(name, "..")) return 1;
	return 0;
}

static void gl_tree_cache_directory_reloading_reload_done(void *target, gl_notice_subscription *sub, void *extra_data)
{
	gl_tree_cache_directory_reloading *obj = (gl_tree_cache_directory_reloading *)target;
	
	gl_workqueue_job *job = extra_data;
	
	struct reload_result_data *result = job->data.jobReturn;
	if (!result) {
		((gl_object *)obj)->f->unref((gl_object *)obj);
		return; // completely failed, too bad.
	}
	
	gl_directory_list_entry *dirList = result->dirList;
	gl_directory *dirObj = result->dirObj;
	
	free (result);
	
	gl_directory_list_entry *currentEntry = dirList;
	size_t dirListCount = 0;
	
	while (currentEntry->name) {
		dirListCount++;
		currentEntry++;
	}
	
	qsort(dirList, dirListCount, sizeof(gl_directory_list_entry), &gl_directory_list_entry_compare);
	
	compare_leafs(obj, dirList);
	
	gl_tree_cache_directory *currentBranch = ((gl_tree_cache_directory *)obj)->data.firstBranch;
	gl_tree_cache_directory_reloading *currentBranchR;
	gl_tree_cache_directory *lastBranch = NULL;
	
	currentEntry = dirList;
	
	int compare_result;
	
	while (currentEntry->name) {
		if (currentEntry->type != gl_directory_entry_type_directory) {
			currentEntry++;
			continue;
		}
		if (is_ignored_dirname(currentEntry->name)) {
			currentEntry++;
			continue;
		}
		
		do {
			if (currentBranch) {
				compare_result = strcoll(currentBranch->data.name, currentEntry->name);
				if (!compare_result) {
					currentBranchR = (gl_tree_cache_directory_reloading *)currentBranch;
					currentBranchR->f->set_loadstate(currentBranchR,
									 gl_tree_cache_directory_reloading_loadstate_outdated);
					lastBranch = currentBranch;
					currentBranch = currentBranch->data.nextSibling;
					currentEntry++;
					currentBranchR = NULL;
					break;
				}
				if (compare_result > 0) {
					// prune currentBranch
					if (!currentBranch->data._url) {
						currentBranch->data._url = currentBranch->f->get_url(currentBranch);
					}
					currentBranch->data.parent = NULL;
					if (!lastBranch) {
						((gl_tree_cache_directory *)obj)->data.firstBranch = currentBranch->data.nextSibling;
					} else {
						lastBranch->data.nextSibling = currentBranch->data.nextSibling;
					}
					gl_tree_cache_directory *nextBranch = currentBranch->data.nextSibling;
					((gl_object *)currentBranch)->f->unref((gl_object *)currentBranch);
					currentBranch = nextBranch;
					
					((gl_tree_cache_directory *)obj)->f->update_count((gl_tree_cache_directory *)obj, 1, -1);
					((gl_tree_cache_directory *)obj)->data._numChildBranches--;
					
					continue;
				}
			}
			gl_tree_cache_directory *newBranchD = ((gl_tree_cache_directory *)obj)->f->new_branch((gl_tree_cache_directory *)obj);
			gl_tree_cache_directory_reloading *newBranch = (gl_tree_cache_directory_reloading *)newBranchD;
			if (!newBranch) {
				goto ERRORNOMEMORY;
			}
			char *newName = strdup(currentEntry->name);
			if (!newName) {
				((gl_object *)newBranch)->f->unref((gl_object *)newBranch);
				goto ERRORNOMEMORY;
			}
			newBranchD->data.name = newName;
			newBranchD->data.parent = (gl_tree_cache_directory *)obj;

			if (!lastBranch) {
				((gl_tree_cache_directory *)obj)->data.firstBranch = newBranchD;
			} else {
				lastBranch->data.nextSibling = newBranchD;
			}
			lastBranch = newBranchD;
			newBranchD->data.nextSibling = currentBranch;
			currentEntry++;
			((gl_tree_cache_directory *)obj)->f->update_count((gl_tree_cache_directory *)obj, 1, 1);
			((gl_tree_cache_directory *)obj)->data._numChildBranches++;
			break;
		} while (1);
	}
	
	// Now reload the branches
	currentBranch = ((gl_tree_cache_directory *)obj)->data.firstBranch;
	if (!currentBranch) {
		obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_loaded);
	} else {
		while (currentBranch) {
			currentBranchR = (gl_tree_cache_directory_reloading *)currentBranch;
			gl_notice_subscription *sub = gl_notice_subscription_new();
			sub->data.target = obj;
			sub->data.action = &load_branch_completed;
			currentBranchR->data.loadedNotice->f->subscribe(currentBranchR->data.loadedNotice, sub);
			((gl_object *)obj)->f->ref((gl_object *)obj);
			obj->data.branchesToReload += currentBranchR->f->trigger_reload(currentBranchR);
			
			currentBranch = currentBranch->data.nextSibling;
		}
		if (!obj->data.branchesToReload) {
			obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_loaded);
		}
	}
	((gl_object *)obj)->f->unref((gl_object *)obj);
	dirObj->f->free_list(dirObj, dirList);
	((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
	return;
	
ERRORNOMEMORY:
	dirObj->f->free_list(dirObj, dirList);
	((gl_object *)dirObj)->f->unref((gl_object *)dirObj);
	obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_loaded);
	// Otherwise everything hangs and we might as well crash
	((gl_object *)obj)->f->unref((gl_object *)obj);
	return;
}

static void load_branch_completed(void *target, gl_notice_subscription *subscription, void *extra_data)
{
	gl_tree_cache_directory_reloading *obj = (gl_tree_cache_directory_reloading *)target;
	
	assert(obj->data.branchesToReload > 0);
	obj->data.branchesToReload--;
	if (!obj->data.branchesToReload) {
		obj->f->set_loadstate(obj, gl_tree_cache_directory_reloading_loadstate_loaded);
	}
}

static void gl_tree_cache_directory_reloading_set_loadstate(gl_tree_cache_directory_reloading *obj, gl_tree_cache_directory_reloading_loadstate loadstate)
{
	obj->data._loadstate = loadstate;
	if (loadstate == gl_tree_cache_directory_reloading_loadstate_loaded) {
		obj->data.loadedNotice->f->fire(obj->data.loadedNotice);
	}
}


gl_tree_cache_directory_reloading *gl_tree_cache_directory_reloading_init(gl_tree_cache_directory_reloading *obj)
{
	gl_tree_cache_directory_ordered_init((gl_tree_cache_directory_ordered *)obj);
	
	obj->f = &gl_tree_cache_directory_reloading_funcs_global;
	
	return obj;
}

gl_tree_cache_directory_reloading *gl_tree_cache_directory_reloading_new()
{
	gl_tree_cache_directory_reloading *ret = calloc(1, sizeof(gl_tree_cache_directory_reloading));
	
	ret->data.loadedNotice = gl_notice_new();
	
	return gl_tree_cache_directory_reloading_init(ret);
}

static void gl_tree_cache_directory_reloading_free(gl_object *obj_obj)
{
	gl_tree_cache_directory_reloading *obj = (gl_tree_cache_directory_reloading *)obj_obj;
	
	((gl_object *)obj->data.loadedNotice)->f->unref((gl_object *)obj->data.loadedNotice);
	obj->data.loadedNotice = NULL;
	
	gl_object_free_org_global(obj_obj);
}
