//
//  smb-rpc-buffer.h
//  Photoframe
//
//  Created by Martijn Vernooij on 30/04/2017.
//
//

#ifndef smb_rpc_buffer_h
#define smb_rpc_buffer_h

#include <stdio.h>

typedef struct smb_rpc_buffer smb_rpc_buffer;

struct smb_rpc_buffer {
	char *buffer;
	size_t contentSize;
	
	size_t _bufferSize;
};

smb_rpc_buffer *smb_rpc_buffer_new();
void smb_rpc_buffer_add(smb_rpc_buffer *buffer, char *data, size_t length);
char *smb_rpc_buffer_get_data(smb_rpc_buffer *buffer, size_t *length);
void smb_rpc_buffer_consume_data(smb_rpc_buffer *buffer, size_t length);

#endif /* smb_rpc_buffer_h */
