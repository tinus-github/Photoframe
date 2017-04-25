//
//  gl-timer-queue.c
//  Photoframe
//
//  Created by Martijn Vernooij on 25/04/2017.
//
//

#include "infrastructure/gl-timer-queue.h"
#include "gl-renderloop.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static gl_timer_queue *global_timer_queue;

static void gl_timer_queue_add_timer(gl_timer_queue *obj, gl_timer *timer);
static void gl_timer_queue_remove_timer(gl_timer_queue *obj, gl_timer *timer);
static void gl_timer_queue_tick(void *target, gl_renderloop_member *renderloop_member, void *action_data);

static struct gl_timer_queue_funcs gl_timer_queue_funcs_global = {
	.add_timer = &gl_timer_queue_add_timer,
	.remove_timer = &gl_timer_queue_remove_timer,
};

void gl_timer_queue_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_timer_queue_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
//	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_timer_queue_funcs_global;
}

static void gl_timer_queue_start(gl_timer_queue *obj)
{
	assert(!obj->data._is_running);
	
	obj->data._renderloopMember->data.target = obj;
	obj->data._renderloopMember->data.action = &gl_timer_queue_tick;
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	
	((gl_object *)obj->data._renderloopMember)->f->ref((gl_object *)obj->data._renderloopMember);
	
	renderloop->f->append_child(renderloop, gl_renderloop_phase_animate, obj->data._renderloopMember);
	obj->data._is_running = 1;
}

static void gl_timer_queue_stop(gl_timer_queue *obj)
{
	assert(obj->data._is_running);
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	renderloop->f->remove_child(renderloop, obj->data._renderloopMember);
	
	obj->data._is_running = 0;
}

// Note this takes the reference on the timer
static void gl_timer_queue_add_timer(gl_timer_queue *obj, gl_timer *timer)
{
	gl_timer *currentEntry = obj->data._timer_list_head;
	currentEntry = currentEntry->data._next;

	while (currentEntry != obj->data._timer_list_head) {
		if (timercmp(&currentEntry->data._endTime, &timer->data._endTime, >) ) {
			break;
		}
		currentEntry = currentEntry->data._next;
	}
	
	// Now currentEntry is either the first entry in the list that triggers later than the new timer,
	// or the list head. In any case, we can prepend the new timer to this one.
	
	timer->data._next = currentEntry;
	timer->data._previous = currentEntry->data._previous;
	currentEntry->data._previous = timer;
	timer->data._previous->data._next = timer;
	
	if (!obj->data._is_running) {
		gl_timer_queue_start(obj);
	}
}

// This releases our reference to the timer
static void gl_timer_queue_remove_timer(gl_timer_queue *obj, gl_timer *timer)
{
	assert (obj->data._is_running);
	
	// We just assume it's on the list.
	timer->data._next->data._previous = timer->data._previous;
	timer->data._previous->data._next = timer->data._next;
	
	timer->data._next = NULL;
	timer->data._previous = NULL;
	
	if (obj->data._timer_list_head->data._next == obj->data._timer_list_head) {
		gl_timer_queue_stop(obj);
	}
	
	((gl_object *)timer)->f->unref((gl_object *)timer);
}

static void gl_timer_queue_tick(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	gl_timer_queue *obj = (gl_timer_queue *)target;
	
	assert (obj->data._is_running);
	
	gl_timer *timer_to_check = obj->data._timer_list_head->data._next;
	
	assert (timer_to_check != obj->data._timer_list_head);
	
	struct timezone tz;
	struct timeval now_time;
	
	gettimeofday(&now_time, &tz);
	if (!timercmp(&now_time, &timer_to_check->data._endTime, < )) {
		timer_to_check->f->_trigger(timer_to_check);
		
		obj->f->remove_timer(obj, timer_to_check);
		// this also stops the queue if it is empty
	}
}

gl_timer_queue *gl_timer_queue_init(gl_timer_queue *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_timer_queue_funcs_global;
	
	
	obj->data._timer_list_head = gl_timer_new();
	obj->data._timer_list_head->data._next = obj->data._timer_list_head;
	obj->data._timer_list_head->data._previous = obj->data._timer_list_head;
	
	obj->data._renderloopMember = gl_renderloop_member_new();
	
	return obj;
}

gl_timer_queue *gl_timer_queue_new()
{
	gl_timer_queue *ret = calloc(1, sizeof(gl_timer_queue));
	
	if (!global_timer_queue) {
		global_timer_queue = ret;
	}
	
	return gl_timer_queue_init(ret);
}

gl_timer_queue *gl_timer_queue_get_global_timer_queue()
{
	if (!global_timer_queue) {
		gl_timer_queue_new();
	}
	
	return global_timer_queue;
}
