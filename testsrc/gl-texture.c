//
//  gl-texture.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#include "gl-texture.h"
#include <string.h>

static GLuint load_image(gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height);
static GLuint load_image_tile(gl_texture *obj, unsigned char *rgba_data,
			      unsigned int image_width, unsigned int image_height,
			      unsigned int tile_width, unsigned int tile_height,
			      unsigned int tile_x, unsigned int tile_y);
static void gl_texture_free(gl_object *obj);

static struct gl_texture_funcs gl_texture_funcs_global = {
	.load_image = &load_image,
	.load_image_tile = &load_image_tile
};

void (*gl_object_free_org_global) (gl_object *obj);

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
	gl_texture *ret = calloc(sizeof(gl_texture));
	
	return gl_texture_init(ret);
}

// TODO: Remember rotation
static GLuint load_image(gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height)
{
	// Texture object handle
	GLuint textureId;
	
	// Use tightly packed data
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	
	// Generate a texture object
	glGenTextures ( 1, &textureId );
	
	// Bind the texture object
	glBindTexture ( GL_TEXTURE_2D, textureId );
	
	// Load the texture
	
	
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
		      width, height,
		      0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data );
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	obj->data.textureId = textureId;
	obj->data.texture_loaded = 1;
	obj->data.width = width;
	obj->data.height = height;
	
	obj->data.orientation = 1; // TODO: always upright for now
	
	return textureId;
}

static GLuint load_image_tile(gl_texture *obj, unsigned char *rgba_data,
			      unsigned int image_width, unsigned int image_height,
			      unsigned int tile_width, unsigned int tile_height,
			      unsigned int tile_x, unsigned int tile_y)
{
	size_t part_size = 4 * sizeof(unsigned char) * tile_width * tile_height;
	unsigned char *image_part = malloc(part_size);
	unsigned char *input_row_start;
	unsigned char *output_row_start;
	unsigned int counter_y;
	size_t row_data_length;
	int tile_is_edge = 0;
	if ((image_width - (tile_x * tile_width)) < tile_width) tile_is_edge = 1;
	if ((image_height - (tile_y * tile_height)) < tile_height) tile_is_edge = 1;
	
	if (tile_is_edge) {
		memset(image_part, 0, part_size);
	}
	
	unsigned int x_start = tile_x * tile_width;
	unsigned int x_end = x_start + tile_width;
	unsigned int y_start = tile_y * tile_height;
	unsigned int y_end = y_start + tile_height;
	if (x_end > image_width) x_end = image_width;
	if (y_end > image_height) y_end = image_height;
	
	row_data_length = 4 * (x_end - x_start);
	input_row_start = rgba_data + (4 * x_start);
	output_row_start = image_part;
	for (counter_y = y_start; counter_y < y_end; counter_y++) {
		memcpy(output_row_start, input_row_start, row_data_length);
		input_row_start += image_width * 4;
		output_row_start += tile_width * 4;
	}
	
	GLuint ret = obj->f->load_image(obj, image_part, tile_width, tile_height);
	free(image_part);
	
	return ret;
}

static void gl_texture_free_texture(gl_texture *obj)
{
	glDeleteTextures ( 1, &obj->data.textureId );
	obj->data.texture_loaded = 0;
}

static void gl_texture_free(gl_object *obj_obj)
{
	gl_texture *obj = (gl_texture *)obj_obj;
	if (obj->data.texture_loaded) {
		gl_texture_free_texture(obj);
	}
	gl_object_free_org_global(obj_obj);
}
