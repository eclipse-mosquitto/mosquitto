/*
Copyright (c) 2021 Roger Light <roger@atchoo.org>

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
#define CJSON_VERSION_FULL (CJSON_VERSION_MAJOR*1000000+CJSON_VERSION_MINOR*1000+CJSON_VERSION_PATCH)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_help.h"
#include "mosquitto_ctrl.h"
#include "mosquitto.h"


void broker__print_usage(void)
{
	printf("\nBroker Control module\n");
	printf("=======================\n");

	printf("List plugins    :          listPlugins\n");
	printf("List listeners  :          listListeners\n");
}


/* ################################################################
 * #
 * # Payload callback
 * #
 * ################################################################ */


static void print_listeners(cJSON *j_response)
{
	cJSON *j_data, *j_listeners, *j_listener, *jtmp;
	const char *stmp;
	int i=1;

	j_data = cJSON_GetObjectItem(j_response, "data");
	if(j_data == NULL || !cJSON_IsObject(j_data)){
		fprintf(stderr, "Error: Invalid response from server.\n");
		return;
	}

	j_listeners = cJSON_GetObjectItem(j_data, "listeners");
	if(j_listeners == NULL || !cJSON_IsArray(j_listeners)){
		fprintf(stderr, "Error: Invalid response from server.\n");
		return;
	}

	cJSON_ArrayForEach(j_listener, j_listeners){
		printf("Listener %d:\n", i);

		jtmp = cJSON_GetObjectItem(j_listener, "port");
		if(jtmp && cJSON_IsNumber(jtmp)){
			printf("  Port:              %d\n", jtmp->valueint);
		}

		if(json_get_string(j_listener, "protocol", &stmp, false) == MOSQ_ERR_SUCCESS){
			printf("  Protocol:          %s\n", stmp);
		}

		if(json_get_string(j_listener, "socket-path", &stmp, false) == MOSQ_ERR_SUCCESS){
			printf("  Socket path:       %s\n", stmp);
		}

		if(json_get_string(j_listener, "bind-address", &stmp, false) == MOSQ_ERR_SUCCESS){
			printf("  Bind address:      %s\n", stmp);
		}

		jtmp = cJSON_GetObjectItem(j_listener, "tls");
		printf("  TLS:               %s\n", jtmp && cJSON_IsBool(jtmp) && cJSON_IsTrue(jtmp)?"true":"false");
		printf("\n");
		i++;
	}
}


static void print_plugin_info(cJSON *j_response)
{
	cJSON *j_data, *j_plugins, *j_plugin, *jtmp, *j_eps;
	const char *stmp;
	bool first;

	j_data = cJSON_GetObjectItem(j_response, "data");
	if(j_data == NULL || !cJSON_IsObject(j_data)){
		fprintf(stderr, "Error: Invalid response from server.\n");
		return;
	}

	j_plugins = cJSON_GetObjectItem(j_data, "plugins");
	if(j_plugins == NULL || !cJSON_IsArray(j_plugins)){
		fprintf(stderr, "Error: Invalid response from server.\n");
		return;
	}

	cJSON_ArrayForEach(j_plugin, j_plugins){
		if(json_get_string(j_plugin, "name", &stmp, false) != MOSQ_ERR_SUCCESS){
			fprintf(stderr, "Error: Invalid response from server.\n");
			return;
		}
		printf("Plugin:            %s\n", stmp);

		if(json_get_string(j_plugin, "version", &stmp, false) == MOSQ_ERR_SUCCESS){
			printf("Version:           %s\n", stmp);
		}

		j_eps = cJSON_GetObjectItem(j_plugin, "control-endpoints");
		if(j_eps && cJSON_IsArray(j_eps)){
			first = true;
			cJSON_ArrayForEach(jtmp, j_eps){
				if(CJSON_IS_VALID_STRING(jtmp)){
					if(first){
						first = false;
						printf("Control endpoints: %s\n", jtmp->valuestring);
					}else{
						printf("                   %s\n", jtmp->valuestring);
					}
				}
			}
		}
	}
}


static void broker__payload_callback(struct mosq_ctrl *ctrl, long payloadlen, const void *payload)
{
	cJSON *tree, *j_responses, *j_response, *j_command;

	UNUSED(ctrl);

#if CJSON_VERSION_FULL < 1007013
	UNUSED(payloadlen);
	tree = cJSON_Parse(payload);
#else
	tree = cJSON_ParseWithLength(payload, (size_t)payloadlen);
#endif
	if(tree == NULL){
		fprintf(stderr, "Error: Payload not JSON.\n");
		return;
	}

	j_responses = cJSON_GetObjectItem(tree, "responses");
	if(j_responses == NULL || !cJSON_IsArray(j_responses)){
		fprintf(stderr, "Error: Payload missing data.\n");
		cJSON_Delete(tree);
		return;
	}

	j_response = cJSON_GetArrayItem(j_responses, 0);
	if(j_response == NULL){
		fprintf(stderr, "Error: Payload missing data.\n");
		cJSON_Delete(tree);
		return;
	}

	j_command = cJSON_GetObjectItem(j_response, "command");
	if(!CJSON_IS_VALID_STRING(j_command)){
		fprintf(stderr, "Error: Payload missing data.\n");
		cJSON_Delete(tree);
		return;
	}

	const char *error;
	if(json_get_string(j_response, "error", &error, false) == MOSQ_ERR_SUCCESS){
		fprintf(stderr, "%s: Error: %s.\n", j_command->valuestring, error);
	}else{
		if(!strcasecmp(j_command->valuestring, "listPlugins")){
			print_plugin_info(j_response);
		}else if(!strcasecmp(j_command->valuestring, "listListeners")){
			print_listeners(j_response);
		}else{
			/* fprintf(stderr, "%s: Success\n", j_command->valuestring); */
		}
	}
	cJSON_Delete(tree);
}


static int broker__list_plugins(int argc, char *argv[], cJSON *j_command)
{
	UNUSED(argc);
	UNUSED(argv);

	if(cJSON_AddStringToObject(j_command, "command", "listPlugins") == NULL
			){

		return MOSQ_ERR_NOMEM;
	}

	return MOSQ_ERR_SUCCESS;
}


static int broker__list_listeners(int argc, char *argv[], cJSON *j_command)
{
	UNUSED(argc);
	UNUSED(argv);

	if(cJSON_AddStringToObject(j_command, "command", "listListeners") == NULL
			){

		return MOSQ_ERR_NOMEM;
	}

	return MOSQ_ERR_SUCCESS;
}


/* ################################################################
 * #
 * # Main
 * #
 * ################################################################ */


int broker__main(int argc, char *argv[], struct mosq_ctrl *ctrl)
{
	int rc = -1;
	cJSON *j_tree;
	cJSON *j_commands, *j_command;

	if(!strcasecmp(argv[0], "help")){
		broker__print_usage();
		return -1;
	}

	/* The remaining commands need a network connection and JSON command. */

	ctrl->payload_callback = broker__payload_callback;
	ctrl->request_topic = strdup("$CONTROL/broker/v1");
	ctrl->response_topic = strdup("$CONTROL/broker/v1/response");
	if(ctrl->request_topic == NULL || ctrl->response_topic == NULL){
		return MOSQ_ERR_NOMEM;
	}
	j_tree = cJSON_CreateObject();
	if(j_tree == NULL){
		return MOSQ_ERR_NOMEM;
	}
	j_commands = cJSON_AddArrayToObject(j_tree, "commands");
	if(j_commands == NULL){
		cJSON_Delete(j_tree);
		j_tree = NULL;
		return MOSQ_ERR_NOMEM;
	}
	j_command = cJSON_CreateObject();
	if(j_command == NULL){
		cJSON_Delete(j_tree);
		j_tree = NULL;
		return MOSQ_ERR_NOMEM;
	}
	cJSON_AddItemToArray(j_commands, j_command);

	if(!strcasecmp(argv[0], "listPlugins")){
		rc = broker__list_plugins(argc-1, &argv[1], j_command);
	}else if(!strcasecmp(argv[0], "listListeners")){
		rc = broker__list_listeners(argc-1, &argv[1], j_command);

	}else{
		fprintf(stderr, "Command '%s' not recognised.\n", argv[0]);
		cJSON_Delete(j_tree);
		j_tree = NULL;
		return MOSQ_ERR_UNKNOWN;
	}

	if(rc == MOSQ_ERR_SUCCESS){
		ctrl->payload = cJSON_PrintUnformatted(j_tree);
		cJSON_Delete(j_tree);
		if(ctrl->payload == NULL){
			fprintf(stderr, "Error: Out of memory.\n");
			return MOSQ_ERR_NOMEM;
		}
	}
	return rc;
}
