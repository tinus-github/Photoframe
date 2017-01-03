//
//  gl-texture.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#include "gl-texture.h"
#include <string.h>
#include <assert.h>

static GLuint load_image(gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height);
static GLuint load_image_monochrome(gl_texture *obj, unsigned char *monochrome_data, unsigned int width, unsigned int height);
static GLuint load_image_tile(gl_texture *obj, unsigned char *rgba_data,
			      unsigned int image_width, unsigned int image_height,
			      unsigned int tile_width, unsigned int tile_height,
			      unsigned int tile_x, unsigned int tile_y);
static GLuint load_image_horizontal_tile(gl_texture *obj, unsigned char *rgba_data,
					 unsigned int image_width, unsigned int image_height,
					 unsigned int tile_height, unsigned int tile_y);
static void gl_texture_free(gl_object *obj);

static struct gl_texture_funcs gl_texture_funcs_global = {
	.load_image = &load_image,
	.load_image_monochrome = &load_image_monochrome,
	.load_image_tile = &load_image_tile,
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

static GLuint load_image_gen(gl_texture *obj, unsigned char *image_data, unsigned int width, unsigned int height)
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
	if (obj->data.dataType != gl_texture_data_type_rgba) {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_ALPHA,
			      width, height,
			      0, GL_ALPHA, GL_UNSIGNED_BYTE, image_data );
	} else {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
			      width, height,
			      0, GL_RGBA, GL_UNSIGNED_BYTE, image_data );
	}
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	obj->data.textureId = textureId;
	obj->data.texture_loaded = 1;
	obj->data.width = width;
	obj->data.height = height;
	
	return textureId;
}

static GLuint load_image(gl_texture *obj, unsigned char *rgba_data, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_rgba;
	return load_image_gen(obj, rgba_data, width, height);
}

static GLuint load_image_monochrome(gl_texture *obj, unsigned char *monochrome_data, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_monochrome;
	return load_image_gen(obj, monochrome_data, width, height);
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

static GLuint load_image_horizontal_tile(gl_texture *obj, unsigned char *rgba_data,
					 unsigned int image_width, unsigned int image_height,
					 unsigned int tile_height, unsigned int tile_y)
{
	assert (tile_y < image_height);
	
	unsigned char *tile_data = rgba_data + (4 * sizeof(unsigned char) * image_width * tile_y);
	
	return obj->f->load_image(obj, tile_data, image_width, tile_height);
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
