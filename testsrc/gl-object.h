//
//  gl-object.h
//  Photoframe
//
//  Created by Martijn Vernooij on 03/05/16.
//
//

#ifndef gl_object_h
#define gl_object_h

#include <stdio.h>
#include <stdatomic.h>

typedef struct gl_object gl_object;

typedef struct gl_object_funcs {
	void (*free) (gl_object *obj);
	void (*free_fp) (gl_object *obj);
	void (*ref) (gl_object *obj);
	void (*unref) (gl_object *obj);
} gl_object_funcs;

typedef struct gl_object_data {
	atomic_uint refcount;
} gl_object_data;

struct gl_object {
	gl_object_funcs *f;
	gl_object_data data;
};

gl_object *gl_object_init(gl_object *obj);
gl_object *gl_object_new();

#endif /* gl_object_h */
