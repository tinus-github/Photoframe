//
//  gl-container.c
//  Photoframe
//
//  Created by Martijn Vernooij on 14/12/2016.
//
//

#include "gl-container.h"

static void gl_container_append_child(gl_container *obj, gl_shape *child);

static struct gl_container_funcs gl_container_funcs_global = {
	.append_child = &gl_container_append_child
};

void gl_container_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_container_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
}

gl_container *gl_container_init(gl_container *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_container_funcs_global;
	
	return obj;
}

gl_container *gl_container_new()
{
	gl_container *ret = malloc(sizeof(gl_container));
	
	return gl_container_init(ret);
}

static void gl_container_append_child(gl_container *obj, gl_shape *child)
{
	if (child->data.container) {
		gl_container *parent = child->data.container;
		//TODO: parent->f->remove_child(parent, child);
	}
	
	child->data.container = obj;
	
	if (!obj->data.first_child) {
		obj->data.first_child = child;
		child->data.siblingL = child;
		child->data.siblingR = child;
	} else {
		gl_shape *first_child = obj->data.first_child;
		gl_shape *last_child = obj->data.siblingL;
		child->data.siblingL = last_child;
		last_child->data.siblingR = child;
		child->data.siblingR = first_child;
		first_child->data.siblingL = child;
	}
	
	child->f->set_computed_projection_dirty(child);
}
