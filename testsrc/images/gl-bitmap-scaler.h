//
//  gl-bitmap-scaler.h
//  Photoframe
//
//  Created by Martijn Vernooij on 31/01/2017.
//
//

#ifndef gl_bitmap_scaler_h
#define gl_bitmap_scaler_h

#include <stdio.h>

#include "gl-object.h"

typedef struct gl_bitmap_scaler gl_bitmap_scaler;

typedef enum {
	gl_bitmap_scaler_type_coarse,
	gl_bitmap_scaler_type_bresenham,
	gl_bitmap_scaler_type_smooth
} gl_bitmap_scaler_type;

typedef enum {
	gl_bitmap_scaler_input_type_rgb,
	gl_bitmap_scaler_input_type_rgba
} gl_bitmap_scaler_input_type;

typedef struct gl_bitmap_scaler_funcs {
	gl_object_funcs p;
	void (*start) (gl_bitmap_scaler *obj);
	void (*process_line) (gl_bitmap_scaler *obj, unsigned char *outputBuf, unsigned char *inputBuf);
	void (*end) (gl_bitmap_scaler *obj);
} gl_bitmap_scaler_funcs;

typedef struct gl_bitmap_scaler_data {
	gl_object_data p;
	
	gl_bitmap_scaler_type horizontalType;
	gl_bitmap_scaler_type verticalType;
	
	gl_bitmap_scaler_input_type inputType;
	
	unsigned int inputWidth;
	unsigned int inputHeight;
	unsigned int outputWidth;
	unsigned int outputHeight;
	
	float _scaleFactor;
	unsigned int *_yContributions;
	unsigned int _current_y_out;
	unsigned int _scaleRest;
} gl_bitmap_scaler_data;

struct gl_bitmap_scaler {
	gl_bitmap_scaler_funcs *f;
	gl_bitmap_scaler_data data;
};

void gl_bitmap_scaler_setup();
gl_bitmap_scaler *gl_bitmap_scaler_init(gl_bitmap_scaler *obj);
gl_bitmap_scaler *gl_bitmap_scaler_new();


#endif /* gl_bitmap_scaler_h */
