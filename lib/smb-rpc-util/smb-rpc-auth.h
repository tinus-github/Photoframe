//
//  smb-rpc-auth.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#ifndef smb_rpc_auth_h
#define smb_rpc_auth_h

#include <stdio.h>

void smb_rpc_auth_set_data(const char *server,
			   const char *workgroup,
			   const char *username,
			   const char *password);

void smb_rpc_auth_get_auth_fn(const char *server,
			      const char *share,
			      char *workgroup,
			      int workgroupSpace,
			      char *username,
			      int usernameSpace,
			      char *password,
			      int passwordSpace);

#endif /* smb_rpc_auth_h */
