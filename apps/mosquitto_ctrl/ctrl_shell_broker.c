/*
Copyright (c) 2023 Roger Light <roger@atchoo.org>

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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "ctrl_shell_internal.h"
#include "json_help.h"

#ifdef WITH_EDITLINE

#define UNUSED(A) (void)(A)

static struct completion_tree_root *commands_broker = NULL;

static void command_tree_create(void)
{
	if(commands_broker) return;

	commands_broker = calloc(1, sizeof(struct completion_tree_root));

	completion_tree_cmd_add(commands_broker, "listPlugins");
	completion_tree_cmd_add(commands_broker, "listListeners");
	completion_tree_cmd_add(commands_broker, "disconnect");
	completion_tree_cmd_add(commands_broker, "help");
	completion_tree_cmd_add(commands_broker, "return");
	completion_tree_cmd_add(commands_broker, "exit");
}


static void print_help(char **saveptr)
{
	char *command = strtok_r(NULL, " ", saveptr);
	if(command){
		if(!strcasecmp(command, "listPlugins")){
			ctrl_shell_print_help_command("listPlugins");
			ctrl_shell_printf("\nLists currently loaded plugins.\n");
		}else if(!strcasecmp(command, "listListeners")){
			ctrl_shell_print_help_command("listListeners");
			ctrl_shell_printf("\nLists current listeners.\n");
		}else{
			ctrl_shell_print_help_final(command, "broker");
		}
	}else{
		ctrl_shell_printf("This is the mosquitto_ctrl interactive shell, for controlling aspects of a mosquitto broker.\n");
		ctrl_shell_printf("You are in broker mode, for controlling some core broker functionality.\n");
		ctrl_shell_printf("Use '%sreturn%s' to leave this mode.\n", ANSI_INPUT, ANSI_RESET);

		ctrl_shell_printf("Find help on a command using '%shelp <command>%s'\n", ANSI_INPUT, ANSI_RESET);
		ctrl_shell_printf("Press tab multiple times to find currently available commands.\n\n");
	}
}

static void line_callback(char *line)
{
	if(!line){
		ctrl_shell_callback_final(NULL);
		return;
	}
	ctrl_shell_rtrim(line);
	if(strlen(line) > 0){
		add_history(line);
	}else{
		free(line);
		return;
	}

	char *saveptr = NULL;
	char *command = strtok_r(line, " ", &saveptr);

	if(!strcasecmp(command, "listPlugins")){
		ctrl_shell_command_generic_arg0("listPlugins");
	}else if(!strcasecmp(command, "listListeners")){
		ctrl_shell_command_generic_arg0("listListeners");
	}else if(!strcasecmp(command, "help")){
		print_help(&saveptr);
	}else{
		if(!ctrl_shell_callback_final(line)){
			ctrl_shell_printf("Unknown command '%s'\n", command);
		}
	}

	free(line);
}

static void print_plugins(cJSON *j_data)
{
	cJSON *j_plugins, *j_plugin;

	j_plugins = cJSON_GetObjectItem(j_data, "plugins");

	cJSON_ArrayForEach(j_plugin, j_plugins){
		const char *name;
		if(json_get_string(j_plugin, "name", &name, false) != MOSQ_ERR_SUCCESS){
			ctrl_shell_printf("Invalid response from broker.\n");
			return;
		}

		ctrl_shell_print_label(0, "Plugin:");
		ctrl_shell_print_value(1, "%s\n", name);

		cJSON *j_endpoints, *j_endpoint;
		j_endpoints = cJSON_GetObjectItem(j_plugin, "control-endpoints");
		if(j_endpoints){
			ctrl_shell_print_label(0, "Control endpoints:");
			cJSON_ArrayForEach(j_endpoint, j_endpoints){
				ctrl_shell_print_value(1, "%s\n", j_endpoint->valuestring);
			}
		}
		ctrl_shell_print_value(0, "\n");
	}
}

static void print_listeners(cJSON *j_data)
{
	cJSON *j_listeners, *j_listener;

	j_listeners = cJSON_GetObjectItem(j_data, "listeners");

	cJSON_ArrayForEach(j_listener, j_listeners){
		int port;
		const char *protocol;
		bool tls;

		if(json_get_int(j_listener, "port", &port, false, -1) != MOSQ_ERR_SUCCESS
				|| json_get_string(j_listener, "protocol", &protocol, false) != MOSQ_ERR_SUCCESS
				|| json_get_bool(j_listener, "tls", &tls, false, false) != MOSQ_ERR_SUCCESS){

			ctrl_shell_printf("Invalid response from broker.\n");
			return;
		}

		ctrl_shell_print_label(0, "Listener:");
		ctrl_shell_print_label(1, "Port:");
		ctrl_shell_print_value(2, "%d\n", port);
		ctrl_shell_print_label(1, "Protocol:");
		ctrl_shell_print_value(2, "%s\n", protocol);
		ctrl_shell_print_label(1, "TLS:");
		ctrl_shell_print_value(2, "%s\n\n", tls?"true":"false");
	}

}

static void handle_response(const char *command, cJSON *j_data, const char *payload)
{
	if(!strcmp(command, "listPlugins")){
		print_plugins(j_data);
	}else if(!strcmp(command, "listListeners")){
		print_listeners(j_data);
	}else{
		ctrl_shell_printf("%s %s\n", command, payload);
	}
}


static void on_subscribe(void)
{
}


static void ctrl_shell__broker_cleanup(void)
{
	completion_tree_free(commands_broker);
	commands_broker = NULL;
}


void ctrl_shell__broker_init(struct ctrl_shell__module *mod)
{
	command_tree_create();

	mod->completion_commands = commands_broker;
	mod->request_topic = "$CONTROL/broker/v1";
	mod->response_topic = "$CONTROL/broker/v1/response";
	mod->line_callback = line_callback;
	mod->response_callback = handle_response;
	mod->on_subscribe = on_subscribe;
	mod->cleanup = ctrl_shell__broker_cleanup;
}
#endif
