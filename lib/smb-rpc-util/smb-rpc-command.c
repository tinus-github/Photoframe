//
//  smb_rpc_command.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#include "smb-rpc-command.h"
#include "smb-rpc-encode.h"
#include <string.h>
#include <stdlib.h>

static smb_rpc_command_argument staticReturns[SMB_RPC_COMMAND_MAX_RETURNS];

static void smb_rpc_command_error(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	abort();
}

static void smb_rpc_send_command_output(appdata *appData,
					uint32_t invocation_id,
					smb_rpc_command_argument *values, size_t value_count)
{
	static char *outBuf = NULL;
	static size_t outBufSize = 0;
	
	size_t required_size;
	int retry;
	
	do {
		retry = 0;
		required_size = smb_rpc_encode_return_packet(outBuf, outBufSize,
							     invocation_id,
							     values, value_count);
		while (required_size > outBufSize) {
			outBufSize = outBufSize ? outBufSize * 2 : 1024;
			retry = 1;
		}
		if (retry) {
			outBuf = outBuf ? realloc(outBuf, outBufSize) : malloc(outBufSize);
		}
	} while (retry);
	
	smb_rpc_buffer_add(appData->outbuf, outBuf, required_size);
}

static void smb_rpc_command_connect(appdata *appData, uint32_t invocation_id, smb_rpc_command_argument *args, size_t arg_count)
{
	if (arg_count != 1) {
		smb_rpc_command_error("Invalid number of arguments to CONNECT");
	}
	if (args[0].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument type to CONNECT");
	}

	int32_t version = args[0].value.int_value;
	if (version != SMB_RPC_VERSION) {
		smb_rpc_command_error("Unexpected version to CONNECT");
	}
	
	staticReturns[0].type = smb_rpc_command_argument_type_int;
	staticReturns[0].value.int_value = SMB_RPC_VERSION;
	
	smb_rpc_send_command_output(appData, invocation_id, staticReturns, 1);
	
	return;
}

static int is_command(char *actual_command, char *sent_command, size_t sent_command_length)
{
	size_t actual_command_length = strlen(actual_command);
	if (sent_command_length != actual_command_length) {
		return -1;
	}
	return (strncmp(actual_command, sent_command, actual_command_length));
}

int smb_rpc_command_execute(appdata *appData, char *command, size_t command_length,
			    uint32_t invocation_id,
			    smb_rpc_command_argument *args, size_t arg_count)
{
	if (!is_command("CONNECT", command, command_length)) {
		smb_rpc_command_connect(appData, invocation_id, args, arg_count);
		return 0;
	}
	return 1;
}
