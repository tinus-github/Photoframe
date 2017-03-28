//
//  gl-sequence-ordered.c
//  Photoframe
//
//  Created by Martijn Vernooij on 28/03/2017.
//
//

#include "gl-sequence-ordered.h"
#include <string.h>
#include <stdlib.h>

static void gl_sequence_ordered_start(gl_sequence *obj_obj);
static int gl_sequence_ordered_get_entry(gl_sequence *obj_obj, size_t *entry);

static struct gl_sequence_ordered_funcs gl_sequence_ordered_funcs_global = {

};

void gl_sequence_ordered_setup()
{
	gl_sequence *parent = gl_sequence_new();
	memcpy(&gl_sequence_ordered_funcs_global.p, parent->f, sizeof(gl_sequence_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_sequence_ordered_funcs_global.p.start = &gl_sequence_ordered_start;
	gl_sequence_ordered_funcs_global.p.get_entry = &gl_sequence_ordered_get_entry;
}

static void gl_sequence_ordered_start(gl_sequence *obj_obj)
{
	gl_sequence_ordered *obj = (gl_sequence_ordered *)obj_obj;
	
	obj->data._current_entry = 0;
}

static int gl_sequence_ordered_get_entry(gl_sequence *obj_obj, size_t *entry)
{
	gl_sequence_ordered *obj = (gl_sequence_ordered *)obj_obj;
	
	size_t ret = ++(obj->data._current_entry);
	if (ret >= obj_obj->f->get_count(obj_obj)) {
		return -1;
	}

	*entry = ret;
	
	return 0;
}

gl_sequence_ordered *gl_sequence_ordered_init(gl_sequence_ordered *obj)
{
	gl_sequence_init((gl_sequence *)obj);
	
	obj->f = &gl_sequence_ordered_funcs_global;
	
	return obj;
}

gl_sequence_ordered *gl_sequence_ordered_new()
{
	gl_sequence_ordered *ret = calloc(1, sizeof(gl_sequence_ordered));
	
	return gl_sequence_ordered_init(ret);
}
