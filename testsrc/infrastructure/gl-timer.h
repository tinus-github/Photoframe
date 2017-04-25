//
//  gl-timer.h
//  Photoframe
//
//  Created by Martijn Vernooij on 24/04/2017.
//
//

#ifndef gl_timer_h
#define gl_timer_h

#include <stdio.h>
#include <sys/time.h>

#include "infrastructure/gl-object.h"

typedef enum {
	gl_timer_state_stopped = 0,
	gl_timer_state_running,
	gl_timer_state_paused,
	gl_timer_state_completed
} gl_timer_state;

typedef struct gl_timer gl_timer;

typedef struct gl_timer_funcs {
	gl_object_funcs p;
	void (*start) (gl_timer *obj);
	void (*pause) (gl_timer *obj);
	void (*stop) (gl_timer *obj);
	
	void (*_trigger) (gl_timer *obj);
	
	void (*set_duration) (gl_timer *obj, float duration);
} gl_timer_funcs;

typedef struct gl_timer_data {
	gl_object_data p;
	
	void *target;
	void (*action) (void *target, void *extraData);
	void *extraData;
	
	struct timeval _endTime;
	float _duration;
	float _elapsed; // only set while paused
	gl_timer_state state;
	
	gl_timer *_previous;
	gl_timer *_next;
} gl_timer_data;

struct gl_timer {
	gl_timer_funcs *f;
	gl_timer_data data;
};

void gl_timer_setup();
gl_timer *gl_timer_init(gl_timer *obj);
gl_timer *gl_timer_new();

#endif /* gl_timer_h */
