//
//  gl-label-renderer.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/01/2017.
//
//

#ifndef gl_label_renderer_h
#define gl_label_renderer_h

#include <stdio.h>

#include "infrastructure/gl-object.h"
#include "gl-tile.h"

typedef struct gl_label_renderer gl_label_renderer;

typedef struct gl_label_glyph_data {
	uint32_t codepoint;
	int32_t x;
	int32_t y;
} gl_label_renderer_glyph_data;

typedef struct gl_label_renderer_funcs {
	gl_object_funcs p;
	void (*layout) (gl_label_renderer *obj);
	gl_tile* (*render) (gl_label_renderer *obj, int32_t windowX, int32_t windowY, int32_t windowWidth, int32_t windowHeight);
} gl_label_renderer_funcs;

typedef struct gl_label_renderer_data {
	gl_object_data p;
	
	char *text; //UTF8
	uint32_t numGlyphs;
	gl_label_renderer_glyph_data *glyphData;
	
	uint32_t totalWidth;
} gl_label_renderer_data;

struct gl_label_renderer {
	gl_label_renderer_funcs *f;
	gl_label_renderer_data data;
};

void gl_label_renderer_setup();
gl_label_renderer *gl_label_renderer_init(gl_label_renderer *obj);
gl_label_renderer *gl_label_renderer_new();

#endif /* gl_label_renderer_h */
