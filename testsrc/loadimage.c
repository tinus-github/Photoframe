//
//  loadimage.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "loadimage.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
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
	unsigned char *last_line;
	unsigned char *combined_line;
} upscalestruct;

/* Scaling functions */
static inline unsigned char average_channel(unsigned int value1, unsigned int value2)
{
	return (unsigned char)((value1 + value2) >> 1);
}

static void *setup_upscale()
{
	struct upscalestruct *ret = calloc(sizeof(struct upscalestruct), 1);
	
	ret->scalerest = 0;
	ret->scalefactor = 0.0f;
	ret->current_y = 0;
	ret->current_y_out = 0;
	ret->y_contributions = NULL;
	return ret;
}

static void smoothscale_h(unsigned char *inputptr, unsigned char *outputptr,
			  unsigned int inputwidth, unsigned int outputwidth)
{
	unsigned int current_x_out = 0;
	unsigned int x_total[3];
	x_total[0] = x_total[1] = x_total[2] = 0;
	unsigned int current_x_in;
	unsigned int x_remaining_contribution;
	unsigned int x_possible_contribution;
	unsigned int x_scalerest = 0;
	unsigned int x_contribution;
	
	for (current_x_in = 0; current_x_in < inputwidth; current_x_in++) {
		x_remaining_contribution = outputwidth;
		do {
			x_possible_contribution = inputwidth - x_scalerest;
			if (x_possible_contribution <= x_remaining_contribution) {
				x_contribution = x_possible_contribution;
				if (x_contribution == inputwidth) {
					outputptr[0] = inputptr[0];
					outputptr[1] = inputptr[1];
					outputptr[2] = inputptr[2];
				} else {
					x_total[0] += x_contribution * inputptr[0];
					x_total[1] += x_contribution * inputptr[1];
					x_total[2] += x_contribution * inputptr[2];
					outputptr[0] = x_total[0] / inputwidth;
					outputptr[1] = x_total[1] / inputwidth;
					outputptr[2] = x_total[2] / inputwidth;
				}
				outputptr += 3;
				
				current_x_out++;
				x_total[0] = x_total[1] = x_total[2] = 0;
				x_remaining_contribution -= x_contribution;
				x_scalerest = 0;
				continue;
			} else {
				x_contribution = x_remaining_contribution;
				x_total[0] += x_contribution * inputptr[0];
				x_total[1] += x_contribution * inputptr[1];
				x_total[2] += x_contribution * inputptr[2];
				x_scalerest += x_remaining_contribution;
				break;
			}
		} while (1);
		
		inputptr += 3;
	}
	if (current_x_out < outputwidth) {
		bzero(outputptr, 3 * (outputwidth - current_x_out));
	}
}

/* Smooth Bresenham speed scaling */

static void smoothscale_h_fast(unsigned char *inputptr, unsigned char *outputptr, unsigned int inputwidth, unsigned int outputwidth)
{
	unsigned int numpixels = outputwidth;
	unsigned int mid = outputwidth / 2;
	int accumulated_error = 0;
	unsigned char pixel_values[3];
	unsigned int input_x = 1;
	
	while (numpixels-- > 0) {
		pixel_values[0] = inputptr[0];
		pixel_values[1] = inputptr[1];
		pixel_values[2] = inputptr[2];
		
		if ((accumulated_error > mid) && (input_x < inputwidth)) {
			pixel_values[0] = average_channel(pixel_values[0], inputptr[3]);
			pixel_values[1] = average_channel(pixel_values[1], inputptr[4]);
			pixel_values[2] = average_channel(pixel_values[2], inputptr[5]);
		}
		outputptr[0] = pixel_values[0];
		outputptr[1] = pixel_values[1];
		outputptr[2] = pixel_values[2];
		
		outputptr += 3;
		
		accumulated_error += inputwidth;
		while (accumulated_error >= outputwidth) {
			accumulated_error -= outputwidth;
			inputptr += 3; input_x++;
		}
	}
}

static void upscaleLine(unsigned char *inputbuf, unsigned int inputwidth, unsigned int inputheight,
		 unsigned char *outputbuf, unsigned int outputwidth, unsigned int outputheight,
		 unsigned int current_line_inputbuf, struct upscalestruct *data)
{
	unsigned char *outputptr;
	unsigned char *inputptr;
	
	if (data->scalefactor == 0.0f) {
		data->scalefactor = (float)outputwidth / inputwidth;
		data->total_x = outputwidth;
		data->total_y = outputheight;
		data->outputbuf = outputbuf;
		data->y_contributions = calloc(3 * sizeof(unsigned int), outputwidth);
	}
	
	/* Possible optimization:
	 * If the image is smaller than the screen, most lines will be scaled horizontally more than once.
	 * This is not very important because in that case it won't take a lot of time anyway
	 */
	
	data->scalerest += outputheight;
	
	while (data->scalerest > inputheight) {
		outputptr = outputbuf + 3 * outputwidth * data->current_y_out;
		inputptr = inputbuf;
		
		smoothscale_h_fast(inputptr, outputptr, inputwidth, outputwidth);
		
		data->current_y_out++;
		data->scalerest -= inputheight;
	}
}

static void upscaleLineSmooth(unsigned char *inputbuf, unsigned int inputwidth, unsigned int inputheight,
		       unsigned char *outputbuf, unsigned int outputwidth, unsigned int outputheight,
		       unsigned int current_line_inputbuf, struct upscalestruct *data)
{
	unsigned int y_contribution;
	unsigned int y_possible_contribution;
	unsigned int y_remaining_contribution;
	
	int counter;
	
	unsigned char *outputptr;
	unsigned char *inputptr;
	
	if (data->scalefactor == 0.0f) {
		data->scalefactor = (float)outputwidth / inputwidth;
		data->total_x = outputwidth;
		data->total_y = outputheight;
		data->outputbuf = outputbuf;
		data->y_contributions = calloc(3 * sizeof(unsigned int), outputwidth);
	}
	
	y_remaining_contribution = outputheight;
	
	/* Possible optimization:
	 * If the image is smaller than the screen, most lines will be scaled horizontally more than once.
	 * This is not very important because in that case it won't take a lot of time anyway
	 */
	
	do {
		y_possible_contribution = inputheight - data->scalerest;
		if (y_possible_contribution <= y_remaining_contribution) {
			y_contribution = y_possible_contribution;
		} else {
			y_contribution = y_remaining_contribution;
		}
		
		outputptr = outputbuf + 3 * outputwidth * data->current_y;
		inputptr = inputbuf;
		
		smoothscale_h(inputptr, outputptr, inputwidth, outputwidth);
		
		if (y_contribution != y_remaining_contribution) {
			if (y_contribution != inputheight) {
				outputptr = outputbuf + 3 * outputwidth * data->current_y;
				for (counter = (3 * outputwidth) - 1 ; counter >= 0; counter--) {
					data->y_contributions[counter] += y_contribution * outputptr[counter];
					outputptr[counter] = data->y_contributions[counter] / inputheight;
				}
			}
			bzero(data->y_contributions, outputwidth * 3 * sizeof(unsigned int));
			
			data->current_y++;
			y_remaining_contribution -= y_contribution;
			data->scalerest = 0;
			continue;
		} else {
			outputptr = outputbuf + 3 * outputwidth * data->current_y;
			for (counter = (3 * outputwidth) - 1 ; counter >= 0; counter--) {
				data->y_contributions[counter] += y_contribution * outputptr[counter];
			}
			data->scalerest += y_remaining_contribution;
			break;
		}
		
	} while (1);
	assert (data->current_y < outputheight);
}

static void upscaleLineSmoothFast(unsigned char *inputbuf, unsigned int inputwidth, unsigned int inputheight,
			   unsigned char *outputbuf, unsigned int outputwidth, unsigned int outputheight,
			   unsigned int current_line_inputbuf, struct upscalestruct *data)
{
	unsigned int accumulated_error;
	unsigned int mid;
	unsigned int current_y;
	
	int counter;
	
	unsigned char *outputptr;
	
	if (data->scalefactor == 0.0f) {
		data->scalefactor = (float)outputwidth / inputwidth;
		data->total_x = outputwidth;
		data->total_y = outputheight;
		data->outputbuf = outputbuf;
		data->current_y = 0;
		data->y_contributions = calloc(3 * sizeof(unsigned int), outputwidth);
		data->y_used_lines = calloc(sizeof(unsigned int), outputheight);
		
		accumulated_error = 0;
		current_y = 0;
		mid = outputheight / 2;
		for (counter = 0; counter < outputheight; counter++) {
			data->y_used_lines[counter] = current_y;
			if ((accumulated_error > mid) && (current_y < (inputheight - 1))) {
				data->y_used_lines[counter] |= 0x80000000;
			}
			accumulated_error += inputheight;
			while (accumulated_error >= outputheight) {
				accumulated_error -= outputheight;
				current_y++;
			}
			
		}
		data->last_line = calloc(sizeof(char) * 3, outputwidth);
		data->combined_line = calloc(sizeof(char) * 3, outputwidth);
		data->current_y_out = 0;
	}
	
	unsigned int wanted_line;
	int want_combine;
	
	while (data->current_y_out < outputheight) {
		wanted_line = data->y_used_lines[data->current_y_out];
		want_combine = !!(wanted_line & 0x80000000);
		wanted_line &= 0x3fffffff;
		
		if (!want_combine) {
			if (wanted_line == data->current_y) {
				outputptr = outputbuf + 3 * outputwidth * data->current_y_out;
				smoothscale_h_fast(inputbuf, outputptr, inputwidth, outputwidth);
				data->current_y_out++;
				continue;
			} else {
				assert (wanted_line > data->current_y);
				break;
			}
		} else {
			if (wanted_line == data->current_y) {
				smoothscale_h_fast(inputbuf, data->last_line, inputwidth, outputwidth);
				break;
			} else if (wanted_line == (data->current_y - 1)) {
				smoothscale_h_fast(inputbuf, data->combined_line, inputwidth, outputwidth);
				outputptr = outputbuf + 3 * outputwidth * data->current_y_out;
				for (counter = (3 * outputwidth) - 1 ; counter >= 0; counter--) {
					outputptr[counter] = average_channel(data->last_line[counter],
									     data->combined_line[counter]);
				}
				data->current_y_out++;
				continue;
			} else {
				assert (wanted_line > data->current_y);
				break;
			}
		}
	}
	
	assert (data->current_y < inputheight);
	data->current_y++;
	if (data->current_y == inputheight) {
		assert (data->current_y_out == outputheight);
	}
}

static void done_upscale(struct upscalestruct *data)
{
	if (data->current_y_out < data->total_y) {
		bzero(data->outputbuf + 3 * data->total_x * data->current_y_out,
		      3 * data->total_x * (data->total_y - data->current_y_out));
	}
	free (data->y_contributions);
	free (data->y_used_lines);
	free (data->last_line);
	free (data->combined_line);
	free (data);
}

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


unsigned char *loadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height )
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
	
	float scalefactor, scalefactortmp;
	void *scaledata = NULL;
	
	cinfo.err = jpeg_std_error(&jerr.org);
	jerr.org.error_exit = handle_decode_error;
	if (setjmp(jerr.setjmp_buffer)) {
		/* Something went wrong, abort! */
		jpeg_destroy_decompress(&cinfo);
		if (scaledata) {
			done_upscale(scaledata);
		}
		fclose(f);
		free(buffer);
		free(scanbuf);
		free(row_pointers);
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	cinfo.client_data = &client_data;
	
	f = fopen(fileName, "rb");
	if (f == NULL) return NULL;
	
	jpeg_stdio_src(&cinfo, f);
	jpeg_read_header(&cinfo, TRUE);
	
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
	
	scaledata = setup_upscale();
	
	*width = cinfo.output_width * scalefactor;
	*height = cinfo.output_height * scalefactor;
	
	buffer = malloc(width[0] * height[0] * 3);
	
	scanbufheight = cinfo.rec_outbuf_height;
	scanbuf = malloc(cinfo.output_width * 3 * scanbufheight);
	row_pointers = malloc(scanbufheight * sizeof(JSAMPROW));
	for (counter = 0; counter < scanbufheight; counter++) {
		row_pointers[counter] = scanbuf + 3 * (lines_in_buf * cinfo.output_width);
	}
	
	while (lines_in_buf < cinfo.output_height) {
		if (!lines_in_scanbuf) {
			lines_in_scanbuf = jpeg_read_scanlines(
							       &cinfo, row_pointers, scanbufheight);
			scanbufcurrentline = scanbuf;
		}
		if (scalefactor != 1.0f) {
			upscaleLineSmoothFast(scanbufcurrentline, cinfo.output_width, cinfo.output_height,
					      buffer, *width, *height, lines_in_buf, scaledata);
		} else {
			memcpy (buffer + 3 * (lines_in_buf * cinfo.output_width),
				scanbufcurrentline,
				3 * cinfo.output_width);
		}
		
		scanbufcurrentline += 3 * cinfo.output_width;
		lines_in_scanbuf--;
		lines_in_buf++;
	}
	
	done_upscale(scaledata);
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(scanbuf);
	free(row_pointers);
	fclose(f);
	
	return buffer;
}


/* Crude, insecure */
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
