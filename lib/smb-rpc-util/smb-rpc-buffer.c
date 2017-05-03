//
//  smb-rpc-buffer.c
//  Photoframe
//
//  Created by Martijn Vernooij on 30/04/2017.
//
//

#include "smb-rpc-buffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define INITIAL_SIZE 10240
#define MAX_SIZE 52428800

smb_rpc_buffer *smb_rpc_buffer_new()
{
	smb_rpc_buffer *ret = calloc(1, sizeof(smb_rpc_buffer));
	ret->buffer = calloc(INITIAL_SIZE, sizeof(char));
	ret->_bufferSize = INITIAL_SIZE;
	return ret;
}

void smb_rpc_buffer_add(smb_rpc_buffer *buffer, char *data, size_t length)
{
	if (length > MAX_SIZE) {
		fprintf(stderr, "Data too large\n");
		abort();
	}
	size_t newContentSize = buffer->contentSize + length;
	size_t newBufSize = buffer->_bufferSize;
	while (newContentSize > newBufSize) {
		newBufSize = 2 * newBufSize;
	}
	if (newBufSize > MAX_SIZE) {
		fprintf(stderr, "New buffer size too large\n");
		abort();
	}
	if (newBufSize > buffer->_bufferSize) {
		buffer->buffer = realloc(buffer->buffer, newBufSize);
		buffer->_bufferSize = newBufSize;
	}
	
	memcpy(&buffer->buffer[buffer->contentSize], data, length);
	buffer->contentSize += length;
}

char *smb_rpc_buffer_get_data(smb_rpc_buffer *buffer, size_t *length)
{
	*length = buffer->contentSize;
	return buffer->buffer;
}

void smb_rpc_buffer_consume_data(smb_rpc_buffer *buffer, size_t length)
{
	assert(length <= buffer->contentSize);
	if (length == buffer->contentSize) {
		buffer->contentSize = 0;
		return;
	}
	
	memmove(buffer->buffer, &buffer->buffer[length], buffer->contentSize - length);
	buffer->contentSize -= length;
}
