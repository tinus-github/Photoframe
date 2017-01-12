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
#include "gl-bitmap.h"
#include "infrastructure/gl-notice-subscription.h"

#define TRUE 1
#define FALSE 0

static void gl_tiled_image_free(gl_object *obj_obj);
static void gl_tiled_image_load_image (gl_tiled_image *obj, unsigned char *rgba_data,
				       unsigned int width, unsigned int height,
				       unsigned int orientation, unsigned int tile_height);
static void gl_tiled_image_draw(gl_shape *shape_obj);

static struct gl_tiled_image_funcs gl_tiled_image_funcs_global = {
	.load_image = &gl_tiled_image_load_image
};

static void (*gl_object_free_org_global) (gl_object *obj);
static void (*gl_shape_draw_org_global) (gl_shape *obj);

void gl_tiled_image_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_tiled_image_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_tiled_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_tiled_image_free;
	
	gl_shape_funcs *shape_funcs_global = (gl_shape_funcs *) &gl_tiled_image_funcs_global;
	gl_shape_free_org_global = shape_funcs->draw;
	shape_funcs->draw = &gl_tiled_image_draw;
	
	gl_object *parent_obj = (gl_object *)parent;
	parent_obj->f->free(parent_obj);
}

static void gl_tiled_image_free(gl_object *obj_obj)
{
	gl_tiled_image *obj = (gl_tiled_image *)obj_obj;

	((gl_container *)obj)->f->remove_child((gl_container *)obj,
					       (gl_shape *)obj->data.orientation_container);
	gl_object_free_org_global(obj_obj);
}

static void gl_tiled_image_calculate_orientation_projection(gl_tiled_image *obj)
{
	gl_shape *shape_obj = (gl_shape *)obj;
	mat4x4 centered;
	mat4x4 flip;
	mat4x4 flipped;
	mat4x4 rotated;
	mat4x4 uncenter;
	mat4x4 identity;
	
	int dimensions_flipped = FALSE;
	
	gl_container *orientation_container = obj->data.orientation_container;
	gl_shape *orientation_container_shape = (gl_shape *)orientation_container;
	orientation_container_shape->f->set_computed_projection_dirty(orientation_container_shape);
	
	mat4x4_identity(identity);
	
	mat4x4_translate(centered, -0.5 * obj->data.unrotatedImageWidth, -0.5 * obj->data.unrotatedImageHeight, 0.0);
	
	switch (obj->data.orientation) {
		case 2:
		case 4:
		case 5:
		case 7:
			mat4x4_scale_aniso(flip, identity, -1.0, 1.0, 1.0);
			mat4x4_mul (flipped, flip, centered);
			break;
		default:
			mat4x4_dup(flipped, centered);
			
	}
	
	GLfloat rotation_amount;
	
	switch (obj->data.orientation) {
		case 1:
		case 2:
		default:
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
	
	mat4x4 rotation;
	
	mat4x4_rotate_Z (rotation, identity, rotation_amount);
	mat4x4_mul(rotated, rotation, flipped);
	
	if (!dimensions_flipped) {
		shape_obj->data.objectWidth = obj->data.unrotatedImageWidth;
		shape_obj->data.objectHeight = obj->data.unrotatedImageHeight;
	} else {
		shape_obj->data.objectWidth = obj->data.unrotatedImageHeight;
		shape_obj->data.objectHeight = obj->data.unrotatedImageWidth;
	}
	mat4x4_translate(uncenter, 0.5 * shape_obj->data.objectWidth, 0.5 * shape_obj->data.objectHeight, 0.0);
	mat4x4_mul(orientation_container->data.projection, uncenter, rotated);
}

static void gl_tiled_image_texture_loaded(void *target,
					  gl_notice_subscription *subscription,
					  void *data)
{
	gl_tiled_image *obj = (gl_tiled_image *)target;
	
	obj->data.tilesToLoad--;
	
	if (!obj->data.tilesToLoad) {
		obj->data.loadedNotice->f->fire(obj->data.loadedNotice);
	}
}

static void gl_tiled_image_draw(gl_shape *shape_obj)
{
	gl_tiled_image *obj = (gl_tiled_image *)shape_obj;
	
	if (!obj->data.tilesToLoad) {
		gl_shape_draw_org_global(shape_obj);
	}
}

static void gl_tiled_image_load_image (gl_tiled_image *obj, unsigned char *rgba_data,
				       unsigned int width, unsigned int height,
				       unsigned int orientation, unsigned int tile_height)
{
	gl_texture *texture;
	gl_tile *tile;
	unsigned int current_y = 0;
	
	gl_container *orientation_container = obj->data.orientation_container;
	gl_shape *shape_tile;
	gl_notice_subscription *subscription;
	
	obj->data.unrotatedImageWidth = width;
	obj->data.unrotatedImageHeight = height;
	
	obj->data.orientation = orientation;
	
	gl_bitmap *bitmap = gl_bitmap_new();
	bitmap->data.bitmap = rgba_data;
	obj->data.tilesToLoad++; // just in case the textures load immediately
	
	while (current_y < height) {
		if ((current_y + tile_height) > height) {
			tile_height = height - current_y;
		}
		
		obj->data.tilesToLoad++;
		
		texture = gl_texture_new();
		
		subscription = gl_notice_subscription_new();
		subscription->data.action = &gl_tiled_image_texture_loaded;
		subscription->data.target = obj;
		
		texture->data.loadedNotice->f->subscribe(texture->data.loadedNotice, subscription);
		
		texture->f->load_image_horizontal_tile(texture, bitmap,
						       width, height,
						       tile_height, current_y);
		
		tile = gl_tile_new();
		tile->f->set_texture(tile, texture);
		
		shape_tile = (gl_shape *)tile;
		shape_tile->data.objectY = current_y;
		
		
		orientation_container->f->append_child (orientation_container, shape_tile);
		
		current_y += tile_height;
	}
	
	gl_tiled_image_texture_loaded(obj, NULL, NULL);
	
	gl_object *bitmap_obj = (gl_object *)bitmap;
	bitmap_obj->f->unref(bitmap_obj);
	
	gl_tiled_image_calculate_orientation_projection(obj);
}

gl_tiled_image *gl_tiled_image_init(gl_tiled_image *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_tiled_image_funcs_global;
	
	obj->data.orientation_container = gl_container_new();
	gl_container *container_obj = (gl_container *)obj;

	container_obj->f->append_child(container_obj, (gl_shape *)obj->data.orientation_container);
	
	obj->data.loadedNotice = gl_notice_new();
	obj->data.tilesToLoad = 0;
	
	return obj;
}

gl_tiled_image *gl_tiled_image_new()
{
	gl_tiled_image *ret = calloc(1, sizeof(gl_tiled_image));
	
	return gl_tiled_image_init(ret);
}
