//
//  loadimage-jpg.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/02/2017.
//
//

#define TRUE 1
#define FALSE 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <stdint.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "images/loadimage-jpg.h"
#include "images/gl-bitmap-scaler.h"
#include "images/loadexif.h"

struct decode_error_manager {
	struct jpeg_error_mgr org;
	jmp_buf setjmp_buffer;
};

typedef struct decode_error_manager * decode_error_manager;

typedef struct {
	struct jpeg_source_mgr pub;
	gl_stream *stream;
	JOCTET *buffer;
} io_source_mgr;

/* Error handling/ignoring */

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

// jpeg custom IO functions

#define INPUT_BUF_SIZE 4096

static void io_init_source(j_decompress_ptr cinfo)
{
	// nothing to do; the stream is already open.
	return;
}


static boolean io_fill_input_buffer(j_decompress_ptr cinfo)
{
	io_source_mgr *src = (io_source_mgr *)cinfo->src;
	gl_stream *stream = src->stream;
	
	size_t num_read = stream->f->read(stream, src->buffer, INPUT_BUF_SIZE);
	if (!num_read) {
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		num_read = 2;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = num_read;
	
	return TRUE;
}

static void io_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	io_source_mgr *src = (io_source_mgr *)cinfo->src;
	
	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		if (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			src->stream->f->read(src->stream, NULL, num_bytes);
			
			src->pub.bytes_in_buffer = 0;
			
			return;
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

static void io_term_source(j_decompress_ptr cinfo)
{
	;
}

static void io_setup_stream_src(j_decompress_ptr cinfo, gl_stream *stream)
{
	io_source_mgr *src;
	
	if (!cinfo->src) {
		src = calloc(1, sizeof(io_source_mgr));
		cinfo->src = (struct jpeg_source_mgr *)src;
		src = (io_source_mgr *)cinfo->src;
		src->buffer = malloc(INPUT_BUF_SIZE * sizeof(JOCTET));
	}
	src = (io_source_mgr *)cinfo->src;
	
	src->pub.init_source = &io_init_source;
	src->pub.fill_input_buffer = &io_fill_input_buffer;
	src->pub.skip_input_data = &io_skip_input_data;
	src->pub.term_source = &io_term_source;
	src->pub.resync_to_restart = &jpeg_resync_to_restart;

	src->stream = stream;
	
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}

static void io_cleanup_stream_src(j_decompress_ptr cinfo)
{
	io_source_mgr *src = (io_source_mgr *)cinfo->src;
	
	free (src->buffer);
	src->buffer = NULL;
	src->stream = NULL;
	
	free(src);
	cinfo->src = NULL;
}

unsigned char *loadJPEG (gl_stream *stream,
			 int wantedwidth, int wantedheight,
			 int *width, int *height,
			 unsigned int *orientation )
{
	struct jpeg_decompress_struct cinfo;
	
	struct decode_error_manager jerr;
	
	struct loadimage_jpeg_client_data client_data;
	
	unsigned char *buffer = NULL;
	
	unsigned char *scanbuf = NULL;
	unsigned char *scanbufcurrentline;
	unsigned int scanbufheight;
	unsigned int lines_in_scanbuf = 0;
	unsigned int lines_in_buf = 0;
	
	JSAMPROW *row_pointers = NULL;
	
	unsigned int counter;
	
	float scalefactor, scalefactortmp;
	
	gl_bitmap_scaler *scaler = NULL;
	
	cinfo.err = jpeg_std_error(&jerr.org);
	jerr.org.error_exit = handle_decode_error;
	if (setjmp(jerr.setjmp_buffer)) {
		/* Something went wrong, abort! */
		cinfo.src->term_source(&cinfo);
		
		jpeg_destroy_decompress(&cinfo);
		
		io_cleanup_stream_src(&cinfo);
		
		stream->f->close(stream);
		
		free(buffer);
		free(scanbuf);
		free(row_pointers);
		if (scaler) {
			((gl_object *)scaler)->f->unref((gl_object *)scaler);
		}
		return NULL;
	}
	
	if (stream->f->open(stream)) {
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	cinfo.client_data = &client_data;
	
	io_setup_stream_src(&cinfo, stream);
	
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
	
	scaler->data.inputWidth = cinfo.output_width;
	scaler->data.inputHeight = cinfo.output_height;
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
		scaler->f->process_line(scaler, buffer, scanbufcurrentline);
		
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
	
	io_cleanup_stream_src(&cinfo);
	
	stream->f->close(stream);
	
	return buffer;
}
