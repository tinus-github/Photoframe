//
//  loadimage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "images/loadimage.h"
#include "images/loadimage-jpg.h"
#include "images/loadimage-png.h"
#include "images/loadimage-bmp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <stdint.h>
#include <setjmp.h>

#include "qdbmp.h"

loadImageFunction functionForLoadingImage(unsigned char* signature)
{
	if ((signature[0] == 0xff) &&
	    (signature[1] == 0xd8) &&
	    (signature[2] == 0xff) &&
	    (signature[3] == 0xe0) &&
	    (signature[6] == 0x4a) &&
	    (signature[7] == 0x46) &&
	    (signature[8] == 0x49) &&
	    (signature[9] == 0x46) &&
	    (signature[10] == 0x00)) {
		return &loadJPEG;
	}
	
	if ((signature[0] == 0x89) &&
	    (signature[1] == 0x50) &&
	    (signature[2] == 0x4e) &&
	    (signature[3] == 0x47) &&
	    (signature[4] == 0x0d) &&
	    (signature[5] == 0x0a) &&
	    (signature[6] == 0x1a) &&
	    (signature[7] == 0x0a)) {
		return &loadPNG;
	}
	
	if ((signature[0] == 0x42) &&
	    (signature[1] == 0x4d)) {
		// A long signature is smart. This is a Microsoft format.
		return &loadBMP;
	}
	
	return NULL;
}

// from esUtil.h

/* Crude, insecure */
/* Needs update for RGBA layout */
unsigned char* loadTGA ( char *fileName, int *width, int *height )
{
	unsigned char *buffer = NULL;
	FILE *f;
	unsigned char tgaheader[12];
	unsigned char attributes[6];
	unsigned int imagesize;
	
	assert("Functionality is not ready for use" == NULL);
	
	f = fopen(fileName, "rb");
	if(f == NULL) return NULL;
	
	if(fread(&tgaheader, sizeof(tgaheader), 1, f) == 0)
	{
		fclose(f);
		return NULL;
	}
	
	if(fread(attributes, sizeof(attributes), 1, f) == 0)
	{
		fclose(f);
		return 0;
	}
	
	*width = attributes[1] * 256 + attributes[0];
	*height = attributes[3] * 256 + attributes[2];
	imagesize = attributes[4] / 8 * *width * *height;
	//imagesize *= 4/3;
	printf("Origin bits: %d\n", attributes[5] & 030);
	printf("Pixel depth %d\n", attributes[4]);
	buffer = malloc(imagesize);
	if (buffer == NULL)
	{
		fclose(f);
		return 0;
	}
	
#if 1
	// invert - should be reflect, easier is 180 rotate
	int n = 1;
	while (n <= imagesize) {
		fread(&buffer[imagesize - n], 1, 1, f);
		n++;
	}
#else
	// as is - upside down
	if(fread(buffer, 1, imagesize, f) != imagesize)
	{
		free(buffer);
		return NULL;
	}
#endif
	fclose(f);
	return buffer;
}
