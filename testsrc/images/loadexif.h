//
//  loadexif.h
//  Photoframe
//
//  Created by Martijn Vernooij on 09/03/16.
//
//
// This creates a JPEG library loader that saves the data before the image
// so the EXIF data can be recovered.

#ifndef loadexif_h
#define loadexif_h

#include <stdio.h>
#include <jpeglib.h>

void loadexif_setup_overlay(j_decompress_ptr cinfo);
boolean loadexif_parse(j_decompress_ptr cinfo);
unsigned int loadexif_get_orientation(j_decompress_ptr cinfo);

// Internal

#define LOADEXIF_BUFFER_SIZE (128*1024)
// 128k should be enough for anybody

#include <stdlib.h>

struct loadexif_org_functions {
	void (*init_source) (j_decompress_ptr cinfo);
	boolean (*fill_input_buffer) (j_decompress_ptr cinfo);
	void (*skip_input_data) (j_decompress_ptr cinfo, long num_bytes);
	boolean (*resync_to_restart) (j_decompress_ptr cinfo, int desired);
	void (*term_source) (j_decompress_ptr cinfo);
};

typedef struct loadexif_client_data {
	unsigned char inputdata[LOADEXIF_BUFFER_SIZE];
	unsigned int inputsize;
	struct loadexif_org_functions orgf;
	unsigned int orientation;
} loadexif_client_data;

#endif /* loadexif_h */
