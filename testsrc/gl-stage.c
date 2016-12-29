//
//  gl-stage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/12/2016.
//
//
//  The stage contains basic information on the size of the screen and other information required for rendering
#include <stdlib.h>
#include <string.h>

#include "gl-stage.h"
#include "gl-renderloop-member.h"

#include "../lib/linmath/linmath.h"

static gl_stage *global_stage = NULL;


static void gl_stage_set_dimensions(gl_stage *obj, uint32_t width, uint32_t height);
static void gl_stage_set_shape(gl_stage *obj, gl_shape *shape);
static gl_shape *gl_stage_get_shape(gl_stage *obj);

static struct gl_stage_funcs gl_stage_funcs_global = {
	.set_dimensions = &gl_stage_set_dimensions,
	.set_shape = &gl_stage_set_shape,
	.get_shape = &gl_stage_get_shape
};

static void gl_stage_set_projection(gl_stage *obj)
{
	mat4x4 projection;
	mat4x4 projection_scaled;
	mat4x4 translation;
	
	mat4x4_identity(projection);
	mat4x4_scale_aniso(projection_scaled, projection,
			   2.0/obj->data.width,
			   -2.0/obj->data.height,
			   1.0);
	mat4x4_translate(translation, -1, 1, 0);
	mat4x4_mul(obj->data.projection, translation, projection_scaled);
}

static void gl_stage_set_shape(gl_stage *obj, gl_shape *shape)
{
	if (obj->data.shape) {
		gl_object *org_obj = (gl_object *)obj->data.shape;
		obj->data.shape = NULL;
		org_obj->f->unref(org_obj);
	}
	
	obj->data.shape = shape;
	shape->f->set_computed_projection_dirty(shape);
}

static gl_shape *gl_stage_get_shape(gl_stage *obj)
{
	return obj->data.shape;
}

static void gl_stage_set_dimensions(gl_stage *obj, uint32_t width, uint32_t height)
{
	obj->data.width = width;
	obj->data.height = height;
	
	gl_stage_set_projection(obj);
}

static void gl_stage_draw(void *global_stage_void, gl_renderloop_member *obj, void *extra_data)
{
	gl_stage *global_stage = (gl_stage *)global_stage_void;
	gl_shape *main_container_shape = global_stage->f->get_shape(global_stage);
	main_container_shape->f->draw(main_container_shape);
}

void gl_stage_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_stage_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	global_stage = gl_stage_new();
	
	gl_renderloop_setup();
	gl_renderloop_member_setup();
	
	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &gl_stage_draw;
	renderloop_member->data.target = global_stage;
	renderloop_member->data.action_data = NULL;
	
	gl_renderloop *global_loop = gl_renderloop_get_global_renderloop();
	global_loop->f->append_child(global_loop, gl_renderloop_phase_draw, renderloop_member);
}

gl_stage *gl_stage_init(gl_stage *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_stage_funcs_global;
	
	return obj;
}

gl_stage *gl_stage_new()
{
	gl_stage *ret = calloc(1, sizeof(gl_stage));
	
	return gl_stage_init(ret);
}

gl_stage *gl_stage_get_global_stage()
{
	return global_stage;
}
