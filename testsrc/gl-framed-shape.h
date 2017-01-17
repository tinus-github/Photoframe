//
//  gl-framed-shape.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#ifndef gl_framed_shape_h
#define gl_framed_shape_h

#include <stdio.h>

#include "gl-container-2d.h"
#include "gl-shape.h"
#include "gl-rectangle.h"

typedef struct gl_framed_shape gl_framed_shape;

typedef struct gl_framed_shape_funcs {
	gl_container_2d_funcs p;
	void (*update_frame) (gl_framed_shape *obj);
} gl_framed_shape_funcs;

typedef struct gl_framed_shape_data {
	gl_container_2d_data p;
	
	gl_shape *shape;
	
	gl_shape *_contained_shape;
	gl_rectangle *_borders[4];
} gl_framed_shape_data;

struct gl_framed_shape {
	gl_framed_shape_funcs *f;
	gl_framed_shape_data data;
};

void gl_framed_shape_setup();
gl_framed_shape *gl_framed_shape_init(gl_framed_shape *obj);
gl_framed_shape *gl_framed_shape_new();


#endif /* gl_framed_shape_h */
