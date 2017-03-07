//
//  loadimage.h
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#ifndef loadimage_h
#define loadimage_h

#include "fs/gl-stream.h"

unsigned char *loadTGA ( char *fileName, int *width, int *height );

typedef unsigned char *(*loadImageFunction) (gl_stream *stream,
						int wantedwidth, int wantedheight,
						int *width, int *height,
						unsigned int *orientation);

// Signature is at least an 11 char string.
loadImageFunction functionForLoadingImage(unsigned char* signature);

#endif /* loadimage_h */
