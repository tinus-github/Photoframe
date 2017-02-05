//
//  gl-label.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#include "labels/gl-label.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define SEGMENT_WIDTH 512

// TODO: This should probably enforce a maximum length as the whole rendered string is in memory

static void gl_label_free(gl_object *obj);
static void gl_label_render(gl_label *obj);

static struct gl_label_funcs gl_label_funcs_global = {
	.render = &gl_label_render
};

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_label_render(gl_label *obj)
{
	gl_label_renderer *renderer = obj->data.renderer;
	renderer->data.text = obj->data.text;
	
	renderer->f->layout(renderer);
	
	obj->data.textWidth = renderer->data.totalWidth;
	
	uint32_t cursor = 0;
	gl_tile *tile;
	gl_shape *tile_shape;
	gl_container *obj_container = (gl_container *)obj;
	
	while (cursor < obj->data.textWidth) {
		tile = renderer->f->render(renderer,
					   cursor, 0,
					   SEGMENT_WIDTH,
					   obj->data.height);
		tile_shape = (gl_shape *)tile;
		tile_shape->data.objectX = cursor;
		
		obj_container->f->append_child(obj_container, tile_shape);
		cursor += SEGMENT_WIDTH;
	}
}

void gl_label_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_label_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_free;
	
	parent_obj->f->free(parent_obj);
	
	gl_label_renderer_setup();
}

gl_label *gl_label_init(gl_label *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_label_funcs_global;
	
	return obj;
}

gl_label *gl_label_new()
{
	gl_label *ret = calloc(1, sizeof(gl_label));
	
	ret->data.renderer = gl_label_renderer_new();
	
	return gl_label_init(ret);
}

void gl_label_free(gl_object *obj_obj)
{
	gl_label *obj = (gl_label *)obj_obj;
	free (obj->data.text);
	
	gl_object *renderer_obj = (gl_object *)obj->data.renderer;
	renderer_obj->f->unref(renderer_obj);
	
	gl_object_free_org_global(obj_obj);
}
