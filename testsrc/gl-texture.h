//
//  gl-texture.h
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#ifndef gl_texture_h
#define gl_texture_h

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gl-object.h"

typedef struct gl_texture gl_texture;

typedef struct gl_texture_funcs {
	gl_object_funcs p;
	GLuint (*load_image) (gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height);
	GLuint (*load_image_tile) (gl_texture *obj, unsigned char *rgba_data,
				   unsigned int image_width, unsigned int image_height,
				   unsigned int tile_width, unsigned int tile_height,
				   unsigned int tile_x, unsigned int tile_y);
	GLuint (*load_image_horizontal_tile) (gl_texture *obj, unsigned char *rgba_data,
				   unsigned int image_width, unsigned int image_height,
				   unsigned_int tile_height, unsigned int tile_y);
} gl_texture_funcs;

typedef struct gl_texture_data {
	gl_object_data p;
	GLuint textureId;
	int texture_loaded;
	GLuint width;
	GLuint height;
	
	GLuint orientation;
} gl_texture_data;

struct gl_texture {
	gl_texture_funcs *f;
	gl_texture_data data;
};

gl_texture *gl_texture_init(gl_texture *obj);
gl_texture *gl_texture_new();

void gl_texture_setup();

#endif /* gl_texture_h */
