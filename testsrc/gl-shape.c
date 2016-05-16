//
//  gl-shape.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/05/16.
//
//

#include "gl-shape.h"
#include <string.h>

static void gl_shape_draw(gl_shape *obj);

static struct gl_shape_funcs gl_shape_funcs_global = {
	.draw = &draw
};

static void gl_shape_draw(gl_shape *obj)
{
	assert(0, "Abstract function");
}

void gl_shape_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_shape_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

