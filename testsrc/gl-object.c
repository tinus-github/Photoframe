//
//  gl-object.c
//  Photoframe
//
//  Created by Martijn Vernooij on 03/05/16.
//
// Implements a very basic object scheme

#include "gl-object.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void gl_object_ref(gl_object *obj)
{
	atomic_fetch_add(&obj->data.refcount, 1);
}

static void gl_object_unref(gl_object *obj)
{
	unsigned int orgcount = atomic_fetch_sub(&obj->data.refcount, 1);
	assert (orgcount);
	if (orgcount == 1) {
		obj->f->free(obj);
	}
}

static void gl_object_free(gl_object *obj)
{
	obj->f->free_fp(obj);
	free (obj);
}

static void gl_object_free_fp(gl_object *obj)
{
	;
}

static struct gl_object_funcs gl_object_funcs_global =
{	.ref = gl_object_ref,
	.unref = gl_object_unref,
	.free = gl_object_free,
	.free_fp = gl_object_free_fp
};

gl_object *gl_object_init(gl_object *obj)
{
	obj->f = &gl_object_funcs_global;
	
	return obj;
}

gl_object *gl_object_new()
{
	gl_object *ret = malloc(sizeof(gl_object));
	
	return gl_object_init(ret);
}
