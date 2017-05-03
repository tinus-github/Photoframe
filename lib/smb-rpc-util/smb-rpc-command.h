//
//  smb_rpc_command.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#ifndef smb_rpc_command_h
#define smb_rpc_command_h

#include <stdio.h>
#include "main.h"
#include "smb-rpc-encode.h"

int smb_rpc_command_execute(appdata *appData,
			    char *command, size_t command_length,
			    uint32_t invocation_id,
			    smb_rpc_command_argument *args, size_t arg_count);

#endif /* smb_rpc_command_h */
