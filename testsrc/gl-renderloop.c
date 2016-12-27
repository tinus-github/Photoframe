//
//  gl-renderloop.c
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#include "gl-renderloop.h"
#include <string.h>

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child);

static struct gl_renderloop_funcs gl_renderloop_funcs_global = {
	.append_child = &gl_renderloop_append_child
};

void gl_renderloop_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_renderloop_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_renderloop *gl_renderloop_init(gl_renderloop *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_renderloop_funcs_global;
	
	return obj;
}

gl_renderloop *gl_renderloop_new()
{
	gl_renderloop *ret = calloc(1, sizeof(gl_renderloop));
	
	return gl_renderloop_init(ret);
}

static void gl_renderloop_append_child(gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child)
{
	return;
}
