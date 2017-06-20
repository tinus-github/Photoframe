//
//  main.c
//  Photoframe
//
//  Created by Martijn Vernooij on 01/05/2017.
//
//

#include <stdlib.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "main.h"
#include "smb-rpc-buffer.h"
#include "smb-rpc-encode.h"
#include "smb-rpc-command.h"

#define READBUFSIZE 1024

static void handle_io(appdata *appData);
static void handle_input(appdata *appData);
static void handle_output(appdata *appData);
static int run_commands(appdata *appData);

int main(int argv, char **argc)
{
	appdata *appData = calloc(1, sizeof(appdata));
	
	appData->inbuf = smb_rpc_buffer_new();
	appData->infilefd = fileno(stdin);
	appData->outbuf = smb_rpc_buffer_new();
	
	// depending on smb.conf, which isn't really controlled,
 	// libsmbclient may spew stuff into stdout. So we redirect that to stderr.
	appData->outfilefd = dup(fileno(stdout));
	close(fileno(stdout));
	dup2(fileno(stderr), fileno(stdout));
	
	appData->maxfd = appData->infilefd;
	if (appData->maxfd < appData->outfilefd) {
		appData->maxfd = appData->outfilefd;
	}
	appData->maxfd++;
	
	fcntl(appData->infilefd, F_SETFL, O_NONBLOCK);
	fcntl(appData->outfilefd, F_SETFL, O_NONBLOCK);
	
	memset(appData->fileFds, 0, sizeof(int) * FD_SETSIZE);
	memset(appData->dirFds, 0, sizeof(int) * FD_SETSIZE);
	
	appData->smb_data = smb_rpc_smb_new_data();
	
	while (1) {
		handle_io(appData);
	}
}

static void handle_io(appdata *appData)
{
	FD_ZERO(&appData->inputset);
	FD_SET(appData->infilefd, &appData->inputset);
	FD_ZERO(&appData->outputset);
	if (appData->outbuf->contentSize) {
		FD_SET(appData->outfilefd, &appData->outputset);
	}
	
	errno = 0;
	int selectret = select(appData->maxfd,
			       &appData->inputset, &appData->outputset, NULL, NULL);
			       //&timeout);
	if (selectret == 0) {
		// timeout
		handle_input(appData);
	} else if (selectret < 0) {
		// < 0 means a problem occurred.
		switch (errno) {
			case EAGAIN:
			case EINTR:
				return;
			default:
				fprintf(stderr, "Select returned an error: %d\n", errno);
				abort();
		}
	} else {
		if (FD_ISSET(appData->infilefd, &appData->inputset)) {
			handle_input(appData);
		}
		if (FD_ISSET(appData->outfilefd, &appData->outputset)) {
			handle_output(appData);
		}
	}
}

static void handle_input(appdata *appData)
{
	char readbuf[READBUFSIZE];
	errno = 0;
	ssize_t num_read = read(appData->infilefd, readbuf, READBUFSIZE);
	if (num_read < 1) {
		switch (errno) {
			case EAGAIN:
			case EINTR:
				return;
			case 0:
				//done
				exit(0);
			default:
				fprintf(stderr, "Read returned an error: %d\n", errno);
				abort();
				
		}
	}
	smb_rpc_buffer_add(appData->inbuf, readbuf, num_read);
	while (run_commands(appData)) {
		;
	}
}

static void handle_output(appdata *appData)
{
	errno = 0;
	ssize_t num_written = write(appData->outfilefd, appData->outbuf->buffer, appData->outbuf->contentSize);
	if (num_written < 1) {
		switch (errno) {
			case EAGAIN:
			case EINTR:
				return;
			default:
				fprintf(stderr, "Write returned an error: %d\n", errno);
				abort();
				
		}
	}
	smb_rpc_buffer_consume_data(appData->outbuf, num_written);
}

static int run_commands(appdata *appData) {
	size_t used_bytes;
	const char *packet_contents;
	size_t packet_length;
	smb_rpc_decode_result decode_result;
	decode_result = smb_rpc_decode_packet(appData->inbuf->buffer, appData->inbuf->contentSize,
					      &used_bytes,
					      &packet_contents, &packet_length);
	
	switch(decode_result) {
		case smb_rpc_decode_result_invalid:
			fprintf(stderr, "Invalid input\n");
			abort();
		case smb_rpc_decode_result_tooshort:
			return 0;
		default:
			;
	}
	
	char *command;
	size_t command_length;
	uint32_t invocation_id;
	smb_rpc_command_argument args[SMB_RPC_COMMAND_MAX_ARGUMENTS];

	size_t arg_count;
	
	decode_result = smb_rpc_decode_command(packet_contents, packet_length,
					       &command, &command_length,
					       &invocation_id,
					       args, &arg_count);
	switch(decode_result) {
		case smb_rpc_decode_result_invalid:
		case smb_rpc_decode_result_tooshort:
			fprintf(stderr, "Invalid input packet\n");
			abort();
		default:
			;
	}
	smb_rpc_command_execute(appData,
				command, command_length,
				invocation_id,
				args, arg_count);
	
	smb_rpc_buffer_consume_data(appData->inbuf, used_bytes);
	
	return 1;
}
