//
//  gl-container.c
//  Photoframe
//
//  Created by Martijn Vernooij on 14/12/2016.
//
//

#include "gl-container.h"

void *gl_container_append_child(gl_container *obj, gl_shape *child);

static struct gl_container_funcs gl_container_funcs_global = {
	.append_child = &gl_container_append_child
};

void gl_container_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_container_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	parent->f->free(parent);
}

gl_container *gl_container_init(gl_container *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_container_funcs_global;
	
	return obj;
}

gl_container *gl_container_new()
{
	gl_stage *ret = malloc(sizeof(gl_stage));
	
	return gl_stage_init(ret);
}

void *gl_container_append_child(gl_container *obj, gl_shape *child)
{
	if (child->data.parent) {
		gl_container *parent = child->data.parent;
		//TODO: parent->f->remove_child(parent, child);
	}
	
	child->data.parent = obj;
	
	if (!obj->data.first_child) {
		obj->data.first_child = obj;
		obj->data.siblingL = obj;
		obj->data.siblingR = obj;
	} else {
		gl_shape *first_child = obj->data.first_child;
		gl_shape *last_child = obj->data.siblingL;
		obj->data.siblingL = last_child;
		last_child->data.siblingR = obj;
		obj->data.siblingR = first_child;
		first_child->data.siblingL = obj;
	}
	
	obj->f->set_computed_projection_dirty(obj);
}
