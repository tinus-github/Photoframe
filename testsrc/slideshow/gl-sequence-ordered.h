//
//  gl-sequence-ordered.h
//  Photoframe
//
//  Created by Martijn Vernooij on 28/03/2017.
//
//

#ifndef gl_sequence_ordered_h
#define gl_sequence_ordered_h

#include <stdio.h>

#include "slideshow/gl-sequence.h"

typedef struct gl_sequence_ordered gl_sequence_ordered;

typedef struct gl_sequence_ordered_funcs {
	gl_sequence_funcs p;
} gl_sequence_ordered_funcs;

typedef struct gl_sequence_ordered_data {
	gl_sequence_data p;
	
	size_t _current_entry;
} gl_sequence_ordered_data;

struct gl_sequence_ordered {
	gl_sequence_ordered_funcs *f;
	gl_sequence_ordered_data data;
};

void gl_sequence_ordered_setup();
gl_sequence_ordered *gl_sequence_ordered_init(gl_sequence_ordered *obj);
gl_sequence_ordered *gl_sequence_ordered_new();

#endif /* gl_sequence_ordered_h */
