//
//  smb_rpc_command.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#include "smb-rpc-command.h"
#include "smb-rpc-encode.h"
#include "smb-rpc-smb.h"
#include "smb-rpc-auth.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define FD_OFFSET 100

static smb_rpc_command_argument staticReturns[SMB_RPC_COMMAND_MAX_RETURNS];

static void smb_rpc_command_error(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	abort();
}

static int alloc_fd(int *fds, int fd)
{
	assert(fd);
	int counter;
	for (counter = 0; counter < FD_SETSIZE; counter++) {
		if (!fds[counter]) {
			fds[counter] = fd;
			return counter;
		}
	}
	
	errno = EMFILE;
	return -1;
}

static void free_fd(int *fds, int fd)
{
	assert(fds[fd]);
	
	fds[fd] = 0;
}

static char *stringz_from_buf(char *buf, size_t buflen)
{
	char *ret = malloc(buflen + 1);
	strncpy(ret, buf, buflen);
	
	return ret;
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
		if (outBufSize > SMB_RPC_MAX_PACKET_SIZE) {
			smb_rpc_command_error("Command output too large");
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

static void smb_rpc_command_setauth(appdata *appData, uint32_t invocation_id,
				    smb_rpc_command_argument *args, size_t arg_count)
{
	if (arg_count != 4) {
		smb_rpc_command_error("Invalid number of arguments to SETAUTH");
	}
	if ((args[0].type != smb_rpc_command_argument_type_string) ||
	    (args[1].type != smb_rpc_command_argument_type_string) ||
	    (args[2].type != smb_rpc_command_argument_type_string) ||
	    (args[3].type != smb_rpc_command_argument_type_string)) {
		smb_rpc_command_error("Invalid type of argument to SETAUTH");
	}
	char *server, *workgroup, *username, *password;
	if (args[0].value.string_value.length) {
		server = stringz_from_buf(args[0].value.string_value.string, args[0].value.string_value.length);
	} else {
		server = NULL;
	}
	workgroup = stringz_from_buf(args[1].value.string_value.string, args[1].value.string_value.length);
	username = stringz_from_buf(args[2].value.string_value.string, args[2].value.string_value.length);
	password = stringz_from_buf(args[3].value.string_value.string, args[3].value.string_value.length);
	
	smb_rpc_auth_set_data(server, workgroup, username, password);
	free (server);
	free (workgroup);
	free (username);
	free (password);
	
	smb_rpc_send_command_output(appData, invocation_id, NULL, 0);
}

static void smb_rpc_command_fopen(appdata *appData, uint32_t invocation_id, smb_rpc_command_argument *args, size_t arg_count)
{
	if (arg_count != 1) {
		smb_rpc_command_error("Invalid number of arguments to FOPEN");
	}
	if (args[0].type != smb_rpc_command_argument_type_string) {
		smb_rpc_command_error("Invalid argument type to FOPEN");
	}
	
	char *url = malloc(args[0].value.string_value.length + 1);
	strncpy(url, args[0].value.string_value.string, args[0].value.string_value.length + 1);
	
	errno = 0;
	int smb_fd = smb_rpc_open_file(appData->smb_data, url);
	free (url);
	
	if (smb_fd < 0) {
		staticReturns[0].type = smb_rpc_command_argument_type_int;
		staticReturns[0].value.int_value = errno;
		smb_rpc_send_command_output(appData, invocation_id, staticReturns, 1);
		return;
	}

	int fd = alloc_fd(appData->fileFds, smb_fd);
	staticReturns[0].type = smb_rpc_command_argument_type_int;
	staticReturns[0].value.int_value = 0;
	staticReturns[1].type = smb_rpc_command_argument_type_int;
	staticReturns[1].value.int_value = fd + FD_OFFSET;
	smb_rpc_send_command_output(appData, invocation_id, staticReturns, 2);
	
	return;
}

#define MAX_READ_SIZE 102400

static void smb_rpc_command_fread(appdata *appData, uint32_t invocation_id, smb_rpc_command_argument *args, size_t arg_count)
{
	char readBuf[MAX_READ_SIZE];
	
	if (arg_count != 2) {
		smb_rpc_command_error("Invalid number of arguments to FREAD");
	}
	if (args[0].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument 1 type to FREAD");
	}
	if (args[1].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument 2 type to FREAD");
	}
	
	int arg_fd = args[0].value.int_value - FD_OFFSET;
	if ((arg_fd >= FD_SETSIZE) || (arg_fd < 0)) {
		smb_rpc_command_error("Invalid fd passed to FREAD");
	}
	int smb_fd = appData->fileFds[arg_fd];
	if (!smb_fd) {
		// Might want to make this non fatal
		smb_rpc_command_error("Unopened fd passed to FREAD");
	}
	size_t bufsize = args[1].value.int_value;
	if (bufsize > MAX_READ_SIZE) {
		bufsize = MAX_READ_SIZE;
	}
	
	ssize_t num_read = smb_rpc_read_file(appData->smb_data, smb_fd, readBuf, bufsize);
	
	staticReturns[0].type = smb_rpc_command_argument_type_int;
	staticReturns[0].value.int_value = errno;
	staticReturns[1].type = smb_rpc_command_argument_type_string;
	
	if (num_read < 1) {
		staticReturns[1].value.string_value.length = 0;
		staticReturns[1].value.string_value.string = "";
	} else {
		staticReturns[1].value.string_value.length = num_read;
		staticReturns[1].value.string_value.string = readBuf;
	}
	
	smb_rpc_send_command_output(appData, invocation_id, staticReturns, 2);
}

static void smb_rpc_command_fseek(appdata *appData, uint32_t invocation_id, smb_rpc_command_argument *args, size_t arg_count)
{
	if (arg_count != 3) {
		smb_rpc_command_error("Invalid number of arguments to FSEEK");
	}
	if (args[0].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument 1 type to FSEEK");
	}
	if (args[1].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument 2 type to FSEEK");
	}
	if (args[2].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument 3 type to FSEEK");
	}
	
	int arg_fd = args[0].value.int_value - FD_OFFSET;
	if ((arg_fd >= FD_SETSIZE) || (arg_fd < 0)) {
		smb_rpc_command_error("Invalid fd passed to FSEEK");
	}
	int smb_fd = appData->fileFds[arg_fd];
	if (!smb_fd) {
		// Might want to make this non fatal
		smb_rpc_command_error("Unopened fd passed to FSEEK");
	}
	
	int offset = args[1].value.int_value;
	int whence = args[2].value.int_value;
	switch (whence) {
		case SEEK_SET:
		case SEEK_CUR:
		case SEEK_END:
			break;
		default:
			smb_rpc_command_error("Invalid argument 3 passed to FSEEK");
	}
	
	off_t seek_ret = smb_rpc_seek_file(appData->smb_data, smb_fd, offset, whence);
	
	staticReturns[0].type = smb_rpc_command_argument_type_int;
	if (seek_ret == -1) {
		staticReturns[0].value.int_value = errno;
	} else {
		staticReturns[0].value.int_value = 0;
	}
	
	smb_rpc_send_command_output(appData, invocation_id, staticReturns, 1);
}


static void smb_rpc_command_fclose(appdata *appData, uint32_t invocation_id, smb_rpc_command_argument *args, size_t arg_count)
{
	if (arg_count != 1) {
		smb_rpc_command_error("Invalid number of arguments to FCLOSE");
	}
	if (args[0].type != smb_rpc_command_argument_type_int) {
		smb_rpc_command_error("Invalid argument type to FCLOSE");
	}
	
	int arg_fd = args[0].value.int_value - FD_OFFSET;
	if ((arg_fd >= FD_SETSIZE) || (arg_fd < 0)) {
		smb_rpc_command_error("Invalid fd passed to FCLOSE");
	}
	int smb_fd = appData->fileFds[arg_fd];
	if (!smb_fd) {
		// Might want to make this non fatal
		smb_rpc_command_error("Unopened fd passed to FCLOSE");
	}
	
	errno = 0;
	
	smb_rpc_close_file(appData->smb_data, smb_fd);
	free_fd(appData->fileFds, arg_fd);
	
	staticReturns[0].type = smb_rpc_command_argument_type_int;
	staticReturns[0].value.int_value = errno;
	smb_rpc_send_command_output(appData, invocation_id, staticReturns, 1);
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
	if (!is_command("FREAD", command, command_length)) {
		smb_rpc_command_fread(appData, invocation_id, args, arg_count);
		return 0;
	}
	
	if (!is_command("FSEEK", command, command_length)) {
		smb_rpc_command_fseek(appData, invocation_id, args, arg_count);
		return 0;
	}
	
	if (!is_command("FOPEN", command, command_length)) {
		smb_rpc_command_fopen(appData, invocation_id, args, arg_count);
		return 0;
	}
	
	if (!is_command("FCLOSE", command, command_length)) {
		smb_rpc_command_fclose(appData, invocation_id, args, arg_count);
		return 0;
	}

	if (!is_command("CONNECT", command, command_length)) {
		smb_rpc_command_connect(appData, invocation_id, args, arg_count);
		return 0;
	}

	if (!is_command("SETAUTH", command, command_length)) {
		smb_rpc_command_setauth(appData, invocation_id, args, arg_count);
		return 0;
	}
	smb_rpc_command_error("Invalid command\n");
	return 1;
}
