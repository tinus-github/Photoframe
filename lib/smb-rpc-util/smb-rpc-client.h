//
//  smb-rpc-client.h
//  Photoframe
//
//  Created by Martijn Vernooij on 23/05/2017.
//
//

#ifndef smb_rpc_client_h
#define smb_rpc_client_h

#include "smb-rpc-encode.h"

typedef enum {
	smb_rpc_dirent_type_none = 0,
	smb_rpc_dirent_type_file = 1,
	smb_rpc_dirent_type_dir = 2,
} smb_rpc_dirent_type;

typedef struct smb_rpc_dirent {
	smb_rpc_dirent_type type;
	char *name;
	
	// excluding the terminating \0
	size_t namelen;
} smb_rpc_dirent;

#define SMB_RPC_COMMAND_MAXARGS 6

#ifdef SMB_RPC_DECLARE_COMMAND_DEFINITIONS

smb_rpc_command_argument_type smb_rpc_arguments_connect[] = {
	// arguments
	smb_rpc_command_argument_type_int, //client version
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //util version
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_setauth[] = {
	// arguments
	smb_rpc_command_argument_type_string, //server
	smb_rpc_command_argument_type_string, //workgroup
	smb_rpc_command_argument_type_string, //user
	smb_rpc_command_argument_type_string, //password
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_end_of_list // nothing
};

smb_rpc_command_argument_type smb_rpc_arguments_fopen[] = {
	// arguments
	smb_rpc_command_argument_type_string, //url
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_fread[] = {
	// arguments
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_int, //length
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_buffer, // data
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_fclose[] = {
	// arguments
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_dopen[] = {
	// arguments
	smb_rpc_command_argument_type_string, //url
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_dread[] = {
	// arguments
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_int, //entrytype
	smb_rpc_command_argument_type_string, //entryname
	smb_rpc_command_argument_type_end_of_list
};

smb_rpc_command_argument_type smb_rpc_arguments_dclose[] = {
	// arguments
	smb_rpc_command_argument_type_int, //fd
	smb_rpc_command_argument_type_end_of_list,
	// return values
	smb_rpc_command_argument_type_int, //errno
	smb_rpc_command_argument_type_end_of_list
};

#else

extern smb_rpc_command_argument_type smb_rpc_arguments_setauth[];
extern smb_rpc_command_argument_type smb_rpc_arguments_fopen[];
extern smb_rpc_command_argument_type smb_rpc_arguments_fread[];
extern smb_rpc_command_argument_type smb_rpc_arguments_fclose[];

extern smb_rpc_command_argument_type smb_rpc_arguments_dopen[];
extern smb_rpc_command_argument_type smb_rpc_arguments_dread[];
extern smb_rpc_command_argument_type smb_rpc_arguments_dclose[];

#endif

#endif /* smb_rpc_client_h */
