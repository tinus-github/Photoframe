//
//  gl-timer-queue.h
//  Photoframe
//
//  Created by Martijn Vernooij on 25/04/2017.
//
//

#ifndef gl_timer_queue_h
#define gl_timer_queue_h

#include <stdio.h>

#include "infrastructure/gl-timer.h"
#include "gl-renderloop-member.h"

typedef struct gl_timer_queue gl_timer_queue;

typedef struct gl_timer_queue_funcs {
	gl_object_funcs p;
	void (*add_timer) (gl_timer_queue *obj, gl_timer *timer);
	void (*remove_timer) (gl_timer_queue *obj, gl_timer *timer);
} gl_timer_queue_funcs;

typedef struct gl_timer_queue_data {
	gl_object_data p;
	
	gl_timer *_timer_list_head;
	int _is_running;
	gl_renderloop_member *_renderloopMember;
} gl_timer_queue_data;

struct gl_timer_queue {
	gl_timer_queue_funcs *f;
	gl_timer_queue_data data;
};

void gl_timer_queue_setup();
gl_timer_queue *gl_timer_queue_init(gl_timer_queue *obj);
gl_timer_queue *gl_timer_queue_new();
gl_timer_queue *gl_timer_queue_get_global_timer_queue();

#endif /* gl_timer_queue_h */
