//
//  gl-value-animation.h
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
//

#ifndef gl_value_animation_h
#define gl_value_animation_h

#include <stdio.h>

#include "gl-includes.h"
#include <sys/time.h>

#include "infrastructure/gl-object.h"
#include "gl-renderloop-member.h"
#include "infrastructure/gl-notice.h"

typedef struct gl_value_animation gl_value_animation;

typedef struct gl_value_animation_funcs {
	gl_object_funcs p;
	void (*start) (gl_value_animation *obj);
	void (*pause) (gl_value_animation *obj);
	GLfloat (*calculate_value) (gl_value_animation *obj,
				    GLfloat normalized_time_elapsed, GLfloat startValue, GLfloat endValue);
	GLfloat (*calculate_value_normalized) (gl_value_animation *obj, GLfloat normalized_time_elapsed);
	void (*set_duration) (gl_value_animation *obj, GLfloat duration);
	void (*set_speed) (gl_value_animation *obj, GLfloat speed);
	void (*done) (gl_value_animation *obj);
	void (*copy) (gl_value_animation *source, gl_value_animation *target);
	gl_value_animation *(*dup) (gl_value_animation *obj);
} gl_value_animation_funcs;

typedef struct gl_value_animation_data {
	gl_object_data p;
	
	GLfloat startValue;
	GLfloat endValue;
	GLfloat _duration;
	unsigned int repeats;
	
	void *target;
	void *extraData;
	
	void (*action) (void *target, void *extra_data, GLfloat value);
	
	gl_notice *animationCompleted;
	
	gl_renderloop_member *_renderloopMember;
	struct timeval _startTime;
	unsigned int _isRunning;
	GLfloat _timeElapsed;
} gl_value_animation_data;

struct gl_value_animation {
	gl_value_animation_funcs *f;
	gl_value_animation_data data;
};

void gl_value_animation_setup();
gl_value_animation *gl_value_animation_init(gl_value_animation *obj);
gl_value_animation *gl_value_animation_new();



#endif /* gl_value_animation_h */
