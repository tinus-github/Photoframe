//
//  gl-sequence-selection.c
//  Photoframe
//
//  Created by Martijn Vernooij on 04/04/2017.
//
//

#include "gl-sequence-selection.h"


#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifndef __APPLE__
#include "arc4random.h"
#endif

static void gl_sequence_selection_start(gl_sequence *obj_obj);
static int gl_sequence_selection_get_entry(gl_sequence *obj_obj, size_t *entry);
void gl_sequence_selection_free(gl_object *obj_obj);

static struct gl_sequence_selection_funcs gl_sequence_selection_funcs_global = {
	
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_sequence_selection_setup()
{
	gl_sequence *parent = gl_sequence_new();
	memcpy(&gl_sequence_selection_funcs_global.p, parent->f, sizeof(gl_sequence_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_sequence_selection_funcs_global.p.start = &gl_sequence_selection_start;
	gl_sequence_selection_funcs_global.p.get_entry = &gl_sequence_selection_get_entry;
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_sequence_selection_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_sequence_selection_free;
}

static int compare_selection(const void *l, const void *r)
{
	uint32_t *leftp = (uint32_t *)l;
	uint32_t left = leftp[0];
	uint32_t *rightp = (uint32_t *)r;
	uint32_t right = rightp[0];
	
	if (left < right) {
		return -1;
	}
	if (left == right) {
		return 0;
	}
	return 1;
}

static void gl_sequence_selection_start(gl_sequence *obj_obj)
{
	gl_sequence_selection *obj = (gl_sequence_selection *)obj_obj;
	
	size_t count = obj_obj->f->get_count(obj_obj);
	
	unsigned int proposed_selection_size = count * 0.6;
	if (proposed_selection_size < obj->data._selection_size) {
		obj->data._selection_size = proposed_selection_size;
	}
	
	if (obj->data._entries) {
		free(obj->data._entries);
		obj->data._entries = NULL;
	}
	
	obj->data._entry_cursor = 0;
	
	if (obj->data._selection_size == 0) {
		return;
	}
	
	obj->data._entries = calloc(sizeof(uint32_t), obj->data._selection_size);
	
	if (!obj->data._entries) {
		obj->data._selection_size = 0;
		return;
	}
	
	unsigned int counter;
	unsigned int check_counter;
	unsigned int attempt;
	int found;
	
	assert (obj->data._selection_size < count);
	
	for (counter = 0; counter < obj->data._selection_size; counter++) {
		do {
			found = 0;
			attempt = arc4random_uniform((uint32_t)count);
			for (check_counter = 0; check_counter < counter; check_counter++) {
				if (attempt == obj->data._entries[check_counter]) {
					found = 1; break;
				}
			}
		} while (found);
		
		obj->data._entries[counter] = attempt;
	}
	
	qsort(obj->data._entries, obj->data._selection_size, sizeof(uint32_t), &compare_selection);
}

static int gl_sequence_selection_get_entry(gl_sequence *obj_obj, size_t *entry)
{
	gl_sequence_selection *obj = (gl_sequence_selection *)obj_obj;
	
	if (obj->data._entry_cursor >= obj->data._selection_size) {
		return -1;
	}
	
	entry[0] = obj->data._entries[obj->data._entry_cursor];
	
	obj->data._entry_cursor++;
	
	return 0;
}

gl_sequence_selection *gl_sequence_selection_init(gl_sequence_selection *obj)
{
	gl_sequence_init((gl_sequence *)obj);
	
	obj->f = &gl_sequence_selection_funcs_global;
	
	((gl_sequence *)obj)->f->start((gl_sequence *)obj);
	
	return obj;
}

gl_sequence_selection *gl_sequence_selection_new()
{
	gl_sequence_selection *ret = calloc(1, sizeof(gl_sequence_selection));
	
	return gl_sequence_selection_init(ret);
}

void gl_sequence_selection_free(gl_object *obj_obj)
{
	gl_sequence_selection *obj = (gl_sequence_selection *)obj_obj;
	
	if (obj->data._entries) {
		free(obj->data._entries);
		obj->data._entries = NULL;
	}
	
	
}
