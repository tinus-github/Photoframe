//
//  gl-stream-smb.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/06/2017.
//
//

#ifndef gl_stream_smb_h
#define gl_stream_smb_h

#include <stdio.h>
#include "fs/gl-stream.h"
#include "fs/gl-smb-util-connection.h"

typedef struct gl_stream_smb gl_stream_smb;

typedef struct gl_stream_smb_funcs {
	gl_stream_funcs p;
} gl_stream_smb_funcs;

typedef struct gl_stream_smb_data {
	gl_stream_data p;
	
	int fd;
	gl_smb_util_connection *connection;
} gl_stream_smb_data;

struct gl_stream_smb {
	gl_stream_smb_funcs *f;
	gl_stream_smb_data data;
};

void gl_stream_smb_setup();
gl_stream_smb *gl_stream_smb_init(gl_stream_smb *obj);
gl_stream_smb *gl_stream_smb_new();

#endif /* gl_stream_smb_h */
