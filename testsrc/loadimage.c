//
//  loadimage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "loadimage.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>

#include <jpeglib.h>

#include <setjmp.h>

// from esUtil.h
#define TRUE 1
#define FALSE 0

typedef struct upscalestruct {
	unsigned int scalerest;
	float scalefactor;
	unsigned int current_y;
	unsigned int current_y_out;
	unsigned int total_x;
	unsigned int total_y;
	void *outputbuf;
	unsigned int *y_contributions;
	
	unsigned int *y_used_lines;
	unsigned int *y_avgs;
	char *last_line;
	char *combined_line;
} upscalestruct;

char *esLoadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height )
{
	return NULL;
}
