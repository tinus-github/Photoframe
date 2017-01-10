//
//  gl-bitmap.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#include "gl-bitmap.h"

static void gl_bitmap_free(gl_object *obj_obj);

static struct gl_bitmap_funcs gl_bitmap_funcs_global = {

};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_bitmap_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_bitmap_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_free_org_global = gl_bitmap_funcs_global.p.free;
	gl_bitmap_funcs_global.p.free = &gl_bitmap_free;
}

gl_bitmap *gl_bitmap_init(gl_bitmap *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_bitmap_funcs_global;
	
	return obj;
}

gl_bitmap *gl_bitmap_new()
{
	gl_bitmap *ret = calloc(1, sizeof(gl_bitmap));
	
	return gl_bitmap_init(ret);
}

static void gl_bitmap_free(gl_object *obj_obj)
{
	gl_bitmap *obj = (gl_bitmap *)obj_obj;
	
	free(obj->data.bitmap);
	gl_object_free_org_global(obj_obj);
}
