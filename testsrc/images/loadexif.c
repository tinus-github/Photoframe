//
//  loadexif.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "images/loadexif.h"
#include "images/loadimage.h"
#include "images/loadimage-jpg.h"

#include <jpeglib.h>
#include "../lib/libexif/libexif/exif-data.h"

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

static void term_source(j_decompress_ptr cinfo)
{
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	struct loadexif_client_data *data = client_data->exif_data;
	void (*org_term_source) (j_decompress_ptr cinfo) = data->orgf.term_source;
	
	free(client_data->exif_data);
	client_data->exif_data = NULL;
	
	org_term_source(cinfo);
	return;
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
	orgsrc->term_source = term_source;
}

#ifdef EXIF_DEBUG
static void exiflogfunc(ExifLog *logger, ExifLogCode code, const char* domain,
			const char *format, va_list args, void *data)
{
	const char *title = exif_log_code_get_title(code);
	const char *message = exif_log_code_get_message(code);
	printf("%s - %s:", title, message);
	
	vprintf(format, args);
	printf("\n");
}
#endif /* EXIF_DEBUG */

boolean loadexif_parse(j_decompress_ptr cinfo)
{
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	struct loadexif_client_data *data = client_data->exif_data;
	ExifData *result = exif_data_new();

#ifdef EXIF_DEBUG
	ExifLog *logger = exif_log_new();
	exif_log_set_func(logger, &exiflogfunc, NULL);

	exif_data_log(result, logger);
#endif /* EXIF_DEBUG */
	
	exif_data_load_data(result, data->inputdata, data->inputsize);

	data->orientation = 1;
	if (result) {
		ExifEntry *entry = exif_data_get_entry(result, EXIF_TAG_ORIENTATION);
		ExifByteOrder byteOrder = exif_data_get_byte_order(result);
		if (entry && (entry->format == EXIF_FORMAT_SHORT)) {
			data->orientation = exif_get_short(entry->data, byteOrder);
			if ((data->orientation < 1) || (data->orientation > 8)) {
				data->orientation = 1;
			}
		}
		exif_data_unref(result);
		return TRUE;
	}
	return FALSE;
}

unsigned int loadexif_get_orientation(j_decompress_ptr cinfo)
{
	loadimage_jpeg_client_data *client_data = (loadimage_jpeg_client_data *)cinfo->client_data;
	struct loadexif_client_data *data = client_data->exif_data;
	
	return data->orientation;
}
