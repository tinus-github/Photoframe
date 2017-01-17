//
//  gl-slideshow.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#include "gl-slideshow.h"
#include "infrastructure/gl-notice-subscription.h"
#include <assert.h>

static void gl_slideshow_set_entrance_animation(gl_slideshow *obj, gl_value_animation *animation);
static void gl_slideshow_set_exit_animation(gl_slideshow *obj, gl_value_animation *animation);
static void gl_slideshow_free(gl_object *obj_obj);
void gl_slideshow_engine(gl_slideshow *obj);

static struct gl_slideshow_funcs gl_slideshow_funcs_global = {
	.set_entrance_animation = &gl_slideshow_set_entrance_animation,
	.set_exit_animation = &gl_slideshow_set_exit_animation,
	.start = &gl_slideshow_engine
};

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_slideshow_set_entrance_animation(gl_slideshow *obj, gl_value_animation *animation)
{
	if (obj->data._entranceAnimation) {
		((gl_object *)obj->data._entranceAnimation)->f->unref((gl_object *)obj->data._entranceAnimation);
	}
	obj->data._entranceAnimation = animation;
}

static void gl_slideshow_set_exit_animation(gl_slideshow *obj, gl_value_animation *animation)
{
	if (obj->data._exitAnimation) {
		((gl_object *)obj->data._exitAnimation)->f->unref((gl_object *)obj->data._exitAnimation);
	}
	obj->data._exitAnimation = animation;
}

void gl_slideshow_engine_notice(void *target, gl_notice_subscription *sub, void *extra_data)
{
	gl_slideshow *obj = (gl_slideshow *)target;
	
	gl_slideshow_engine(obj);
}

void gl_slideshow_engine_get_new_slide(gl_slideshow *obj)
{
	assert (!obj->data._incomingSlide);
	
	gl_slide *newSlide = obj->data.getNextSlideCallback(obj->data.callbackTarget,
							    obj->data.callbackExtraData);
	obj->data._incomingSlide = newSlide;
	((gl_container *)obj)->f->append_child((gl_container *)obj, (gl_shape *)newSlide);
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action = &gl_slideshow_engine_notice;
	
	newSlide->data.loadstateChanged->f->subscribe(newSlide->data.loadstateChanged, sub);
	
	// add animations
	gl_value_animation *new_animation;
	if (obj->data._entranceAnimation) {
 		new_animation = obj->data._entranceAnimation->f->dup(obj->data._entranceAnimation);
		newSlide->f->set_entrance_animation(newSlide, new_animation);
	}
	
	if (obj->data._exitAnimation) {
		new_animation = obj->data._exitAnimation->f->dup(obj->data._exitAnimation);
		newSlide->f->set_exit_animation(newSlide, new_animation);
	}
	
	newSlide->f->load(newSlide);
}

void gl_slideshow_engine(gl_slideshow *obj)
{
	// call this when the state changes and it'll decide what to do
	
	if (!obj->data._incomingSlide) {
		gl_slideshow_engine_get_new_slide(obj);
	} else {
		gl_slide *incomingSlide = obj->data._incomingSlide;
		
		if (incomingSlide->data.loadstate == gl_slide_loadstate_ready) {
			// if it's time to switch {
			incomingSlide->f->enter(incomingSlide);
			if (obj->data._currentSlide) {
				obj->data._currentSlide->f->exit(obj->data._currentSlide);
			}
			//}
		} else if (incomingSlide->data.loadstate == gl_slide_loadstate_onscreen) {
			gl_slide *currentSlide = obj->data._currentSlide;
			if (!currentSlide || (currentSlide->data.loadstate == gl_slide_loadstate_offscreen)) {
				if (currentSlide) {
					((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)currentSlide);
				}
				obj->data._currentSlide = incomingSlide;
				obj->data._incomingSlide = NULL;
				
				gl_slideshow_engine_get_new_slide(obj);
			}
		}
	}
}

void gl_slideshow_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_slideshow_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slideshow_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slideshow_free;
}

gl_slideshow *gl_slideshow_init(gl_slideshow *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_slideshow_funcs_global;
	
	return obj;
}

gl_slideshow *gl_slideshow_new()
{
	gl_slideshow *ret = calloc(1, sizeof(gl_slideshow));
	
	return gl_slideshow_init(ret);
}

static void gl_slideshow_free(gl_object *obj_obj)
{
	gl_slideshow *obj = (gl_slideshow *)obj_obj;
	
	if (obj->data._entranceAnimation) {
		((gl_object *)obj->data._entranceAnimation)->f->unref((gl_object *)obj->data._entranceAnimation);
		obj->data._entranceAnimation = NULL;
	}
	if (obj->data._exitAnimation) {
		((gl_object *)obj->data._exitAnimation)->f->unref((gl_object *)obj->data._exitAnimation);
		obj->data._exitAnimation = NULL;
	}
	
	
	//TODO: Remove slides
	
	gl_object_free_org_global(obj_obj);
}
