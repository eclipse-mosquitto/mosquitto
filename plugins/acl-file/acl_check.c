/*
Copyright (c) 2011-2021 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "acl_file.h"
#include "mosquitto.h"


int acl__check(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_acl_check *ed = event_data;
	struct acl__user *acl_user;
	struct acl__entry *acl_root;
	bool result;
	struct acl_file_data *data = userdata;
	const char *clientid;
	const char *username;

	UNUSED(event);

	// FIXME if(ed->client->bridge) return MOSQ_ERR_SUCCESS;
	if(ed->access == MOSQ_ACL_SUBSCRIBE || ed->access == MOSQ_ACL_UNSUBSCRIBE) return MOSQ_ERR_SUCCESS; /* FIXME - implement ACL subscription strings. */

	clientid = mosquitto_client_id(ed->client);
	username = mosquitto_client_username(ed->client);

	if(!data->acl_file && !data->acl_users && !data->acl_patterns){
		return MOSQ_ERR_PLUGIN_IGNORE;
	}

	if(username){
		HASH_FIND(hh, data->acl_users, username, strlen(username), acl_user);
	}else{
		acl_user = &data->acl_anon;
	}
	if(!acl_user && !data->acl_patterns) return MOSQ_ERR_ACL_DENIED;

	if(acl_user){
		acl_root = acl_user->acl;
	}else{
		acl_root = NULL;
	}

	/* Loop through all ACLs for this client. ACL denials are iterated over first. */
	while(acl_root){
		/* Loop through the topic looking for matches to this ACL. */

		/* If subscription starts with $, acl_root->topic must also start with $. */
		if(ed->topic[0] == '$' && acl_root->topic[0] != '$'){
			acl_root = acl_root->next;
			continue;
		}
		mosquitto_topic_matches_sub(acl_root->topic, ed->topic, &result);
		if(result){
			if(acl_root->access == MOSQ_ACL_NONE){
				/* Access was explicitly denied for this topic. */
				return MOSQ_ERR_ACL_DENIED;
			}
			if(ed->access & acl_root->access){
				/* And access is allowed. */
				return MOSQ_ERR_SUCCESS;
			}
		}
		acl_root = acl_root->next;
	}

	acl_root = data->acl_patterns;

	if(acl_root){
		/* We are using pattern based acls. Check whether the username or
		 * client id contains a + or # and if so deny access.
		 *
		 * Without this, a malicious client may configure its username/client
		 * id to bypass ACL checks (or have a username/client id that cannot
		 * publish or receive messages to its own place in the hierarchy).
		 */
		if(username && strpbrk(username, "+#")){
			mosquitto_log_printf(MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous username \"%s\"", username);
			return MOSQ_ERR_ACL_DENIED;
		}

		if(clientid && strpbrk(clientid, "+#")){
			mosquitto_log_printf(MOSQ_LOG_NOTICE, "ACL denying access to client with dangerous client id \"%s\"", clientid);
			return MOSQ_ERR_ACL_DENIED;
		}
	}

	/* Loop through all pattern ACLs. ACL denial patterns are iterated over first. */
	if(!clientid) return MOSQ_ERR_ACL_DENIED;

	while(acl_root){
		if(acl_root->ucount && !username){
			acl_root = acl_root->next;
			continue;
		}

		if(mosquitto_topic_matches_sub_with_pattern(acl_root->topic, ed->topic, clientid, username, &result)){
			return MOSQ_ERR_ACL_DENIED;
		}
		if(result){
			if(acl_root->access == MOSQ_ACL_NONE){
				/* Access was explicitly denied for this topic pattern. */
				return MOSQ_ERR_ACL_DENIED;
			}
			if(ed->access & acl_root->access){
				/* And access is allowed. */
				return MOSQ_ERR_SUCCESS;
			}
		}

		acl_root = acl_root->next;
	}

	return MOSQ_ERR_ACL_DENIED;
}
