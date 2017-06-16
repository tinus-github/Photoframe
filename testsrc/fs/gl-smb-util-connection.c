//
//  gl-smb-util-connection.c
//  Photoframe
//
//  Created by Martijn Vernooij on 05/06/2017.
//
//

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include "fs/gl-smb-util-connection.h"
#include "config/gl-configuration.h"
#define INITIAL_PACKET_BUFFER_SIZE 1024
#define MAX_PACKET_BUFFER_SIZE 10485760

static gl_smb_util_connection *global_connection;

static smb_rpc_decode_result gl_smb_util_connection_run_command_sync(gl_smb_util_connection *obj,
					const char **responsePacket, size_t *responsePacketSize,
					smb_rpc_command_argument *args,
					const smb_rpc_command_definition *commandDefinition,
					...);
static void gl_smb_util_connection_authenticate(gl_smb_util_connection *obj, const char *server, const char *workgroup, const char *username, const char *password);

static int do_auth(gl_smb_util_connection *obj,
		   const char *server,
		   const char *workgroup,
		   const char *username,
		   const char *password);
static void append_authentication(gl_smb_util_connection *obj,
				  const char *server,
				  const char *workgroup,
				  const char *username,
				  const char *password);
static int rerun_auth(gl_smb_util_connection *obj);
static void gl_smb_util_connection_free(gl_object *obj_obj);

static int check_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace);
static int write_packet(gl_smb_util_connection *obj, size_t packetSize);
static int read_packet(gl_smb_util_connection *obj, size_t *packetSize);

static struct gl_smb_util_connection_funcs gl_smb_util_connection_funcs_global = {
	.run_command_sync = &gl_smb_util_connection_run_command_sync,
	.authenticate = &gl_smb_util_connection_authenticate,
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_smb_util_connection_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_smb_util_connection_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_smb_util_connection_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_smb_util_connection_free;
}

static uint32_t get_invocation_id(gl_smb_util_connection *obj)
{
	return obj->data.currentInvocationId++;
}

static smb_rpc_decode_result gl_smb_util_connection_run_command_sync(gl_smb_util_connection *obj,
					   const char **responsePacketRet, size_t *responsePacketSizeRet,
					   smb_rpc_command_argument *args,
					   const smb_rpc_command_definition *commandDefinition,
					   ...)
{
	const char **responsePacket;
	const char *responsePacketStore;
	size_t *responsePacketSize;
	size_t responsePacketSizeStore;
	
	if (responsePacketRet) {
		responsePacket = responsePacketRet;
	} else {
		responsePacket = &responsePacketStore;
	}
	if (responsePacketSizeRet) {
		responsePacketSize = responsePacketSizeRet;
	} else {
		responsePacketSize = &responsePacketSizeStore;
	}
	
	va_list ap;
	
	pthread_mutex_lock(&obj->data.syncMutex);
	
	uint32_t invocationId = get_invocation_id(obj);
	
	va_start(ap, commandDefinition);
	size_t packetSize = smb_rpc_vencode_command_packet(NULL, 0,
						   invocationId,
						   commandDefinition, ap);
	va_end(ap);
	
	if (check_packet_buffer_size(obj, packetSize)) {
		// out of memory
		pthread_mutex_unlock(&obj->data.syncMutex);
		return smb_rpc_decode_result_invalid;
	}
	
	va_start(ap, commandDefinition);
	packetSize = smb_rpc_vencode_command_packet(obj->data.packetBuf, obj->data.packetBufSize,
					    invocationId,
					    commandDefinition,
					    ap);
	int write_ret = write_packet(obj, packetSize);
	if (write_ret) {
		// TODO: socket failed, reopen?
		pthread_mutex_unlock(&obj->data.syncMutex);
		return smb_rpc_decode_result_invalid;
	}
	size_t packetLength;
	int read_ret = read_packet(obj, &packetLength);
	
	if (read_ret) {
		pthread_mutex_unlock(&obj->data.syncMutex);
		return smb_rpc_decode_result_invalid;
	}
	
	smb_rpc_decode_result decode_ret;
	size_t used_bytes;
	
	decode_ret = smb_rpc_decode_packet(obj->data.packetBuf, packetLength, &used_bytes, responsePacket, responsePacketSize);
	
	if (decode_ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Packet decode error\n");
		pthread_mutex_unlock(&obj->data.syncMutex);
		return decode_ret;
	}
	
	decode_ret = smb_rpc_decode_response_complete(*responsePacket,
						      *responsePacketSize,
						      invocationId,
						      args,
						      commandDefinition->definition);
	pthread_mutex_unlock(&obj->data.syncMutex);
	
	if (decode_ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Decode error\n");
		return decode_ret;
	}
	return smb_rpc_decode_result_ok;
}

static void gl_smb_util_connection_authenticate(gl_smb_util_connection *obj, const char *server, const char *workgroup, const char *username, const char *password)
{
	assert(server);
	assert(workgroup);
	assert(username);
	assert(password);
	
	append_authentication(obj, server, workgroup, username, password);
	do_auth(obj, server, workgroup, username, password);
}

static int do_connect(gl_smb_util_connection *obj)
{
	const char *responsePacket;
	size_t responsePacketSize;
	smb_rpc_command_argument args[2];
	smb_rpc_decode_result run_ret;
	run_ret = obj->f->run_command_sync(obj, &responsePacket, &responsePacketSize,
					   args, &smb_rpc_arguments_connect, SMB_RPC_VERSION);
	
	if (run_ret != smb_rpc_decode_result_ok) {
		return -1;
	}
	if (args[0].value.int_value != SMB_RPC_VERSION) {
		return -2;
	}
	return 0;
}

static int do_auth(gl_smb_util_connection *obj,
		   const char *server,
		   const char *workgroup,
		   const char *username,
		   const char *password)
{
	const char *responsePacket;
	size_t responsePacketSize;
	smb_rpc_command_argument args[2];
	smb_rpc_decode_result run_ret;
	run_ret = obj->f->run_command_sync(obj, &responsePacket, &responsePacketSize,
					   args, &smb_rpc_arguments_setauth,
					   server,
					   workgroup,
					   username,
					   password);
	
	if (run_ret == smb_rpc_decode_result_ok) {
		return 0;
	}
	return run_ret;
}

static int check_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace)
{
	if (requiredSpace <= obj->data.packetBufSize) {
		return 0;
	}
	if (requiredSpace > MAX_PACKET_BUFFER_SIZE) {
		return -1;
	}
	
	do {
		obj->data.packetBufSize = obj->data.packetBufSize * 2;
	} while (obj->data.packetBufSize <= requiredSpace);
	// It's not an exact limit
	
	char *newPacketBuf = realloc(obj->data.packetBuf, obj->data.packetBufSize);
	if (!newPacketBuf) {
		return -1;
	}
	
	obj->data.packetBuf = newPacketBuf;
	return 0;
}

static int write_packet(gl_smb_util_connection *obj, size_t packetSize)
{
	ssize_t num_written;
	char *buf = obj->data.packetBuf;
	while (packetSize) {
		errno = 0;
		num_written = write(obj->data.commandFd, buf, packetSize);
		if (num_written < 1) {
			switch (errno) {
				case 0:
				case EAGAIN:
				case EINTR:
					break;
				default:
					fprintf(stderr, "Write error: %d\n", errno);
					return -1;
			}
		} else {
			buf += num_written;
			packetSize -= num_written;
		}
	}
	return 0;
}

static int read_packet(gl_smb_util_connection *obj, size_t *packetSize)
{
	size_t to_read;
	size_t have_read = 0;
	char *packetBufCursor = obj->data.packetBuf;
	ssize_t numread = 0;
	
	do {
		to_read = obj->data.packetBufSize - have_read;
		if (!to_read) {
			int enlarge_ret = check_packet_buffer_size(obj, have_read * 2);
			if (enlarge_ret) {
				// The utility sent a unexpectantly large packet
				// or we're out of memory
				return enlarge_ret;
			}
			to_read = obj->data.packetBufSize - have_read;
			assert (to_read);
		}
		errno = 0;
		numread = read(obj->data.responseFd, packetBufCursor, to_read);
		if (numread < 0) {
			switch (errno) {
				case EAGAIN:
				case EINTR:
				case 0:
					continue; //try again
				default:
					perror("read_packet");
					return -1;
			}
		}
		if (!numread) {
			fprintf(stderr, "EOF\n");
			return -1;
		}
		have_read += numread;
		const char *packetContents;
		size_t packetContentsLength;
		smb_rpc_decode_result parseRet = smb_rpc_decode_packet(obj->data.packetBuf, have_read,
								       packetSize,
								       &packetContents, &packetContentsLength);
		switch (parseRet) {
			case smb_rpc_decode_result_ok:
				if (*packetSize != have_read) {
					fprintf(stderr, "Extra data after packet.\n");
				}
				return 0;
			case smb_rpc_decode_result_invalid:
				fprintf(stderr, "Invalid packet.\n");
				return -1;
			case smb_rpc_decode_result_tooshort:
				break;
			default:
				// This isn't supposed to happen
				abort();
		}
		
		packetBufCursor += numread;
	} while (1);
}

static void update_last_authentication(gl_smb_util_connection *obj)
{
	if (!obj->data.authentication) {
		return;
	}
	if (!obj->data.last_authentication) {
		obj->data.last_authentication = obj->data.authentication;
	}
	
	while (obj->data.last_authentication->next) {
		obj->data.last_authentication = obj->data.last_authentication->next;
	}
}

static void append_authentication(gl_smb_util_connection *obj,
				  const char *server,
				  const char *workgroup,
				  const char *username,
				  const char *password)
{
	assert(server);
	assert(workgroup);
	assert(username);
	assert(password);
	
	gl_smb_util_connection_auth *auth_entry = calloc(1, sizeof(gl_smb_util_connection_auth));
	// TODO out of memory checks?
	
	auth_entry->server = strdup(server);
	auth_entry->workgroup = strdup(workgroup);
	auth_entry->username = strdup(username);
	auth_entry->password = strdup(password);
	
	update_last_authentication(obj);
	if (obj->data.last_authentication) {
		obj->data.last_authentication->next = auth_entry;
	} else {
		obj->data.authentication = auth_entry;
	}
	obj->data.last_authentication = auth_entry;
}

static int rerun_auth(gl_smb_util_connection *obj)
{
	int ret;
	gl_smb_util_connection_auth *auth_entry = obj->data.authentication;
	
	while (auth_entry) {
		ret = do_auth(obj, auth_entry->server, auth_entry->workgroup, auth_entry->username, auth_entry->password);

		if (ret != smb_rpc_decode_result_ok) {
			return ret;
		}
		auth_entry = auth_entry->next;
	}
	return smb_rpc_decode_result_ok;
}

static void setup_connection(gl_smb_util_connection *obj)
{
	int commandfds[2];
	int responsefds[2];
	
	errno = 0;
	if (pipe(commandfds)) {
		fprintf(stderr, "smb-rpc-util: Failed to create pipe 1: %d\n", errno);
		abort();
	}
	errno = 0;
	if (pipe(responsefds)) {
		fprintf(stderr, "smb-rpc-util: Failed to create pipe 2: %d\n", errno);
		abort();
	}
	if (!fork()) {
		// This is the child
		dup2(commandfds[0], fileno(stdin));
		dup2(responsefds[1], fileno(stdout));
		close(commandfds[0]);
		close(responsefds[1]);
		close(commandfds[1]);
		close(responsefds[0]);
		
		execl("./smb-rpc-util", "smb-rpc-util", 0);
		// TODO: Check if this is even possible in the parent process
		perror("Exec smb-rpc-util");
		abort();
	}
	obj->data.commandFd = commandfds[1];
	obj->data.responseFd = responsefds[0];
	close (commandfds[0]);
	close (responsefds[1]);
	
	if (do_connect(obj)) {
		fprintf(stderr, "smb-rpc-util: Bad response to connect\n");
		abort();
	}
	rerun_auth(obj);
}

static void gl_smb_util_connection_free(gl_object *obj_obj)
{
	gl_smb_util_connection *obj = (gl_smb_util_connection *)obj_obj;
	
	free(obj->data.packetBuf);
	obj->data.packetBuf = NULL;
	
	if (obj->data.commandFd) {
		close (obj->data.commandFd);
		obj->data.commandFd = 0;
	}
	if (obj->data.responseFd) {
		close (obj->data.responseFd);
		obj->data.responseFd = 0;
	}
	
	gl_object_free_org_global(obj_obj);
}

gl_smb_util_connection *gl_smb_util_connection_init(gl_smb_util_connection *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_smb_util_connection_funcs_global;
	
	obj->data.packetBuf = calloc(1, INITIAL_PACKET_BUFFER_SIZE);
	obj->data.packetBufSize = INITIAL_PACKET_BUFFER_SIZE;
	
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_ERRORCHECK);
	
	pthread_mutex_init(&obj->data.syncMutex, &attributes);
	
	setup_connection(obj);
	
	return obj;
}

gl_smb_util_connection *gl_smb_util_connection_new()
{
	gl_smb_util_connection *ret = calloc(1, sizeof(gl_smb_util_connection));
	
	return gl_smb_util_connection_init(ret);
}

gl_smb_util_connection *gl_smb_util_connection_get_global_connection()
{
	if (!global_connection) {
		global_connection = gl_smb_util_connection_new();
	}
	return global_connection;
}
