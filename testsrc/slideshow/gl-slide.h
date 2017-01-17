//
//  gl-slide.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/01/2017.
//
//

#ifndef gl_slide_h
#define gl_slide_h

#include <stdio.h>

#include "gl-container-2d.h"
#include "gl-notice.h"

typedef enum {
	gl_slide_loadstate_new = 0,
	gl_slide_loadstate_loading,
	gl_slide_loadstate_ready
} gl_slide_loadstate;

typedef struct gl_slide gl_slide;

typedef struct gl_slide_funcs {
	gl_container_2d_funcs p;
	void (*load) (gl_slide *obj);
	void (*set_loadstate)(gl_slide *obj, gl_slide_loadstate new_state);
} gl_slide_funcs;

typedef struct gl_slide_data {
	gl_container_2d_data p;
	
	gl_slide_loadstate loadstate;
	gl_notice *loadstateChanged;
} gl_slide_data;

struct gl_slide {
	gl_slide_funcs *f;
	gl_slide_data data;
};

void gl_slide_setup();
gl_slide *gl_slide_init(gl_slide *obj);
gl_slide *gl_slide_new();

#endif /* gl_slide_h */
