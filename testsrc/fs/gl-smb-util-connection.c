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
static smb_rpc_decode_result gl_smb_util_connection_run_command_async(gl_smb_util_connection *obj,
								      const char **responsePacket, size_t *responsePacketSize,
								      smb_rpc_command_argument *args,
								      const smb_rpc_command_definition *commandDefinition,
								      ...);
static void gl_smb_util_connection_authenticate(gl_smb_util_connection *obj, const char *server, const char *workgroup, const char *username, const char *password);
static void async_run_packet_completed(gl_smb_util_connection *obj);

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

static int check_output_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace);
static int check_input_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace);
static int write_packet(gl_smb_util_connection *obj, size_t packetSize);
static int read_packet(gl_smb_util_connection *obj);
static void discard_read_packet(gl_smb_util_connection *obj);
static gl_smb_util_connection_request *gl_smb_util_connection_request_new(gl_smb_util_connection *obj);
static void gl_smb_util_connection_set_async(gl_smb_util_connection *obj);
static void gl_smb_util_connection_request_free(gl_smb_util_connection *obj, gl_smb_util_connection_request *request);

static struct gl_smb_util_connection_funcs gl_smb_util_connection_funcs_global = {
	.run_command_sync = &gl_smb_util_connection_run_command_sync,
	.run_command_async = &gl_smb_util_connection_run_command_async,
	.run_command_async_completed = &async_run_packet_completed,
	.authenticate = &gl_smb_util_connection_authenticate,
	.set_async = &gl_smb_util_connection_set_async,
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
	pthread_mutex_lock(&obj->data.syncMutex);
	uint32_t ret = obj->data.currentInvocationId++;
	pthread_mutex_unlock(&obj->data.syncMutex);
	return ret;
}

// Call with syncMutex locked
// Returns with it locked again
static smb_rpc_decode_result async_run_packet(gl_smb_util_connection *obj, uint32_t invocationId, size_t packetSize)
{
	gl_smb_util_connection_request *request = gl_smb_util_connection_request_new(obj);
	request->next = obj->data.requestQueue;
	request->invocationId = invocationId;
	obj->data.requestQueue = request;
	
	write_packet(obj, packetSize);
	pthread_cond_wait(&request->responseReady, &obj->data.syncMutex);
	
	gl_smb_util_connection_request_free(obj, request);
	return smb_rpc_decode_result_ok;
}

static void async_run_packet_completed(gl_smb_util_connection *obj)
{
	pthread_cond_signal(&obj->data.readingCompleteCondition);
}

// Call with syncMutex locked
// Returns with it locked again
static smb_rpc_decode_result sync_run_packet(gl_smb_util_connection *obj, uint32_t invocationId, size_t packetSize)
{
	int write_ret = write_packet(obj, packetSize);
	if (write_ret) {
		// TODO: socket failed, reopen?
		return smb_rpc_decode_result_invalid;
	}
	
	int read_ret = read_packet(obj);
	
	if (read_ret) {
		return smb_rpc_decode_result_invalid;
	}
	
	return smb_rpc_decode_result_ok;
}

static smb_rpc_decode_result gl_smb_util_connection_run_command_v(gl_smb_util_connection *obj,
					   const char **responsePacketRet, size_t *responsePacketSizeRet,
					   smb_rpc_command_argument *args,
					   const smb_rpc_command_definition *commandDefinition,
					   va_list ap)
{
	const char **responsePacket;
	const char *responsePacketStore;
	size_t *responsePacketSize;
	size_t responsePacketSizeStore;
	
	va_list apc;
	
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
	
	uint32_t invocationId = get_invocation_id(obj);
	pthread_mutex_lock(&obj->data.syncMutex);
	
	va_copy(apc, ap);
	
	size_t packetSize = smb_rpc_vencode_command_packet(NULL, 0,
						   invocationId,
						   commandDefinition, apc);
	
	va_end(apc);
	
	if (check_output_packet_buffer_size(obj, packetSize)) {
		// out of memory
		pthread_mutex_unlock(&obj->data.syncMutex);
		return smb_rpc_decode_result_invalid;
	}
	
	packetSize = smb_rpc_vencode_command_packet(obj->data.oPacketBuf, obj->data.oPacketBufSize,
					    invocationId,
					    commandDefinition,
					    ap);
	
	smb_rpc_decode_result run_ret;
	if (!obj->data._isAsync) {
		run_ret = sync_run_packet(obj, invocationId, packetSize);
	} else {
		run_ret = async_run_packet(obj, invocationId, packetSize);
	}
	if (run_ret != smb_rpc_decode_result_ok) {
		pthread_mutex_unlock(&obj->data.syncMutex);
		return run_ret;
	}
	smb_rpc_decode_result decode_ret;
	size_t used_bytes;
	
	decode_ret = smb_rpc_decode_packet(obj->data.iPacketBuf, obj->data.iPacketSize, &used_bytes, responsePacket, responsePacketSize);
	
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

static smb_rpc_decode_result gl_smb_util_connection_run_command_sync(gl_smb_util_connection *obj,
					   const char **responsePacketRet, size_t *responsePacketSizeRet,
					   smb_rpc_command_argument *args,
					   const smb_rpc_command_definition *commandDefinition,
					   ...)
{
	assert (!obj->data._isAsync);
	va_list ap;
	va_start(ap, commandDefinition);
	
	smb_rpc_decode_result ret = gl_smb_util_connection_run_command_v(obj, responsePacketRet, responsePacketSizeRet, args, commandDefinition, ap);
	discard_read_packet(obj);
	
	va_end(ap);
	return ret;
}

static smb_rpc_decode_result gl_smb_util_connection_run_command_async(gl_smb_util_connection *obj,
								     const char **responsePacketRet, size_t *responsePacketSizeRet,
								     smb_rpc_command_argument *args,
								     const smb_rpc_command_definition *commandDefinition,
								     ...)
{
	assert (obj->data._isAsync);
	va_list ap;
	va_start(ap, commandDefinition);
	
	return gl_smb_util_connection_run_command_v(obj, responsePacketRet, responsePacketSizeRet, args, commandDefinition, ap);
	
	va_end(ap);
}

static void *gl_smb_util_connection_async_thread_runloop(void *obj_void)
{
	gl_smb_util_connection *obj = (gl_smb_util_connection *)obj_void;
	
	do {
		int read_ret = read_packet(obj);
		if (read_ret) {
			fprintf(stderr, "Packet read error\n");
			break;
		}
		
		// There is a packet ready in obj
		uint32_t invocationId;
		smb_rpc_decode_result decode_ret = smb_rpc_decode_packet_get_invocation_id(obj->data.iPacketBuf, obj->data.iPacketSize, &invocationId);
		if (decode_ret != smb_rpc_decode_result_ok) {
			fprintf(stderr, "Error getting invocation id\n");
			break;
		}
		pthread_mutex_lock(&obj->data.syncMutex);
		gl_smb_util_connection_request *currentRequest = obj->data.requestQueue;
		gl_smb_util_connection_request *lastRequest = NULL;
		while (currentRequest) {
			if (currentRequest->invocationId == invocationId) {
				if (!lastRequest) {
					obj->data.requestQueue = currentRequest->next;
				} else {
					lastRequest->next = currentRequest->next;
				}
				currentRequest->next = NULL;
				break;
			}
			lastRequest = currentRequest;
			currentRequest = currentRequest->next;
		}
		// if the request was found it is out of the queue now
		if (!currentRequest) {
			fprintf(stderr, "Invocation Id not found\n");
			abort();
		}
		pthread_cond_signal(&currentRequest->responseReady);
		
		pthread_cond_wait(&obj->data.readingCompleteCondition, &obj->data.syncMutex);
		
		discard_read_packet(obj);
		pthread_mutex_unlock(&obj->data.syncMutex);
	} while (1);
	
	return NULL;
}

static void spawn_async_thread(gl_smb_util_connection *obj)
{
	assert (obj->data.commandFd);
	assert (obj->data.responseFd);
	assert (!obj->data._isAsync);
	assert (!obj->data.iPacketBufContents);
	
	obj->data._isAsync = 1;

	pthread_attr_t attr;
	pthread_t thread_id;
	
	assert (!pthread_attr_init(&attr));
	
	assert (!pthread_create(&thread_id, &attr, &gl_smb_util_connection_async_thread_runloop, obj));
	
	assert (!pthread_attr_destroy(&attr));
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

static int check_packet_buffer_size(gl_smb_util_connection *obj, char **packetBufp, size_t *bufSize, size_t requiredSpace)
{
	if (requiredSpace <= *bufSize) {
		return 0;
	}
	if (requiredSpace > MAX_PACKET_BUFFER_SIZE) {
		return -1;
	}
	
	do {
		*bufSize = (*bufSize) * 2;
	} while (*bufSize <= requiredSpace);
	// It's not an exact limit
	
	char *newPacketBuf = realloc(*packetBufp, *bufSize);
	if (!newPacketBuf) {
		return -1;
	}
	
	*packetBufp = newPacketBuf;
	return 0;
}

static int check_input_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace)
{
	return check_packet_buffer_size(obj, &obj->data.iPacketBuf, &obj->data.iPacketBufSize, requiredSpace);
}

static int check_output_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace)
{
	return check_packet_buffer_size(obj, &obj->data.oPacketBuf, &obj->data.oPacketBufSize, requiredSpace);
}

static int write_packet(gl_smb_util_connection *obj, size_t packetSize)
{
	ssize_t num_written;
	char *buf = obj->data.oPacketBuf;
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

static void discard_read_packet(gl_smb_util_connection *obj)
{
	if ((obj->data.iPacketSize) && (obj->data.iPacketSize < obj->data.iPacketBufContents)) {
		memmove(obj->data.iPacketBuf, obj->data.iPacketBuf + obj->data.iPacketSize, obj->data.iPacketBufContents - obj->data.iPacketSize);
	}
	obj->data.iPacketBufContents -= obj->data.iPacketSize;
	obj->data.iPacketSize = 0;
}

static int read_packet(gl_smb_util_connection *obj)
{
	char *packetBufCursor = obj->data.iPacketBuf + obj->data.iPacketBufContents;
	ssize_t numread = 0;
	int tried_initial_contents = 0;
	
	if (!obj->data.iPacketBufContents) {
		tried_initial_contents = 1;
	}
	
	do {
		if (tried_initial_contents) {
			size_t to_read;
			to_read = obj->data.iPacketBufSize - obj->data.iPacketBufContents;
			if (!to_read) {
				int enlarge_ret = check_input_packet_buffer_size(obj, obj->data.iPacketBufContents * 2);
				if (enlarge_ret) {
					// The utility sent a unexpectantly large packet
					// or we're out of memory
					return enlarge_ret;
				}
				to_read = obj->data.iPacketBufSize - obj->data.iPacketBufContents;
				packetBufCursor = obj->data.iPacketBuf + obj->data.iPacketBufContents;
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
			packetBufCursor += numread;
			obj->data.iPacketBufContents += numread;
		} else {
			tried_initial_contents = 1;
		}
		const char *packetContents;
		size_t packetContentsLength;
		smb_rpc_decode_result parseRet = smb_rpc_decode_packet(obj->data.iPacketBuf, obj->data.iPacketBufContents,
								       &obj->data.iPacketSize,
								       &packetContents, &packetContentsLength);
		switch (parseRet) {
			case smb_rpc_decode_result_ok:
				if ((!obj->data._isAsync) && (obj->data.iPacketSize != obj->data.iPacketBufContents)) {
					fprintf(stderr, "Extra data after packet.\n");
				}
				return 0;
			case smb_rpc_decode_result_invalid:
				fprintf(stderr, "Invalid packet.\n");
				abort();
				return -1;
			case smb_rpc_decode_result_tooshort:
				break;
			default:
				// This isn't supposed to happen
				abort();
		}
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

static void gl_smb_util_connection_set_async(gl_smb_util_connection *obj)
{
	if (obj->data._isAsync) {
		return;
	}
	
	assert (!obj->data.iPacketBufContents);
	
	spawn_async_thread(obj);
}

static gl_smb_util_connection_request *gl_smb_util_connection_request_new(gl_smb_util_connection *obj)
{
	gl_smb_util_connection_request *ret = calloc(1, sizeof(gl_smb_util_connection_request));
	if (!ret) {
		return ret;
	}
	
	pthread_cond_init(&ret->responseReady, NULL);
	
	return ret;
}

static void gl_smb_util_connection_request_free(gl_smb_util_connection *obj, gl_smb_util_connection_request *request)
{
	pthread_cond_destroy(&request->responseReady);
	free (request);
}

static void gl_smb_util_connection_free(gl_object *obj_obj)
{
	gl_smb_util_connection *obj = (gl_smb_util_connection *)obj_obj;
	
	free(obj->data.iPacketBuf);
	obj->data.iPacketBuf = NULL;
	free(obj->data.oPacketBuf);
	obj->data.oPacketBuf = NULL;
	
	if (obj->data.commandFd) {
		close (obj->data.commandFd);
		obj->data.commandFd = 0;
	}
	if (obj->data.responseFd) {
		close (obj->data.responseFd);
		obj->data.responseFd = 0;
	}
	
	pthread_mutex_destroy(&obj->data.syncMutex);
	pthread_cond_destroy(&obj->data.readingCompleteCondition);
	
	gl_object_free_org_global(obj_obj);
}

gl_smb_util_connection *gl_smb_util_connection_init(gl_smb_util_connection *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_smb_util_connection_funcs_global;
	
	obj->data.iPacketBuf = calloc(1, INITIAL_PACKET_BUFFER_SIZE);
	obj->data.iPacketBufSize = INITIAL_PACKET_BUFFER_SIZE;
	obj->data.oPacketBuf = calloc(1, INITIAL_PACKET_BUFFER_SIZE);
	obj->data.oPacketBufSize = INITIAL_PACKET_BUFFER_SIZE;
	
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_ERRORCHECK);
	
	pthread_mutex_init(&obj->data.syncMutex, &attributes);
	pthread_mutexattr_destroy(&attributes);
	
	pthread_cond_init(&obj->data.readingCompleteCondition, NULL);
	
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
