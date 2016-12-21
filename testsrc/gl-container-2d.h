//
//  gl-container-2d.h
//  Photoframe
//
//  Created by Martijn Vernooij on 21/12/2016.
//
//

#ifndef gl_container_2d_h
#define gl_container_2d_h

#include <stdio.h>
#include "gl-container.h"

typedef struct gl_container_2d gl_container_2d;

typedef struct gl_container_2d_funcs {
	gl_container_funcs p;
//	void (*append_child) (gl_container *obj, gl_shape *child);
} gl_container_2d_funcs;

typedef struct gl_container_2d_data {
	gl_container_data p;
	
	GLfloat scaleH;
	GLfloat scaleV;
} gl_container_data;

struct gl_container_2d {
	gl_container_2d_funcs *f;
	gl_container_2d_data data;
};

void gl_container_2d_setup();
gl_container_2d *gl_container_2d_init(gl_container_2d *obj);
gl_container_2d *gl_container_2d_new();


#endif /* gl_container_2d_h */
