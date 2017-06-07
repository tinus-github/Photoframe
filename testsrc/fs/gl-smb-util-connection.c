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
static int load_config(gl_smb_util_connection *obj);
static void gl_smb_util_connection_free(gl_object *obj_obj);

static int check_packet_buffer_size(gl_smb_util_connection *obj, size_t requiredSpace);
static int write_packet(gl_smb_util_connection *obj, size_t packetSize);
static int read_packet(gl_smb_util_connection *obj, size_t *packetSize);

static struct gl_smb_util_connection_funcs gl_smb_util_connection_funcs_global = {
	.run_command_sync = &gl_smb_util_connection_run_command_sync,
	.load_config = &load_config,
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
					   const char **responsePacket, size_t *responsePacketSize,
					   smb_rpc_command_argument *args,
					   const smb_rpc_command_definition *commandDefinition,
					   ...)
{
	va_list ap;
	
	uint32_t invocationId = get_invocation_id(obj);
	
	va_start(ap, commandDefinition);
	size_t packetSize = smb_rpc_vencode_command_packet(NULL, 0,
						   invocationId,
						   commandDefinition, ap);
	va_end(ap);
	
	if (check_packet_buffer_size(obj, packetSize)) {
		// out of memory
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
		return smb_rpc_decode_result_invalid;
	}
	size_t packetLength;
	int read_ret = read_packet(obj, &packetLength);
	
	if (read_ret) {
		return smb_rpc_decode_result_invalid;
	}
	
	smb_rpc_decode_result decode_ret;
	size_t used_bytes;
	
	decode_ret = smb_rpc_decode_packet(obj->data.packetBuf, packetLength, &used_bytes, responsePacket, responsePacketSize);
	
	if (decode_ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Packet decode error\n");
		return decode_ret;
	}
	
	decode_ret = smb_rpc_decode_response_complete(*responsePacket,
						      *responsePacketSize,
						      invocationId,
						      args,
						      commandDefinition->definition);
	if (decode_ret != smb_rpc_decode_result_ok) {
		fprintf(stderr, "Decode error\n");
		return decode_ret;
	}
	return smb_rpc_decode_result_ok;
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
					   "workgroup",
					   username,
					   password);
	
	if (run_ret == smb_rpc_decode_result_ok) {
		return 0;
	}
	return run_ret;
}

static int load_config(gl_smb_util_connection *obj)
{
#define SECTIONNAMELENGTH 13
	gl_configuration *config = gl_configuration_get_global_configuration();
	
	assert(config);
	
	uint32_t counter = 1;
	char sectionName[SECTIONNAMELENGTH]; // 999 servers should be enough for anyone
	do {
		int l = snprintf(sectionName, SECTIONNAMELENGTH, "SmbServer%i", counter);
		if (l >= SECTIONNAMELENGTH) {
			return -1; // Shouldn't happen
		}
		
		gl_config_section *config_section = config->f->get_section(config, sectionName);
		
		if (!config_section) {
			return 0;
		}
		
		gl_config_value *val = config_section->f->get_value(config_section, "username");
		if (!val) {
			fprintf(stderr, "Config error: No username in SmbServer section %i\n", counter);
			return 0;
		}
		const char *username = val->f->get_value_string(val);
		if (!username) {
			fprintf(stderr, "Config error: Username is not a string in SmbServer section %i\n", counter);
			return 0;
		}
		val = config_section->f->get_value(config_section, "password");
		if (!val) {
			fprintf(stderr, "Config error: No password in SmbServer section %i\n", counter);
			return 0;
		}
		const char *password = val->f->get_value_string(val);
		if (!username) {
			fprintf(stderr, "Config error: Password is not a string in SmbServer section %i\n", counter);
			return 0;
		}
		
		const char *server = "";
		val = config_section->f->get_value(config_section, "server");
		if (val) {
			server = val->f->get_value_string(val);
			if (!server) {
				fprintf(stderr, "Config error: Server is not a string in SmbServer section %i\n", counter);
				return 0;
			}
		}
		
		int res = do_auth(obj, server, username, password);
		if (res) {
			return res;
		}
		counter++;
	} while(counter < 999);
	
	return 0;
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
	obj->f->load_config(obj);
}

static void gl_smb_util_connection_free(gl_object *obj_obj)
{
	gl_smb_util_connection *obj = (gl_smb_util_connection *)obj_obj;
	
	free(obj->data.packetBuf);
	obj->data.packetBuf = NULL;
	
	gl_object_free_org_global(obj_obj);
}

gl_smb_util_connection *gl_smb_util_connection_init(gl_smb_util_connection *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_smb_util_connection_funcs_global;
	
	obj->data.packetBuf = calloc(1, INITIAL_PACKET_BUFFER_SIZE);
	obj->data.packetBufSize = INITIAL_PACKET_BUFFER_SIZE;
	
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
