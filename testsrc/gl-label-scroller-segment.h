//
//  gl-label-scroller-segment.h
//  Photoframe
//
//  Created by Martijn Vernooij on 06/01/2017.
//
//

#ifndef gl_label_scroller_segment_h
#define gl_label_scroller_segment_h

#include <stdio.h>

#include "gl-container-2d.h"
#include "gl-tile.h"
#include "gl-label-renderer.h"

typedef struct gl_label_scroller_segment gl_label_scroller_segment;
typedef struct gl_label_scroller_segment_child_data gl_label_scroller_segment_child_data;

typedef struct gl_label_scroller_segment_child_data {
	gl_label_scroller_segment_child_data *siblingL;
	gl_label_scroller_segment_child_data *siblingR;
	uint32_t tileIndex;
	gl_tile *tile;
} gl_label_scroller_segment_child_data;

typedef struct gl_label_scroller_segment_funcs {
	gl_container_2d_funcs p;
	void (*render) (gl_label_scroller_segment *obj);
	void (*layout) (gl_label_scroller_segment *obj);
} gl_label_funcs;

typedef struct gl_label_scroller_segment_data {
	gl_container_2d_data p;
	
	char *text;
	uint32_t width;
	uint32_t height;
	
	uint32_t textWidth;
	
	int32_t exposedSectionLeft;
	int32_t exposedSectionWidth;
	
	gl_label_renderer *renderer;
	
	gl_label_scroller_segment_child_data childDataHead;
} gl_label_data;

struct gl_label {
	gl_label_scroller_segment_funcs *f;
	gl_label_scroller_segment_data data;
};

void gl_label_scroller_segment_setup();
gl_scroller_segment_label *gl_label_scroller_segment_init(gl_label_scroller_segment *obj);
gl_scroller_segment_label *gl_label_scroller_segment_new();

#endif /* gl_label_scroller_segment_h */
