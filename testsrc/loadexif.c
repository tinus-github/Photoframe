//
//  loadexif.c
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//

#include "loadexif.h"
#include "loadimage.h"

void loadexif_setup_overlay(j_decompress_ptr cinfo)
{
	struct loadexif_client_data *data = calloc(sizeof (struct loadexif_client_data), 1);
	loadimage_client_data *client_data = (loadimage_client_data *)cinfo->client_data;
	
	data->orgf.init_source = cinfo->src.init_source;
	data->orgf.fill_input_buffer = cinfo->src.fill_input_buffer;
	data->orgf.skip_input_data = cinfo->src.skip_input_data;
	data->orgf.resync_to_restart = cinfo->src.resync_to_restart;
	data->orgf.term_source = cinfo->src.term_source;
	
	client_data->exif_data = data;
}
