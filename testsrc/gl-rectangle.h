//
//  gl-rectangle.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#ifndef gl_rectangle_h
#define gl_rectangle_h

#include <stdio.h>

#include <stdio.h>

#include "gl-shape.h"
#include "gl-texture.h"

typedef struct gl_rectangle gl_rectangle;

typedef struct gl_rectangle_funcs {
	gl_shape_funcs p;
	void (*set_color) (gl_rectangle *obj, GLfloat r, GLfloat g, GLfloat b, GLfloat alpha);
} gl_rectangle_funcs;

typedef struct gl_rectangle_data {
	gl_shape_data p;
	vec4 color;
} gl_rectangle_data;

struct gl_rectangle {
	gl_rectangle_funcs *f;
	gl_rectangle_data data;
};

void gl_rectangle_setup();
gl_rectangle *gl_rectangle_init(gl_rectangle *obj);
gl_rectangle *gl_rectangle_new();


#endif /* gl_rectangle_h */
