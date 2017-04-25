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
#include "slideshow/gl-slide.h"
#include "config/gl-configuration.h"
#include "infrastructure/gl-timer.h"

typedef enum {
	gl_slideshow_timer_status_notstarted = 0,
	gl_slideshow_timer_status_running,
	gl_slideshow_timer_status_done
} gl_slideshow_timer_status;

typedef struct gl_slideshow gl_slideshow;

typedef struct gl_slideshow_funcs {
	gl_slide_funcs p;
	void (*set_entrance_animation) (gl_slideshow *obj, gl_value_animation *animation);
	void (*set_exit_animation) (gl_slideshow *obj, gl_value_animation *animation);
	void (*set_configuration) (gl_slideshow *obj, gl_config_section *config);
} gl_slideshow_funcs;

typedef struct gl_slideshow_data {
	gl_slide_data p;
	
	GLfloat slideDuration;
	gl_slide *(*getNextSlideCallback) (void *target, void *extra_data);
	void *callbackTarget;
	void *callbackExtraData;
	
	gl_value_animation *_slideEntranceAnimation;
	gl_value_animation *_slideExitAnimation;
	gl_timer *_onScreenTimer;
	
	gl_slideshow_timer_status _timerStatus;
	
	gl_slide *_currentSlide;
	gl_slide *_incomingSlide;
	
	int _isRunning;
	
	gl_config_section *_configuration;
} gl_slideshow_data;

struct gl_slideshow {
	gl_slideshow_funcs *f;
	gl_slideshow_data data;
};

void gl_slideshow_setup();
gl_slideshow *gl_slideshow_init(gl_slideshow *obj);
gl_slideshow *gl_slideshow_new();

#endif /* gl_slideshow_h */
