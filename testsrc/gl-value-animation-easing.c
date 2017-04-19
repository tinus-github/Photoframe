//
//  gl-value-animation-easing.c
//  Photoframe
//
//  Created by Martijn Vernooij on 30/12/2016.
//
// Uses code from webcore:
// platform/graphics/UnitBezier.h
/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//

#include "gl-value-animation-easing.h"
#include "math.h"

#include <stdlib.h>
#include <string.h>

typedef struct gl_value_animation_easing_bezier_data {
	GLfloat cx;
	GLfloat bx;
	GLfloat ax;
	GLfloat cy;
	GLfloat by;
	GLfloat ay;
} gl_value_animation_easing_bezier_data;

static GLfloat gl_value_animation_easing_calculate_value_normalized(gl_value_animation *obj, GLfloat normalized_time_elapsed);
static void gl_value_animation_easing_copy(gl_value_animation *source, gl_value_animation *target);
static gl_value_animation *gl_value_animation_easing_dup(gl_value_animation *source);

static void calculate_cubic_bezier_coefficients(GLfloat x1, GLfloat y1,
						GLfloat x2, GLfloat y2,
						gl_value_animation_easing_bezier_data *coefficients);

static void (*gl_value_animation_copy_org_global) (gl_value_animation *source, gl_value_animation *target);

static struct gl_value_animation_easing_funcs gl_value_animation_easing_funcs_global = {
	
};

static gl_value_animation_easing_bezier_data fastout_slowin_data;

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
	
	// note: If custom values are required, we need to check if they meet the requirements
	calculate_cubic_bezier_coefficients(0.4, 0,
					    0.2, 1,
					    &fastout_slowin_data);
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

static void calculate_cubic_bezier_coefficients(GLfloat x1, GLfloat y1,
						GLfloat x2, GLfloat y2,
						gl_value_animation_easing_bezier_data *coefficients)
{
	coefficients->cx = 3.0 * x1;
	coefficients->bx = 3.0 * (x2 - x1) - coefficients->cx;
	coefficients->ax = 1.0 - coefficients->cx - coefficients->bx;
	
	coefficients->cy = 3.0 * y1;
	coefficients->by = 3.0 * (y2 - y1) - coefficients->cy;
	coefficients->ay = 1.0 - coefficients->cy - coefficients->by;
}

static GLfloat sample_cubic_bezier_x(GLfloat n, gl_value_animation_easing_bezier_data *coefficients)
{
	return ((coefficients->ax * n + coefficients->bx) * n + coefficients->cx) * n;
}

static GLfloat sample_cubic_bezier_y(GLfloat n, gl_value_animation_easing_bezier_data *coefficients)
{
	return ((coefficients->ay * n + coefficients->by) * n + coefficients->cy) * n;
}

static GLfloat sample_cubic_bezier_derivative_x(GLfloat n, gl_value_animation_easing_bezier_data *coefficients)
{
	return (3.0 * coefficients->ax * n + 2.0 * coefficients->bx) * n + coefficients->cx;
}

static GLfloat solve_cubic_bezier_x(GLfloat x, GLfloat epsilon, gl_value_animation_easing_bezier_data *coefficients)
{
	GLfloat t0, t1, t2, x2, d2;
	int i;
	
	t2 = x;
	for (i = 0; i < 8; i++) {
		x2 = sample_cubic_bezier_x(t2, coefficients) - x;
		if (fabsf(x2) < epsilon) {
			return t2;
		}
		d2 = sample_cubic_bezier_derivative_x(t2, coefficients);
		if (fabsf(d2) < 0.0001) {
			break;
		}
		t2 = t2 - x2 / d2;
	}
	
	t0 = 0.0;
	t1 = 1.0;
	t2 = x;
	
	if (t2 < t0) {
		return t0;
	}
	if (t2 > t1) {
		return t1;
	}
	
	while (t0 < t1) {
		x2 = sample_cubic_bezier_x(t2, coefficients);
		if (fabsf(x2 - x) < epsilon) {
			return t2;
		}
		if (x > x2) {
			t0 = t2;
		} else {
			t1 = t2;
		}
		
		t2 = (t1 - t0) * 0.5 + t0;
	}
	
	return t2;
}

static GLfloat find_cubic_bezier_y(GLfloat x, GLfloat epsilon, gl_value_animation_easing_bezier_data *coefficients)
{
	return sample_cubic_bezier_y(solve_cubic_bezier_x(x, epsilon, coefficients), coefficients);
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
			return find_cubic_bezier_y(normalized_time_elapsed, 0.0001, &fastout_slowin_data);
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

