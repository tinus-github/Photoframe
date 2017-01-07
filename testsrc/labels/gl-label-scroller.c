//
//  gl-label-scroller.c
//  Photoframe
//
//  Created by Martijn Vernooij on 07/01/2017.
//
//

#define TRUE 1
#define FALSE 0

#include "labels/gl-label-scroller.h"

static void (*gl_object_free_org_global) (gl_object *obj);
static void gl_label_scroller_free(gl_object *obj);
static void gl_label_scroller_start(gl_label_scroller *obj);

static struct gl_label_scroller_funcs gl_label_scroller_funcs_global = {
	.start = &gl_label_scroller_start,
};

static void gl_label_scroller_scroll(gl_label_scroller *obj, GLfloat scrollTo)
{
	gl_shape *segment_shape = (gl_shape *)obj->data.segment;
	gl_label_scroller_segment *segment = obj->data.segment;
	obj->data.scrollAmount = scrollTo;
	segment_shape->data.objectX = obj->data.scrollAmount;
	segment_shape->f->set_computed_projection_dirty(segment_shape);
	
	segment->data.exposedSectionLeft = -obj->data.scrollAmount;
	segment->f->render(segment);
}

static void gl_label_scroller_animate(void *target, void *extra_data, GLfloat value)
{
	gl_label_scroller_scroll((gl_label_scroller *)target, value);
}

static void gl_label_scroller_setup_scroller(gl_label_scroller *obj)
{
	gl_shape *obj_shape = (gl_shape *)obj;
	gl_container *obj_container = (gl_container *)obj;
	
	obj->data.segment = gl_label_scroller_segment_new();
	
	gl_label_scroller_segment *segment = obj->data.segment;
	
	segment->data.height = obj_shape->data.objectHeight;
	segment->data.exposedSectionWidth = obj_shape->data.objectWidth;
	segment->data.text = strdup(obj->data.text);
	
	segment->f->layout(segment);
	obj_container->f->append_child(obj_container, (gl_shape *)segment);
	
	gl_shape *segment_shape = (gl_shape *)segment;
	
	segment_shape->data.objectX = obj_shape->data.objectWidth;
	obj->data.scrollAmount = obj_shape->data.objectWidth;
}

static void gl_label_scroller_start(gl_label_scroller *obj)
{
	gl_shape *obj_shape = (gl_shape *)obj;
	gl_value_animation *animation = gl_value_animation_new();
	animation->data.target = obj;
	animation->data.action = &gl_label_scroller_animate;
	animation->data.startValue = obj_shape->data.objectWidth;
	gl_label_scroller_segment *segment = obj->data.segment;
	animation->data.endValue = 0.0 - segment->data.textWidth;
	animation->f->set_speed(animation, 180);
	animation->data.repeats = TRUE;
	
	animation->f->start(animation);
}

void gl_label_scroller_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	gl_object *parent_obj = (gl_object *)parent;

	memcpy(&gl_label_scroller_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	parent_obj->f->free(parent_obj);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_scroller_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_scroller_free;
}

gl_label_scroller *gl_label_scroller_init(gl_label_scroller *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_label_scroller_funcs_global;
	
	return obj;
}

gl_label_scroller *gl_label_scroller_new()
{
	gl_label_scroller *ret = calloc(1, sizeof(gl_label_scroller));
	
	return gl_label_scroller_init(ret);
}

static void gl_label_scroller_free(gl_object *obj_obj)
{
	gl_label_scroller *obj = (gl_label_scroller *)obj_obj;
	
	free (obj->data.text);
	gl_object *segment_obj = (gl_object *)obj->data.segment;
	
	if (segment_obj) {
		segment_obj->f->unref(segment_obj);
	}
	
	if (obj->data.animation) {
		gl_value_animation *animation = obj->data.animation;
		animation->f->pause(animation);
		gl_object *animation_obj = (gl_object *)animation;
		animation_obj->f->unref(animation_obj);
	}
	
	gl_object_free_org_global(obj_obj);
}
