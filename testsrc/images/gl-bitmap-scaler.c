//
//  gl-bitmap-scaler.c
//  Photoframe
//
//  Created by Martijn Vernooij on 31/01/2017.
//
//

#include "gl-bitmap-scaler.h"
#include <string.h>
#include <stdlib.h>

static void gl_bitmap_scaler_start(gl_bitmap_scaler *obj);
static void gl_bitmap_scaler_end(gl_bitmap_scaler *obj);

static void add_line_coarse(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf);

static struct gl_bitmap_scaler_funcs gl_bitmap_scaler_funcs_global = {
	.start = &gl_bitmap_scaler_start,
	.end = &gl_bitmap_scaler_end,
	.process_line = &add_line_coarse
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_bitmap_scaler_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_bitmap_scaler_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_bitmap_scaler_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_bitmap_scaler_free;
}

static void gl_bitmap_scaler_start(gl_bitmap_scaler *obj)
{
	obj->data._scaleFactor = (float)obj->data.outputwidth / obj->data.inputwidth;
	
	obj->data._yContributions = calloc(4 * sizeof(unsigned int), obj->data.outputwidth);
	
	obj->data._scaleRest = 0;
	obj->data._current_y_out = 0;
}

static void gl_bitmap_scaler_end(gl_bitmap_scaler *obj)
{
	free(obj->data._yContributions);
	obj->data._yContributions = NULL;
}

gl_bitmap_scaler *gl_bitmap_scaler_init(gl_bitmap_scaler *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_bitmap_scaler_funcs_global;
	
	return obj;
}

gl_bitmap_scaler *gl_bitmap_scaler_new()
{
	gl_bitmap_scaler *ret = calloc(1, sizeof(gl_bitmap_scaler));
	
	return gl_bitmap_scaler_init(ret);
}


// scaling
// helper funcs
static unsigned int inputBytesPerPixel(gl_bitmap_scaler *obj)
{
	switch (obj->data.inputType) {
		case gl_bitmap_scaler_input_type_rgb:
			return 3;
		case gl_bitmap_scaler_input_type_rgba:
			return 4;
	}
	assert(obj->data.inputType <= gl_bitmap_scaler_input_type_rgba);
	return 0;
}

static inline unsigned char average_channel(unsigned char value1, unsigned char value2)
{
	return (unsigned char)((value1 + value2) >> 1);
}

static void copy_pixel_rgb_to_rgba(unsigned char *outputptr, const unsigned char *inputptr)
{
	outputptr[0] = inputptr[0];
	outputptr[1] = inputptr[1];
	outputptr[2] = inputptr[2];
	outputptr[3] = 255;
}

static void copy_pixel_rgba_to_rgba(unsigned char *outputptr, const unsigned char *inputptr)
{
	outputptr[0] = inputptr[0];
	outputptr[1] = inputptr[1];
	outputptr[2] = inputptr[2];
	outputptr[3] = inputptr[3];
}

// horizontal scaling funcs
static void scale_line_coarse(gl_bitmap_scaler *obj, unsigned char *outputptr, const unsigned char *inputptr)
{
	unsigned int current_x_in = 0;
	unsigned int current_x_out = 0;
	int scalerest = 0;
	unsigned int bytesPerPixel = inputBytesPerPixel(obj);
	
	void (* copy_pixel_func) (unsigned char *outputptr, const unsigned char *inputptr);
	
	switch (obj->data.inputType) {
		case gl_bitmap_scaler_input_type_rgb:
			copy_pixel_func = &copy_pixel_rgb_to_rgba;
			break;
		case gl_bitmap_scaler_input_type_rgba:
			copy_pixel_func = &copy_pixel_rgba_to_rgba;
			break;
	}
	
	while (current_x_in < obj->data.inputWidth) {
		scalerest += outputWidth;
		while (scalerest > obj->data.inputWidth) {
			copy_pixel_func(outputptr + (4 * current_x_out), inputptr + (bytesPerPixel * current_x_in));
			
			current_x_out++;
			scalerest -= obj->data.inputWidth;
		}
		current_x_in++;
	}
}

// vertical scaling funcs
static void add_line_coarse(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf)
{
	obj->data._scaleRest += obj->data.outputHeight;

	while (obj->data._scaleRest > obj->data.inputHeight) {
		const unsigned char *inputptr = inputbuf;
		unsigned char *outputptr = outputbuf + 4 * obj->data.outputWidth * obj->data._current_y_out;
		
		scale_line_coarse(obj, outputptr, inputptr);
		
		obj->data._current_y_out++;
		obj->data._scaleRest -= obj->data.inputHeight;
	}
}
