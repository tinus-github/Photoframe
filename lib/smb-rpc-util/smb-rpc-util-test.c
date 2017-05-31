//
//  smb-rpc-util-test.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/05/2017.
//
//

#include "smb-rpc-util-test.h"
#include "smb-rpc-client.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include "minIni.h"

#define MAXARGS 5

static smb_rpc_command_argument cmd_args[MAXARGS];
static char packetbuf[1024];
static size_t packetbufSize = 1024;

uint32_t get_invocationId()
{
	static int32_t currentId = 1;
	return currentId++;
}

void args_add_string(smb_rpc_command_argument *arglist, int *offset, const char *string)
{
	assert(*offset < MAXARGS);
	
	arglist[*offset].type = smb_rpc_command_argument_type_string;
	arglist[*offset].value.string_value.string = string;
	arglist[*offset].value.string_value.length = strlen(string);
	
	(*offset)++;
}

void args_add_int(smb_rpc_command_argument *arglist, int *offset, int value)
{
	assert(*offset < MAXARGS);
	
	arglist[*offset].type = smb_rpc_command_argument_type_int;
	arglist[*offset].value.int_value = value;
	
	(*offset)++;
}

void write_completely(int fd, char *buf, size_t buflen)
{
	ssize_t num_written;
	while (buflen) {
		errno = 0;
		num_written = write(fd, buf, buflen);
		if (num_written < 1) {
			switch (errno) {
				case 0:
				case EAGAIN:
				case EINTR:
					break;
				default:
					fprintf(stderr, "Write error: %d\n", errno);
					abort();
			}
		} else {
			buf += num_written;
			buflen -= num_written;
		}
	}
}

size_t encode_command_packet(char *packetbuf, size_t packetbufSize,
			     uint32_t invocationId,
			     const smb_rpc_command_argument_type *argTypes,
			     ...)
{
	int argCounter = 0;
	
	va_list ap;
	
	va_start(ap, argTypes);
	size_t counter = 0;
	
	const char *commandName = va_arg(ap, const char *);
	args_add_string(cmd_args, &argCounter, commandName);
	
	while (argTypes[counter]) {
		switch (argTypes[counter]) {
			case smb_rpc_command_argument_type_string:
				args_add_string(cmd_args, &argCounter, va_arg(ap, const char *));
				break;
			case smb_rpc_command_argument_type_int:
				args_add_int(cmd_args, &argCounter, va_arg(ap, uint32_t));
				break;
			default:
				fprintf(stderr, "Illegal command argument type\n");
				abort();
		}
		counter++;
	}
	
	va_end(ap);

	return smb_rpc_encode_packet(packetbuf, packetbufSize,
				     invocationId,
				     cmd_args, argCounter);
}

void read_packet(int fd, char *buf, size_t buflen, size_t *packetSize)
{
	char *packetbufCursor = buf;
	size_t remaining = buflen;
	size_t bufUsed = 0;
	char *packetContents;
	size_t packetContentsLength;
	ssize_t numread;
	
	do {
		errno = 0;
		numread = read(fd, packetbufCursor, remaining);
		if (numread < 0) {
			switch (errno) {
				case EAGAIN:
				case EINTR:
				case 0:
					continue;
				default:
					fprintf(stderr, "Read error: %d\n", errno);
					abort();
			}
		}
		if (!numread) { // If this were acceptable, we would have found out the previous round
			fprintf(stderr, "EOF while reading.\n");
			abort();
		}
		bufUsed += numread;
		int parseRet = smb_rpc_decode_packet(buf, bufUsed,
						     packetSize, &packetContents, &packetContentsLength);
		switch (parseRet) {
			case smb_rpc_decode_result_ok:
				if (*packetSize != bufUsed) {
					fprintf(stderr, "Extra data after packet: %zu vs %zu\n", bufUsed, *packetSize);
					// just drop it?
				}
				return;
			case smb_rpc_decode_result_invalid:
				fprintf(stderr, "Invalid packet.\n");
				abort();
			case smb_rpc_decode_result_tooshort:
				break;
			default:
				abort();
		}
		
		packetbufCursor += numread;
		remaining -= numread;
	} while (numread);
}

void do_auth(int commandFd, int responseFd, const char *iniFilename)
{
	char usernamebuf[1024];
	char passwdbuf[1024];
	size_t packetSize;
	
	uint32_t invocationId = get_invocationId();
	
	char inbuf[1024];
	ini_gets("auth", "username", "", usernamebuf, 1024, iniFilename);
	ini_gets("auth", "password", "", passwdbuf, 1024, iniFilename);
	
	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_setauth,
					   "SETAUTH",
					   "", 		//specific server
					   "WORKGROUP", //workgroup
					   usernamebuf,
					   passwdbuf
					   );
	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, inbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(inbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
				      &responseInvocationId,
				      cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on SETAUTH\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on SETAUTH\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 0);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to SETAUTH\n");
		abort();
	}
	return;
}

int do_fopen(int commandFd, int responseFd, char *url)
{
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	uint32_t invocationId = get_invocationId();
	
	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_fopen,
					   "FOPEN",
					   url);
	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, packetbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on FOPEN\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on FOPEN\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 2,
				     smb_rpc_command_argument_type_int,
				     smb_rpc_command_argument_type_int);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to FOPEN\n");
		abort();
	}
	
	if (cmd_args[0].value.int_value != 0) {
		errno = cmd_args[0].value.int_value;
		perror("FOPEN");
		exit(1);
	}
	
	return cmd_args[1].value.int_value;
}

#define PACKETSIZE 103424
ssize_t do_read(int commandFd, int responseFd, int smb_fd, char *buf, size_t bufSize)
{
	char packetbuf[PACKETSIZE];
	size_t packetSize = PACKETSIZE;
	smb_rpc_command_argument cmd_args[3];
	uint32_t invocationId = get_invocationId();
	
	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_fread,
					   "FREAD",
					   smb_fd,
					   (int)bufSize);
	
	write_completely(commandFd, packetbuf, packetSize);

	size_t numread;
	read_packet(responseFd, packetbuf, PACKETSIZE, &numread);
	size_t packetContentSize;
	char *packetContents;

	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on FREAD\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on FREAD\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 2,
				     smb_rpc_command_argument_type_int,
				     smb_rpc_command_argument_type_string);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to FREAD\n");
		abort();
	}
	
	if (cmd_args[0].value.int_value != 0) {
		errno = cmd_args[0].value.int_value;
		perror("FREAD");
		return -1;
	}
	memcpy(buf, cmd_args[1].value.string_value.string, cmd_args[1].value.string_value.length);
	return cmd_args[1].value.string_value.length;
}

int do_fclose(int commandFd, int responseFd, int smbfd)
{
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	uint32_t invocationId = get_invocationId();

	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_fclose,
					   "FCLOSE",
					   smbfd);

	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, packetbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on FCLOSE\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on FCLOSE\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 1,
				     smb_rpc_command_argument_type_int);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to FCLOSE\n");
		abort();
	}
	if (cmd_args[0].value.int_value) {
			errno = cmd_args[0].value.int_value;
			perror("FCLOSE");
		exit(1);
	}
	
	return cmd_args[0].value.int_value;
}

int do_dopen(int commandFd, int responseFd, char *url)
{
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	uint32_t invocationId = get_invocationId();
	
	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_dopen,
					   "DOPEN",
					   url);

	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, packetbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on DOPEN\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on DOPEN\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 2,
				     smb_rpc_command_argument_type_int,
				     smb_rpc_command_argument_type_int);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to DOPEN\n");
		abort();
	}
	
	if (cmd_args[0].value.int_value != 0) {
		errno = cmd_args[0].value.int_value;
		perror("DOPEN");
		exit(1);
	}
	
	return cmd_args[1].value.int_value;
}

int do_dread(int commandFd, int responseFd, int smbfd, smb_rpc_dirent_type *entryType, char **entryName)
{
	static char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[3];
	uint32_t invocationId = get_invocationId();
	
	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_dread,
					   "DREAD",
					   smbfd);
	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, packetbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on DREAD\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on DREAD\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 3,
				     smb_rpc_command_argument_type_int,
				     smb_rpc_command_argument_type_int,
				     smb_rpc_command_argument_type_string);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to DREAD\n");
		abort();
	}
	if (cmd_args[0].value.int_value) {
		return cmd_args[0].value.int_value;
	}
	
	*entryType = cmd_args[1].value.int_value;
	*entryName = strndup(cmd_args[2].value.string_value.string, cmd_args[2].value.string_value.length);
	
	return 0;
}

int do_dclose(int commandFd, int responseFd, int smbfd)
{
	char packetbuf[1024];
	size_t packetSize = 1024;
	smb_rpc_command_argument cmd_args[2];
	uint32_t invocationId = get_invocationId();

	packetSize = encode_command_packet(packetbuf, packetbufSize,
					   invocationId,
					   smb_rpc_arguments_dclose,
					   "DCLOSE",
					   smbfd);
	write_completely(commandFd, packetbuf, packetSize);
	
	size_t numread;
	read_packet(responseFd, packetbuf, 1024, &numread);
	size_t packetContentSize;
	char *packetContents;
	
	smb_rpc_decode_packet(packetbuf, numread, &packetSize, &packetContents, &packetContentSize);
	
	uint32_t responseInvocationId;
	size_t argCount;
	
	int ret = smb_rpc_decode_response(packetContents, packetContentSize,
					  &responseInvocationId,
					  cmd_args, &argCount);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Can't parse response on DCLOSE\n");
		abort();
	}
	if (responseInvocationId != invocationId) {
		fprintf(stderr, "Wrong invocationId on DCLOSE\n");
		abort();
	}
	ret = smb_rpc_check_response(cmd_args, argCount, 1,
				     smb_rpc_command_argument_type_int);
	if (ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Wrong response to DCLOSE\n");
		abort();
	}
	if (cmd_args[0].value.int_value) {
		errno = cmd_args[0].value.int_value;
		perror("DCLOSE");
		exit(1);
	}
	
	return cmd_args[0].value.int_value;
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
	
	do_auth(commandfds[1], responsefds[0], argc[1]);
	
	char urlbuf[1024];
	ini_gets("", "url", "", urlbuf, 1024, argc[1]);

	int smbfd = do_fopen(commandfds[1], responsefds[0], urlbuf);
	char readbuf[102400];
	
	errno = 0;
	int outputFd = open("./copy.bin",O_RDWR | O_CREAT, 00660);
	if (outputFd < 0) {
		perror ("Open output");
		abort();
	}
	
	ssize_t num_read;
	while ((num_read = do_read(commandfds[1], responsefds[0],
				   smbfd, readbuf, 102400))) {
		if (num_read < 0) {
			perror("Reading file");
			abort();
		}
		
		write_completely(outputFd, readbuf, num_read);
	}
	close (outputFd);
	
	ini_gets("", "dirurl", "", urlbuf, 1024, argc[1]);

	do_fclose(commandfds[1], responsefds[0], smbfd);
	smbfd = do_dopen(commandfds[1], responsefds[0], urlbuf);
	
	do {
		smb_rpc_dirent_type entryType;
		char *entryName;
		
		int ret = do_dread(commandfds[1], responsefds[0], smbfd, &entryType, &entryName);
		if (ret) {
			errno = ret;
			perror("Reading dir");
			abort();
		}
		char typeChar;
	
		if (entryType == smb_rpc_dirent_type_none) {
			break;
		}

		switch(entryType) {
			case smb_rpc_dirent_type_file:
				typeChar = 'F';
				break;
			case smb_rpc_dirent_type_dir:
				typeChar = 'D';
				break;
			default:
				fprintf(stderr, "Bad direntry type %i\n", entryType);
				abort();
		}
		
		fprintf(stderr, "%c %s\n", typeChar, entryName);
		free(entryName);
		entryName = NULL;
	} while (1);

	do_dclose(commandfds[1], responsefds[0], smbfd);
}
