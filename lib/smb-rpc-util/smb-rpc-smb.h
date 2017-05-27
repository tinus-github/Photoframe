//
//  smb-rpc-smb.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#ifndef smb_rpc_smb_h
#define smb_rpc_smb_h

#include <stdio.h>
#include "smb-rpc-client.h"

typedef struct smb_rpc_smb_data smb_rpc_smb_data;

smb_rpc_smb_data *smb_rpc_smb_new_data();
// Must make sure to close everything associaled with this context,
// otherwise freeing it will fail
void smb_rpc_smb_free_data(smb_rpc_smb_data *data);
int smb_rpc_open_file(smb_rpc_smb_data *data, const char *url);
ssize_t smb_rpc_read_file(smb_rpc_smb_data *data, int fd, void *buf, size_t bufsize);
off_t smb_rpc_seek_file(smb_rpc_smb_data *data, int fd, off_t offset, int whence);
int smb_rpc_close_file(smb_rpc_smb_data *data, int fd);

int smb_rpc_open_dir(smb_rpc_smb_data *data, const char *url);
const smb_rpc_dirent *smb_rpc_read_dir(smb_rpc_smb_data *data, int fd);
void smb_rpc_close_dir(smb_rpc_smb_data *data, int fd);

#endif /* smb_rpc_smb_h */
