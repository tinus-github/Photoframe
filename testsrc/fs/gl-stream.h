//
//  gl-stream.h
//  Photoframe
//
//  Created by Martijn Vernooij on 14/02/2017.
//
//

#ifndef gl_stream_h
#define gl_stream_h

#include <stdio.h>

#include "infrastructure/gl-object.h"
#include "fs/gl-url.h"

typedef enum {
	gl_stream_error_ok = 0,
	gl_stream_error_notfound,
	gl_stream_error_end_of_file,
	gl_stream_error_invalid_operation,
	gl_stream_error_timeout,
	gl_stream_error_unspecified_error
} gl_stream_error;


typedef struct gl_stream gl_stream;

typedef struct gl_stream_funcs {
	gl_object_funcs p;
	gl_stream_error (*open) (gl_stream *obj);
	gl_stream_error (*close) (gl_stream *obj);
	gl_stream_error (*set_url) (gl_stream *obj, const char* URLstring);

	// if buffer is NULL, skip bytes instead
	size_t (*read) (gl_stream *obj, void *buffer, size_t size);
	size_t (*skip) (gl_stream *obj, size_t size);
	
	gl_stream_error (*return_error) (gl_stream *obj, gl_stream_error error);
} gl_stream_funcs;

typedef struct gl_stream_data {
	gl_object_data p;
	
	gl_url *_URL;
	gl_stream_error lastError;
} gl_stream_data;

struct gl_stream {
	gl_stream_funcs *f;
	gl_stream_data data;
};

void gl_stream_setup();
gl_stream *gl_stream_init(gl_stream *obj);
gl_stream *gl_stream_new();
gl_stream *gl_stream_new_for_url(const char *URLstring);


#endif /* gl_stream_h */
