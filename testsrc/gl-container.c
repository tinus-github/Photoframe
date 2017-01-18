//
//  gl-container.c
//  Photoframe
//
//  Created by Martijn Vernooij on 14/12/2016.
//
//

#include <assert.h>
#include "gl-container.h"
#include "../lib/linmath/linmath.h"

#define TRUE 1
#define FALSE 0

static void gl_container_append_child(gl_container *obj, gl_shape *child);
static void gl_container_remove_child(gl_container *obj, gl_shape *child);
static void gl_container_set_computed_projection_dirty(gl_shape *shape_obj);
static void gl_container_set_computed_alpha_dirty(gl_shape *shape_obj);
static void gl_container_compute_projection(gl_shape *shape_obj);
static void gl_container_draw(gl_shape *shape_obj);
static void gl_container_free(gl_object *obj);

static void (*gl_object_free_org_global) (gl_object *obj);

static struct gl_container_funcs gl_container_funcs_global = {
	.append_child = &gl_container_append_child,
	.remove_child = &gl_container_remove_child
};

void gl_container_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_container_funcs_global.p, parent->f, sizeof(gl_shape_funcs));

	gl_shape_funcs *shapef = (gl_shape_funcs *)&gl_container_funcs_global;
	shapef->set_computed_projection_dirty = &gl_container_set_computed_projection_dirty;
	shapef->set_computed_alpha_dirty = &gl_container_set_computed_alpha_dirty;
	shapef->compute_projection = &gl_container_compute_projection;
	shapef->draw = &gl_container_draw;
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_container_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_container_free;
}

gl_container *gl_container_init(gl_container *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_container_funcs_global;
	obj->data.first_child = NULL;
	mat4x4_identity(obj->data.projection);
	
	return obj;
}

gl_container *gl_container_new()
{
	gl_container *ret = calloc(1, sizeof(gl_container));
	
	return gl_container_init(ret);
}

static void gl_container_free(gl_object *obj_obj)
{
	gl_container *obj = (gl_container *)obj_obj;
	
	while (obj->data.first_child) {
		obj->f->remove_child(obj, obj->data.first_child);
	}
	
	gl_object_free_org_global(obj_obj);
}

static void gl_container_append_child(gl_container *obj, gl_shape *child)
{
	gl_object *obj_child = (gl_object *)child;
	obj_child->f->ref(obj_child); // Prevent the child from being deallocated as it is being removed from the parent
	
	if (child->data.container) {
		gl_container *parent = child->data.container;
		parent->f->remove_child(parent, child);
	}
	
	child->data.container = obj;
	
	if (!obj->data.first_child) {
		obj->data.first_child = child;
		child->data.siblingL = child;
		child->data.siblingR = child;
	} else {
		gl_shape *first_child = obj->data.first_child;
		gl_shape *last_child = first_child->data.siblingL;
		child->data.siblingL = last_child;
		last_child->data.siblingR = child;
		child->data.siblingR = first_child;
		first_child->data.siblingL = child;
	}
	
	child->f->set_computed_projection_dirty(child);
	obj_child->f->unref(obj_child);
}

static void gl_container_remove_child(gl_container *obj, gl_shape *child)
{
	assert(child->data.container == obj);
	gl_object *obj_child = (gl_object *)child;
	
	if (child->data.siblingL == child) {
		child->data.siblingL = NULL;
		child->data.siblingR = NULL;
		obj->data.first_child = NULL;
	} else {
		if (obj->data.first_child == child) {
			obj->data.first_child = child->data.siblingR;
		}
		gl_shape *siblingR = child->data.siblingR;
		gl_shape *siblingL = child->data.siblingL;
		
		siblingR->data.siblingL = child->data.siblingL;
		siblingL->data.siblingR = child->data.siblingR;
	}
	
	child->data.container = NULL;

	obj_child->f->unref(obj_child);
}

static void gl_container_compute_projection(gl_shape *shape_obj)
{
	gl_container *obj = (gl_container *)shape_obj;
	
	if (!shape_obj->data.computed_projection_dirty) {
		return;
	}
	shape_obj->data.computed_projection_dirty = FALSE;
	
	if (!shape_obj->data.container) {
		mat4x4_dup (shape_obj->data.computed_modelView, obj->data.projection);
		return;
	}
	
	gl_shape *my_container_shape = (gl_shape *)shape_obj->data.container;
	my_container_shape->f->compute_projection(my_container_shape);
	
	mat4x4_mul(shape_obj->data.computed_modelView, my_container_shape->data.computed_modelView, obj->data.projection);
}

static void gl_container_set_computed_projection_dirty(gl_shape *shape_obj)
{
	if (shape_obj->data.computed_projection_dirty) {
		return;
	}
	
	shape_obj->data.computed_projection_dirty = TRUE;
	
	gl_container *obj = (gl_container *)shape_obj;
	
	gl_shape *first_child = obj->data.first_child;
	if (first_child) {
		gl_shape *child = first_child;
		do {
			child->f->set_computed_projection_dirty(child);
			child = child->data.siblingR;
		} while (child != first_child);
	}
}

static void gl_container_set_computed_alpha_dirty(gl_shape *shape_obj)
{
	if (shape_obj->data.computed_alpha_dirty) {
		return;
	}
	
	shape_obj->data.computed_alpha_dirty = TRUE;
	
	gl_container *obj = (gl_container *)shape_obj;
	
	gl_shape *first_child = obj->data.first_child;
	if (first_child) {
		gl_shape *child = first_child;
		do {
			child->f->set_computed_alpha_dirty(child);
			child = child->data.siblingR;
		} while (child != first_child);
	}
}

static void gl_container_draw(gl_shape *shape_obj)
{
	gl_container *obj = (gl_container *)shape_obj;
	
	gl_shape *first_child = obj->data.first_child;
	if (first_child) {
		gl_shape *child = first_child;
		do {
			child->f->draw(child);
			child = child->data.siblingR;
		} while (child != first_child);
	}
}
