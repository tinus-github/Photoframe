//
//  gl-tile.c
//  Photoframe
//
//  Created by Martijn Vernooij on 15/11/2016.
//
//

#include "gl-tile.h"

static void gl_tile_set_texture(gl_tile *obj, gl_texture *texture);

static struct gl_tile_funcs gl_tile_funcs_global = {
	.set_texture = &gl_tile_set_texture,
};

// Takes over the reference that was held by the caller
// You may also send NULL to remove the texture
static void gl_tile_set_texture(gl_tile *obj, gl_texture *texture)
{
	gl_texture *org_texture = obj->data.texture;
	
	if (org_texture) {
		gl_object *org_texture_obj = (gl_object *)org_texture;
		
		org_texture_obj->f->unref(org_texture_obj);
	}
	
	obj->data.texture = texture;
}

void gl_tile_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_tile_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free((gl_object *)parent);
}

gl_tile *gl_tile_init(gl_tile *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_tile_funcs_global;
	
	return obj;
}

gl_tile *gl_tile_new()
{
	gl_tile *ret = malloc(sizeof(gl_tile));
	
	return gl_tile_init(ret);
}
