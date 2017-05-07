//
//  smb-rpc-util-test.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/05/2017.
//
//

#include "smb-rpc-util-test.h"
#include "smb-rpc-encode.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "minini.h"

#define MAXARGS 5

static smb_rpc_command_argument cmd_args[MAXARGS];
static char packetbuf[1024];
static size_t packetbufSize = 1024;

void args_add_string(smb_rpc_command_argument *arglist, int *offset, char *string)
{
	assert(*offset < MAXARGS);
	
	arglist[*offset].type = smb_rpc_command_argument_type_string;
	arglist[*offset].value.string_value.string = string;
	arglist[*offset].value.string_value.length = strlen(string);
	
	(*offset)++;
}

void auth(int commandFd, int responseFd, const char *iniFilename)
{
	char usernamebuf[1024];
	char passwdbuf[1024];
	size_t packetSize;
	
	uint32_t invocationId = 3;
	
	char inbuf[1024];
	ini_gets("auth", "username", "", usernamebuf, 1024, iniFilename);
	ini_gets("auth", "password", "", passwdbuf, 1024, iniFilename);
	
	int argCounter = 0;
	args_add_string(cmd_args, &argCounter, "SETAUTH");
	args_add_string(cmd_args, &argCounter, ""); //specific server
	args_add_string(cmd_args, &argCounter, "WORKGROUP"); //workgroup
	args_add_string(cmd_args, &argCounter, usernamebuf);
	args_add_string(cmd_args, &argCounter, passwdbuf);
	
	packetSize = smb_rpc_encode_return_packet(packetbuf, packetbufSize, invocationId,
						  cmd_args, argCounter);
	size_t numwritten = write(commandFd, packetbuf, packetSize);
	size_t numread = read(responseFd, inbuf, 1024);
	char *packetContents;
	size_t packetContentSize;
	
	int ret = smb_rpc_decode_packet(inbuf, numread, &packetSize, &packetContents, &packetContentSize);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Failed to parse auth response: %d\n", ret);
		abort();
	}
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	ret = smb_rpc_decode_response(packetContents, packetContentSize,
				      &responseInvocationId,
				      cmd_args, &argCount);
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on SETAUTH\n");
		abort();
	}
	if (argCount != 0) {
		fprintf(stderr, "Wrong response to SETAUTH\n");
		abort();
	}
	return;
}

int main(int argv, char **argc)
{
	int commandfds[2];
	int responsefds[2];
	char *args[2] = { "smb-rpc-util", NULL };
	
	if (argv < 2) {
		fprintf(stderr, "Usage: %s inifile\n", argc[0]);
		exit(1);
	}
	
	errno = 0;
	if (pipe(commandfds)) {
		fprintf(stderr, "Failed to create pipe 1: %d\n", errno);
		abort();
	}
	errno = 0;
	if (pipe(responsefds)) {
		fprintf(stderr, "Failed to create pipe 2: %d\n", errno);
		abort();
	}
	
	if (!fork()) {
		fprintf(stderr, "Child\n");
		// This is the child
		dup2(commandfds[0], fileno(stdin));
		dup2(responsefds[1], fileno(stdout));
		close(commandfds[0]);
		close(responsefds[1]);
		close(commandfds[1]);
		close(responsefds[0]);
		
		execv("./smb-rpc-util", args);
		fprintf(stderr, "Notreached\n");
	}
	
	auth(commandfds[1], responsefds[0], argc[1]);
	
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	cmd_args[0].type = smb_rpc_command_argument_type_string;
	cmd_args[0].value.string_value.string = "FOPEN";
	cmd_args[0].value.string_value.length = strlen(cmd_args[0].value.string_value.string);
	cmd_args[1].type = smb_rpc_command_argument_type_string;

	char urlbuf[1024];
	ini_gets("", "url", "", urlbuf, 1024, argc[1]);

	
	cmd_args[1].value.string_value.string = urlbuf;
	cmd_args[1].value.string_value.length = strlen(cmd_args[1].value.string_value.string);
	
	packetSize = smb_rpc_encode_return_packet(packetbuf, packetbufSize, 1,
				     cmd_args, 2);
	size_t numwritten = write(commandfds[1], packetbuf, packetSize);
	
	char inbuf[1024];
	size_t numread = read(responsefds[0], inbuf, 1024);
	
}
