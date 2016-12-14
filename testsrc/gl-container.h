//
//  gl-container.h
//  Photoframe
//
//  Created by Martijn Vernooij on 14/12/2016.
//
//

#ifndef gl_container_h
#define gl_container_h

#include <stdio.h>

#include "gl-shape.h"

typedef struct gl_container gl_container;

typedef struct gl_container_funcs {
	gl_shape_funcs p;
	void (*append_child) (gl_container *obj, gl_shape *child);
} gl_container_funcs;

typedef struct gl_container_data {
	gl_shape_data p;
	
	gl_shape *first_child;
} gl_container_data;

struct gl_container {
	gl_container_funcs *f;
	gl_container_data data;
};

void gl_container_setup();
gl_container *gl_container_init(gl_container *obj);
gl_container *gl_container_new();

#endif /* gl_container_h */
