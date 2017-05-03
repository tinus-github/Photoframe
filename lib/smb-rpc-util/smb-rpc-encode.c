//
//  smb-rpc-encode.c
//  Photoframe
//
//  Created by Martijn Vernooij on 28/04/2017.
//
//

#include "smb-rpc-encode.h"
#include <string.h>

static smb_rpc_command_argument staticArguments[SMB_RPC_COMMAND_MAX_ARGUMENTS];

struct smb_rpc_command {
	uint32_t invocation_id;
	uint32_t argument_count;
	uint32_t command_type;
	uint32_t command_length;
	char command[1];
};

struct smb_rpc_argument {
	uint32_t argument_type;
	union {
		int32_t int_value;
		int32_t string_length;
	} value;
	char string[1];
};

struct smb_rpc_packet {
	uint32_t length;
	char contents[1];
};

// Since the server and client are on the same machine, endianness issues aren't
// really relevant
static size_t output_int(char *output, int value, size_t *space)
{
	if (*space < 4) {
		*space = 0;
		return 4;
	}
	if (output) {
		int *outputI = (int *)output;
		*outputI = value;
	}
	*space -= 4;
	return 4;
}

static size_t output_string(char *output, char *string, size_t length, size_t *space)
{
	if (*space < length) {
		*space = 0;
		return length;
	}
	if (output) {
		memcpy(output, string, length);
	}
	*space -= length;
	return length;
}

size_t smb_rpc_encode_int(char *output, size_t *output_length, int value)
{
	size_t cursor = 0;
	int doit = 0;
	if (output) {
		doit = 1;
	}
	
	cursor += output_int(doit ? &output[cursor] : NULL, SMB_RPC_VALUE_TYPE_INT, output_length);
	cursor += output_int(doit ? &output[cursor] : NULL, value, output_length);
	
	return cursor;
}

size_t smb_rpc_encode_string(char *output, size_t *output_size, char *string, size_t length)
{
	size_t cursor = 0;
	
	int doit = 0;
	if (output) {
		doit = 1;
	}
	
	cursor += output_int(doit ? &output[cursor] : NULL, SMB_RPC_VALUE_TYPE_STRING, output_size);
	cursor += output_int(doit ? &output[cursor] : NULL, (int)length, output_size);
	cursor += output_string(doit ? &output[cursor] : NULL, string, length, output_size);
	return cursor;
}

size_t smb_rpc_encode_return_packet(char *output, size_t output_length,
				    uint32_t invocation_id,
				    smb_rpc_command_argument *values, size_t value_count)
{
	int doit = output ? 1 : 0;
	size_t cursor = 0;
	
	size_t dummy = output_length;
	cursor += output_int(doit ? &output[cursor] : NULL, 0, &output_length); // This will be the packet length
	
	cursor += output_int(doit ? &output[cursor] : NULL, invocation_id, &output_length);
	cursor += output_int(doit ? &output[cursor] : NULL, (int)value_count, &output_length);
	
	int value_counter;
	for (value_counter = 0; value_counter < value_count; value_counter++) {
		switch (values[value_counter].type) {
			case smb_rpc_command_argument_type_int:
				cursor += smb_rpc_encode_int(doit ? &output[cursor] : NULL,
							     &output_length,
							     values[value_counter].value.int_value);
				break;
			case smb_rpc_command_argument_type_string:
				cursor += smb_rpc_encode_string(doit ? &output[cursor] : NULL,
								&output_length,
								values[value_counter].value.string_value.string,
								values[value_counter].value.string_value.length);
				break;
		}
	}
	
	output_int(doit ? output : NULL, (int)(cursor - 4), &dummy);
	
	return cursor;
}

smb_rpc_decode_result smb_rpc_decode_packet(char *input, size_t buflen, size_t *used_bytes, char **contents, size_t *contents_length )
{
	struct smb_rpc_packet *packet = (struct smb_rpc_packet *)input;
	*contents_length = (size_t)packet->length;
	if (buflen < 4) {
		return smb_rpc_decode_result_tooshort;
	}
	if ((*contents_length) < 1) {
		return smb_rpc_decode_result_invalid;
	}
	if ((*contents_length) > SMB_RPC_MAX_PACKET_SIZE) {
		return smb_rpc_decode_result_invalid;
	}
	if ((buflen - 4) < (*contents_length)) {
		return smb_rpc_decode_result_tooshort;
	}
	
	*contents = packet->contents;
	*used_bytes = *contents_length + 4;
	
	return smb_rpc_decode_result_ok;
}

smb_rpc_decode_result smb_rpc_decode_command(char *input, size_t inputlen,
					     char **command, size_t *command_length,
					     uint32_t *invocation_id,
					     smb_rpc_command_argument **args, size_t *arg_count)
{
	size_t used_bytes = 0;
	if (inputlen < 17) {
		return smb_rpc_decode_result_tooshort;
	}
	
	struct smb_rpc_command *decoded_command = (struct smb_rpc_command *)input;
	*invocation_id = decoded_command->invocation_id;
	*arg_count = decoded_command->argument_count - 1;
	if ((*arg_count) > (size_t)SMB_RPC_COMMAND_MAX_ARGUMENTS) {
		return smb_rpc_decode_result_invalid;
	}
	if (decoded_command->command_type != SMB_RPC_VALUE_TYPE_STRING) {
		return smb_rpc_decode_result_invalid;
	}
	*command_length = decoded_command->command_length;
	used_bytes = 16;
	
	if ((inputlen - used_bytes) < (*command_length)) {
		return smb_rpc_decode_result_tooshort;
	}
	
	*command = decoded_command->command;
	used_bytes += *command_length;
	
	struct smb_rpc_argument *currentArgument;
	
	size_t argument_counter = 0;
	while (argument_counter < (*arg_count)) {
		if ((inputlen - used_bytes) < 4) {
			return smb_rpc_decode_result_tooshort;
		}
		currentArgument = (struct smb_rpc_argument *)(input + used_bytes);
		switch (currentArgument->argument_type) {
			case SMB_RPC_VALUE_TYPE_INT:
				if ((inputlen - used_bytes) < 8) {
					return smb_rpc_decode_result_tooshort;
				}
				staticArguments[argument_counter].type = smb_rpc_command_argument_type_int;
				staticArguments[argument_counter].value.int_value = currentArgument->value.int_value;
				used_bytes += 8;
				break;
			case SMB_RPC_VALUE_TYPE_STRING:
				if ((inputlen - used_bytes) < 8) {
					return smb_rpc_decode_result_tooshort;
				}
				size_t string_length = currentArgument->value.string_length;
				used_bytes += 8;
				if ((inputlen - used_bytes) < string_length) {
					return smb_rpc_decode_result_tooshort;
				}
				
				staticArguments[argument_counter].type = smb_rpc_command_argument_type_string;
				staticArguments[argument_counter].value.string_value.length = currentArgument->value.string_length;
				staticArguments[argument_counter].value.string_value.string = currentArgument->string;
				used_bytes += string_length;
				break;
			default:
				return smb_rpc_decode_result_invalid;
		}
		
		argument_counter++;
	}
	
	// The packet size makes sure we got exactly the whole command so if there's leftover bytes,
	// something went wrong.
	if (inputlen != used_bytes) {
		return smb_rpc_decode_result_invalid;
	}
	
	*args = &staticArguments[0];
	return smb_rpc_decode_result_ok;
}
