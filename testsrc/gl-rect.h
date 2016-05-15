//
//  gl-rect.h
//  Photoframe
//
//  Created by Martijn Vernooij on 24/04/16.
//
//

#ifndef gl_rect_h
#define gl_rect_h

#include "gl-object.h"

typedef struct gl_rect gl_rect;

typedef struct gl_rect_funcs {
	void (*dummy) (gl_rect obj);
} gl_rect_funcs;

#include <stdio.h>

#endif /* gl_rect_h */
