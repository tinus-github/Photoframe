//
//  gl-value-animation-easing.c
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
//

#include "gl-value-animation-easing.h"
#include "math.h"

static GLfloat gl_value_animation_easing_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed);

static struct gl_value_animation_easing_funcs gl_value_animation_easing_funcs_global = {
	
};

void gl_value_animation_easing_setup()
{
	gl_value_animation *parent = gl_value_animation_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_value_animation_easing_funcs_global.p, parent->f, sizeof(gl_value_animation_funcs));
	parent_obj->f->free(parent_obj);
	
	gl_value_animation_funcs *parent_f = (gl_value_animation_funcs *)&gl_value_animation_easing_funcs_global;
	parent_f->calculate_value_normalized = &gl_value_animation_easing_calculate_value_normalized;
}

GLfloat calculate_linear(GLfloat n)
{
	return n;
}

GLfloat calculate_quinticEaseInOut(GLfloat n)
{
	if(n < 0.5)
	{
		return 16 * n * n * n * n * n;
	}
	else
	{
		GLfloat f = ((2 * n) - 2);
		return  0.5 * f * f * f * f * f + 1;
	}
}

GLfloat calculate_sine(GLfloat n)
{
	return sinf(n/(2 * M_PI));
}

static GLfloat gl_value_animation_easing_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed)
{
	gl_value_animation_easing *obj_easing = (gl_value_animation_easing *)obj;
	
	switch (obj_easing->data.easingType) {
		default:
		case gl_value_animation_ease_linear:
			return calculate_linear(normalized_time_elapsed);
		case gl_value_animation_ease_QuinticEaseInOut:
			return calculate_quinticEaseInOut(normalized_time_elapsed);
		case gl_value_animation_ease_Sine:
			return calculate_sine(normalized_time_elapsed);
	}
}

gl_value_animation_easing *gl_value_animation_easing_init(gl_value_animation_easing *obj)
{
	gl_value_animation_init((gl_value_animation *)obj);
	
	obj->f = &gl_value_animation_easing_funcs_global;
	
	return obj;
}

gl_value_animation_easing *gl_value_animation_easing_new()
{
	gl_value_animation_easing *ret = calloc(1, sizeof(gl_value_animation_easing));
	
	return gl_value_animation_easing_init(ret);
}

