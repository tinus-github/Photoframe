//
//  gl-timer.c
//  Photoframe
//
//  Created by Martijn Vernooij on 24/04/2017.
//
//

#include "infrastructure/gl-timer.h"
#include "infrastructure/gl-timer-queue.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static void gl_timer_free(gl_object *obj_obj);
static void gl_timer_start(gl_timer *obj);
static void gl_timer_pause(gl_timer *obj);
static void gl_timer_stop(gl_timer *obj);
static void gl_timer_trigger(gl_timer *obj);
static void gl_timer_set_duration(gl_timer *obj, float duration);

static struct gl_timer_funcs gl_timer_funcs_global = {
	.start = &gl_timer_start,
	.pause = &gl_timer_pause,
	.stop = &gl_timer_stop,
	._trigger = &gl_timer_trigger,
	.set_duration = &gl_timer_set_duration,
};

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_timer_start(gl_timer *obj)
{
	if (obj->data.state == gl_timer_state_running) {
		return;
	}
	
	if (obj->data.state == gl_timer_state_completed) {
		obj->data.state = gl_timer_state_stopped;
	}
	
	struct timezone tz;
	struct timeval nowTime;
	
	gettimeofday(&nowTime, &tz);
	
	float duration = obj->data._duration;
	float duration_integral;
	
	struct timeval duration_tv;
	
	if (obj->data.state == gl_timer_state_paused) {
		duration -= obj->data._elapsed;
	}
	
	duration_tv.tv_usec = modff(duration, &duration_integral) * 1e6;
	duration_tv.tv_sec = duration_integral;
	
	timeradd(&nowTime, &duration_tv, &obj->data._endTime);
	
	gl_timer_queue *global_queue = gl_timer_queue_get_global_timer_queue();
	
	((gl_object *)obj)->f->ref((gl_object *)obj);
	// otherwise add_timer will take the reference and this object will
	// be deleted on pause/stop/trigger
	global_queue->f->add_timer(global_queue, obj);
}

static void gl_timer_pause(gl_timer *obj)
{
	if (obj->data.state != gl_timer_state_running) {
		return;
	}
	
	struct timezone tz;
	struct timeval nowTime;
	struct timeval timeRemainingTV;
	
	gettimeofday(&nowTime, &tz);
	
	timersub(&obj->data._endTime, &nowTime, &timeRemainingTV);
	
	float timeRemaining = timeRemainingTV.tv_sec + (timeRemainingTV.tv_usec * 1e-6);
	
	obj->data._elapsed = obj->data._duration - timeRemaining;

	gl_timer_queue *global_queue = gl_timer_queue_get_global_timer_queue();
	global_queue->f->remove_timer(global_queue, obj);
	
	obj->data.state = gl_timer_state_paused;
}

static void gl_timer_stop(gl_timer *obj)
{
	if (obj->data.state == gl_timer_state_running) {
		gl_timer_queue *global_queue = gl_timer_queue_get_global_timer_queue();
		global_queue->f->remove_timer(global_queue, obj);
	}
	obj->data.state = gl_timer_state_stopped;
}

static void gl_timer_trigger(gl_timer *obj)
{
	obj->data.action(obj->data.target, obj->data.extraData);
	
	// now we're done with it
	((gl_object *)obj)->f->unref((gl_object *)obj);
}

static void gl_timer_set_duration(gl_timer *obj, float duration)
{
	int was_running = 0;
	if (obj->data.state == gl_timer_state_running) {
		was_running = 1;
		obj->f->pause(obj);
	}
	
	obj->data._duration = duration;
	
	if (was_running) {
		obj->f->start(obj);
	}
}

void gl_timer_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_timer_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_timer_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_timer_free;
}

gl_timer *gl_timer_init(gl_timer *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_timer_funcs_global;
	
	return obj;
}

gl_timer *gl_timer_new()
{
	gl_timer *ret = calloc(1, sizeof(gl_timer));
	
	return gl_timer_init(ret);
}

static void gl_timer_free(gl_object *obj_obj)
{
	gl_timer *obj = (gl_timer *)obj_obj;
	
	if (obj->data.state == gl_timer_state_running) {
		obj->f->stop(obj);
	}
	
	gl_object_free_org_global(obj_obj);
}
