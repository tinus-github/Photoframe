//
//  loadimage-bmp.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#include "loadimage-bmp.h"

//
//  loadimage-png.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#include "loadimage-bmp.h"
#include "images/gl-bitmap-scaler.h"

#include "qdbmp.h"

#include <stdlib.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0

unsigned char* loadBMP(char *fileName, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation )
{
	BMP *bmp = NULL;
	
	unsigned int imageWidth;
	unsigned int imageHeight;
	
	gl_bitmap_scaler *scaler = NULL;
	unsigned char* row = NULL;
	unsigned char* buffer = NULL;
	
	bmp = BMP_CreateReadStruct();
	if (!bmp || (BMP_GetError() != BMP_OK)) {
		goto loadBMPCancel0;
	}
	
	BMP_OpenFile(bmp, fileName);
	if (BMP_GetError() != BMP_OK) {
		goto loadBMPCancel1;
	}
	
	if (BMP_ReadHeader(bmp) != BMP_OK) {
		goto loadBMPCancel1;
	}
	
	imageWidth = (unsigned int)BMP_GetWidth(bmp);
	imageHeight = (unsigned int)BMP_GetHeight(bmp);
	
	float scalefactor = (float)wantedwidth / imageWidth;
	float scalefactortmp = (float)wantedheight / imageHeight;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}
	
	ssize_t rowBytes = BMP_GetBytesPerRow(bmp);
	row = calloc(1, rowBytes);
	
	scaler = gl_bitmap_scaler_new();
	
	scaler->data.inputWidth = imageWidth;
	scaler->data.inputHeight = imageHeight;
	scaler->data.outputWidth = imageHeight * scalefactor;
	scaler->data.outputHeight = imageHeight * scalefactor;
	scaler->data.inputType = gl_bitmap_scaler_input_type_rgb;
	
	scaler->data.horizontalType = gl_bitmap_scaler_type_bresenham;
	scaler->data.verticalType = gl_bitmap_scaler_type_bresenham;
	
	scaler->f->start(scaler);
	
	buffer = calloc(sizeof(unsigned char) * 4, scaler->data.outputWidth * scaler->data.outputHeight);

	unsigned int counter;
	for (counter = 0; counter < imageHeight; counter++) {
		if (BMP_ReadRow(bmp, row) != BMP_OK) {
			goto loadBMPCancel2;
		}
		scaler->f->process_line(scaler, buffer, row);
	}
	
	scaler->f->end(scaler);
	
	free (row);
	((gl_object *)scaler)->f->unref((gl_object *)scaler);
	
	BMP_Free(bmp);
	
	*width = scaler->data.outputWidth;
	*height = scaler->data.outputHeight;
	*orientation = 4; // Flip vertically
	
	return buffer;
	
loadBMPCancel2:
	free(buffer);
	if (scaler) {
		((gl_object *)scaler)->f->unref((gl_object *)scaler);
	}
	free(row);
	
loadBMPCancel1:
	BMP_Free(bmp);
loadBMPCancel0:
	return NULL;
}
