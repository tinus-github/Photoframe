//
//  gl-value-animation.c
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
//

#include "gl-value-animation.h"
#include <assert.h>
#include "math.h"

#define TRUE 1
#define FALSE 0

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_value_animation_free(gl_object *obj_obj);
static void gl_value_animation_start(gl_value_animation *obj);
static void gl_value_animation_pause(gl_value_animation *obj);
static void gl_value_animation_done(gl_value_animation *obj);
static void gl_value_animation_tick(void *target, gl_renderloop_member *renderloop_member, void *action_data);
static void gl_value_animation_set_speed(gl_value_animation *obj, GLfloat speed);
static GLfloat gl_value_animation_calculate_value(gl_value_animation *obj,
						  GLfloat normalized_time_elapsed, GLfloat startValue, GLfloat endValue);
static GLfloat gl_value_animation_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed);
static void gl_value_animation_animate(gl_value_animation *obj, GLfloat normalized_time_elapsed);
static gl_value_animation *gl_value_animation_dup(gl_value_animation *obj);
static void gl_value_animation_copy(gl_value_animation *obj);

static struct gl_value_animation_funcs gl_value_animation_funcs_global = {
	.start = &gl_value_animation_start,
	.pause = &gl_value_animation_pause,
	.done = &gl_value_animation_done,
	.calculate_value = &gl_value_animation_calculate_value,
	.calculate_value_normalized = &gl_value_animation_calculate_value_normalized,
	.set_speed = &gl_value_animation_set_speed,
	.dup = &gl_value_animation_dup,
	.copy = &gl_value_animation_copy
};

static void gl_value_animation_start(gl_value_animation *obj)
{
	obj->data.renderloopMember = gl_renderloop_member_new();
	gl_renderloop_member *renderloop_member = obj->data.renderloopMember;
	gl_object *renderloop_member_obj = (gl_object *)renderloop_member;
	renderloop_member_obj->f->ref(renderloop_member_obj);
	
	struct timezone tz;
	struct timeval now_time;
	gettimeofday(&now_time, &tz);

	float time_elapsed_integral_f;
	struct timeval time_elapsed_tv;
	
	time_elapsed_tv.tv_sec = modff(obj->data.timeElapsed, &time_elapsed_integral_f);
	time_elapsed_tv.tv_usec = time_elapsed_integral_f * 1e6;
	
	timersub(&now_time, &time_elapsed_tv, &obj->data.startTime);
	
	renderloop_member->data.target = obj;
	renderloop_member->data.action = &gl_value_animation_tick;
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	renderloop->f->append_child(renderloop, gl_renderloop_phase_animate, renderloop_member);
	obj->data.isRunning = TRUE;
	
	gl_value_animation_animate(obj, 0.0);
}

static void gl_value_animation_pause(gl_value_animation *obj)
{
	struct timezone tz;
	struct timeval now_time;
	
	assert (obj->data.isRunning);
	
	gettimeofday(&now_time, &tz);
	obj->data.timeElapsed = (GLfloat)(now_time.tv_sec - obj->data.startTime.tv_sec +
					  (now_time.tv_usec - obj->data.startTime.tv_usec) * 1e-6);

	gl_renderloop_member *renderloop_member = obj->data.renderloopMember;
	assert(renderloop_member);
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	
	renderloop->f->remove_child(renderloop, renderloop_member);
	
	gl_object *renderloop_member_obj = (gl_object *)renderloop_member;
	renderloop_member_obj->f->unref(renderloop_member_obj);
	obj->data.renderloopMember = NULL;
}

static void gl_value_animation_animate(gl_value_animation *obj, GLfloat normalized_time_elapsed)
{
	GLfloat value = obj->f->calculate_value(obj, normalized_time_elapsed, obj->data.startValue, obj->data.endValue);
	
	obj->data.action(obj->data.target, obj->data.extraData, value);
}

static void gl_value_animation_tick(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	gl_value_animation *obj = (gl_value_animation *)target;
	
	struct timezone tz;
	struct timeval now_time;
	
	gettimeofday(&now_time, &tz);
	obj->data.timeElapsed = (GLfloat)(now_time.tv_sec - obj->data.startTime.tv_sec +
					(now_time.tv_usec - obj->data.startTime.tv_usec) * 1e-6);
	
	GLfloat normalized_time_elapsed = obj->data.timeElapsed / obj->data.duration;
	if (normalized_time_elapsed < 0.0) normalized_time_elapsed = 0.0;
	if (normalized_time_elapsed > 1.0) normalized_time_elapsed = 1.0;
	
	gl_value_animation_animate(obj, normalized_time_elapsed);
	
	if (obj->data.timeElapsed > obj->data.duration) {
		obj->f->done(obj);
	}
}

/* Calculates the duration to match this speed (for a linear animation) and set it */
static void gl_value_animation_set_speed(gl_value_animation *obj, GLfloat speed)
{
	GLfloat distance = obj->data.endValue - obj->data.startValue;
	if (distance < 0.0) {
		distance = -distance;
	} else if (distance == 0.0) {
		obj->data.duration = 1.0; return;
	}
	
	obj->data.duration = distance / speed;
}

static GLfloat gl_value_animation_calculate_value(gl_value_animation *obj, GLfloat normalized_time_elapsed, GLfloat startValue, GLfloat endValue)
{
	GLfloat normalized_current_value = obj->f->calculate_value_normalized(obj, normalized_time_elapsed);
	
	return startValue + (normalized_current_value * (endValue - startValue));
}

static GLfloat gl_value_animation_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed)
{
	return normalized_time_elapsed;
}

static void gl_value_animation_done(gl_value_animation *obj)
{
	struct timezone tz;

	if (obj->data.repeats) {
		obj->data.timeElapsed = 0.0;
		gettimeofday(&obj->data.startTime, &tz);
	} else {
		obj->f->pause(obj);
	}

	obj->data.animationCompleted->f->fire(obj->data.animationCompleted);
}

static void gl_value_animation_copy(gl_value_animation *source, gl_value_animation *target)
{
	assert (!source->data.isRunning);
	
	target->data.timeElapsed = source->data.timeElapsed;
	target->data.startValue = source->data.startValue;
	target->data.endValue = source->data.endValue;
	target->data.duration = source->data.duration;
	target->data.repeats = source->data.repeats;
	target->data.target = source->data.target;
	target->data.extraData = source->data.extraData;
	target->data.action = source->data.action;
	
	// Note that the notice is not copied
}

static gl_value_animation *gl_value_animation_dup(gl_value_animation *source)
{
	gl_value_animation *ret = gl_value_animation_new();
	ret->f->copy(source, ret);
	
	return ret;
}

void gl_value_animation_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_value_animation_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_value_animation_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_value_animation_free;
}

gl_value_animation *gl_value_animation_init(gl_value_animation *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_value_animation_funcs_global;
	
	obj->data.animationCompleted = gl_notice_new();
	
	return obj;
}

gl_value_animation *gl_value_animation_new()
{
	gl_value_animation *ret = calloc(1, sizeof(gl_value_animation));
	
	return gl_value_animation_init(ret);
}

void gl_value_animation_free(gl_object *obj_obj)
{
	gl_value_animation *obj = (gl_value_animation *)obj_obj;
	
	if (obj->data.animationCompleted) {
		((gl_object *)obj->data.animationCompleted)->f->unref((gl_object *)obj->data.animationCompleted);
		obj->data.animationCompleted = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}
