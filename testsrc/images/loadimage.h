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
		  int *width, int *height, unsigned int *orientation );

unsigned char *loadTGA ( char *fileName, int *width, int *height );

unsigned char* loadPNG(char *fileName, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation );

// internal

typedef struct loadimage_jpeg_client_data {
	void *image_data;
	void *exif_data;
} loadimage_jpeg_client_data;

#endif /* loadimage_h */
