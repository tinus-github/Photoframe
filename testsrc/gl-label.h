//
//  gl-label.h
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#ifndef gl_label_h
#define gl_label_h

#include <stdio.h>

#include "gl-shape.h"
#include "gl-tile.h"

typedef struct gl_label gl_label;

typedef struct gl_label_funcs {
	gl_shape_funcs p;
	void (*render) (gl_label *obj);
} gl_label_funcs;

typedef struct gl_label_data {
	gl_shape_data p;
	
	char *text;
	uint32_t windowX;
	uint32_t windowY;
	uint32_t windowWidth;
	uint32_t windowHeight;
	
	uint32_t usedWidth;
	
	gl_tile *tile;
} gl_label_data;

struct gl_label {
	gl_label_funcs *f;
	gl_label_data data;
};

void gl_label_setup();
gl_label *gl_label_init(gl_label *obj);
gl_label *gl_label_new();


#endif /* gl_label_h */
