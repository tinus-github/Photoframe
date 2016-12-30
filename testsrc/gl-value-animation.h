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

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <sys/time.h>

#include "gl-object.h"
#include "gl-renderloop-member.h"


typedef struct gl_value_animation gl_value_animation;

typedef struct gl_value_animation_funcs {
	gl_object_funcs p;
	void (*start) (gl_value_animation *obj);
	void (*pause) (gl_value_animation *obj);
	GLfloat (*calculate_value) (gl_value_animation *obj,
				    GLfloat normalized_time_elapsed, GLfloat startValue, GLfloat endValue);
	GLfloat (*calculate_value_normalized) (gl_value_animation *obj, GLfloat normalized_time_elapsed);
} gl_value_animation_funcs;

typedef struct gl_value_animation_data {
	gl_object_data p;
	
	gl_renderloop_member *renderloopMember;
	struct timeval startTime;
	unsigned int isRunning;
	GLfloat timeElapsed;
	
	GLfloat startValue;
	GLfloat endValue;
	GLfloat duration;
	unsigned int repeats;
	
	void *target;
	void *extraData;
	
	void (*action) (void *target, void *extra_data, GLfloat value);
	void (*final_action) (void *target, void *extra_data);
} gl_value_animation_data;

struct gl_value_animation {
	gl_value_animation_funcs *f;
	gl_value_animation_data data;
};

void gl_value_animation_setup();
gl_value_animation *gl_value_animation_init(gl_value_animation *obj);
gl_value_animation *gl_value_animation_new();



#endif /* gl_value_animation_h */
