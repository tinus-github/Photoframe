//
//  gl-texture.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#include "gl-texture.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"
#include <string.h>
#include <assert.h>

static void load_image(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height);
static void load_image_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *rgba_data, unsigned int width, unsigned int height);
static void load_image_monochrome(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height);
static void load_image_horizontal_tile(gl_texture *obj, gl_bitmap *bitmap,
				       unsigned int image_width, unsigned int image_height,
				       unsigned int tile_height, unsigned int tile_y);
static void gl_texture_free(gl_object *obj);

static struct gl_texture_funcs gl_texture_funcs_global = {
	.load_image = &load_image,
	.load_image_r = &load_image_r,
	.load_image_monochrome = &load_image_monochrome,
	.load_image_horizontal_tile = &load_image_horizontal_tile
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_texture_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_texture_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_free_org_global = gl_texture_funcs_global.p.free;
	gl_texture_funcs_global.p.free = &gl_texture_free;
}

gl_texture *gl_texture_init(gl_texture *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_texture_funcs_global;
	
	return obj;
}

gl_texture *gl_texture_new()
{
	gl_texture *ret = calloc(1, sizeof(gl_texture));
	
	return gl_texture_init(ret);
}

static void load_image_gen_r_work(void *target, gl_renderloop_member *renderloop_member, void *extra_data)
{
	gl_texture *obj = (gl_texture *)target;

	obj->data.imageDataBitmap = data;
	
	// Texture object handle
	GLuint textureId;
	
	// Use tightly packed data
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	
	// Generate a texture object
	glGenTextures ( 1, &textureId );
	
	// Bind the texture object
	glBindTexture ( GL_TEXTURE_2D, textureId );
	
	// Load the texture
	if (obj->data.dataType != gl_texture_data_type_rgba) {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_ALPHA,
			      obj->data.width, obj->data.height,
			      0, GL_ALPHA, GL_UNSIGNED_BYTE, obj->data.imageDataBitmap );
	} else {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
			      obj->data.width, obj->data.height,
			      0, GL_RGBA, GL_UNSIGNED_BYTE, obj->data.imageDataBitmap );
	}
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	// Wrapping mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	obj->data.textureId = textureId;
	obj->data.loadState = gl_texture_loadstate_done;
	bitmap_obj = (gl_object *)obj->data.imageDataStore;
	bitmap_obj->f->unref(bitmap_obj);
	obj->data.imageDataStore = NULL;
	obj->data.imageDataBitmap = NULL;
	((gl_object *)obj->data.uploadRenderloopMember)->f->unref((gl_object *)obj->data.uploadRenderloopMember);
	obj->data.uploadRenderloopMember = NULL;
}

static void load_image_gen_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *data)
{
	obj->data.loadState = gl_texture_loadstate_started;
	obj->data.imageDataStore = bitmap;
	gl_object *bitmap_obj = (gl_object *)bitmap;
	bitmap_obj->f->ref(bitmap_obj);

	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.target = obj;
	renderloop_member->data.action = &load_image_gen_r_work;
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	renderloop->f->append_child(renderloop, gl_renderloop_phase_load, renderloop_member);
	obj->data.uploadRenderloopMember = renderloop_member;
	((gl_object *)renderloop_member)->f->ref((gl_object *)renderloop_member);
}

static void load_image_gen(gl_texture *obj, gl_bitmap *bitmap)
{
	load_image_gen_r(obj, bitmap, bitmap->data.bitmap);
}

static void load_image(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_rgba;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen(obj, bitmap);
}

static void load_image_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *rgba_data, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_rgba;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen_r(obj, bitmap, rgba_data);
}

static void load_image_monochrome(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_monochrome;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen(obj, bitmap);
}

static void load_image_horizontal_tile(gl_texture *obj, gl_bitmap *bitmap,
					 unsigned int image_width, unsigned int image_height,
					 unsigned int tile_height, unsigned int tile_y)
{
	assert (tile_y < image_height);
	
	unsigned char *tile_data = (bitmap->data.bitmap) + (4 * sizeof(unsigned char) * image_width * tile_y);
	
	obj->f->load_image_r(obj, bitmap, tile_data, image_width, tile_height);
}

static void gl_texture_free_texture(gl_texture *obj)
{
	// TODO: Cancel loading
	glDeleteTextures ( 1, &obj->data.textureId );
	obj->data.loadState = gl_texture_loadstate_clear;
}

static void gl_texture_free(gl_object *obj_obj)
{
	gl_texture *obj = (gl_texture *)obj_obj;
	switch (obj->data.loadState) {
		case gl_texture_loadstate_clear:
			break;
		case gl_texture_loadstate_done:
			gl_texture_free_texture(obj);
			break;
		case gl_texture_loadstate_started:
			// TODO: Cancel loading
			break;
	}
	gl_object_free_org_global(obj_obj);
}
