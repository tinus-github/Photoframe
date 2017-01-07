//
//  gl-label-scroller.h
//  Photoframe
//
//  Created by Martijn Vernooij on 07/01/2017.
//
//

#ifndef gl_label_scroller_h
#define gl_label_scroller_h

#include <stdio.h>

include "gl-container-2d.h"
#include "gl-tile.h"
#include "gl-value-animation.h"
#include "labels/gl-label-scroller-segment.h"

typedef struct gl_label_scroller gl_label_scroller;

typedef struct gl_label_scroller_funcs {
	gl_container_2d_funcs p;
	void (*start) (gl_label_scroller *obj);
} gl_label_scroller_funcs;

typedef struct gl_label_scroller_data {
	gl_container_2d_data p;
	
	char *text;
	uint32_t width;
	uint32_t height;
	
	int32_t offset;
	
	gl_label_scroller_segment *segment;
	GLfloat scrollAmount;
	
	gl_value_animation *animation;
} gl_label_scroller_data;

struct gl_label_scroller {
	gl_label_scroller_funcs *f;
	gl_label_scroller_data data;
};

void gl_label_scroller_setup();
gl_label_scroller *gl_label_scroller_init(gl_label_scroller *obj);
gl_label_scroller *gl_label_scroller_new();

#endif /* gl_label_scroller_h */
