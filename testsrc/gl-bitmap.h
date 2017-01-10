//
//  gl-bitmap.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
// A simple refcounted data storage

#ifndef gl_bitmap_h
#define gl_bitmap_h

#include <stdio.h>
#include "gl-object.h"

typedef struct gl_bitmap gl_bitmap;

typedef struct gl_bitmap_funcs {
	gl_object_funcs p;
} gl_bitmap_funcs;

typedef struct gl_bitmap_data {
	gl_object_data p;
	
	unsigned int width;
	unsigned int height;
	
	unsigned char* bitmap;
} gl_bitmap_data;

struct gl_bitmap {
	gl_bitmap_funcs *f;
	gl_bitmap_data data;
};

void gl_bitmap_setup();
gl_bitmap *gl_bitmap_init(gl_bitmap *obj);
gl_bitmap *gl_bitmap_new();

#endif /* gl_bitmap_h */
