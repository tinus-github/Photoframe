//
//  loadimage-png.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#include "loadimage-png.h"
#include "images/gl-bitmap-scaler.h"

#include <png.h>
#include <zlib.h>

#include <stdlib.h>

#define TRUE 1
#define FALSE 0

#define PNG_HEADER_SIZE 8

static void custom_user_read_data(png_structp png_ptr,
				  png_bytep data,
				  png_size_t length)
{
	gl_stream *stream = (gl_stream *)png_get_io_ptr(png_ptr);
	
	gl_stream_error oRet;
	size_t num_read;
	
	num_read = stream->f->read(stream, data, length);
	
	if (num_read != length) {
		oRet = stream->data.lastError;
		
		png_error(png_ptr, "Read error");
	}
}

unsigned char* loadPNG(gl_stream *stream, int wantedwidth, int wantedheight,
		       int *width, int *height, unsigned int *orientation )
{
	
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
	
	gl_stream_error oRet;
	
	oRet = stream->f->open(stream);
	
	if (oRet != gl_stream_error_ok) {
		return NULL;
	}
	
	num_read = stream->f->read(stream, header, PNG_HEADER_SIZE);
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
	
	png_set_read_fn(png_ptr, stream, &custom_user_read_data);
	
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
	
	stream->f->close(stream);
	
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
	stream->f->close(stream);
	return NULL;
}
