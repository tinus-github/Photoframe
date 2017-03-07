//
//  gl-stream-rewindable.h
//  Photoframe
//
//  Created by Martijn Vernooij on 22/02/2017.
//
//

#ifndef gl_stream_rewindable_h
#define gl_stream_rewindable_h

#include <stdio.h>

#include "fs/gl-stream.h"

typedef struct gl_stream_rewindable gl_stream_rewindable;

typedef struct gl_stream_rewindable_funcs {
	gl_stream_funcs p;
	
	void (*set_stream) (gl_stream_rewindable *obj, gl_stream *stream);
	gl_stream_error (*rewind) (gl_stream_rewindable *obj, const unsigned char* buffer, size_t size);
} gl_stream_rewindable_funcs;

typedef struct gl_stream_rewindable_data {
	gl_stream_data p;
	
	gl_stream *_orgStream;
	unsigned char *_buffer;
	size_t _bufferSize;
	size_t _cursor;
	
	unsigned int _open_count;
} gl_stream_rewindable_data;

struct gl_stream_rewindable {
	gl_stream_rewindable_funcs *f;
	gl_stream_rewindable_data data;
};

void gl_stream_rewindable_setup();
gl_stream_rewindable *gl_stream_rewindable_init(gl_stream_rewindable *obj);
gl_stream_rewindable *gl_stream_rewindable_new();

#endif /* gl_stream_rewindable_h */
