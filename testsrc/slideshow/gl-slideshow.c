//
//  gl-slideshow.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#include "gl-slideshow.h"
#include "infrastructure/gl-notice-subscription.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"
#include "gl-value-animation-easing.h"
#include "gl-stage.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef enum  {
	gl_slideshow_transition_appear = 0,
	gl_slideshow_transition_fade,
	gl_slideshow_transition_fade_through_black,
	gl_slideshow_transition_swipe
} gl_slideshow_transition_type;

static void gl_slideshow_set_entrance_animation(gl_slideshow *obj, gl_value_animation *animation);
static void gl_slideshow_set_exit_animation(gl_slideshow *obj, gl_value_animation *animation);
static void gl_slideshow_free(gl_object *obj_obj);
static void gl_slideshow_engine(gl_slideshow *obj);
static void gl_slideshow_slide_load(gl_slide *obj_slide);
static void gl_slideshow_slide_enter(gl_slide *obj_slide);
static void gl_slideshow_slide_exit(gl_slide *obj_slide);
static void gl_slideshow_slide_set_loadstate(gl_slide *obj, gl_slide_loadstate new_state);
static void gl_slideshow_set_configuration(gl_slideshow *obj, gl_config_section *config);

static struct gl_slideshow_funcs gl_slideshow_funcs_global = {
	.set_entrance_animation = &gl_slideshow_set_entrance_animation,
	.set_exit_animation = &gl_slideshow_set_exit_animation,
	.set_configuration = &gl_slideshow_set_configuration,
};

static void (*gl_object_free_org_global) (gl_object *obj);
static void (*gl_slide_enter_org_global) (gl_slide *obj);
static void (*gl_slide_exit_org_global) (gl_slide *obj);
static void (*gl_slide_set_loadstate_org_global) (gl_slide *obj, gl_slide_loadstate new_state);

static void animate_alpha(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)extra_data;
	
	image_shape->data.alpha = value;
	image_shape->f->set_computed_alpha_dirty(image_shape);
}

static void animate_position(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)extra_data;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	GLfloat screenWidth = global_stage->data.width;
	
	image_shape->data.objectX = value * screenWidth;
	image_shape->f->set_computed_projection_dirty(image_shape);
}

static void animate_nothing(void *target, void *extra_data, GLfloat value)
{
	return;
}

static void set_transition_animations_for_type(gl_slideshow *obj, gl_slideshow_transition_type type)
{
	gl_value_animation *animation;
	gl_value_animation_easing *animation_e;
	
	gl_config_value *value;
	GLfloat duration = 0.4;
	
	switch (type) {
		case gl_slideshow_transition_fade:
		case gl_slideshow_transition_fade_through_black:
		case gl_slideshow_transition_swipe:
			value = obj->data._configuration->f->get_value(obj->data._configuration, "transitionDuration");
			if (value) {
				int durationMS = value->f->get_value_int(value);
				
				if (durationMS != GL_CONFIG_VALUE_NOT_FOUND) {
					duration = 0.001 * durationMS;
				}
			}
			break;
		default:
			break;
	}
	
	switch (type) {
		default:
		case gl_slideshow_transition_appear:
			animation = gl_value_animation_new();
			animation->data.startValue = 0.0;
			animation->data.endValue = 1.0;
			animation->f->set_duration(animation, 0.0);
			animation->data.action = &animate_nothing;
			
			obj->f->set_entrance_animation(obj, animation);
			
			animation = gl_value_animation_new();
			animation->data.startValue = 0.0;
			animation->data.endValue = 1.0;
			animation->f->set_duration(animation, 0.0);
			animation->data.action = &animate_nothing;
			
			obj->f->set_exit_animation(obj, animation);
			
			break;
		case gl_slideshow_transition_fade:
			animation = gl_value_animation_new();
			animation->data.startValue = 0.0;
			animation->data.endValue = 1.0;
			animation->f->set_duration(animation, duration);
			animation->data.action = &animate_alpha;
			
			obj->f->set_entrance_animation(obj, animation);
			
			animation = gl_value_animation_new();
			animation->data.startValue = 0.0;
			animation->data.endValue = 1.0;
			animation->f->set_duration(animation, duration);
			animation->data.action = &animate_nothing;
			
			obj->f->set_exit_animation(obj, animation);
			break;
		case gl_slideshow_transition_fade_through_black:
			animation = gl_value_animation_new();
			animation->data.startValue = 1.0;
			animation->data.endValue = 0.0;
			animation->f->set_duration(animation, 0.5 * duration);
			animation->data.action = &animate_alpha;
			
			obj->f->set_exit_animation(obj, animation);
			
			animation = gl_value_animation_new();
			animation->data.startValue = 0.0;
			animation->data.endValue = 1.0;
			animation->data.startDelay = 0.5 * duration;
			animation->f->set_duration(animation, 0.5 * duration);
			animation->data.action = &animate_alpha;
			
			obj->f->set_entrance_animation(obj, animation);
			break;
		case gl_slideshow_transition_swipe:
			animation_e = gl_value_animation_easing_new();
			animation_e->data.easingType = gl_value_animation_ease_FastOutSlowIn;
			animation = (gl_value_animation *)animation_e;
			animation->data.startValue = 1.0;
			animation->data.endValue = 0.0;
			animation->f->set_duration(animation, duration);
			animation->data.action = &animate_position;
			
			obj->f->set_entrance_animation(obj, animation);
			
			animation_e = gl_value_animation_easing_new();
			animation_e->data.easingType = gl_value_animation_ease_FastOutSlowIn;
			animation = (gl_value_animation *)animation_e;
			animation->data.startValue = 0.0;
			animation->data.endValue = -1.0;
			animation->f->set_duration(animation, duration);
			animation->data.action = &animate_position;
			
			obj->f->set_exit_animation(obj, animation);
			break;
	}
}

static void gl_slideshow_set_configuration(gl_slideshow *obj, gl_config_section *config)
{
	// TODO: Check if already running? If so, what?
	
	if (obj->data._configuration) {
		((gl_object *)obj->data._configuration)->f->unref((gl_object *)obj->data._configuration);
		
		obj->data._configuration = NULL;
	}
	
	obj->data._configuration = config;
	((gl_object *)config)->f->ref((gl_object *)config);

	gl_config_value *value = config->f->get_value(config, "duration");
	if (value) {
		int durationMS = value->f->get_value_int(value);
		if (durationMS != GL_CONFIG_VALUE_NOT_FOUND) {
			obj->data.slideDuration = .001 * durationMS;
		}
		value = NULL;
	}
	
	value = config->f->get_value(config, "transition");
	
	static gl_config_value_selection transitions[] = {
		{"fade",	gl_slideshow_transition_fade},
		{"fadethrough", gl_slideshow_transition_fade_through_black},
		{"swipe", 	gl_slideshow_transition_swipe},
		NULL };
	
	if (value) {
		uint32_t transition_type_int = value->f->get_value_string_selection(value, transitions);
		
		if (transition_type_int == GL_CONFIG_VALUE_SELECTION_NOT_FOUND) {
			transition_type_int = gl_slideshow_transition_appear;
		}
		
		set_transition_animations_for_type(obj, (gl_slideshow_transition_type) transition_type_int);
		
		value = NULL;
	}
}

static void gl_slideshow_set_entrance_animation(gl_slideshow *obj, gl_value_animation *animation)
{
	if (obj->data._slideEntranceAnimation) {
		((gl_object *)obj->data._slideEntranceAnimation)->f->unref((gl_object *)obj->data._slideEntranceAnimation);
	}
	obj->data._slideEntranceAnimation = animation;
}

static void gl_slideshow_set_exit_animation(gl_slideshow *obj, gl_value_animation *animation)
{
	if (obj->data._slideExitAnimation) {
		((gl_object *)obj->data._slideExitAnimation)->f->unref((gl_object *)obj->data._slideExitAnimation);
	}
	obj->data._slideExitAnimation = animation;
}

static void gl_slideshow_engine_notice(void *target, gl_notice_subscription *sub, void *extra_data)
{
	gl_slideshow *obj = (gl_slideshow *)target;
	
	gl_slideshow_engine(obj);
}

static void gl_slideshow_engine_get_new_slide(gl_slideshow *obj);

static void gl_slideshow_engine_retry_action(void *obj_obj, gl_renderloop_member *member, void *action_data)
{
	gl_slideshow *obj = (gl_slideshow *)obj_obj;
	gl_slideshow_engine_get_new_slide(obj);
}

static void gl_slideshow_engine_get_new_slide(gl_slideshow *obj)
{
	assert (!obj->data._incomingSlide);
	
	gl_slide *newSlide = obj->data.getNextSlideCallback(obj->data.callbackTarget,
							    obj->data.callbackExtraData);
	if (!newSlide) {
		gl_renderloop_member *m = gl_renderloop_member_new();
		m->data.action = &gl_slideshow_engine_retry_action;
		m->data.target = obj;
		
		gl_renderloop *global_renderloop = gl_renderloop_get_global_renderloop();
		global_renderloop->f->append_child(global_renderloop, gl_renderloop_phase_load, m);
		return;
	}
	
	obj->data._incomingSlide = newSlide;
	((gl_container *)obj)->f->append_child((gl_container *)obj, (gl_shape *)newSlide);
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action = &gl_slideshow_engine_notice;
	
	newSlide->data.loadstateChanged->f->subscribe(newSlide->data.loadstateChanged, sub);
	
	// add animations
	gl_value_animation *new_animation;
	if (obj->data._slideEntranceAnimation) {
 		new_animation = obj->data._slideEntranceAnimation->f->dup(obj->data._slideEntranceAnimation);
		new_animation->data.extraData = newSlide;
		newSlide->f->set_entrance_animation(newSlide, new_animation);
	}
	
	if (obj->data._slideExitAnimation) {
		new_animation = obj->data._slideExitAnimation->f->dup(obj->data._slideExitAnimation);
		new_animation->data.extraData = newSlide;
		newSlide->f->set_exit_animation(newSlide, new_animation);
	}
	
	newSlide->f->load(newSlide);
}

static void gl_slideshow_engine_slide_done(void *target, gl_notice_subscription *sub, void *extra_data)
{
	gl_slideshow *obj = (gl_slideshow *)target;
	
	obj->data._timerStatus = gl_slideshow_timer_status_done;
	
	gl_slideshow_engine(obj);
}

// call this when the state changes and it'll decide what to do
static void gl_slideshow_engine(gl_slideshow *obj)
{
	if (obj->data._incomingSlide &&
	    (obj->data._incomingSlide->data.loadstate == gl_slide_loadstate_failed)) {
		gl_slide *incomingSlide = obj->data._incomingSlide;
		((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)incomingSlide);
		obj->data._incomingSlide = NULL;
		
		// fallthrough: try again
	}
	
	if (!obj->data._isRunning) {
		if (obj->data._incomingSlide) {
			gl_slide *incomingSlide = obj->data._incomingSlide;
			
			if (incomingSlide->data.loadstate == gl_slide_loadstate_ready) {
				incomingSlide->f->enter(incomingSlide); // should be immediate as we override the duration to 0
			} else if (incomingSlide->data.loadstate == gl_slide_loadstate_onscreen) {
				((gl_slide *)obj)->f->set_loadstate((gl_slide *)obj, gl_slide_loadstate_ready);
			}
		} else {
			gl_slideshow_engine_get_new_slide(obj);
		}
		return;
	}
	
	if (!obj->data._incomingSlide) {
		gl_slideshow_engine_get_new_slide(obj);
	} else {
		gl_slide *incomingSlide = obj->data._incomingSlide;
		
		if (incomingSlide->data.loadstate == gl_slide_loadstate_ready) {
			if ((!obj->data.slideDuration) ||
			    (!obj->data._currentSlide) ||
			    (obj->data._timerStatus == gl_slideshow_timer_status_done)) {
				incomingSlide->f->enter(incomingSlide);
				if (obj->data._currentSlide) {
					obj->data._currentSlide->f->exit(obj->data._currentSlide);
				}
			}
		} else if (incomingSlide->data.loadstate == gl_slide_loadstate_onscreen) {
			gl_slide *currentSlide = obj->data._currentSlide;
			if (!currentSlide || (currentSlide->data.loadstate == gl_slide_loadstate_offscreen)) {
				if (currentSlide) {
					((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)currentSlide);
				}
				obj->data._currentSlide = incomingSlide;
				obj->data._incomingSlide = NULL;
				
				gl_slideshow_engine_get_new_slide(obj);
				
				if (obj->data._onScreenTimer) {
					((gl_object *)obj->data._onScreenTimer)->f->unref((gl_object *)obj->data._onScreenTimer);
					obj->data._onScreenTimer = NULL;
				}
				obj->data._timerStatus = gl_slideshow_timer_status_notstarted;
				
				if (obj->data.slideDuration) {
					
					gl_value_animation *durationTimer = gl_value_animation_new();
					durationTimer->data.startValue = 0.0;
					durationTimer->data.endValue = 1.0;
					durationTimer->f->set_duration(durationTimer, obj->data.slideDuration);
					durationTimer->data.action = &animate_nothing;
					
					gl_notice_subscription *sub = gl_notice_subscription_new();
					sub->data.target = obj;
					sub->data.action = &gl_slideshow_engine_slide_done;
					
					durationTimer->data.animationCompleted->f->subscribe(durationTimer->data.animationCompleted, sub);
					
					obj->data._onScreenTimer = durationTimer;
					obj->data._timerStatus = gl_slideshow_timer_status_running;
					durationTimer->f->start(durationTimer);
				}
			}
		}
	}
}

static void gl_slideshow_slide_load(gl_slide *obj_slide)
{
	gl_slideshow *obj = (gl_slideshow *)obj_slide;
	
	assert (!obj->data._incomingSlide);
	
	gl_slideshow_engine_get_new_slide(obj);
	
	obj_slide->f->set_loadstate(obj_slide, gl_slide_loadstate_loading);
	
	gl_slide *newSlide = obj->data._incomingSlide;
	if (newSlide) {
		gl_value_animation *slideAnimation = newSlide->f->get_entrance_animation(newSlide);
		
		slideAnimation->data.startDelay = 0.0;
		slideAnimation->f->set_duration(slideAnimation, 0); // So the slide is ready immediately
		
		newSlide->f->set_entrance_animation(newSlide, slideAnimation); // this releases our reference to the animation
	}
}

static void gl_slideshow_slide_enter(gl_slide *obj_slide)
{
	gl_slideshow *obj = (gl_slideshow *)obj_slide;
	
	obj->data._isRunning = 1;
	
	gl_slide_enter_org_global(obj_slide);
}

static void gl_slideshow_slide_exit(gl_slide *obj_slide)
{
	gl_slideshow *obj = (gl_slideshow *)obj_slide;
	
	obj->data._isRunning = 0;
	
	gl_slide_exit_org_global(obj_slide);
}

static void gl_slideshow_slide_set_loadstate(gl_slide *obj, gl_slide_loadstate new_state)
{
	gl_slide_set_loadstate_org_global(obj, new_state);
	
	gl_slideshow_engine((gl_slideshow *)obj);
}

void gl_slideshow_setup()
{
	gl_slide *parent = gl_slide_new();
	memcpy(&gl_slideshow_funcs_global.p, parent->f, sizeof(gl_slide_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slideshow_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slideshow_free;
	
	gl_slide_funcs *slide_funcs_global = (gl_slide_funcs *) &gl_slideshow_funcs_global;
	gl_slide_enter_org_global = slide_funcs_global->enter;
	slide_funcs_global->enter = &gl_slideshow_slide_enter;
	// The original of load is abstract
	slide_funcs_global->load = &gl_slideshow_slide_load;
	gl_slide_exit_org_global = slide_funcs_global->exit;
	slide_funcs_global->exit = &gl_slideshow_slide_exit;
	
	gl_slide_set_loadstate_org_global = slide_funcs_global->set_loadstate;
	slide_funcs_global->set_loadstate = &gl_slideshow_slide_set_loadstate;
}

gl_slideshow *gl_slideshow_init(gl_slideshow *obj)
{
	gl_slide_init((gl_slide *)obj);
	
	obj->f = &gl_slideshow_funcs_global;
	
	obj->data.slideDuration = 3;
	
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
	
	if (obj->data._slideEntranceAnimation) {
		((gl_object *)obj->data._slideEntranceAnimation)->f->unref((gl_object *)obj->data._slideEntranceAnimation);
		obj->data._slideEntranceAnimation = NULL;
	}
	if (obj->data._slideExitAnimation) {
		((gl_object *)obj->data._slideExitAnimation)->f->unref((gl_object *)obj->data._slideExitAnimation);
		obj->data._slideExitAnimation = NULL;
	}
	
	gl_slide *slide = obj->data._currentSlide;
	if (slide) {
		((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)slide);
	}
	obj->data._currentSlide = NULL;

	slide = obj->data._incomingSlide;
	if (slide) {
		((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)slide);
	}
	obj->data._incomingSlide = NULL;
	
	if (obj->data._configuration) {
		((gl_object *)obj->data._configuration)->f->unref((gl_object *)obj->data._configuration);
		
		obj->data._configuration = NULL;
	}
	
	if (obj->data._onScreenTimer) {
		obj->data._onScreenTimer->f->pause(obj->data._onScreenTimer);
		((gl_object *)obj->data._onScreenTimer)->f->unref((gl_object *)obj->data._onScreenTimer);
		
		obj->data._onScreenTimer = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}
