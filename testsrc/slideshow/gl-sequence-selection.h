//
//  gl-sequence-selection.h
//  Photoframe
//
//  Created by Martijn Vernooij on 04/04/2017.
//
//

#ifndef gl_sequence_selection_h
#define gl_sequence_selection_h

#include <stdio.h>
#include <inttypes.h>

#include "slideshow/gl-sequence.h"

typedef struct gl_sequence_selection gl_sequence_selection;

typedef struct gl_sequence_selection_funcs {
	gl_sequence_funcs p;
} gl_sequence_selection_funcs;

typedef struct gl_sequence_selection_data {
	gl_sequence_data p;
	
	uint32_t *_entries;
	unsigned int _entry_cursor;
	unsigned int _selection_size;
} gl_sequence_selection_data;

struct gl_sequence_selection {
	gl_sequence_selection_funcs *f;
	gl_sequence_selection_data data;
};

void gl_sequence_selection_setup();
gl_sequence_selection *gl_sequence_selection_init(gl_sequence_selection *obj);
gl_sequence_selection *gl_sequence_selection_new();

#endif /* gl_sequence_selection_h */

