//
//  gl-directory-smb.h
//  Photoframe
//
//  Created by Martijn Vernooij on 16/06/2017.
//
//

#ifndef gl_directory_smb_h
#define gl_directory_smb_h

#include <stdio.h>
#include <dirent.h>

#include "fs/gl-directory.h"
#include "fs/gl-smb-util-connection.h"

typedef struct gl_directory_smb gl_directory_smb;

typedef struct gl_directory_smb_funcs {
	gl_directory_funcs p;
} gl_directory_smb_funcs;

typedef struct gl_directory_smb_data {
	gl_directory_data p;
	
	gl_directory_read_entry _current_entry;
	
	gl_smb_util_connection *connection;
	int dirFd;
} gl_directory_smb_data;

struct gl_directory_smb {
	gl_directory_smb_funcs *f;
	gl_directory_smb_data data;
};

void gl_directory_smb_setup();
gl_directory_smb *gl_directory_smb_init(gl_directory_smb *obj);
gl_directory_smb *gl_directory_smb_new();

#endif /* gl_directory_smb_h */
