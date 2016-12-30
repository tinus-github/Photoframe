//
//  gl-value-animation-easing.h
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
//

#ifndef gl_value_animation_easing_h
#define gl_value_animation_easing_h

#include <stdio.h>

#include "gl-value-animation.h"

typedef struct gl_value_animation_easing gl_value_animation_easing;

typedef enum {
	gl_value_animation_ease_linear = 0,
	gl_value_animation_ease_QuinticEaseInOut
} gl_value_animation_easing_type;

typedef struct gl_value_animation_easing_funcs {
	gl_value_animation_funcs p;
} gl_value_animation_easing_funcs;

typedef struct gl_value_animation_easing_data {
	gl_value_animation_data p;
	
	gl_value_animation_easing_type easingType;
} gl_value_animation_easing_data;

struct gl_value_animation_easing {
	gl_value_animation_easing_funcs *f;
	gl_value_animation_easing_data data;
};

void gl_value_animation_easing_setup();
gl_value_animation_easing *gl_value_animation_easing_init(gl_value_animation_easing *obj);
gl_value_animation_easing *gl_value_animation_easing_new();

#endif /* gl_value_animation_easing_h */
