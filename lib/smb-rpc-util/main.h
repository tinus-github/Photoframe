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
#include <sys/select.h>
#include "smb-rpc-buffer.h"
#include "smb-rpc-smb.h"

typedef struct appdata {
	smb_rpc_buffer *inbuf;
	smb_rpc_buffer *outbuf;
	fd_set inputset;
	fd_set outputset;
	
	int infilefd;
	int outfilefd;
	
	int maxfd;
	
	int fileFds[FD_SETSIZE];
	int dirFds[FD_SETSIZE];
	
	smb_rpc_smb_data *smb_data;
} appdata;

#endif /* main_h */
