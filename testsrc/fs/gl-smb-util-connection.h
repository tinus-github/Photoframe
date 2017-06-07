//
//  gl-smb-util-connection.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/06/2017.
//
//

#ifndef gl_smb_util_connection_h
#define gl_smb_util_connection_h

#include <stdio.h>
#include <stdarg.h>
#include "../../lib/smb-rpc-util/smb-rpc-client.h"

#include "infrastructure/gl-object.h"

typedef struct gl_smb_util_connection gl_smb_util_connection;

typedef struct gl_smb_util_connection_funcs {
	gl_object_funcs p;
	smb_rpc_decode_result (*run_command_sync) (gl_smb_util_connection *obj,
						   const char **responsePacket, size_t *responsePacketSize,
						   smb_rpc_command_argument *args,
						   const smb_rpc_command_definition *commandDefinition,
						   ...);
	int (*load_config) (gl_smb_util_connection *obj);
} gl_smb_util_connection_funcs;

typedef struct gl_smb_util_connection_data {
	gl_object_data p;
	
	int commandFd;
	int responseFd;
	
	uint32_t currentInvocationId;
	
	char *packetBuf;
	size_t packetBufSize;
} gl_smb_util_connection_data;

struct gl_smb_util_connection {
	gl_smb_util_connection_funcs *f;
	gl_smb_util_connection_data data;
};

void gl_smb_util_connection_setup();
gl_smb_util_connection *gl_smb_util_connection_init(gl_smb_util_connection *obj);
gl_smb_util_connection *gl_smb_util_connection_new();
gl_smb_util_connection *gl_smb_util_connection_get_global_connection();

#endif /* gl_smb_util_connection_h */
