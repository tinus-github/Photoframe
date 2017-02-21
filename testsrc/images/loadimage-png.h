//
//  loadimage-png.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#ifndef loadimage_png_h
#define loadimage_png_h

#include <stdio.h>

#include "fs/gl-stream.h"

unsigned char* loadPNG(gl_stream *stream, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation );

#endif /* loadimage_png_h */
