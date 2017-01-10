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

typedef enum {
	gl_texture_data_type_rgba = 0,
	gl_texture_data_type_monochrome,
	gl_texture_data_type_alpha
} gl_texture_data_type;

typedef enum {
	gl_texture_loadstate_clear = 0,
	gl_texture_loadstate_started,
	gl_texture_loadstate_done
} gl_texture_loadstate;

typedef struct gl_texture_funcs {
	gl_object_funcs p;
	void (*load_image) (gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height);
	void (*load_image_monochrome) (gl_texture *obj, unsigned char *monochrome_data, unsigned int width, unsigned int height);
	void (*load_image_tile) (gl_texture *obj, unsigned char *rgba_data,
				 unsigned int image_width, unsigned int image_height,
				 unsigned int tile_width, unsigned int tile_height,
				 unsigned int tile_x, unsigned int tile_y);
	void (*load_image_horizontal_tile) (gl_texture *obj, unsigned char *rgba_data,
					    unsigned int image_width, unsigned int image_height,
					    unsigned int tile_height, unsigned int tile_y);
} gl_texture_funcs;

typedef struct gl_texture_data {
	gl_object_data p;
	GLuint textureId;
	gl_texture_loadstate loadState;
	GLuint width;
	GLuint height;
	
	gl_texture_data_type dataType;
} gl_texture_data;

struct gl_texture {
	gl_texture_funcs *f;
	gl_texture_data data;
};

gl_texture *gl_texture_init(gl_texture *obj);
gl_texture *gl_texture_new();

void gl_texture_setup();

#endif /* gl_texture_h */
