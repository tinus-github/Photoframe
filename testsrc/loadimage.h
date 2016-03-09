//
//  loadimage.h
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#ifndef loadimage_h
#define loadimage_h

unsigned char *loadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height );

unsigned char *loadTGA ( char *fileName, int *width, int *height );

#endif /* loadimage_h */
