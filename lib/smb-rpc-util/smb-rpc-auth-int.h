//
//  smb-rpc-auth-int.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#ifndef smb_rpc_auth_int_h
#define smb_rpc_auth_int_h

typedef struct smb_rpc_auth_data smb_rpc_auth_data;

struct smb_rpc_auth_data {
	char *servername;
	char *workgroup;
	char *username;
	char *password;
	smb_rpc_auth_data *next;
};


#endif /* smb_rpc_auth_int_h */
