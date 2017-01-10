//
//  gl-shape.h
//  Photoframe
//
//  Created by Martijn Vernooij on 16/05/16.
//
//

#ifndef gl_shape_h
#define gl_shape_h

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "infrastructure/gl-object.h"

typedef float vec4[4];
typedef vec4 mat4x4[4];

typedef struct gl_shape gl_shape;

typedef struct gl_container gl_container;

typedef struct gl_shape_funcs {
	gl_object_funcs p;
	void (*set_computed_projection_dirty) (gl_shape *obj);
	void (*compute_projection) (gl_shape *obj);
	void (*draw) (gl_shape *obj);
	void (*set_computed_alpha_dirty) (gl_shape *obj);
	void (*compute_alpha) (gl_shape *obj);
} gl_shape_funcs;

typedef struct gl_shape_data {
	gl_object_data p;
	gl_container *container;
	
	mat4x4 computed_modelView;
	unsigned int computed_projection_dirty;

// TODO make this optional
	GLfloat objectX;
	GLfloat objectY;
	GLfloat objectWidth;
	GLfloat objectHeight;
	
	GLfloat alpha;
	GLfloat computedAlpha;
	unsigned int computed_alpha_dirty;
	
	gl_shape *siblingL;
	gl_shape *siblingR;
} gl_shape_data;

struct gl_shape {
	gl_shape_funcs *f;
	gl_shape_data data;
};

void gl_shape_setup();
gl_shape *gl_shape_init(gl_shape *obj);
gl_shape *gl_shape_new();

#endif /* gl_shape_h */
