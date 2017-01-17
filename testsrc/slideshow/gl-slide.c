//
//  gl-slide.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#include "gl-slide.h"

#include "gl-stage.h"
#include "infrastructure/gl-notice.h"
#include <assert.h>

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_slide_free(gl_object *obj_obj);
static void gl_slide_load(gl_slide *obj);
static void gl_slide_set_loadstate(gl_slide *obj, gl_slide_loadstate new_state);

static struct gl_slide_funcs gl_slide_funcs_global = {
	.load = &gl_slide_load,
	.set_loadstate = &gl_slide_set_loadstate
};

void gl_slide_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_slide_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slide_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slide_free;
}

static void gl_slide_set_loadstate(gl_slide *obj, gl_slide_loadstate new_state)
{
	obj->data.loadstate = new_state;
	
	obj->data.loadstateChanged->f->fire(obj->data.loadstateChanged);
}

static void gl_slide_load(gl_slide *obj)
{
	assert(!"This is an abstract function");
}

gl_slide *gl_slide_init(gl_slide *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_slide_funcs_global;
	
	obj->data.loadstateChanged = gl_notice_new();
	
	gl_shape *obj_shape = (gl_shape *)obj;
	gl_stage *stage = gl_stage_get_global_stage();
	obj_shape->data.objectWidth = stage->data.width;
	obj_shape->data.objectHeight = stage->data.height;
	
	return obj;
}

gl_slide *gl_slide_new()
{
	gl_slide *ret = calloc(1, sizeof(gl_slide));
	
	return gl_slide_init(ret);
}

static void gl_slide_free(gl_object *obj_obj)
{
	gl_slide *obj = (gl_slide *)obj_obj;
	
	((gl_object *)obj->data.loadstateChanged)->f->unref((gl_object *)obj->data.loadstateChanged);
	obj->data.loadstateChanged = NULL;
	
	gl_object_free_org_global(obj_obj);
}
