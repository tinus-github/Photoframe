//
//  gl-stage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/12/2016.
//
//
//  The stage contains basic information on the size of the screen and other information required for rendering
#include <stdlib.h>
#include <string.h>

#include "gl-stage.h"

static void gl_stage_set_dimensions(gl_stage *obj, uint32_t width, uint32_t height);

static struct gl_stage_funcs gl_stage_funcs_global = {
	.set_dimensions = &gl_stage_set_dimensions
};

static void gl_stage_set_dimensions(gl_stage *obj, uint32_t width, uint32_t height)
{
	obj->data.width = width;
	obj->data.height = height;
}

void gl_stage_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_stage_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_stage *gl_stage_init(gl_stage *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_stage_funcs_global;
	
	return obj;
}

gl_stage *gl_stage_new()
{
	gl_stage *ret = malloc(sizeof(gl_stage));
	
	return gl_stage_init(ret);
}
