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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <utlist.h>

#include "mosquitto.h"
#include "acl_file.h"


static int add__acl(struct acl_file_data *data, const char *user, const char *topic, int access)
{
	struct acl__user *acl_user=NULL;
	struct acl__entry *acl;
	char *local_topic;
	bool new_user = false;

	if(!data || !topic) return MOSQ_ERR_INVAL;

	local_topic = mosquitto_strdup(topic);
	if(!local_topic){
		return MOSQ_ERR_NOMEM;
	}

	if(user == NULL){
		acl_user = &data->acl_anon;
	}else{
		HASH_FIND(hh, data->acl_users, user, strlen(user), acl_user);

		if(!acl_user){
			acl_user = mosquitto_calloc(1, sizeof(struct acl__user));
			if(!acl_user){
				mosquitto_FREE(local_topic);
				return MOSQ_ERR_NOMEM;
			}
			new_user = true;
			if(user){
				acl_user->username = mosquitto_strdup(user);
				if(!acl_user->username){
					mosquitto_FREE(local_topic);
					mosquitto_FREE(acl_user);
					return MOSQ_ERR_NOMEM;
				}
			}
		}
	}

	acl = mosquitto_calloc(1, sizeof(struct acl__entry));
	if(!acl){
		mosquitto_FREE(local_topic);
		mosquitto_FREE(acl_user->username);
		mosquitto_FREE(acl_user);
		return MOSQ_ERR_NOMEM;
	}
	acl->access = access;
	acl->topic = local_topic;

	/* Add acl to user acl list */
	if(access == MOSQ_ACL_NONE){
		/* Put "deny" acls at front of the list */
		LL_PREPEND(acl_user->acl, acl);
	}else{
		LL_APPEND(acl_user->acl, acl);
	}

	if(new_user){
		HASH_ADD_KEYPTR(hh, data->acl_users, acl_user->username, strlen(acl_user->username), acl_user);
	}

	return MOSQ_ERR_SUCCESS;
}


static int add__acl_pattern(struct acl_file_data *data, const char *topic, int access)
{
	struct acl__entry *acl, *acl_tail;
	char *local_topic;
	char *s;

	if(!data| !topic) return MOSQ_ERR_INVAL;

	local_topic = mosquitto_strdup(topic);
	if(!local_topic){
		return MOSQ_ERR_NOMEM;
	}

	acl = mosquitto_malloc(sizeof(struct acl__entry));
	if(!acl){
		mosquitto_FREE(local_topic);
		return MOSQ_ERR_NOMEM;
	}
	acl->access = access;
	acl->topic = local_topic;
	acl->next = NULL;

	acl->ccount = 0;
	s = local_topic;
	while(s){
		s = strstr(s, "%c");
		if(s){
			acl->ccount++;
			s+=2;
		}
	}

	acl->ucount = 0;
	s = local_topic;
	while(s){
		s = strstr(s, "%u");
		if(s){
			acl->ucount++;
			s+=2;
		}
	}

	if(acl->ccount == 0 && acl->ucount == 0){
		mosquitto_log_printf(MOSQ_LOG_WARNING,
				"Warning: ACL pattern '%s' does not contain '%%c' or '%%u'.",
				topic);
	}

	if(data->acl_patterns){
		acl_tail = data->acl_patterns;
		if(access == MOSQ_ACL_NONE){
			/* Put "deny" acls at front of the list */
			acl->next = acl_tail;
			data->acl_patterns = acl;
		}else{
			while(acl_tail->next){
				acl_tail = acl_tail->next;
			}
			acl_tail->next = acl;
		}
	}else{
		data->acl_patterns = acl;
	}

	return MOSQ_ERR_SUCCESS;
}


int acl_file__parse(struct acl_file_data *data)
{
	FILE *aclfptr = NULL;
	char *token;
	char *user = NULL;
	char *topic;
	char *access_s;
	int access;
	int rc = MOSQ_ERR_SUCCESS;
	size_t slen;
	int topic_pattern;
	char *saveptr = NULL;
	char *buf = NULL;
	int buflen = 256;

	if(!data) return MOSQ_ERR_INVAL;
	if(!data->acl_file) return MOSQ_ERR_SUCCESS;

	buf = mosquitto_calloc((size_t)buflen, 1);
	if(buf == NULL){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Out of memory.");
		return MOSQ_ERR_NOMEM;
	}

	aclfptr = mosquitto_fopen(data->acl_file, "rt", true);
	if(!aclfptr){
		mosquitto_FREE(buf);
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Unable to open acl_file \"%s\".", data->acl_file);
		return MOSQ_ERR_UNKNOWN;
	}

	/* topic [read|write] <topic>
	 * user <user>
	 */

	while(mosquitto_fgets(&buf, &buflen, aclfptr)){
		slen = strlen(buf);
		while(slen > 0 && isspace(buf[slen-1])){
			buf[slen-1] = '\0';
			slen = strlen(buf);
		}
		if(buf[0] == '#'){
			continue;
		}
		token = strtok_r(buf, " ", &saveptr);
		if(token){
			if(!strcmp(token, "topic") || !strcmp(token, "pattern")){
				if(!strcmp(token, "topic")){
					topic_pattern = 0;
				}else{
					topic_pattern = 1;
				}

				access_s = strtok_r(NULL, " ", &saveptr);
				if(!access_s){
					mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Empty topic in acl_file \"%s\".", data->acl_file);
					rc = MOSQ_ERR_INVAL;
					break;
				}
				token = strtok_r(NULL, "", &saveptr);
				if(token){
					topic = mosquitto_trimblanks(token);
				}else{
					topic = access_s;
					access_s = NULL;
				}
				if(access_s){
					if(!strcmp(access_s, "read")){
						access = MOSQ_ACL_READ;
					}else if(!strcmp(access_s, "write")){
						access = MOSQ_ACL_WRITE;
					}else if(!strcmp(access_s, "readwrite")){
						access = MOSQ_ACL_READ | MOSQ_ACL_WRITE;
					}else if(!strcmp(access_s, "deny")){
						access = MOSQ_ACL_NONE;
					}else{
						mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Invalid topic access type \"%s\" in acl_file \"%s\".", access_s, data->acl_file);
						rc = MOSQ_ERR_INVAL;
						break;
					}
				}else{
					access = MOSQ_ACL_READ | MOSQ_ACL_WRITE;
				}
				rc = mosquitto_sub_topic_check(topic);
				if(rc != MOSQ_ERR_SUCCESS){
					mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Invalid ACL topic \"%s\" in acl_file \"%s\".", topic, data->acl_file);
					rc = MOSQ_ERR_INVAL;
					break;
				}

				if(topic_pattern == 0){
					rc = add__acl(data, user, topic, access);
				}else{
					rc = add__acl_pattern(data, topic, access);
				}
				if(rc){
					break;
				}
			}else if(!strcmp(token, "user")){
				token = strtok_r(NULL, "", &saveptr);
				if(token){
					token = mosquitto_trimblanks(token);
					if(slen == 0){
						mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Missing username in acl_file \"%s\".", data->acl_file);
						rc = MOSQ_ERR_INVAL;
						break;
					}
					mosquitto_FREE(user);
					user = mosquitto_strdup(token);
					if(!user){
						rc = MOSQ_ERR_NOMEM;
						break;
					}
				}else{
					mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Missing username in acl_file \"%s\".", data->acl_file);
					rc = MOSQ_ERR_INVAL;
					break;
				}
			}else{
				mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Invalid line in acl_file \"%s\": %s.", data->acl_file, buf);
				rc = MOSQ_ERR_INVAL;
				break;
			}
		}
	}

	mosquitto_FREE(buf);
	mosquitto_FREE(user);
	fclose(aclfptr);

	return rc;
}


static void acl__free_entries(struct acl__entry *entry)
{
	while(entry){
		struct acl__entry *next = entry->next;

		mosquitto_FREE(entry->topic);
		mosquitto_FREE(entry);

		entry = next;
	}
}

void acl_file__cleanup(struct acl_file_data *data)
{
	struct acl__user *user, *user_tmp;

	HASH_ITER(hh, data->acl_users, user, user_tmp){
		HASH_DELETE(hh, data->acl_users, user);
		mosquitto_FREE(user->username);
		acl__free_entries(user->acl);
		mosquitto_FREE(user);
	}

	acl__free_entries(data->acl_anon.acl);
	data->acl_anon.acl = NULL;

	acl__free_entries(data->acl_patterns);
	data->acl_patterns = NULL;
}
