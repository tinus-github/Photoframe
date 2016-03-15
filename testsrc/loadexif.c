//
//  loadexif.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "loadexif.h"
#include "loadimage.h"

#include <jpeglib.h>
#include <libexif/exif-data.h>

#include <string.h>

/* Taken from jdatasrc.c; we need this behavior because we need
 * all bytes from the file */

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	struct jpeg_source_mgr *src = cinfo->src;
	
	/* Just a dumb implementation for now.  Could use fseek() except
	 * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0) {
		while (num_bytes > (long) src->bytes_in_buffer) {
			num_bytes -= (long) src->bytes_in_buffer;
			(void) (*src->fill_input_buffer) (cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			 * so suspension need not be handled.
			 */
		}
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}

static boolean fill_input_buffer_and_record(j_decompress_ptr cinfo)
{
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	struct loadexif_client_data *data = client_data->exif_data;
	
	boolean ret = data->orgf.fill_input_buffer(cinfo);
	
	if (!ret) return ret;
	
	size_t bytes_to_copy = cinfo->src->bytes_in_buffer;
	if (bytes_to_copy > (LOADEXIF_BUFFER_SIZE - data->inputsize)) {
		bytes_to_copy = LOADEXIF_BUFFER_SIZE - data->inputsize;
	}
	
	if (bytes_to_copy) {
		memcpy (data->inputdata + data->inputsize, cinfo->src->next_input_byte, bytes_to_copy);
		data->inputsize += bytes_to_copy;
	}
	
	return ret;
}



void loadexif_setup_overlay(j_decompress_ptr cinfo)
{
	struct loadexif_client_data *data = calloc(sizeof (struct loadexif_client_data), 1);
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	
	struct jpeg_source_mgr *orgsrc = cinfo->src;
	
	data->orgf.init_source = orgsrc->init_source;
	data->orgf.fill_input_buffer = orgsrc->fill_input_buffer;
	data->orgf.skip_input_data = orgsrc->skip_input_data;
	data->orgf.resync_to_restart = orgsrc->resync_to_restart;
	data->orgf.term_source = orgsrc->term_source;
	
	client_data->exif_data = data;
	
	orgsrc->skip_input_data = skip_input_data;
	orgsrc->fill_input_buffer = fill_input_buffer_and_record;
}

boolean loadexif_parse(j_decompress_ptr cinfo)
{
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	struct loadexif_client_data *data = client_data->exif_data;
	
	ExifData *result = exif_data_new_from_data(data->inputdata, data->inputsize);
	if (result) {
		exif_data_dump(result);
		return true;
	}
	return false;
}