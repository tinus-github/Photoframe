//
//  gl-renderloop.c
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#include "gl-renderloop.h"
#include "gl-renderloop-member.h"
#include <string.h>
#include <assert.h>

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child);
static void gl_renderloop_remove_child(gl_renderloop *obj, gl_renderloop_member *child);

static struct gl_renderloop_funcs gl_renderloop_funcs_global = {
	.append_child = &gl_renderloop_append_child,
	.remove_child = &gl_renderloop_remove_child
};

void gl_renderloop_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_renderloop_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_renderloop *gl_renderloop_init(gl_renderloop *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_renderloop_funcs_global;
	
	return obj;
}

gl_renderloop *gl_renderloop_new()
{
	gl_renderloop *ret = calloc(1, sizeof(gl_renderloop));
	
	return gl_renderloop_init(ret);
}

static void gl_renderloop_remove_child(gl_renderloop *obj, gl_renderloop_member *child)
{
	assert (child->data.owner == obj);
	gl_object *obj_child = (gl_object *)child;
	gl_renderloop_phase phase = child->data.renderloopPhase;
	
	if (child->data.siblingL == child) {
		child->data.siblingL = NULL;
		child->data.siblingR = NULL;
		obj->data.phaseFirstChild[phase] = NULL;
	} else {
		if (obj->data.phaseFirstChild[phase] == child) {
			obj->data.phaseFirstChild[phase] = child->data.siblingR;
		}
		
		gl_renderloop_member *siblingR = child->data.siblingR;
		gl_renderloop_member *siblingL = child->data.siblingL;
		
		siblingR->data.siblingL = child->data.siblingL;
		siblingL->data.siblingR = child->data.siblingR;
	}
	
	child->data.owner = NULL;
	obj_child->f->unref(obj_child);
}

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child)
{
	gl_object *obj_child = (gl_object *)child;
	obj_child->f->ref(obj_child); // Prevent the child from being deallocated as it is being removed from the parent

	if (child->data.owner) {
		gl_renderloop *owner = child->data.owner;
		owner->f->remove_child(owner, child);
	}
	
	child->data.owner = obj;
	child->data.renderloopPhase = phase;
	
	if (!obj->data.phaseFirstChild[phase]) {
		obj->data.phaseFirstChild[phase] = child;
		child->data.siblingL = child;
		child->data.siblingR = child;
	} else {
		gl_renderloop_member *first_child = obj->data.phaseFirstChild[phase];
		gl_renderloop_member *last_child = first_child->data.siblingL;
		child->data.siblingL = last_child;
		last_child->data.siblingR = child;
		child->data.siblingR = first_child;
		first_child->data.siblingL = child;
	}
	obj_child->f->unref(obj_child);
}

static void gl_renderloop_run_phase(gl_renderloop *obj, gl_renderloop_phase phase)
{
	unsigned int done = 0;
	unsigned int is_final = 0;
	
	unsigned int restarted = 0;
	gl_object *current_child_object;
	gl_object *next_child_object;
	
	if (!obj->data.phaseFirstChild[phase]) {
		return;
	}
	
	gl_renderloop_member *current_child = obj->data.phaseFirstChild[phase];
	
	while (!done) {
		assert (current_child->data.owner = obj);
		gl_renderloop_member *next_child = current_child->siblingR;
		
		current_child_object = (gl_object *)current_child;
		current_child_object->f->ref(current_child_object);
		
		if (next_child == obj->data.phaseFirstChild[phase]) {
			is_final = 1;
		}
		
		current_child->f->run_action(current_child);
		if (current_child->data.owner == obj) {
			assert (next_child == current_child->siblingR);
		} else {
			if (next_child == current_child) {
				done = 1;
			}
		}
		
		current_child_obj->f->unref(current_child_obj);

		if (is_final || (!obj->data.phaseFirstChild[phase])) {
			done = 1;
		} else {
			current_child = next_child;
		}
	}
}
