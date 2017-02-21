//
//  loadimage-jpg.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#ifndef loadimage_jpg_h
#define loadimage_jpg_h

#include <stdio.h>
#include "fs/gl-stream.h"

unsigned char *loadJPEG ( gl_stream *stream, int wantedwidth, int wantedheight,
		  int *width, int *height, unsigned int *orientation );

// Private, but shared with the exif loading code

typedef struct loadimage_jpeg_client_data {
	void *image_data;
	void *exif_data;
} loadimage_jpeg_client_data;

#endif /* loadimage_jpg_h */
