//
//  gl-shape.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/05/16.
//
//

#include "gl-shape.h"
#include "gl-display.h"
#include "gl-stage.h"
#include "gl-container.h"
#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

#include "../lib/linmath/linmath.h"

static void gl_shape_draw(gl_shape *obj);
static void gl_shape_set_computed_projection_dirty(gl_shape *obj);
static void gl_shape_compute_projection(gl_shape *obj);

static struct gl_shape_funcs gl_shape_funcs_global = {
	.draw = &gl_shape_draw,
	.set_computed_projection_dirty = &gl_shape_set_computed_projection_dirty,
	.compute_projection = &gl_shape_compute_projection
};

static void gl_shape_draw(gl_shape *obj)
{
	printf("%s\n", "gl_shape_draw is an abstract function");
	abort();
}

static void gl_shape_set_computed_projection_dirty(gl_shape *obj)
{
	obj->data.computed_projection_dirty = TRUE;
}

static void gl_shape_get_container_projection(gl_shape *obj, mat4x4 ret)
{
	if (!obj->data.container) {
		mat4x4_identity(ret);
		return;
	}
	
	gl_container *container = obj->data.container;
	mat4x4_dup(ret, container->data.projection);
}

static void gl_shape_compute_projection(gl_shape *obj)
{
	mat4x4 projection;
	mat4x4 projection_scaled;
	mat4x4 translation;
	mat4x4 container_projection;

	if (!obj->data.computed_projection_dirty) {
		return;
	}

	//TODO: Make this optional
	
	gl_shape_get_container_projection(obj, container_projection);
	
//### dynamic
	mat4x4_translate(translation, obj->data.objectX, obj->data.objectY, 0.0);
	mat4x4_identity(projection);
	
	mat4x4_scale_aniso(projection_scaled, projection,
			   obj->data.objectWidth,
			   obj->data.objectHeight,
			   1.0);
	mat4x4_mul(projection, translation, projection_scaled);
	mat4x4_mul(obj->data.computed_modelView, container_projection, projection);
	
	obj->data.computed_projection_dirty = FALSE;
}

void gl_shape_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_shape_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_shape *gl_shape_init(gl_shape *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_shape_funcs_global;
	
	obj->f->set_computed_projection_dirty(obj);

	obj->data.siblingL = NULL;
	obj->data.siblingR = NULL;
	obj->data.container = NULL;
	obj->data.objectX = 0;
	obj->data.objectY = 0;
	obj->data.objectWidth = 0;
	obj->data.objectHeight = 0;
	
	return obj;
}

gl_shape *gl_shape_new()
{
	gl_shape *ret = calloc(1, sizeof(gl_shape));
	
	return gl_shape_init(ret);
}
