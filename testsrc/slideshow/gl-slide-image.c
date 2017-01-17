//
//  gl-slide-image.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

// TODO: separate out slide functionality

#include "slideshow/gl-slide-image.h"
#include "gl-image.h"
#include "gl-stage.h"
#include "infrastructure/gl-notice-subscription.h"

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_slide_image_free(gl_object *obj_obj);

static struct gl_slide_image_funcs gl_slide_image_funcs_global = {
	.load = &gl_slide_image_load
};

void gl_slide_image_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_slide_image_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slide_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slide_image_free;
}

static void gl_slide_image_set_loadstate(gl_slide_image *obj, gl_slide_loadstate new_state)
{
	obj->data.loadstate = new_state;
	
	obj->data.loadstateChanged->f->fire(obj->data.loadstateChanged);
}

static void gl_slide_image_loaded_notice(void *action, gl_notice_subscription *sub, void *action_data)
{
	gl_slide_image *obj = (gl_slide_image *)action;
	gl_image *complete_image = =(gl_image *)action_data;
	
	gl_framed_shape *frame = gl_framed_shape_new();
	frame->data.shape = (gl_shape *)complete_image;
	((gl_shape *)frame)->data.objectWidth = ((gl_shape *)obj)->data.objectWidth;
	((gl_shape *)frame)->data.objectHeight = ((gl_shape *)obj)->data.objectHeight;
	
	frame->f->update_frame(frame);
	
	obj->data._slideShape = frame;
	((gl_object *)frame)->f->ref((gl_object *)frame);
	
	((gl_container *)obj)->f->append_child((gl_container *)obj, (gl_shape *)frame);
	gl_slide_image_set_loadstate(obj, gl_slide_loadstate_ready);
}

static void gl_slide_image_load(gl_slide_image *obj)
{
	gl_image *complete_image;
	
	complete_image = gl_image_new();
	complete_image->data.desiredWidth = ((gl_shape *)obj)->data.objectWidth;
	complete_image->data.desiredHeight = ((gl_shape *)obj)->data.objectHeight;
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action_data = complete_image;
	sub->data.action = &gl_slide_image_loaded_notice;
	
	complete_image->data.readyNotice->f->subscribe(complete_image->data.readyNotice, sub);
	
	gl_slide_image_set_loadstate(obj, gl_slide_loadstate_loading);
	complete_image->f->load_file(complete_image, obj->data.filename);
}

gl_slide_image *gl_slide_image_init(gl_slide_image *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_slide_image_funcs_global;
	
	obj->data.loadstateChanged = gl_notice_new();
	
	gl_shape *obj_shape = (gl_shape *)obj;
	gl_stage *stage = gl_stage_get_global_stage();
	obj_shape->data.objectWidth = stage->data.width;
	obj_shape->data.objectHeight = stage->data.height;
	
	return obj;
}

gl_slide_image *gl_slide_image_new()
{
	gl_slide_image *ret = calloc(1, sizeof(gl_slide_image));
	
	return gl_slide_image_init(ret);
}

static void gl_slide_image_free(gl_object *obj_obj)
{
	gl_slide_image *obj = (gl_slide_image *)obj_obj;
	
	free (obj->data.filename);
	obj->data.filename = NULL;
	
	if (obj->data._slideShape) {
		((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)obj->data._slideShape);
		((gl_object *)obj->data._slideShape)->f->unref((gl_object *)obj->data._slideShape);
		obj->data._slideShape = NULL;
	}
	
	((gl_object *)obj->data.loadstateChanged)->f->unref((gl_object *)obj->data.loadstateChanged);
	obj->data.loadstateChanged = NULL;
	
	gl_object_free_org_global(obj_obj);
}
