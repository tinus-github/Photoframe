//
//  gl-tile.h
//  Photoframe
//
//  Created by Martijn Vernooij on 15/11/2016.
//
//

#ifndef gl_tile_h
#define gl_tile_h

#include <stdio.h>

#include "gl-shape.h"
#include "gl-texture.h"

typedef struct gl_tile gl_tile;

typedef struct gl_tile_funcs {
	gl_object_funcs p;
	void (*set_texture) (gl_tile *obj, gl_texture *texture);
} gl_tile_funcs;

typedef struct gl_tile_data {
	gl_shape_data p;
	gl_texture texture;
} gl_tile_data;

struct gl_tile {
	gl_tile_funcs *f;
	gl_tile_data data;
};

void gl_tile_setup();
gl_tile *gl_tile_init(gl_tile *obj);
gl_tile *gl_tile_new();

#endif /* gl_tile_h */
