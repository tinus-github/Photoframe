//
//  gl-stream-file.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/02/2017.
//
//

#ifndef gl_stream_file_h
#define gl_stream_file_h

#include <stdio.h>
#include "fs/gl-stream.h"

typedef struct gl_stream_file gl_stream_file;

typedef struct gl_stream_file_funcs {
	gl_stream_funcs p;
} gl_stream_file_funcs;

typedef struct gl_stream_file_data {
	gl_stream_data p;

	FILE *f;
} gl_stream_file_data;

struct gl_stream_file {
	gl_stream_file_funcs *f;
	gl_stream_file_data data;
};

void gl_stream_file_setup();
gl_stream_file *gl_stream_file_init(gl_stream_file *obj);
gl_stream_file *gl_stream_file_new();


#endif /* gl_stream_file_h */
