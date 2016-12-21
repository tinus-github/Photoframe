//
//  gl-container-2d.c
//  Photoframe
//
//  Created by Martijn Vernooij on 21/12/2016.
//
//

#include "gl-container-2d.h"
#include "../lib/linmath/linmath.h"

static struct gl_container_2d_funcs gl_container_2d_funcs_global = {
//	.append_child = &gl_container_append_child
};

static void (*parent_compute_projection) (gl_shape *obj);

static void gl_container_2d_compute_projection(gl_shape *obj_shape)
{
	gl_container_2d *obj = (gl_container_2d *)obj_shape;
	
	if (!obj->data.computed_projection_dirty) {
		return;
	}
	
	mat4x4 start;
	mat4x4 scaled;
	
	mat4x4_identity(start);
	mat4x4_scale_aniso(scaled, start, obj->data.scaleH, obj->data.scaleV, 1.0);
	
	mat4x4 translation;
	mat4x4_translate(translation, obj_shape->data.objectX, obj_shape->data.objectY, 0.0);
	
	mat4x4_mul(obj_shape->data.projection, translation, scaled);
		
	parent_compute_projection(obj_shape);
}

void gl_container_2d_setup()
{
	gl_container *parent = gl_container_new();
	gl_shape *parent_shape = (gl_shape *)parent;
	parent_compute_projection = parent_shape->f->compute_projection;
	
	memcpy(&gl_container_2d_funcs_global.p, parent->f, sizeof(gl_container_funcs));

	gl_shape_funcs *shapef = (gl_shape_funcs *)&gl_container_2d_funcs_global
	shapef->f->compute_projection = gl_container_2d_compute_projection;
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
}

gl_container_2d *gl_container_2d_init(gl_container_2d *obj)
{
	gl_container_init((gl_container *)obj);
	
	obj->f = &gl_container_2d_funcs_global;
	
	obj->data.scaleH = 1.0;
	obj->data.scaleV = 1.0;
	
	return obj;
}

gl_container_2d *gl_container_2d_new()
{
	gl_container_2d *ret = calloc(1, sizeof(gl_container_2d));
	
	return gl_container_2d_init(ret);
}
