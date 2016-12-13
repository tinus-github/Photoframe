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
#include <stdint.h>
#include "gl-object.h"

ypedef float vec4[4];
typedef vec4 mat4x4[4];

typedef struct gl_stage gl_stage;

typedef struct gl_stage_funcs {
	gl_object_funcs p;
	void (*set_dimensions) (gl_stage *obj, uint32_t width, uint32_t height);
} gl_stage_funcs;

typedef struct gl_stage_data {
	gl_object_data p;
	
	uint32_t width;
	uint32_t height;
	
	mat4x4 projection;
} gl_stage_data;

struct gl_stage {
	gl_stage_funcs *f;
	gl_stage_data data;
};

void gl_stage_setup();
gl_stage *gl_stage_init(gl_stage *obj);
gl_stage *gl_stage_new();

gl_stage *gl_stage_get_global_stage();

#endif /* gl_stage_h */
