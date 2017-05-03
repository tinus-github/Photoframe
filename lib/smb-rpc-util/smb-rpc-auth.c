//
//  smb-rpc-auth.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/04/2017.
//
//

#include "smb-rpc-auth.h"
#include "smb-rpc-auth-int.h"

#include <stdlib.h>
#include <string.h>

static smb_rpc_auth_data *auth_data_default;
static smb_rpc_auth_data *auth_data;

static void free_auth_data(smb_rpc_auth_data *d)
{
	free(d->servername);
	free(d->workgroup);
	free(d->username);
	free(d->password);
}

void smb_rpc_auth_set_data(char *server, char *workgroup, char *username, char *password)
{
	smb_rpc_auth_data *new_entry = calloc(1, sizeof(smb_rpc_auth_data));
	
	new_entry->workgroup = workgroup;
	new_entry->username = username;
	new_entry->password = password;
	
	new_entry->servername = server;
	
	if (!server) {
		if (auth_data_default) {
			free_auth_data(auth_data_default);
			free(auth_data_default);
			auth_data_default = new_entry;
			return;
		}
	}
	
	smb_rpc_auth_data *current_entry = auth_data;
	while (current_entry) {
		if (!strcasecmp(server, current_entry->servername)) {
			free_auth_data(current_entry);
			current_entry->servername = server;
			current_entry->workgroup = workgroup;
			current_entry->username = username;
			current_entry->password = password;
			
			free(new_entry);
			
			return;
		}
		current_entry = current_entry->next;
	}
	
	new_entry->next = auth_data;
	auth_data = new_entry;
}

void smb_rpc_auth_get_auth_fn(const char *server,
			      const char *share,
			      char *workgroup,
			      int workgroupSpace,
			      char *username,
			      int usernameSpace,
			      char *password,
			      int passwordSpace)
{
	smb_rpc_auth_data *current_entry = auth_data;
	
	while (current_entry) {
		if (!strcasecmp(server, current_entry->servername)) {
			break;
		}
		current_entry = current_entry->next;
	}
	
	if (!current_entry) {
		current_entry = auth_data_default;
	}
	
	if (current_entry->workgroup) {
		strlcpy(workgroup, current_entry->workgroup, workgroupSpace);
	}
	strlcpy(username, current_entry->username, usernameSpace);
	strlcpy(password, current_entry->password, passwordSpace);
}

