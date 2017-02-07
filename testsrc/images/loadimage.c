//
//  loadimage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "images/loadimage.h"
#include "images/loadexif.h"
#include "images/gl-bitmap-scaler.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <stdint.h>

#include <jpeglib.h>

#include <png.h>
#include <zlib.h>

// from esUtil.h
#define TRUE 1
#define FALSE 0

/* Error handling/ignoring */

struct decode_error_manager {
	struct jpeg_error_mgr org;
	jmp_buf setjmp_buffer;
};

typedef struct decode_error_manager * decode_error_manager;

static void handle_decode_error(j_common_ptr info)
{
	decode_error_manager jerr = (decode_error_manager)info->err;
	(*info->err->output_message) (info);
	longjmp (jerr->setjmp_buffer, 1);
}

/* optimized builtin scaling */
static void setup_dct_scale(struct jpeg_decompress_struct *cinfo, float scalefactor)
{
	/* The library provides for accelerated scaling at fixed ratios of 1/4 and 1/2.
	 * This keeps some margin to prevent scaling artifacts
	 */
	
	if (scalefactor < 0.23f) {
		cinfo->scale_num = 1; cinfo->scale_denom = 4;
		return;
	}
	if (scalefactor < 0.46f) {
		cinfo->scale_num = 1; cinfo->scale_denom = 2;
		return;
	}
	return;
}

/* Returns true if this orientation flips width and height */
static boolean orientation_flips(unsigned int orientation)
{
	switch (orientation) {
		case 1:
		case 2:
		case 3:
		case 4:
			return FALSE;
		case 5:
		case 6:
		case 7:
		case 8:
			return TRUE;
		default:
			return FALSE;
	}
}

unsigned char *loadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height, unsigned int *orientation )
{
	struct jpeg_decompress_struct cinfo;
	
	struct decode_error_manager jerr;
	
	struct loadimage_jpeg_client_data client_data;
	
	FILE *f;
	
	unsigned char *buffer = NULL;
	
	unsigned char *scanbuf = NULL;
	unsigned char *scanbufcurrentline;
	unsigned int scanbufheight;
	unsigned int lines_in_scanbuf = 0;
	unsigned int lines_in_buf = 0;
	
	JSAMPROW *row_pointers = NULL;
	
	unsigned int counter;
	unsigned int inputoffset;
	unsigned int outputoffset;
	unsigned char *outputcurrentline;
	
	float scalefactor, scalefactortmp;
	
	gl_bitmap_scaler *scaler = NULL;
	
	cinfo.err = jpeg_std_error(&jerr.org);
	jerr.org.error_exit = handle_decode_error;
	if (setjmp(jerr.setjmp_buffer)) {
		/* Something went wrong, abort! */
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
		free(buffer);
		free(scanbuf);
		free(row_pointers);
		if (scaler) {
			((gl_object *)scaler)->f->unref((gl_object *)scaler);
		}
		return NULL;
	}
	
	f = fopen(fileName, "rb");
	if (!f) {
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	cinfo.client_data = &client_data;
	
	jpeg_stdio_src(&cinfo, f);
	
	loadexif_setup_overlay(&cinfo);
	
	jpeg_read_header(&cinfo, TRUE);
	
	loadexif_parse(&cinfo);
	*orientation = loadexif_get_orientation(&cinfo);
	
	if (orientation_flips(*orientation)) {
		int tmpheight = wantedheight;
		wantedheight = wantedwidth;
		wantedwidth = tmpheight;
	}
	
	cinfo.out_color_space = JCS_RGB;
	
	scalefactor = (float)wantedwidth / cinfo.image_width;
	scalefactortmp = (float)wantedheight / cinfo.image_height;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}
	setup_dct_scale(&cinfo, scalefactor);
	
	jpeg_start_decompress(&cinfo);
	
	scalefactor = (float)wantedwidth / cinfo.output_width;
	scalefactortmp = (float)wantedheight / cinfo.output_height;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}

	scaler = gl_bitmap_scaler_new();
	
	scaler->data.inputWidth = cinfo.image_width;
	scaler->data.inputHeight = cinfo.image_height;
	scaler->data.outputWidth = cinfo.output_width * scalefactor;
	scaler->data.outputHeight = cinfo.output_height * scalefactor;
	scaler->data.inputType = gl_bitmap_scaler_input_type_rgb;
	
	scaler->data.horizontalType = gl_bitmap_scaler_type_bresenham;
	scaler->data.verticalType = gl_bitmap_scaler_type_bresenham;
	
	scaler->f->start(scaler);

	*width = cinfo.output_width * scalefactor;
	*height = cinfo.output_height * scalefactor;
	
	buffer = malloc(width[0] * height[0] * 4);
	
	scanbufheight = cinfo.rec_outbuf_height;
	scanbuf = malloc(cinfo.output_width * 3 * scanbufheight);
	row_pointers = malloc(scanbufheight * sizeof(JSAMPROW));
	for (counter = 0; counter < scanbufheight; counter++) {
		row_pointers[counter] = scanbuf + 3 * (counter * cinfo.output_width);
	}
	
	while (lines_in_buf < cinfo.output_height) {
		if (!lines_in_scanbuf) {
			lines_in_scanbuf = jpeg_read_scanlines(
							       &cinfo, row_pointers, scanbufheight);
			scanbufcurrentline = scanbuf;
		}
		if (scalefactor != 1.0f) {
			scaler->f->process_line(scaler, buffer, scanbufcurrentline);
		} else {
			inputoffset = outputoffset = 0;
			outputcurrentline = buffer + 4  * (lines_in_buf * cinfo.output_width);
			for (counter = 0; counter < cinfo.output_width; counter++) {
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = scanbufcurrentline[inputoffset++];
				outputcurrentline[outputoffset++] = 255;
			}
		}
		
		scanbufcurrentline += 3 * cinfo.output_width;
		lines_in_scanbuf--;
		lines_in_buf++;
	}
	
	scaler->f->end(scaler);
	((gl_object *)scaler)->f->unref((gl_object *)scaler);
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(scanbuf);
	free(row_pointers);
	fclose(f);
	
	return buffer;
}

#define PNG_HEADER_SIZE 8

unsigned char* loadPNG(char *fileName, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation )
{
	FILE *f;
	unsigned char header[PNG_HEADER_SIZE];
	ssize_t num_read;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	
	int color_type;
	int bit_depth;
	
	unsigned int imageWidth;
	unsigned int imageHeight;
	
	gl_bitmap_scaler *scaler = NULL;
	unsigned char* row = NULL;
	unsigned char* buffer = NULL;
	
	f = fopen(fileName, "rb");
	if (!f) {
		return NULL;
	}
	
	num_read = fread(header, 1, PNG_HEADER_SIZE, f);
	if (num_read != PNG_HEADER_SIZE) {
		// file is too short to be a PNG
		goto loadPNGCancel0;
	}
	
	if (png_sig_cmp(header, 0, num_read)) {
		// not a PNG
		goto loadPNGCancel0;
	}
	png_ptr = png_create_read_struct(
					 PNG_LIBPNG_VER_STRING,
					 NULL, //(png_voidp)user_error_ptr,
					 NULL, //user_error_fn,
					 NULL //user_warning_fn
					 );
	
	if (!png_ptr) {
		goto loadPNGCancel0;
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		goto loadPNGCancel1;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		goto loadPNGCancel2;
	}
	
	// TODO: Custom io
	png_init_io(png_ptr, f);
	
	png_set_sig_bytes(png_ptr, PNG_HEADER_SIZE);
	
	png_set_user_limits(png_ptr, 10000, 10000); // 100k should be enough for everyone
	
	png_color_16 background_color;
	background_color.red = 0;
	background_color.green = 0;
	background_color.blue = 0; //black
	
	png_read_info(png_ptr, info_ptr);
	
	png_uint_32 png_height;
	png_uint_32 png_width;
	
	png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &color_type, NULL, NULL, NULL);
	
	imageWidth = (unsigned int)png_width;
	imageHeight = (unsigned int)png_height;
	
	float scalefactor = (float)wantedwidth / imageWidth;
	float scalefactortmp = (float)wantedheight / imageHeight;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}

	
	png_set_background(png_ptr,
			   &background_color,
			   PNG_BACKGROUND_GAMMA_SCREEN, 0, 1);
	
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	
	ssize_t rowBytes = 4 * imageWidth;
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
	
	// TODO: This is wrong if the image is interlaced.
	unsigned int counter;
	for (counter = 0; counter < imageHeight; counter++) {
		png_read_row(png_ptr, row, NULL);
		scaler->f->process_line(scaler, buffer, row);
	}
	
	scaler->f->end(scaler);
	
	free (row);
	((gl_object *)scaler)->f->unref((gl_object *)scaler);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	
	*width = scaler->data.outputWidth;
	*height = scaler->data.outputHeight;
	*orientation = 0;
	
	return buffer;
	
loadPNGCancel2:
	free(buffer);
	if (scaler) {
		((gl_object *)scaler)->f->unref((gl_object *)scaler);
	}
	free(row);
	
	
loadPNGCancel1:
	
	if (info_ptr) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	} else {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
	}
	
loadPNGCancel0:
	fclose(f);
	return NULL;
}


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
