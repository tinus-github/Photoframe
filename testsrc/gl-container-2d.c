//
//  gl-container-2d.c
//  Photoframe
//
//  Created by Martijn Vernooij on 21/12/2016.
//
//

#include "gl-container-2d.h"

static struct gl_container_2d_funcs gl_container_2d_funcs_global = {
//	.append_child = &gl_container_append_child
};

static void (*parent_compute_projection) (gl_shape *obj);

void gl_container_2d_setup()
{
	gl_container *parent = gl_container_new();
	gl_shape *parent_shape = (gl_shape *)parent;
	parent_compute_projection = parent_shape->f->compute_projection;
	
	memcpy(&gl_container_2d_funcs_global.p, parent->f, sizeof(gl_container_funcs));
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
}

gl_container_2d *gl_container_2d_init(gl_container_2d *obj)
{
	gl_container_init((gl_container *)obj);
	
	obj->f = &gl_container_2d_funcs_global;
	
	return obj;
}

gl_container_2d *gl_container_2d_new()
{
	gl_container_2d *ret = calloc(1, sizeof(gl_container_2d));
	
	return gl_container_2d_init(ret);
}
