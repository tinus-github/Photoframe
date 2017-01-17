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
#include "gl-container-2d.h"
#include "gl-framed-shape.h"
#include "infrastructure/gl-notice.h"

typedef enum {
	gl_slide_loadstate_new = 0,
	gl_slide_loadstate_loading,
	gl_slide_loadstate_ready
} gl_slide_loadstate;

typedef struct gl_slide_image gl_slide_image;

typedef struct gl_slide_image_funcs {
	gl_container_2d_funcs p;
	void (*load) (gl_slide_image *obj);
} gl_slide_image_funcs;

typedef struct gl_slide_image_data {
	gl_container_2d_data p;
	
	gl_slide_loadstate loadstate;
	gl_notice loadstateChanged;
	
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
