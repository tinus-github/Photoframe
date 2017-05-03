//
//  main.h
//  Photoframe
//
//  Created by Martijn Vernooij on 01/05/2017.
//
//

#ifndef main_h
#define main_h

#include <stdio.h>
#include "smb-rpc-buffer.h"

typedef struct appdata {
	smb_rpc_buffer *inbuf;
	smb_rpc_buffer *outbuf;
	fd_set inputset;
	fd_set outputset;
	
	FILE *infile;
	int infilefd;
	FILE *outfile;
	int outfilefd;
	
	int maxfd;
} appdata;

#endif /* main_h */
