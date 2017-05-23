//
//  smb-rpc-client.h
//  Photoframe
//
//  Created by Martijn Vernooij on 23/05/2017.
//
//

#ifndef smb_rpc_client_h
#define smb_rpc_client_h

#include "smb-rpc-encode.h"

typedef enum {
	smb_rpc_dirent_type_none = 0,
	smb_rpc_dirent_type_file = 1,
	smb_rpc_dirent_type_dir = 2,
} smb_rpc_dirent_type;

typedef struct smb_rpc_dirent {
	smb_rpc_dirent_type type;
	char *name;
	
	// excluding the terminating \0
	size_t namelen;
} smb_rpc_dirent;

#endif /* smb_rpc_client_h */
