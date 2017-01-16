//
//  gl-image.h
//  Photoframe
//
//  Created by Martijn Vernooij on 16/01/2017.
//
//

#ifndef gl_image_h
#define gl_image_h

#include <stdio.h>

#include "gl-tiled-image.h"

typedef struct gl_image gl_image;

typedef struct gl_image_funcs {
	gl_tiled_image_funcs p;
	void (*load_file) (gl_image *obj, const char* filename);
} gl_image_funcs;

typedef struct gl_image_data {
	gl_tiled_image_data p;
	
	unsigned int desiredWidth;
	unsigned int desiredHeight;
	
	const char *format;
	
	gl_notice *readyNotice;
	
	char *_filename;
	int _width;
	int _height;
	int _orientation;
} gl_image_data;

struct gl_image {
	gl_image_funcs *f;
	gl_image_data data;
};

void gl_image_setup();
gl_image *gl_image_init(gl_image *obj);
gl_image *gl_image_new();

#endif /* gl_image_h */
