/*
Copyright (c) 2020 Roger Light <roger@atchoo.org>

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

#include <cjson/cJSON.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef WIN32
#  include <strings.h>
#endif

#include "json_help.h"
#include "mosquitto.h"
#include "mosquitto_broker.h"
#include "mosquitto_plugin.h"
#include "mqtt_protocol.h"

#include "dynamic_security.h"

static mosquitto_plugin_id_t *plg_id = NULL;
static char *config_file = NULL;
struct dynsec__acl_default_access default_access = {false, false, false, false};

#ifdef WIN32
#  include <winsock2.h>
#  include <aclapi.h>
#  include <io.h>
#  include <lmcons.h>
#  include <fcntl.h>
#  define PATH_MAX MAX_PATH
#else
#  include <sys/stat.h>
#  include <pwd.h>
#  include <grp.h>
#  include <unistd.h>
#endif
/* Temporary - remove in 2.1 */
FILE *mosquitto__fopen(const char *path, const char *mode, bool restrict_read)
{
#ifdef WIN32
	char buf[4096];
	int rc;
	int flags = 0;

	rc = ExpandEnvironmentStringsA(path, buf, 4096);
	if(rc == 0 || rc > 4096){
		return NULL;
	}else{
		if (restrict_read) {
			HANDLE hfile;
			SECURITY_ATTRIBUTES sec;
			EXPLICIT_ACCESS_A ea;
			PACL pacl = NULL;
			char username[UNLEN + 1];
			DWORD ulen = UNLEN;
			SECURITY_DESCRIPTOR sd;
			DWORD dwCreationDisposition;
			int fd;
			FILE *fptr;

			switch(mode[0]){
				case 'a':
					dwCreationDisposition = OPEN_ALWAYS;
					flags = _O_APPEND;
					break;
				case 'r':
					dwCreationDisposition = OPEN_EXISTING;
					flags = _O_RDONLY;
					break;
				case 'w':
					dwCreationDisposition = CREATE_ALWAYS;
					break;
				default:
					return NULL;
			}

			GetUserNameA(username, &ulen);
			if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
				return NULL;
			}
			BuildExplicitAccessWithNameA(&ea, username, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
			if (SetEntriesInAclA(1, &ea, NULL, &pacl) != ERROR_SUCCESS) {
				return NULL;
			}
			if (!SetSecurityDescriptorDacl(&sd, TRUE, pacl, FALSE)) {
				LocalFree(pacl);
				return NULL;
			}

			memset(&sec, 0, sizeof(sec));
			sec.nLength = sizeof(SECURITY_ATTRIBUTES);
			sec.bInheritHandle = FALSE;
			sec.lpSecurityDescriptor = &sd;

			hfile = CreateFileA(buf, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
				&sec,
				dwCreationDisposition,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			LocalFree(pacl);

			fd = _open_osfhandle((intptr_t)hfile, flags);
			if (fd < 0) {
				return NULL;
			}

			fptr = _fdopen(fd, mode);
			if (!fptr) {
				_close(fd);
				return NULL;
			}
			if(mode[0] == 'a'){
				fseek(fptr, 0, SEEK_END);
			}
			return fptr;

		}else {
			return fopen(buf, mode);
		}
	}
#else
	FILE *fptr;
	struct stat statbuf;

	if (restrict_read) {
		mode_t old_mask;

		old_mask = umask(0077);
		fptr = fopen(path, mode);
		umask(old_mask);
	}else{
		fptr = fopen(path, mode);
	}
	if(!fptr) return NULL;

	if(fstat(fileno(fptr), &statbuf) < 0){
		fclose(fptr);
		return NULL;
	}

	if(restrict_read){
		if(statbuf.st_mode & S_IRWXO){
#ifdef WITH_BROKER
			log__printf(NULL, MOSQ_LOG_WARNING,
#else
			fprintf(stderr,
#endif
					"Warning: File %s has world readable permissions. Future versions will refuse to load this file."
					"To fix this, use `chmod 0700 %s`.",
					path, path);
#if 0
			return NULL;
#endif
		}
		if(statbuf.st_uid != getuid()){
			char buf[4096];
			struct passwd pw, *result;

			getpwuid_r(getuid(), &pw, buf, sizeof(buf), &result);
			if(result){
#ifdef WITH_BROKER
				log__printf(NULL, MOSQ_LOG_WARNING,
#else
				fprintf(stderr,
#endif
						"Warning: File %s owner is not %s. Future versions will refuse to load this file."
						"To fix this, use `chown %s %s`.",
						path, result->pw_name, result->pw_name, path);
			}
#if 0
			// Future version
			return NULL;
#endif
		}
		if(statbuf.st_gid != getgid()){
			char buf[4096];
			struct group grp, *result;

			getgrgid_r(getgid(), &grp, buf, sizeof(buf), &result);
			if(result){
#ifdef WITH_BROKER
				log__printf(NULL, MOSQ_LOG_WARNING,
#else
				fprintf(stderr,
#endif
						"Warning: File %s group is not %s. Future versions will refuse to load this file.",
						path, result->gr_name);
			}
#if 0
			// Future version
			return NULL
#endif
		}
	}


	if(!S_ISREG(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode)){
#ifdef WITH_BROKER
		log__printf(NULL, MOSQ_LOG_ERR, "Error: %s is not a file.", path);
#endif
		fclose(fptr);
		return NULL;
	}
	return fptr;
#endif
}


void dynsec__command_reply(cJSON *j_responses, struct mosquitto *context, const char *command, const char *error, const char *correlation_data)
{
	cJSON *j_response;

	UNUSED(context);

	j_response = cJSON_CreateObject();
	if(j_response == NULL) return;

	if(cJSON_AddStringToObject(j_response, "command", command) == NULL
			|| (error && cJSON_AddStringToObject(j_response, "error", error) == NULL)
			|| (correlation_data && cJSON_AddStringToObject(j_response, "correlationData", correlation_data) == NULL)
			){

		cJSON_Delete(j_response);
		return;
	}

	cJSON_AddItemToArray(j_responses, j_response);
}


static void send_response(cJSON *tree)
{
	char *payload;
	size_t payload_len;

	payload = cJSON_PrintUnformatted(tree);
	cJSON_Delete(tree);
	if(payload == NULL) return;

	payload_len = strlen(payload);
	if(payload_len > MQTT_MAX_PAYLOAD){
		free(payload);
		return;
	}
	mosquitto_broker_publish(NULL, "$CONTROL/dynamic-security/v1/response",
			(int)payload_len, payload, 0, 0, NULL);
}


static int dynsec_control_callback(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_control *ed = event_data;
	cJSON *tree, *commands;
	cJSON *j_response_tree, *j_responses;

	UNUSED(event);
	UNUSED(userdata);

	/* Create object for responses */
	j_response_tree = cJSON_CreateObject();
	if(j_response_tree == NULL){
		return MOSQ_ERR_NOMEM;
	}
	j_responses = cJSON_CreateArray();
	if(j_responses == NULL){
		cJSON_Delete(j_response_tree);
		return MOSQ_ERR_NOMEM;
	}
	cJSON_AddItemToObject(j_response_tree, "responses", j_responses);


	/* Parse cJSON tree.
	 * Using cJSON_ParseWithLength() is the best choice here, but Mosquitto
	 * always adds an extra 0 to the end of the payload memory, so using
	 * cJSON_Parse() on its own will still not overrun. */
#if CJSON_VERSION_FULL < 1007013
	tree = cJSON_Parse(ed->payload);
#else
	tree = cJSON_ParseWithLength(ed->payload, ed->payloadlen);
#endif
	if(tree == NULL){
		dynsec__command_reply(j_responses, ed->client, "Unknown command", "Payload not valid JSON", NULL);
		send_response(j_response_tree);
		return MOSQ_ERR_SUCCESS;
	}
	commands = cJSON_GetObjectItem(tree, "commands");
	if(commands == NULL || !cJSON_IsArray(commands)){
		cJSON_Delete(tree);
		dynsec__command_reply(j_responses, ed->client, "Unknown command", "Invalid/missing commands", NULL);
		send_response(j_response_tree);
		return MOSQ_ERR_SUCCESS;
	}

	/* Handle commands */
	dynsec__handle_control(j_responses, ed->client, commands);
	cJSON_Delete(tree);

	send_response(j_response_tree);

	return MOSQ_ERR_SUCCESS;
}

static int dynsec__process_set_default_acl_access(cJSON *j_responses, struct mosquitto *context, cJSON *command, char *correlation_data)
{
	cJSON *j_actions, *j_action;
	const char *admin_clientid, *admin_username;

	j_actions = cJSON_GetObjectItem(command, "acls");
	if(j_actions == NULL || !cJSON_IsArray(j_actions)){
		dynsec__command_reply(j_responses, context, "setDefaultACLAccess", "Missing/invalid actions array", correlation_data);
		return MOSQ_ERR_INVAL;
	}

	admin_clientid = mosquitto_client_id(context);
	admin_username = mosquitto_client_username(context);

	cJSON_ArrayForEach(j_action, j_actions){
		char *acltype;
		bool allow;

		if(json_get_string(j_action, "acltype", &acltype, false) == MOSQ_ERR_SUCCESS
				&& json_get_bool(j_action, "allow", &allow, false, false) == MOSQ_ERR_SUCCESS){

			if(!strcasecmp(acltype, ACL_TYPE_PUB_C_SEND)){
				default_access.publish_c_send = allow;
			}else if(!strcasecmp(acltype, ACL_TYPE_PUB_C_RECV)){
				default_access.publish_c_recv = allow;
			}else if(!strcasecmp(acltype, ACL_TYPE_SUB_GENERIC)){
				default_access.subscribe = allow;
			}else if(!strcasecmp(acltype, ACL_TYPE_UNSUB_GENERIC)){
				default_access.unsubscribe = allow;
			}
			mosquitto_log_printf(MOSQ_LOG_INFO, "dynsec: %s/%s | setDefaultACLAccess | acltype=%s | allow=%s",
					admin_clientid, admin_username, acltype, allow?"true":"false");
		}
	}

	dynsec__config_save();
	dynsec__command_reply(j_responses, context, "setDefaultACLAccess", NULL, correlation_data);
	return MOSQ_ERR_SUCCESS;
}


static int dynsec__process_get_default_acl_access(cJSON *j_responses, struct mosquitto *context, cJSON *command, char *correlation_data)
{
	cJSON *tree, *jtmp, *j_data, *j_acls, *j_acl;
	const char *admin_clientid, *admin_username;

	UNUSED(command);

	tree = cJSON_CreateObject();
	if(tree == NULL){
		dynsec__command_reply(j_responses, context, "getDefaultACLAccess", "Internal error", correlation_data);
		return MOSQ_ERR_NOMEM;
	}

	admin_clientid = mosquitto_client_id(context);
	admin_username = mosquitto_client_username(context);
	mosquitto_log_printf(MOSQ_LOG_INFO, "dynsec: %s/%s | getDefaultACLAccess",
			admin_clientid, admin_username);

	if(cJSON_AddStringToObject(tree, "command", "getDefaultACLAccess") == NULL
		|| ((j_data = cJSON_AddObjectToObject(tree, "data")) == NULL)

			){
		goto internal_error;
	}

	j_acls = cJSON_AddArrayToObject(j_data, "acls");
	if(j_acls == NULL){
		goto internal_error;
	}

	/* publishClientSend */
	j_acl = cJSON_CreateObject();
	if(j_acl == NULL){
		goto internal_error;
	}
	cJSON_AddItemToArray(j_acls, j_acl);
	if(cJSON_AddStringToObject(j_acl, "acltype", ACL_TYPE_PUB_C_SEND) == NULL
			|| cJSON_AddBoolToObject(j_acl, "allow", default_access.publish_c_send) == NULL
			){

		goto internal_error;
	}

	/* publishClientReceive */
	j_acl = cJSON_CreateObject();
	if(j_acl == NULL){
		goto internal_error;
	}
	cJSON_AddItemToArray(j_acls, j_acl);
	if(cJSON_AddStringToObject(j_acl, "acltype", ACL_TYPE_PUB_C_RECV) == NULL
			|| cJSON_AddBoolToObject(j_acl, "allow", default_access.publish_c_recv) == NULL
			){

		goto internal_error;
	}

	/* subscribe */
	j_acl = cJSON_CreateObject();
	if(j_acl == NULL){
		goto internal_error;
	}
	cJSON_AddItemToArray(j_acls, j_acl);
	if(cJSON_AddStringToObject(j_acl, "acltype", ACL_TYPE_SUB_GENERIC) == NULL
			|| cJSON_AddBoolToObject(j_acl, "allow", default_access.subscribe) == NULL
			){

		goto internal_error;
	}

	/* unsubscribe */
	j_acl = cJSON_CreateObject();
	if(j_acl == NULL){
		goto internal_error;
	}
	cJSON_AddItemToArray(j_acls, j_acl);
	if(cJSON_AddStringToObject(j_acl, "acltype", ACL_TYPE_UNSUB_GENERIC) == NULL
			|| cJSON_AddBoolToObject(j_acl, "allow", default_access.unsubscribe) == NULL
			){

		goto internal_error;
	}

	cJSON_AddItemToArray(j_responses, tree);

	if(correlation_data){
		jtmp = cJSON_AddStringToObject(tree, "correlationData", correlation_data);
		if(jtmp == NULL){
			goto internal_error;
		}
	}

	return MOSQ_ERR_SUCCESS;

internal_error:
	cJSON_Delete(tree);
	dynsec__command_reply(j_responses, context, "getDefaultACLAccess", "Internal error", correlation_data);
	return MOSQ_ERR_NOMEM;
}


int mosquitto_plugin_version(int supported_version_count, const int *supported_versions)
{
	int i;

	for(i=0; i<supported_version_count; i++){
		if(supported_versions[i] == 5){
			return 5;
		}
	}
	return -1;
}

static int dynsec__general_config_load(cJSON *tree)
{
	cJSON *j_default_access;

	j_default_access = cJSON_GetObjectItem(tree, "defaultACLAccess");
	if(j_default_access && cJSON_IsObject(j_default_access)){
		json_get_bool(j_default_access, ACL_TYPE_PUB_C_SEND, &default_access.publish_c_send, true, false);
		json_get_bool(j_default_access, ACL_TYPE_PUB_C_RECV, &default_access.publish_c_recv, true, false);
		json_get_bool(j_default_access, ACL_TYPE_SUB_GENERIC, &default_access.subscribe, true, false);
		json_get_bool(j_default_access, ACL_TYPE_UNSUB_GENERIC, &default_access.unsubscribe, true, false);
	}
	return MOSQ_ERR_SUCCESS;
}

static int dynsec__general_config_save(cJSON *tree)
{
	cJSON *j_default_access;

	j_default_access = cJSON_CreateObject();
	if(j_default_access == NULL){
		return 1;
	}
	cJSON_AddItemToObject(tree, "defaultACLAccess", j_default_access);

	if(cJSON_AddBoolToObject(j_default_access, ACL_TYPE_PUB_C_SEND, default_access.publish_c_send) == NULL
			|| cJSON_AddBoolToObject(j_default_access, ACL_TYPE_PUB_C_RECV, default_access.publish_c_recv) == NULL
			|| cJSON_AddBoolToObject(j_default_access, ACL_TYPE_SUB_GENERIC, default_access.subscribe) == NULL
			|| cJSON_AddBoolToObject(j_default_access, ACL_TYPE_UNSUB_GENERIC, default_access.unsubscribe) == NULL
			){

		return 1;
	}

	return MOSQ_ERR_SUCCESS;
}

static int dynsec__config_load(void)
{
	FILE *fptr;
	long flen_l;
	size_t flen;
	char *json_str;
	cJSON *tree;

	/* Load from file */
	errno = 0;
	fptr = mosquitto__fopen(config_file, "rb", true);
	if(fptr == NULL){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error loading Dynamic security plugin config: File is not readable - check permissions.\n");
		return MOSQ_ERR_ERRNO;
	}
#ifndef WIN32
	if(errno == ENOTDIR || errno == EISDIR){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error loading Dynamic security plugin config: Config is not a file.\n");
		return MOSQ_ERR_ERRNO;
	}
#endif

	fseek(fptr, 0, SEEK_END);
	flen_l = ftell(fptr);
	if(flen_l < 0){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error loading Dynamic security plugin config: %s\n", strerror(errno));
		fclose(fptr);
		return MOSQ_ERR_ERRNO;
	}else if(flen_l == 0){
		fclose(fptr);
		return 0;
	}
	flen = (size_t)flen_l;
	fseek(fptr, 0, SEEK_SET);
	json_str = mosquitto_calloc(flen+1, sizeof(char));
	if(json_str == NULL){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Out of memory.");
		fclose(fptr);
		return MOSQ_ERR_NOMEM;
	}
	if(fread(json_str, 1, flen, fptr) != flen){
		mosquitto_log_printf(MOSQ_LOG_WARNING, "Error loading Dynamic security plugin config: Unable to read file contents.\n");
		mosquitto_free(json_str);
		fclose(fptr);
		return MOSQ_ERR_ERRNO;
	}
	fclose(fptr);

	tree = cJSON_Parse(json_str);
	mosquitto_free(json_str);
	if(tree == NULL){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error loading Dynamic security plugin config: File is not valid JSON.\n");
		return MOSQ_ERR_INVAL;
	}

	if(dynsec__general_config_load(tree)
			|| dynsec_roles__config_load(tree)
			|| dynsec_clients__config_load(tree)
			|| dynsec_groups__config_load(tree)
			){

		cJSON_Delete(tree);
		return MOSQ_ERR_NOMEM;
	}

	cJSON_Delete(tree);
	return 0;
}


void dynsec__config_save(void)
{
	cJSON *tree;
	size_t file_path_len;
	char *file_path;
	FILE *fptr;
	size_t json_str_len;
	char *json_str;

	tree = cJSON_CreateObject();
	if(tree == NULL) return;

	if(dynsec__general_config_save(tree)
			|| dynsec_clients__config_save(tree)
			|| dynsec_groups__config_save(tree)
			|| dynsec_roles__config_save(tree)){

		cJSON_Delete(tree);
		return;
	}

	/* Print json to string */
	json_str = cJSON_Print(tree);
	if(json_str == NULL){
		cJSON_Delete(tree);
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error saving Dynamic security plugin config: Out of memory.\n");
		return;
	}
	cJSON_Delete(tree);
	json_str_len = strlen(json_str);

	/* Save to file */
	file_path_len = strlen(config_file) + strlen(".new") + 1;
	file_path = mosquitto_malloc(file_path_len);
	if(file_path == NULL){
		mosquitto_free(json_str);
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error saving Dynamic security plugin config: Out of memory.\n");
		return;
	}
	snprintf(file_path, file_path_len, "%s.new", config_file);

	fptr = mosquitto__fopen(file_path, "wt", true);
	if(fptr == NULL){
		mosquitto_free(json_str);
		mosquitto_free(file_path);
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error saving Dynamic security plugin config: File is not writable - check permissions.\n");
		return;
	}
	fwrite(json_str, 1, json_str_len, fptr);
	cJSON_free(json_str);
	fclose(fptr);

	/* Everything is ok, so move new file over proper file */
	if(rename(file_path, config_file) < 0){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error updating dynsec config file: %s", strerror(errno));
	}
	mosquitto_free(file_path);
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *options, int option_count)
{
	int i;
	int rc;

	UNUSED(user_data);

	for(i=0; i<option_count; i++){
		if(!strcasecmp(options[i].key, "config_file")){
			config_file = mosquitto_strdup(options[i].value);
			if(config_file == NULL){
				return MOSQ_ERR_NOMEM;
			}
			break;
		}
	}
	if(config_file == NULL){
		mosquitto_log_printf(MOSQ_LOG_WARNING, "Warning: Dynamic security plugin has no plugin_opt_config_file defined. The plugin will not be activated.");
		return MOSQ_ERR_SUCCESS;
	}

	plg_id = identifier;

	dynsec__config_load();

	rc = mosquitto_callback_register(plg_id, MOSQ_EVT_CONTROL, dynsec_control_callback, "$CONTROL/dynamic-security/v1", NULL);
	if(rc == MOSQ_ERR_ALREADY_EXISTS){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Dynamic security plugin can currently only be loaded once.");
		mosquitto_log_printf(MOSQ_LOG_ERR, "Note that this was previously incorrectly allowed but could cause problems with duplicate entries in the config.");
		goto error;
	}else if(rc == MOSQ_ERR_NOMEM){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Out of memory.");
		goto error;
	}else if(rc != MOSQ_ERR_SUCCESS){
		goto error;
	}

	rc = mosquitto_callback_register(plg_id, MOSQ_EVT_BASIC_AUTH, dynsec_auth__basic_auth_callback, NULL, NULL);
	if(rc == MOSQ_ERR_ALREADY_EXISTS){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Dynamic security plugin can only be loaded once.");
		goto error;
	}else if(rc == MOSQ_ERR_NOMEM){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Out of memory.");
		goto error;
	}else if(rc != MOSQ_ERR_SUCCESS){
		goto error;
	}

	rc = mosquitto_callback_register(plg_id, MOSQ_EVT_ACL_CHECK, dynsec__acl_check_callback, NULL, NULL);
	if(rc == MOSQ_ERR_ALREADY_EXISTS){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Dynamic security plugin can only be loaded once.");
		goto error;
	}else if(rc == MOSQ_ERR_NOMEM){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Out of memory.");
		goto error;
	}else if(rc != MOSQ_ERR_SUCCESS){
		goto error;
	}

	return MOSQ_ERR_SUCCESS;
error:
	mosquitto_free(config_file);
	config_file = NULL;
	return rc;
}

int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *options, int option_count)
{
	UNUSED(user_data);
	UNUSED(options);
	UNUSED(option_count);

	if(plg_id){
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_CONTROL, dynsec_control_callback, "$CONTROL/dynamic-security/v1");
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_BASIC_AUTH, dynsec_auth__basic_auth_callback, NULL);
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_ACL_CHECK, dynsec__acl_check_callback, NULL);
	}
	dynsec_groups__cleanup();
	dynsec_clients__cleanup();
	dynsec_roles__cleanup();

	mosquitto_free(config_file);
	config_file = NULL;
	return MOSQ_ERR_SUCCESS;
}

/* ################################################################
 * #
 * # $CONTROL/dynamic-security/v1 handler
 * #
 * ################################################################ */

int dynsec__handle_control(cJSON *j_responses, struct mosquitto *context, cJSON *commands)
{
	int rc = MOSQ_ERR_SUCCESS;
	cJSON *aiter;
	char *command;
	char *correlation_data = NULL;

	cJSON_ArrayForEach(aiter, commands){
		if(cJSON_IsObject(aiter)){
			if(json_get_string(aiter, "command", &command, false) == MOSQ_ERR_SUCCESS){
				if(json_get_string(aiter, "correlationData", &correlation_data, true) != MOSQ_ERR_SUCCESS){
					dynsec__command_reply(j_responses, context, command, "Invalid correlationData data type.", NULL);
					return MOSQ_ERR_INVAL;
				}

				/* Plugin */
				if(!strcasecmp(command, "setDefaultACLAccess")){
					rc = dynsec__process_set_default_acl_access(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "getDefaultACLAccess")){
					rc = dynsec__process_get_default_acl_access(j_responses, context, aiter, correlation_data);

				/* Clients */
				}else if(!strcasecmp(command, "createClient")){
					rc = dynsec_clients__process_create(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "deleteClient")){
					rc = dynsec_clients__process_delete(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "getClient")){
					rc = dynsec_clients__process_get(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "listClients")){
					rc = dynsec_clients__process_list(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "modifyClient")){
					rc = dynsec_clients__process_modify(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "setClientPassword")){
					rc = dynsec_clients__process_set_password(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "setClientId")){
					rc = dynsec_clients__process_set_id(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "addClientRole")){
					rc = dynsec_clients__process_add_role(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "removeClientRole")){
					rc = dynsec_clients__process_remove_role(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "enableClient")){
					rc = dynsec_clients__process_enable(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "disableClient")){
					rc = dynsec_clients__process_disable(j_responses, context, aiter, correlation_data);

				/* Groups */
				}else if(!strcasecmp(command, "addGroupClient")){
					rc = dynsec_groups__process_add_client(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "createGroup")){
					rc = dynsec_groups__process_create(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "deleteGroup")){
					rc = dynsec_groups__process_delete(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "getGroup")){
					rc = dynsec_groups__process_get(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "listGroups")){
					rc = dynsec_groups__process_list(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "modifyGroup")){
					rc = dynsec_groups__process_modify(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "removeGroupClient")){
					rc = dynsec_groups__process_remove_client(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "addGroupRole")){
					rc = dynsec_groups__process_add_role(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "removeGroupRole")){
					rc = dynsec_groups__process_remove_role(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "setAnonymousGroup")){
					rc = dynsec_groups__process_set_anonymous_group(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "getAnonymousGroup")){
					rc = dynsec_groups__process_get_anonymous_group(j_responses, context, aiter, correlation_data);

				/* Roles */
				}else if(!strcasecmp(command, "createRole")){
					rc = dynsec_roles__process_create(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "getRole")){
					rc = dynsec_roles__process_get(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "listRoles")){
					rc = dynsec_roles__process_list(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "modifyRole")){
					rc = dynsec_roles__process_modify(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "deleteRole")){
					rc = dynsec_roles__process_delete(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "addRoleACL")){
					rc = dynsec_roles__process_add_acl(j_responses, context, aiter, correlation_data);
				}else if(!strcasecmp(command, "removeRoleACL")){
					rc = dynsec_roles__process_remove_acl(j_responses, context, aiter, correlation_data);

				/* Unknown */
				}else{
					dynsec__command_reply(j_responses, context, command, "Unknown command", correlation_data);
					rc = MOSQ_ERR_INVAL;
				}
			}else{
				dynsec__command_reply(j_responses, context, "Unknown command", "Missing command", correlation_data);
				rc = MOSQ_ERR_INVAL;
			}
		}else{
			dynsec__command_reply(j_responses, context, "Unknown command", "Command not an object", correlation_data);
			rc = MOSQ_ERR_INVAL;
		}
	}

	return rc;
}
