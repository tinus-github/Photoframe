//
//  gl-framed-shape.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "gl-framed-shape.h"
#include "gl-rectangle.h"

static void (*gl_object_free_org_global) (gl_object *obj);
static void gl_framed_shape_update_frame(gl_framed_shape *obj);
static void gl_framed_shape_free(gl_object *obj_obj);

static struct gl_framed_shape_funcs gl_framed_shape_funcs_global = {
	.update_frame = &gl_framed_shape_update_frame
};

static void gl_framed_shape_replace_contained_shape(gl_framed_shape *obj, gl_shape *new_shape)
{
	if (obj->data._contained_shape) {
		((gl_container *)obj)->f->remove_child((gl_container *)obj, obj->data._contained_shape);
		((gl_object *)obj->data._contained_shape)->f->unref((gl_object *)obj->data._contained_shape);
		obj->data._contained_shape = NULL;
	}
	
	if (new_shape) {
		((gl_container *)obj)->f->append_child((gl_container *)obj, new_shape);
		obj->data._contained_shape = new_shape;
		((gl_object *)obj->data._contained_shape)->f->ref((gl_object *)obj->data._contained_shape);
	}
}

static void gl_framed_shape_remove_borders(gl_framed_shape *obj)
{
	int counter;
	for (counter = 0; counter < 4; counter++) {
		gl_shape *border = (gl_shape *)obj->data._borders[counter];
		
		if (border) {
			((gl_container *)obj)->f->remove_child((gl_container *)obj, border);
			obj->data._borders[counter] = NULL;
		}
	}
}

static void gl_framed_shape_add_border(gl_framed_shape *obj, unsigned int index,
				       GLfloat x, GLfloat y, GLfloat width, GLfloat height)
{
	assert (index < 4);
	assert (!obj->data._borders[index]);
	
	gl_rectangle *rectangle = gl_rectangle_new();
	gl_shape *rectangle_shape = (gl_shape *)rectangle;
	rectangle_shape->data.objectX = x;
	rectangle_shape->data.objectY = y;
	rectangle_shape->data.objectWidth = width;
	rectangle_shape->data.objectHeight = height;
	
	obj->data._borders[index] = rectangle;
	((gl_container *)obj)->f->append_child((gl_container *)obj, rectangle_shape);
	
	rectangle->f->set_color(rectangle, 0.0, 0.0, 0.0, 1.0);
}

static void gl_framed_shape_update_frame(gl_framed_shape *obj)
{
	gl_framed_shape_remove_borders(obj);
	
	if (obj->data.shape != obj->data._contained_shape) {
		gl_framed_shape_replace_contained_shape(obj, obj->data.shape);
	}
	
	if (!obj->data._contained_shape) {
		return;
	}
	
	gl_shape *contained_shape = obj->data._contained_shape;
	
	GLfloat ownWidth = ((gl_shape *)obj)->data.objectWidth;
	GLfloat ownHeight = ((gl_shape *)obj)->data.objectHeight;
	
	GLfloat shapeWidth = contained_shape->data.objectWidth;
	GLfloat shapeHeight = contained_shape->data.objectHeight;
	
	GLfloat leftBorder = floorf(((ownWidth - shapeWidth) / 2.0));
	GLfloat topBorder = floorf(((ownHeight - shapeHeight) / 2.0));
	GLfloat rightBorder = ownWidth - (leftBorder + shapeWidth);
	GLfloat bottomBorder = ownHeight - (topBorder + shapeHeight);
	
	contained_shape->data.objectX = leftBorder;
	contained_shape->data.objectY = topBorder;
	if (leftBorder > 0.0) {
		gl_framed_shape_add_border(obj, 0,
					   0.0, topBorder,
					   leftBorder, shapeHeight);
	}
	if (rightBorder > 0.0) {
		gl_framed_shape_add_border(obj, 1,
					   leftBorder + shapeWidth, topBorder,
					   rightBorder, shapeHeight);
	}
	if (topBorder > 0.0) {
		gl_framed_shape_add_border(obj, 2,
					   0.0, 0.0,
					   ownWidth, topBorder);
	}
	if (bottomBorder > 0.0) {
		gl_framed_shape_add_border(obj, 3,
					   0.0, topBorder + shapeHeight,
					   ownWidth, bottomBorder);
	}
	
	((gl_shape *)obj)->f->set_computed_projection_dirty((gl_shape *)obj);
}

void gl_framed_shape_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_framed_shape_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_framed_shape_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_framed_shape_free;
}

gl_framed_shape *gl_framed_shape_init(gl_framed_shape *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_framed_shape_funcs_global;
	
	return obj;
}

gl_framed_shape *gl_framed_shape_new()
{
	gl_framed_shape *ret = calloc(1, sizeof(gl_framed_shape));
	
	return gl_framed_shape_init(ret);
}

static void gl_framed_shape_free(gl_object *obj_obj)
{
	gl_framed_shape *obj = (gl_framed_shape *)obj_obj;
	
	gl_framed_shape_replace_contained_shape(obj, NULL);
	
	gl_framed_shape_remove_borders(obj);

	gl_object_free_org_global(obj_obj);
}
