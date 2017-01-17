//
//  gl-slide-image.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#ifndef gl_slide_image_h
#define gl_slide_image_h

#include <stdio.h>
#include "slideshow/gl-slide.h"
#include "gl-framed-shape.h"
#include "infrastructure/gl-notice.h"

typedef struct gl_slide_image gl_slide_image;

typedef struct gl_slide_image_funcs {
	gl_slide_funcs p;
} gl_slide_image_funcs;

typedef struct gl_slide_image_data {
	gl_slide_data p;
	
	char *filename;
	
	gl_framed_shape *_slideShape;
} gl_slide_image_data;

struct gl_slide_image {
	gl_slide_image_funcs *f;
	gl_slide_image_data data;
};

void gl_slide_image_setup();
gl_slide_image *gl_slide_image_init(gl_slide_image *obj);
gl_slide_image *gl_slide_image_new();


#endif /* gl_slide_image_h */
