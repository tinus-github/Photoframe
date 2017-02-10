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

unsigned char* loadPNG(char *fileName, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation );

#endif /* loadimage_png_h */
