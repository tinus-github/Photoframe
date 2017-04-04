//
//  gl-sequence-random.h
//  Photoframe
//
//  Created by Martijn Vernooij on 31/03/2017.
//
//

#ifndef gl_sequence_random_h
#define gl_sequence_random_h

#define GL_SEQUENCE_RANDOM_MEMORY_SIZE 32

#include <stdio.h>
#include <inttypes.h>

#include "slideshow/gl-sequence.h"

typedef struct gl_sequence_random gl_sequence_random;

typedef struct gl_sequence_random_funcs {
	gl_sequence_funcs p;
} gl_sequence_random_funcs;

typedef struct gl_sequence_random_data {
	gl_sequence_data p;
	
	uint32_t _previous_entries[GL_SEQUENCE_RANDOM_MEMORY_SIZE];
	unsigned int _previous_entry_cursor;
} gl_sequence_random_data;

struct gl_sequence_random {
	gl_sequence_random_funcs *f;
	gl_sequence_random_data data;
};

void gl_sequence_random_setup();
gl_sequence_random *gl_sequence_random_init(gl_sequence_random *obj);
gl_sequence_random *gl_sequence_random_new();

#endif /* gl_sequence_random_h */
