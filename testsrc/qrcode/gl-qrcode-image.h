//
//  gl-qrcode.h
//  Photoframe
//
//  Created by Martijn Vernooij on 11/04/2017.
//
//

#ifndef gl_qrcode_image_h
#define gl_qrcode_image_h

#include <stdio.h>
#include "gl-container-2d.h"
#include "gl-tile.h"

typedef struct gl_qrcode_image gl_qrcode_image;

typedef struct gl_qrcode_image_funcs {
	gl_container_2d_funcs p;
	int (*set_string) (gl_qrcode_image *obj, const char* string);
} gl_qrcode_image_funcs;

typedef struct gl_qrcode_image_data {
	gl_container_2d_data p;
	
	gl_tile *_image;
	unsigned int size;
} gl_qrcode_image_data;

struct gl_qrcode_image {
	gl_qrcode_image_funcs *f;
	gl_qrcode_image_data data;
};

void gl_qrcode_image_setup();
gl_qrcode_image *gl_qrcode_image_init(gl_qrcode_image *obj);
gl_qrcode_image *gl_qrcode_image_new();


#endif /* gl_qrcode_image_h */
