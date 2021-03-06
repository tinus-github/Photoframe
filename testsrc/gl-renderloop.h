//
//  gl-renderloop.h
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#ifndef gl_renderloop_h
#define gl_renderloop_h

#include <stdio.h>

#include "infrastructure/gl-object.h"

typedef struct gl_renderloop_member gl_renderloop_member;

typedef struct gl_renderloop gl_renderloop;

#define GL_RENDERLOOP_PHASES 6
typedef enum {
	gl_renderloop_phase_start = 0,
	gl_renderloop_phase_animate,
	// Load is a special phase: this one runs only one item per iteration. The item is
 	// automatically removed. High priority items are run first.
	gl_renderloop_phase_load,
	gl_renderloop_phase_clear,
	gl_renderloop_phase_draw,
	gl_renderloop_phase_show
} gl_renderloop_phase;


typedef struct gl_renderloop_funcs {
	gl_object_funcs p;
	void (*append_child) (gl_renderloop *obj, gl_renderloop_phase phase, gl_renderloop_member *child);
	void (*remove_child) (gl_renderloop *obj, gl_renderloop_member *child);
	void (*run) (gl_renderloop *obj);
} gl_renderloop_funcs;

typedef struct gl_renderloop_data {
	gl_object_data p;
	
	gl_renderloop_member *phaseHead[GL_RENDERLOOP_PHASES];
} gl_renderloop_data;

struct gl_renderloop {
	gl_renderloop_funcs *f;
	gl_renderloop_data data;
};

void gl_renderloop_setup();
gl_renderloop *gl_renderloop_init(gl_renderloop *obj);
gl_renderloop *gl_renderloop_new();
void gl_renderloop_loop();
void gl_renderloop_loop_once();

gl_renderloop *gl_renderloop_get_global_renderloop();

#endif /* gl_renderloop_h */
