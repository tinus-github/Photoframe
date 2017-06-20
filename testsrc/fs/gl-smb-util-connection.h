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
#include <pthread.h>

#include "../../lib/smb-rpc-util/smb-rpc-client.h"

#include "infrastructure/gl-object.h"

typedef struct gl_smb_util_connection_auth gl_smb_util_connection_auth;

struct gl_smb_util_connection_auth {
	const char *server;
	const char *workgroup;
	const char *username;
	const char *password;
	gl_smb_util_connection_auth *next;
};

typedef struct gl_smb_util_connection_request gl_smb_util_connection_request;

struct gl_smb_util_connection_request {
	uint32_t invocationId;
	
	pthread_cond_t responseReady;
	
	gl_smb_util_connection_request *next;
};

typedef struct gl_smb_util_connection gl_smb_util_connection;

typedef struct gl_smb_util_connection_funcs {
	gl_object_funcs p;
	void (*set_async) (gl_smb_util_connection *obj);
	smb_rpc_decode_result (*run_command_sync) (gl_smb_util_connection *obj,
						   const char **responsePacket, size_t *responsePacketSize,
						   smb_rpc_command_argument *args,
						   const smb_rpc_command_definition *commandDefinition,
						   ...);
	smb_rpc_decode_result (*run_command_async) (gl_smb_util_connection *obj,
						   const char **responsePacket, size_t *responsePacketSize,
						   smb_rpc_command_argument *args,
						   const smb_rpc_command_definition *commandDefinition,
						   ...);
	void (*run_command_async_completed) (gl_smb_util_connection *obj);

	void (*authenticate) (gl_smb_util_connection *obj, const char *server, const char *workgroup, const char *username, const char *password);
} gl_smb_util_connection_funcs;

typedef struct gl_smb_util_connection_data {
	gl_object_data p;
	
	int commandFd;
	int responseFd;
	
	int _isAsync;
	
	uint32_t currentInvocationId;
	
	char *iPacketBuf;
	size_t iPacketBufSize;
	size_t iPacketBufContents;
	size_t iPacketSize;
	
	char *oPacketBuf;
	size_t oPacketBufSize;
		
	gl_smb_util_connection_auth *authentication;
	gl_smb_util_connection_auth *last_authentication;
	
	pthread_mutex_t syncMutex;
	pthread_cond_t readingCompleteCondition;
	
	gl_smb_util_connection_request *requestQueue;
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
