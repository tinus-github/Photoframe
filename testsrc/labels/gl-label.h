//
//  gl-label.h
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#ifndef gl_label_h
#define gl_label_h

#include <stdio.h>

#include "gl-container-2d.h"
#include "gl-tile.h"
#include "labels/gl-label-renderer.h"

typedef struct gl_label gl_label;

typedef struct gl_label_funcs {
	gl_container_2d_funcs p;
	void (*render) (gl_label *obj);
} gl_label_funcs;

typedef struct gl_label_data {
	gl_container_2d_data p;
	
	char *text;
	uint32_t width;
	uint32_t height;
	
	uint32_t textWidth;
	
	gl_label_renderer *renderer;
} gl_label_data;

struct gl_label {
	gl_label_funcs *f;
	gl_label_data data;
};

void gl_label_setup();
gl_label *gl_label_init(gl_label *obj);
gl_label *gl_label_new();


#endif /* gl_label_h */
