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

int main(int argv, char **argc)
{
	int commandfds[2];
	int responsefds[2];
	char *args[2] = { "smb-rpc-util", NULL };
	
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
	
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	cmd_args[0].type = smb_rpc_command_argument_type_string;
	cmd_args[0].value.string_value.string = "CONNECT";
	cmd_args[0].value.string_value.length = 7;
	cmd_args[1].type = smb_rpc_command_argument_type_int;
	cmd_args[1].value.int_value = 1;
	
	packetSize = smb_rpc_encode_return_packet(packetbuf, packetSize, 1,
				     cmd_args, 2);
	size_t numwritten = write(commandfds[1], packetbuf, packetSize);
	
	char inbuf[1024];
	size_t numread = read(responsefds[0], inbuf, 1024);
	
}
