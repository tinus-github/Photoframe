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

static smb_rpc_auth_data *auth_data_default = NULL;
static smb_rpc_auth_data *auth_data = NULL;

static void free_auth_data(smb_rpc_auth_data *d)
{
	free(d->servername);
	free(d->workgroup);
	free(d->username);
	free(d->password);
}

void smb_rpc_auth_set_data(const char *server,
			   const char *workgroup,
			   const char *username,
			   const char *password)
{
	smb_rpc_auth_data *new_entry = calloc(1, sizeof(smb_rpc_auth_data));
	
	new_entry->workgroup = strdup(workgroup);
	new_entry->username = strdup(username);
	new_entry->password = strdup(password);
	
	if (!server) {
		new_entry->servername = NULL;
		if (auth_data_default) {
			free_auth_data(auth_data_default);
			free(auth_data_default);
		}
		auth_data_default = new_entry;
		return;
	}
	new_entry->servername = strdup(server);
	
	smb_rpc_auth_data *current_entry = auth_data;
	while (current_entry) {
		if (!strcasecmp(server, current_entry->servername)) {
			free_auth_data(current_entry);
			current_entry->servername = new_entry->servername;
			current_entry->workgroup = new_entry->workgroup;
			current_entry->username = new_entry->username;
			current_entry->password = new_entry->password;
			
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
	
	if (!current_entry) {
		return;
	}
	
	if (current_entry->workgroup) {
		strlcpy(workgroup, current_entry->workgroup, workgroupSpace);
	}
	strlcpy(username, current_entry->username, usernameSpace);
	strlcpy(password, current_entry->password, passwordSpace);
}

