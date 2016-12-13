//
//  gl-stage.h
//  Photoframe
//
//  Created by Martijn Vernooij on 13/12/2016.
//
//

#ifndef gl_stage_h
#define gl_stage_h

#include <stdio.h>
#include "gl-object.h"

typedef struct gl_stage gl_stage;

typedef struct gl_stage_funcs {
	gl_object_funcs p;
	void (*set_dimensions) (gl_stage *obj, uint32_t width, uint32_t height);
} gl_stage_funcs;

typedef struct gl_stage_data {
	gl_object_data p;
	
	uint32_t width;
	uint32_t height;
} gl_stage_data;

struct gl_stage {
	gl_stage_funcs *f;
	gl_stage_data data;
};

void gl_stage_setup();
gl_stage *gl_stage_init(gl_stage *obj);
gl_stage *gl_stage_new();


#endif /* gl_stage_h */
