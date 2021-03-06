//
//  gl-slide-image.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

// TODO: separate out slide functionality

#include "slideshow/gl-slide-image.h"

#include <stdlib.h>
#include <string.h>

#include "gl-image.h"
#include "infrastructure/gl-notice-subscription.h"

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_slide_image_free(gl_object *obj_obj);
static void gl_slide_image_load(gl_slide *obj);

static struct gl_slide_image_funcs gl_slide_image_funcs_global = {

};

void gl_slide_image_setup()
{
	gl_slide *parent = gl_slide_new();
	memcpy(&gl_slide_image_funcs_global.p, parent->f, sizeof(gl_slide_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_slide_funcs *slide_funcs_global = (gl_slide_funcs *) &gl_slide_image_funcs_global;
	slide_funcs_global->load = &gl_slide_image_load;
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slide_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slide_image_free;
}

static void gl_slide_image_loaded_notice(void *target, gl_notice_subscription *sub, void *action_data)
{
	gl_slide_image *obj = (gl_slide_image *)target;
	gl_image *complete_image = (gl_image *)action_data;
	
	gl_framed_shape *frame = gl_framed_shape_new();
	frame->data.shape = (gl_shape *)complete_image;
	((gl_shape *)frame)->data.objectWidth = ((gl_shape *)obj)->data.objectWidth;
	((gl_shape *)frame)->data.objectHeight = ((gl_shape *)obj)->data.objectHeight;
	
	frame->f->update_frame(frame);
	
	obj->data._slideShape = frame;
	((gl_object *)frame)->f->ref((gl_object *)frame);
	
	((gl_container *)obj)->f->append_child((gl_container *)obj, (gl_shape *)frame);
	((gl_slide *)obj)->f->set_loadstate((gl_slide *)obj, gl_slide_loadstate_ready);
}

static void gl_slide_image_failed_notice(void *target, gl_notice_subscription *sub, void *action_data)
{
	gl_slide_image *obj = (gl_slide_image *)target;
	gl_image *complete_image = (gl_image *)action_data;
	
	((gl_object *)complete_image)->f->unref((gl_object *)complete_image);
	
	((gl_slide *)obj)->f->set_loadstate((gl_slide *)obj, gl_slide_loadstate_failed);
}

static void gl_slide_image_load(gl_slide *obj_slide)
{
	gl_slide_image *obj = (gl_slide_image *)obj_slide;
	gl_image *complete_image;
	
	complete_image = gl_image_new();
	complete_image->data.desiredWidth = ((gl_shape *)obj)->data.objectWidth;
	complete_image->data.desiredHeight = ((gl_shape *)obj)->data.objectHeight;
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action_data = complete_image;
	sub->data.action = &gl_slide_image_loaded_notice;
	
	complete_image->data.readyNotice->f->subscribe(complete_image->data.readyNotice, sub);
	
	sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action_data = complete_image;
	sub->data.action = &gl_slide_image_failed_notice;
	complete_image->data.readyNotice->f->subscribe(complete_image->data.failedNotice, sub);
	
	((gl_slide *)obj)->f->set_loadstate((gl_slide *)obj, gl_slide_loadstate_loading);
	complete_image->f->load_file(complete_image, obj->data.filename);
}

gl_slide_image *gl_slide_image_init(gl_slide_image *obj)
{
	gl_slide_init((gl_slide *)obj);
	
	obj->f = &gl_slide_image_funcs_global;
	
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
		
	gl_object_free_org_global(obj_obj);
}
