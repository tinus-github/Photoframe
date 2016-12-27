//
//  gl-tiled-image.h
//  Photoframe
//
//  Created by Martijn Vernooij on 25/12/2016.
//
//

#ifndef gl_tiled_image_h
#define gl_tiled_image_h

#include <stdio.h>

#include "gl-container-2d.h"

typedef struct gl_tiled_image gl_tiled_image;

typedef struct gl_tiled_image_funcs {
	gl_container_2d_funcs p;
	void (*load_image) (gl_tiled_image *obj, unsigned char *rgba_data,
			    unsigned int width, unsigned int height,
			    unsigned int orientation, unsigned int tile_height);
} gl_tiled_image_funcs;

typedef struct gl_tiled_image_data {
	gl_container_2d_data p;
	
	unsigned int keep_image_data;
	
	unsigned int tile_height;
	unsigned char *rgba_data;
	unsigned int orientation;
	
	unsigned int unrotatedImageWidth;
	unsigned int unrotatedImageHeight;
	
	gl_container *orientation_container;
} gl_tiled_image_data;

struct gl_tiled_image {
	gl_tiled_image_funcs *f;
	gl_tiled_image_data data;
};

void gl_tiled_image_setup();
gl_tiled_image *gl_tiled_image_init(gl_tiled_image *obj);
gl_tiled_image *gl_tiled_image_new();


#endif /* gl_tiled_image_h */
