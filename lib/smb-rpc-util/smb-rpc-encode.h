//
//  smb-rpc-encode.h
//  Photoframe
//
//  Created by Martijn Vernooij on 28/04/2017.
//
//

#ifndef smb_rpc_encode_h
#define smb_rpc_encode_h

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define SMB_RPC_COMMAND_MAX_ARGUMENTS 5
#define SMB_RPC_COMMAND_MAX_RETURNS 3
#define SMB_RPC_MAX_PACKET_SIZE 10240000
#define SMB_RPC_VERSION 1


typedef enum {
	SMB_RPC_VALUE_TYPE_STRING = 0xaa01,
	SMB_RPC_VALUE_TYPE_INT = 0xaa02,
} smb_rpc_value_type;

typedef enum {
	smb_rpc_decode_result_ok = 0,
	smb_rpc_decode_result_tooshort = 1,
	smb_rpc_decode_result_invalid = 2,
	smb_rpc_decode_result_nomatch = 3
} smb_rpc_decode_result;

typedef enum {
	smb_rpc_command_argument_type_int = 1,
	smb_rpc_command_argument_type_string = 2,
	smb_rpc_command_argument_type_buffer = 101, // return value only
	smb_rpc_command_argument_type_end_of_list = 0,
} smb_rpc_command_argument_type;

typedef struct {
	smb_rpc_command_argument_type type;
	union {
		int int_value;
		struct {
			size_t length;
			const char *string;
		} string_value;
	} value;
} smb_rpc_command_argument;

// Pass NULL for output to get the required size

size_t smb_rpc_encode_stringz(char *output, size_t *output_length, char *string);
size_t smb_rpc_encode_string(char *output, size_t *output_length, const char *string, size_t length);
size_t smb_rpc_encode_int(char *output, size_t *output_length, int value);
size_t smb_rpc_encode_packet(char *output, size_t output_length,
				    uint32_t invocation_id,
				    smb_rpc_command_argument *values, size_t value_count);

smb_rpc_decode_result smb_rpc_decode_packet(char *input, size_t buflen, size_t *used_bytes, char **contents, size_t *contents_length);
smb_rpc_decode_result smb_rpc_decode_command(char *input, size_t inputlen,
					     char **command, size_t *command_length,
					     uint32_t *invocation_id,
					     smb_rpc_command_argument **args, size_t *arg_count);
smb_rpc_decode_result smb_rpc_decode_packet_get_invocation_id(char *input, size_t inputlen,
							      uint32_t *invocation_id);
smb_rpc_decode_result smb_rpc_decode_response_complete(char *input, size_t inputlen,
						       uint32_t wanted_invocation_id,
						       smb_rpc_command_argument *args,
						       smb_rpc_command_argument_type *template);

#endif /* smb_rpc_encode_h */
