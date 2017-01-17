//
//  gl-slideshow.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#ifndef gl_slideshow_h
#define gl_slideshow_h

#include <stdio.h>
#include "gl-container-2d.h"
#include "gl-value-animation.h"

typedef struct gl_slideshow gl_slideshow;

typedef enum {
	gl_slideshow_state_empty = 0,
	gl_slideshow_state_
} gl_slideshow_state;

typedef struct gl_slideshow_funcs {
	gl_container_2d_funcs p;
	void (*set_entrance_animation) (gl_slideshow *obj, gl_value_animation *animation);
	void (*set_exit_animation) (gl_slideshow *obj, gl_value_animation *animation);
	void (*start) (gl_slideshow *obj);
} gl_slideshow_funcs;

typedef struct gl_slideshow_data {
	gl_container_2d_data p;
	
	GLfloat slideDuration;
	gl_slide *(getNextSlideCallback) (void *target, void *extra_data);
	void *callbackTarget;
	void *callbackExtraData;
	
	
	gl_value_animation *_entranceAnimation;
	gl_value_animation *_exitAnimation;
	
	gl_slide *_currentSlide;
	gl_slide *_incomingSlide;
} gl_slideshow_data;

struct gl_slideshow {
	gl_slideshow_funcs *f;
	gl_slideshow_data data;
};

void gl_slideshow_setup();
gl_slideshow *gl_slideshow_init(gl_slideshow *obj);
gl_slideshow *gl_slideshow_new();

#endif /* gl_slideshow_h */
