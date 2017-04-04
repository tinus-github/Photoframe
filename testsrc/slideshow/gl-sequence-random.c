//
//  gl-sequence-random.c
//  Photoframe
//
//  Created by Martijn Vernooij on 31/03/2017.
//
//

#include "gl-sequence-random.h"

#include <string.h>
#include <stdlib.h>

#ifndef __APPLE__
#include "arc4random.h"
#endif

#define GL_SEQUENCE_RANDOM_MAX_TRIES 32

static void gl_sequence_random_start(gl_sequence *obj_obj);
static int gl_sequence_random_get_entry(gl_sequence *obj_obj, size_t *entry);

static struct gl_sequence_random_funcs gl_sequence_random_funcs_global = {
	
};


void gl_sequence_random_setup()
{
	gl_sequence *parent = gl_sequence_new();
	memcpy(&gl_sequence_random_funcs_global.p, parent->f, sizeof(gl_sequence_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_sequence_random_funcs_global.p.start = &gl_sequence_random_start;
	gl_sequence_random_funcs_global.p.get_entry = &gl_sequence_random_get_entry;
}

static void gl_sequence_random_start(gl_sequence *obj_obj)
{
	gl_sequence_random *obj = (gl_sequence_random *)obj_obj;
	
	unsigned int counter;
	for (counter = GL_SEQUENCE_RANDOM_MEMORY_SIZE; counter > 0; counter--) {
		obj->data._previous_entries[counter - 1] = UINT32_MAX;
	}
	
	obj->data._previous_entry_cursor = 0;
}

static int gl_sequence_random_get_entry(gl_sequence *obj_obj, size_t *entry)
{
	gl_sequence_random *obj = (gl_sequence_random *)obj_obj;
	
	uint32_t count = (uint32_t)obj_obj->f->get_count(obj_obj);
	
	if (!count) {
		return -1;
	}
	
	if (count == 1) {
		return 0;
	}
	
	if (count < 5) {
		return arc4random_uniform(count);
	}
	
	uint32_t entriesToTest = count / 2;
	if (entriesToTest > GL_SEQUENCE_RANDOM_MAX_TRIES) {
		entriesToTest = GL_SEQUENCE_RANDOM_MAX_TRIES;
	}
	
	if (obj->data._previous_entry_cursor >= entriesToTest) {
		obj->data._previous_entry_cursor = 0;
	}
	
	uint32_t ret;
	int tryCount = 0;
	int testCounter;
	int found;
 
	do {
		ret = arc4random_uniform(count);
		found = 0;
		for (testCounter = 0; testCounter < entriesToTest; testCounter++) {
			if (obj->data._previous_entries[testCounter] == ret) {
				found = 1;
			}
		}
		tryCount++;
	} while (found && (tryCount < GL_SEQUENCE_RANDOM_MAX_TRIES));
	
	obj->data._previous_entries[obj->data._previous_entry_cursor++] = ret;

	entry[0] = ret;
	return 0;
}

gl_sequence_random *gl_sequence_random_init(gl_sequence_random *obj)
{
	gl_sequence_init((gl_sequence *)obj);
	
	obj->f = &gl_sequence_random_funcs_global;
	
	((gl_sequence *)obj)->f->start((gl_sequence *)obj);
	
	return obj;
}

gl_sequence_random *gl_sequence_random_new()
{
	gl_sequence_random *ret = calloc(1, sizeof(gl_sequence_random));
	
	return gl_sequence_random_init(ret);
}
