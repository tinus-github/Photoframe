//
//  gl-sequence.c
//  Photoframe
//
//  Created by Martijn Vernooij on 28/03/2017.
//
//

#include "slideshow/gl-sequence.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "slideshow/gl-sequence-ordered.h"
#include "slideshow/gl-sequence-random.h"
#include "slideshow/gl-sequence-selection.h"

static void gl_sequence_set_count(gl_sequence *obj, size_t count);
static size_t gl_sequence_get_count(gl_sequence *obj);
static void gl_sequence_start(gl_sequence *obj);
static int gl_sequence_get_entry(gl_sequence *obj, size_t *entry);
static void gl_sequence_set_configuration(gl_sequence *obj, gl_config_section *config);

static struct gl_sequence_funcs gl_sequence_funcs_global = {
	.set_count = &gl_sequence_set_count,
	.get_count = &gl_sequence_get_count,
	.start = &gl_sequence_start,
	.get_entry = &gl_sequence_get_entry,
	.set_configuration = &gl_sequence_set_configuration,
};

static void gl_sequence_set_count(gl_sequence *obj, size_t count)
{
	obj->data._count = count;
}

static size_t gl_sequence_get_count(gl_sequence *obj)
{
	return obj->data._count;
}

static void gl_sequence_start(gl_sequence *obj)
{
	;
}

static void gl_sequence_set_configuration(gl_sequence *obj, gl_config_section *config)
{
	;
}

static int gl_sequence_get_entry(gl_sequence *obj, size_t *entry)
{
	printf("Sequence->get_entry is an abstract function\n");
	assert(0);
	exit(1);
}

void gl_sequence_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_sequence_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
}

gl_sequence *gl_sequence_init(gl_sequence *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_sequence_funcs_global;
	
	return obj;
}

gl_sequence *gl_sequence_new()
{
	gl_sequence *ret = calloc(1, sizeof(gl_sequence));
	
	return gl_sequence_init(ret);
}

gl_sequence *gl_sequence_new_with_type(gl_sequence_type type)
{
	switch (type) {
		case gl_sequence_type_ordered:
			return (gl_sequence *)gl_sequence_ordered_new();
		case gl_sequence_type_selection:
			return (gl_sequence *)gl_sequence_selection_new();
		case gl_sequence_type_random:
		default:
			return (gl_sequence *)gl_sequence_random_new();
	}
}
