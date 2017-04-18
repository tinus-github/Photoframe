//
//  gl-value-animation-easing.c
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
//

#include "gl-value-animation-easing.h"
#include "math.h"

#include <stdlib.h>
#include <string.h>

static GLfloat gl_value_animation_easing_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed);
static void gl_value_animation_easing_copy(gl_value_animation *source, gl_value_animation *target);
static gl_value_animation *gl_value_animation_easing_dup(gl_value_animation *source);

static void (*gl_value_animation_copy_org_global) (gl_value_animation *source, gl_value_animation *target);

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
	parent_f->dup = &gl_value_animation_easing_dup;
	gl_value_animation_copy_org_global = parent_f->copy;
	parent_f->copy = &gl_value_animation_easing_copy;
}

static GLfloat calculate_linear(GLfloat n)
{
	return n;
}

static GLfloat calculate_quinticEaseInOut(GLfloat n)
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

static void pointInLine(GLfloat x0, GLfloat y0,
			GLfloat x1, GLfloat y1,
			GLfloat t,
			GLfloat *xOut, GLfloat *yOut)
{
	*xOut = x0 + (t * (x1 - x0));
	*yOut = y0 + (t * (y1 - y0));
}

static void pointInQuadCurve(GLfloat x0, GLfloat y0,
			     GLfloat x1, GLfloat y1,
			     GLfloat x2, GLfloat y2,
			     GLfloat t,
			     GLfloat *xOut, GLfloat *yOut)
{
	GLfloat xA, yA, xB, yB;
	
	pointInLine(x0, y0,
		    x1, y2,
		    t,
		    &xA, &yA);
	pointInLine(x1, y1,
		    x2, y2,
		    t,
		    &xB, &yB);
	pointInLine(xA, yA,
		    xB, yB,
		    t,
		    xOut, yOut);
}

static void pointInCubicCurve(GLfloat x0, GLfloat y0,
			      GLfloat x1, GLfloat y1,
			      GLfloat x2, GLfloat y2,
			      GLfloat x3, GLfloat y3,
			      GLfloat t,
			      GLfloat *xOut, GLfloat *yOut)
{
	GLfloat xA, yA, xB, yB;
	
	pointInQuadCurve(x0, y0,
			 x1, y2,
			 x2, y2,
			 t,
			 &xA, &yA);
	pointInQuadCurve(x1, y1,
			 x2, y2,
			 x3, y3,
			 t,
			 &xB, &yB);
	pointInLine(xA, yA,
		    xB, yB,
		    t,
		    xOut, yOut);
}

static GLfloat calculate_cubic_bezier_ease(GLfloat n,
					   GLfloat x1, GLfloat y1,
					   GLfloat x2, GLfloat y2)
{
	GLfloat xOut, yOut;
	pointInCubicCurve(0, 0,
			  x1, y1,
			  x2, y2,
			  1, 1,
			  n,
			  &xOut, &yOut);
	
	return yOut;
}

static GLfloat calculate_sine(GLfloat n)
{
	GLfloat ret = (sinf((n - 0.25) * (2.0 * M_PI)) + 1.0) / 2.0;
	return ret;
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
		case gl_value_animation_ease_FastOutSlowIn:
			return calculate_cubic_bezier_ease(normalized_time_elapsed,
							   0.4, 0,
							   0.2, 1);
	}
}

static void gl_value_animation_easing_copy(gl_value_animation *source, gl_value_animation *target)
{
	gl_value_animation_copy_org_global(source, target);
	
	gl_value_animation_easing *source_easing = (gl_value_animation_easing *)source;
	gl_value_animation_easing *target_easing = (gl_value_animation_easing *)target;
	
	target_easing->data.easingType = source_easing->data.easingType;
}

static gl_value_animation *gl_value_animation_easing_dup(gl_value_animation *source)
{
	gl_value_animation_easing *ret = gl_value_animation_easing_new();
	source->f->copy(source, (gl_value_animation *)ret);
	
	return (gl_value_animation *)ret;
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

