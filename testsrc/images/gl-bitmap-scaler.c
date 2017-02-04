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
#include <assert.h>

static void gl_bitmap_scaler_start(gl_bitmap_scaler *obj);
static void gl_bitmap_scaler_end(gl_bitmap_scaler *obj);

static void add_line(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf);
static void add_line_smooth_fast_prepare(gl_bitmap_scaler *obj);

static struct gl_bitmap_scaler_funcs gl_bitmap_scaler_funcs_global = {
	.start = &gl_bitmap_scaler_start,
	.end = &gl_bitmap_scaler_end,
	.process_line = &add_line
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_bitmap_scaler_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_bitmap_scaler_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);

#if 0
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_bitmap_scaler_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_bitmap_scaler_free;
#endif
}

static void gl_bitmap_scaler_start(gl_bitmap_scaler *obj)
{
	obj->data._scaleFactor = (float)obj->data.outputWidth / obj->data.inputWidth;
	
	obj->data._scaleRest = 0;
	obj->data._current_y_out = 0;
	obj->data._current_y_in = 0;
	switch (obj->data.verticalType) {
		case gl_bitmap_scaler_type_coarse:
			obj->data._yContributions = NULL;
			break;
		case gl_bitmap_scaler_type_smooth:
			obj->data._yContributions = calloc(4 * sizeof(unsigned int), obj->data.outputWidth);
			break;
		case gl_bitmap_scaler_type_bresenham:
			add_line_smooth_fast_prepare(obj);
			break;
	}
}

static void gl_bitmap_scaler_end(gl_bitmap_scaler *obj)
{
	free(obj->data._yContributions);
	obj->data._yContributions = NULL;
	free(obj->data._yUsedLines);
	obj->data._yUsedLines = NULL;
	free(obj->data._lastLine);
	obj->data._lastLine = NULL;
	free(obj->data._combinedLine);
	obj->data._combinedLine = NULL;
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

typedef void (scale_line_func)(gl_bitmap_scaler *obj, unsigned char *outputptr, const unsigned char *inputptr);
typedef void (copy_pixel_func)(unsigned char *outputptr, const unsigned char *inputptr);

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

static copy_pixel_func *copy_pixel_func_for_type(gl_bitmap_scaler_input_type t)
{
	switch (t) {
		case gl_bitmap_scaler_input_type_rgb:
			return &copy_pixel_rgb_to_rgba;
		case gl_bitmap_scaler_input_type_rgba:
			return &copy_pixel_rgba_to_rgba;
	}
	return NULL;
}

// horizontal scaling funcs
static void scale_line_coarse(gl_bitmap_scaler *obj, unsigned char *outputptr, const unsigned char *inputptr)
{
	unsigned int current_x_in = 0;
	unsigned int current_x_out = 0;
	int scalerest = 0;
	unsigned int bytesPerPixel = inputBytesPerPixel(obj);
	
	copy_pixel_func *pixel_func = copy_pixel_func_for_type(obj->data.inputType);
	
	while (current_x_in < obj->data.inputWidth) {
		scalerest += obj->data.outputWidth;
		while (scalerest > obj->data.inputWidth) {
			pixel_func(outputptr + (4 * current_x_out), inputptr + (bytesPerPixel * current_x_in));
			
			current_x_out++;
			scalerest -= obj->data.inputWidth;
		}
		current_x_in++;
	}
}

static void scale_line_smooth(gl_bitmap_scaler *obj, unsigned char *outputptr, const unsigned char *inputptr)
{
	unsigned int current_x_in = 0;
	unsigned int current_x_out = 0;
	unsigned int x_total[3];
	x_total[0] = x_total[1] = x_total[2] = 0;
	
	unsigned int remaining_contribution;
	unsigned int possible_contribution;
	unsigned int scalerest = 0;
	unsigned int contribution;
	
	unsigned int bytesPerPixel = inputBytesPerPixel(obj);
	
	copy_pixel_func *pixel_func = copy_pixel_func_for_type(obj->data.inputType);
	
	while (current_x_in < obj->data.inputWidth) {
		remaining_contribution = obj->data.outputWidth;
		do {
			possible_contribution = obj->data.inputWidth - scalerest;
			if (possible_contribution <= remaining_contribution) {
				contribution = possible_contribution;
				if (contribution == obj->data.inputWidth) { // if (!scalerest)?
					pixel_func(outputptr, inputptr);
				} else {
					x_total[0] += contribution * inputptr[0];
					x_total[1] += contribution * inputptr[1];
					x_total[2] += contribution * inputptr[2];
					float inputWidthFactor = 1.0 / obj->data.inputWidth;
					outputptr[0] = x_total[0] * inputWidthFactor;
					outputptr[1] = x_total[1] * inputWidthFactor;
					outputptr[2] = x_total[2] * inputWidthFactor;
					outputptr[3] = 255;
				}
				outputptr += 4;
				current_x_out++;
				
				x_total[0] = x_total[1] = x_total[2] = 0;
				remaining_contribution -= contribution;
				scalerest = 0;
				continue;
			} else {
				contribution = remaining_contribution;
				
				x_total[0] += contribution * inputptr[0];
				x_total[1] += contribution * inputptr[1];
				x_total[2] += contribution * inputptr[2];
				scalerest += remaining_contribution;
				break;
			}
		} while(1);
		
		inputptr += bytesPerPixel;
		current_x_in++;
	}
	if (current_x_out < obj->data.outputWidth) {
		memset(outputptr, 0, 4 * (obj->data.outputWidth - current_x_out));
	}
}

static void scale_line_smooth_fast(gl_bitmap_scaler *obj, unsigned char *outputptr, const unsigned char *inputptr)
{
	unsigned int numpixels = obj->data.outputWidth;
	unsigned int mid = numpixels / 2;
	unsigned int accumulated_error = 0;
	unsigned int input_x = 1;
	
	unsigned int bytesPerPixel = inputBytesPerPixel(obj);
	
	copy_pixel_func *pixel_func = copy_pixel_func_for_type(obj->data.inputType);
	
	while (numpixels-- > 0) {
		if ((accumulated_error > mid) && (input_x < (obj->data.inputWidth - 1))) {
			outputptr[0] = average_channel(inputptr[bytesPerPixel], inputptr[0]);
			outputptr[1] = average_channel(inputptr[bytesPerPixel + 1], inputptr[1]);
			outputptr[2] = average_channel(inputptr[bytesPerPixel + 2], inputptr[2]);
			outputptr[3] = 255;
		} else {
			pixel_func(outputptr, inputptr);
		}
		
		outputptr += 4;
		
		accumulated_error += obj->data.inputWidth;
		unsigned int steps = accumulated_error / obj->data.outputWidth;
		accumulated_error -= steps * obj->data.outputWidth;
		inputptr += bytesPerPixel*steps;
		input_x += steps;
	}
}

static scale_line_func *scale_line_func_for_type(gl_bitmap_scaler_type t)
{
	switch (t) {
		case gl_bitmap_scaler_type_coarse:
			return &scale_line_coarse;
		case gl_bitmap_scaler_type_smooth:
			return &scale_line_smooth;
		case gl_bitmap_scaler_type_bresenham:
			return &scale_line_smooth_fast;
	}
	return NULL;
}

// vertical scaling funcs
static void add_line_coarse(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf)
{
	scale_line_func *line_func = scale_line_func_for_type(obj->data.horizontalType);
	
	obj->data._scaleRest += obj->data.outputHeight;

	while (obj->data._scaleRest > obj->data.inputHeight) {
		const unsigned char *inputptr = inputbuf;
		unsigned char *outputptr = outputbuf + 4 * obj->data.outputWidth * obj->data._current_y_out;
		
		line_func(obj, outputptr, inputptr);
		
		obj->data._current_y_out++;
		obj->data._scaleRest -= obj->data.inputHeight;
	}
}

static void add_line_smooth(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf)
{
	scale_line_func *line_func = scale_line_func_for_type(obj->data.horizontalType);

	unsigned int contribution;
	unsigned int possible_contribution;
	unsigned int remaining_contribution = obj->data.outputHeight;
	
	unsigned char *outputptr;
	const unsigned char *inputptr;
	
	unsigned int counter;
	
	do {
		outputptr = outputbuf + 4 * obj->data.outputWidth * obj->data._current_y_out;
		inputptr = inputbuf;
		
		line_func(obj, outputptr, inputptr);
		
		possible_contribution = obj->data.inputHeight - obj->data._scaleRest;
		if (possible_contribution <= remaining_contribution) {
			contribution = possible_contribution;
			if (contribution != obj->data.inputHeight) {
				for (counter = 0; counter < obj->data.outputWidth; counter++) {
					unsigned int offset = counter * 4;
					obj->data._yContributions[offset] += outputptr[offset] * contribution;
					obj->data._yContributions[offset + 1] += outputptr[offset + 1] * contribution;
					obj->data._yContributions[offset + 2] += outputptr[offset + 2] * contribution;
					outputptr[offset] = obj->data._yContributions[offset] / obj->data.inputHeight;
					outputptr[offset + 1] = obj->data._yContributions[offset + 1] / obj->data.inputHeight;
					outputptr[offset + 2] = obj->data._yContributions[offset + 2] / obj->data.inputHeight;
					outputptr[offset + 3] = 255;
				}
			}
			memset(obj->data._yContributions, 0, obj->data.outputWidth * 4 * sizeof(unsigned int));
			obj->data._current_y_out++;
			remaining_contribution -= contribution;
			obj->data._scaleRest = 0;
			continue;
		} else {
			contribution = remaining_contribution;

			for (counter = 0; counter < obj->data.outputWidth; counter++) {
				unsigned int offset = counter * 4;
				obj->data._yContributions[offset] += contribution * outputptr[offset];
				obj->data._yContributions[offset + 1] += contribution * outputptr[offset + 1];
				obj->data._yContributions[offset + 2] += contribution * outputptr[offset + 2];
			}
			obj->data._scaleRest += remaining_contribution;
			break;
		}
	} while (obj->data._current_y_out < obj->data.outputHeight); // avoid running outside the buffer
}

static void add_line_smooth_fast(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf)
{
	unsigned int wanted_line;
	int want_combine;
	unsigned int counter;
	unsigned int offset;
	
	unsigned char *outputptr;
	const unsigned char *inputptr = inputbuf;
	
	scale_line_func *line_func = scale_line_func_for_type(obj->data.horizontalType);
	
	while (obj->data._current_y_out < obj->data.outputHeight) {
		wanted_line = obj->data._yUsedLines[obj->data._current_y_out];
		want_combine = !!(wanted_line & 0x80000000);
		wanted_line &= 0x3fffffff;
		
		outputptr = outputbuf + 4 * obj->data.outputWidth * obj->data._current_y_out;
		
		if (!want_combine) {
			if (wanted_line == obj->data._current_y_in) {
				line_func(obj, outputptr, inputptr);
				obj->data._current_y_out++;
				continue;
			} else {
				assert (wanted_line > obj->data._current_y_in);
				break;
			}
		} else {
			if (wanted_line == obj->data._current_y_in) {
				line_func(obj, obj->data._lastLine, inputptr);
				break;
			} else if (wanted_line == (obj->data._current_y_in - 1)) {
				line_func(obj, obj->data._combinedLine, inputptr);
				
				for (counter = 0, offset = 0; counter < obj->data.outputWidth; counter++, offset += 4) {
					outputptr[offset] = ((unsigned int)obj->data._lastLine[offset] +
							     obj->data._combinedLine[offset] / 2);
					outputptr[offset+1] = ((unsigned int)obj->data._lastLine[offset+1] +
							       obj->data._combinedLine[offset+1] / 2);
					outputptr[offset+2] = ((unsigned int)obj->data._lastLine[offset+2] +
							       obj->data._combinedLine[offset+2] / 2);
					outputptr[offset+3] = 255;
				}
				
				obj->data._current_y_out++;
				continue;
			} else {
				assert (wanted_line > obj->data._current_y_in);
				break;
			}
		}
	}
	obj->data._current_y_in++;
}

static void add_line_smooth_fast_prepare(gl_bitmap_scaler *obj)
{
	unsigned int accumulated_error = 0;
	unsigned int mid = obj->data.outputHeight / 2;
	unsigned int current_y = 0;
	unsigned int counter;
	
	obj->data._yContributions = calloc(4 * sizeof(unsigned int), obj->data.outputWidth);
	obj->data._yUsedLines = calloc(sizeof(unsigned int), obj->data.outputHeight);
	obj->data._lastLine = calloc(sizeof(unsigned char) * 4, obj->data.outputWidth);
	obj->data._combinedLine = calloc(sizeof(unsigned char) * 4, obj->data.outputWidth);
	
	for (counter = 0; counter < obj->data.outputHeight; counter++) {
		obj->data._yUsedLines[counter] = current_y;
		if ((accumulated_error > mid) && (current_y < (obj->data.outputHeight - 1))) {
			obj->data._yUsedLines[counter] |= 0x80000000;
		}
		accumulated_error += obj->data.inputHeight;
		while (accumulated_error >= obj->data.outputHeight) {
			accumulated_error -= obj->data.outputHeight;
			current_y++;
		}
	}
}

static void add_line(gl_bitmap_scaler *obj, unsigned char *outputbuf, const unsigned char *inputbuf)
{
	switch (obj->data.verticalType) {
		case gl_bitmap_scaler_type_coarse:
			add_line_coarse(obj, outputbuf, inputbuf);
			break;
		case gl_bitmap_scaler_type_smooth:
			add_line_smooth(obj, outputbuf, inputbuf);
			break;
		case gl_bitmap_scaler_type_bresenham:
			add_line_smooth_fast(obj, outputbuf, inputbuf);
			break;
	}
}
