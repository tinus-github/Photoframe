//
//  gl-tiled-image.c
//  Photoframe
//
//  Created by Martijn Vernooij on 25/12/2016.
//
//

#include "gl-tiled-image.h"

#include "gl-texture.h"
#include "gl-tile.h"
#include "../lib/linmath/linmath.h"

#define TRUE 1
#define FALSE 0

static void gl_tiled_image_free(gl_object *obj_obj);
static void gl_tiled_image_load_image (gl_tiled_image *obj, unsigned char *rgba_data,
				       unsigned int width, unsigned int height,
				       unsigned int orientation, unsigned int tile_height);

static struct gl_tiled_image_funcs gl_tiled_image_funcs_global = {
	.load_image = &gl_tiled_image_load_image
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_tiled_image_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_tiled_image_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_tiled_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_tiled_image_free;
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
}

static void gl_tiled_image_free(gl_object *obj_obj)
{
	gl_tiled_image *obj = (gl_tiled_image *)obj_obj;
	if (!obj->data.keep_image_data) {
		free(obj->data.rgba_data);
	}
	
	gl_object_free_org_global(obj_obj);
}

static void gl_tiled_image_calculate_orientation_projection(gl_tiled_image *obj)
{
	gl_shape *shape_obj = (gl_shape *)obj;
	mat4x4 centered;
	mat4x4 flipped;
	mat4x4 rotated;
	int dimensions_flipped = FALSE;
	
	mat4x4_translate(centered, -0.5 * shape_obj->data.objectWidth, -0.5 * shape_obj->data.objectHeight, 0.0);
	switch (obj->data.orientation) {
		case 2:
		case 4:
		case 5:
		case 7:
			mat4x4_scale_aniso(flipped, centered, -1.0, 1.0, 1.0);
			break
		default:
			mat4x4_dup(flipped, centered);
			
	}
	
	GLfloat rotation_amount;
	
	switch (obj->data.orientation) {
		case 1:
		case 2:
			rotation_amount = 0.0; break;
		case 3:
		case 4:
			rotation_amount = M_PI; break;
		case 5:
		case 8:
			dimensions_flipped = TRUE;
			rotation_amount = 1.5 * M_PI; break;
		case 6:
		case 7:
			dimensions_flipped = TRUE;
			rotation_amount = 0.5 * M_PI; break;
	}
	mat4x4_rotate_Z (rotated, flipped, rotation_amount);
	
	gl_container *orientation_container = obj->data.orientation_container;
	
	if (!dimensions_flipped) {
		mat4x4_translate(orientation_container->data.projection,
				 0.5 * shape_obj->data.objectWidth, 0.5 * shape_obj->data.objectHeight, 0.0);
	} else {
		mat4x4_translate(orientation_container->data.projection,
				 0.5 * shape_obj->data.objectHeight, 0.5 * shape_obj->data.objectWidth, 0.0);
	}
	
	gl_shape *orientation_container_shape = (gl_shape *)orientation_container;
	
	orientation_container_shape->f->set_computed_projection_dirty(orientation_container_shape);
}

static void gl_tiled_image_load_image (gl_tiled_image *obj, unsigned char *rgba_data,
				       unsigned int width, unsigned int height,
				       unsigned int orientation, unsigned int tile_height)
{
	gl_texture *texture;
	gl_tile *tile;
	unsigned int current_y = 0;
	
	gl_shape *shape_obj = (gl_shape *)obj;
	gl_container *container_obj = (gl_container *)obj;
	gl_container *orientation_container = obj->data.orientation_container;
	gl_shape *shape_tile;
	
	shape_obj->data.objectWidth = width;
	shape_obj->data.objectHeight = height;
	
	obj->data.orientation = orientation;
	
	while (current_y < height) {
		if ((current_y + tile_height) > height) {
			tile_height = height - current_y;
		}
		
		texture = gl_texture_new();
		texture->f->load_image_horizontal_tile(texture, rgba_data,
						       width, height,
						       tile_height, current_y);
		
		tile = gl_tile_new();
		tile->f->set_texture(tile, texture);
		
		shape_tile = (gl_shape *)tile;
		shape_tile->data.objectY = current_y;
		
		
		orientation_container->f->append_child (orientation_container, shape_tile);
		
		current_y += tile_height;
	}
	
	if (!obj->data.keep_image_data) {
		free(rgba_data);
	}
	
	gl_tiled_image_calculate_orientation_projection(obj);
}

gl_tiled_image *gl_tiled_image_init(gl_tiled_image *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_tiled_image_funcs_global;
	
	obj->data.orientation_container = gl_container_new();
	gl_container *container_obj = (gl_container *)obj;

	container_obj->f->append_child(container_obj, (gl_shape *)obj->data.orientation_container);
	return obj;
}

gl_tiled_image *gl_tiled_image_new()
{
	gl_tiled_image *ret = calloc(1, sizeof(gl_tiled_image));
	
	return gl_tiled_image_init(ret);
}
