//
//  gl-sequence.h
//  Photoframe
//
//  Created by Martijn Vernooij on 28/03/2017.
//
//

#ifndef gl_sequence_h
#define gl_sequence_h

#include <stdio.h>

#include "infrastructure/gl-object.h"

typedef enum {
	gl_sequence_type_ordered,
	gl_sequence_type_random,
	gl_sequence_type_selection
} gl_sequence_type;

typedef struct gl_sequence gl_sequence;

typedef struct gl_sequence_funcs {
	gl_object_funcs p;
	void (*set_count) (gl_sequence *obj, size_t count);
	size_t (*get_count) (gl_sequence *obj);
	void (*start) (gl_sequence *obj);
	int (*get_entry) (gl_sequence *obj, size_t *entry);
} gl_sequence_funcs;

typedef struct gl_sequence_data {
	gl_object_data p;
	
	size_t _count;
} gl_sequence_data;

struct gl_sequence {
	gl_sequence_funcs *f;
	gl_sequence_data data;
};

void gl_sequence_setup();
gl_sequence *gl_sequence_init(gl_sequence *obj);
gl_sequence *gl_sequence_new();

gl_sequence *gl_sequence_new_with_type(gl_sequence_type type);

#endif /* gl_sequence_h */
