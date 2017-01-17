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
#include "infrastructure/gl-notice-subscription.h"
#include <assert.h>

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_slide_free(gl_object *obj_obj);
static void gl_slide_load(gl_slide *obj);
static void gl_slide_set_loadstate(gl_slide *obj, gl_slide_loadstate new_state);
static void gl_slide_enter(gl_slide *obj);
static void gl_slide_exit(gl_slide *obj);
static void gl_slide_done_entering(void *target, gl_notice_subscription *sub, void *action_data);
static void gl_slide_done_exiting(void *target, gl_notice_subscription *sub, void *action_data);

static struct gl_slide_funcs gl_slide_funcs_global = {
	.load = &gl_slide_load,
	.set_loadstate = &gl_slide_set_loadstate,
	.enter = &gl_slide_enter,
	.exit = &gl_slide_exit
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

static void gl_slide_enter(gl_slide *obj)
{
	if (obj->data._entrance_animation) {
		gl_notice_subscription *sub = gl_notice_subscription_new();
		sub->data.action = &gl_slide_done_entering;
		sub->data.target = obj;
		
		gl_notice *notice = obj->data._entrance_animation->data.animationCompleted;
		notice->f->subscribe(notice, sub);
		obj->data._entrance_animation->f->start(obj->data._entrance_animation);
	} else {
		gl_slide_done_entering(obj, NULL, NULL);
	}
}

static void gl_slide_done_entering(void *target, gl_notice_subscription *sub, void *action_data)
{
	gl_slide *obj = (gl_slide *)target;
	
	obj->f->set_loadstate(obj, gl_slide_loadstate_onscreen);
}

static void gl_slide_exit(gl_slide *obj)
{
	if (obj->data._exit_animation) {
		gl_notice_subscription *sub = gl_notice_subscription_new();
		sub->data.action = &gl_slide_done_exiting;
		sub->data.target = obj;
		
		gl_notice *notice = obj->data._exit_animation->data.animationCompleted;
		notice->f->subscribe(notice, sub);
		obj->data._exit_animation->f->start(obj->data._exit_animation);
	} else {
		gl_slide_done_exiting(obj, NULL, NULL);
	}
}

static void gl_slide_done_exiting(void *target, gl_notice_subscription *sub, void *action_data)
{
	gl_slide *obj = (gl_slide *)target;
	
	obj->f->set_loadstate(obj, gl_slide_loadstate_offscreen);
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
	
	if (obj->data._entranceAnimation) {
		((gl_object *)obj->data._entranceAnimation)->f->unref((gl_object *)obj->data._entranceAnimation);
		obj->data._entranceAnimation = NULL
	}
	if (obj->data._exitAnimation) {
		((gl_object *)obj->data._exitAnimation)->f->unref((gl_object *)obj->data._exitAnimation);
		obj->data._exitAnimation = NULL
	}
	gl_object_free_org_global(obj_obj);
}
