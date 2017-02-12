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

unsigned char* loadBMP(char *fileName, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation );

#endif /* loadimage_bmp_h */
