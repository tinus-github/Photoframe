//
//  loadimage-bmp.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#ifndef loadimage_bmp_h
#define loadimage_bmp_h

#include <stdio.h>

#include "fs/gl-url.h"
#include "fs/gl-stream.h"

unsigned char* loadBMP(gl_stream *stream, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation );

#endif /* loadimage_bmp_h */
