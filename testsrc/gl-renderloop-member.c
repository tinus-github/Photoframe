//
//  gl-renderloop-member.c
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#include "gl-renderloop-member.h"
#include <string.h>

static void gl_renderloop_member_run(gl_memberloop_member *obj);

static struct gl_renderloop_member_funcs gl_renderloop_member_funcs_global = {
	.dummy = &gl_renderloop_member_run;
};

void gl_renderloop_member_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_renderloop_member_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_renderloop_member *gl_renderloop_member_init(gl_renderloop_member *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_renderloop_member_funcs_global;
	
	return obj;
}

gl_renderloop_member *gl_renderloop_member_new()
{
	gl_renderloop_member *ret = calloc(1, sizeof(gl_renderloop_member));
	
	return gl_renderloop_member_init(ret);
}

static void gl_renderloop_member_run(gl_memberloop_member *obj)
{
	obj->data.action(obj->data.target, obj, obj->data.action_data);
}
