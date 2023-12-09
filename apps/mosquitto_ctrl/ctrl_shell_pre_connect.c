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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl_shell_internal.h"

#ifdef WITH_EDITLINE

#define UNUSED(A) (void)(A)

static struct completion_tree_root *commands_pre_connect = NULL;

static void command_tree_create(void)
{
	if(commands_pre_connect) return;

	commands_pre_connect = calloc(1, sizeof(struct completion_tree_root));

	completion_tree_cmd_add(commands_pre_connect, "auth");
	completion_tree_cmd_add(commands_pre_connect, "connect");
	completion_tree_cmd_add(commands_pre_connect, "help");
	completion_tree_cmd_add(commands_pre_connect, "exit");
}


void print_help(char **saveptr)
{
	char *command = strtok_r(NULL, " ", saveptr);
	if(command){
		if(!strcasecmp(command, "auth")){
			ctrl_shell_print_help_command("auth [username]");
			ctrl_shell_printf("\nSet a username and password prior to connecting to a broker.\n");
		}else if(!strcasecmp(command, "connect")){
			ctrl_shell_print_help_command("connect");
			ctrl_shell_print_help_command("connect mqtt://hostname[:port]");
			ctrl_shell_print_help_command("connect mqtts://hostname[:port]");
			ctrl_shell_print_help_command("connect ws://hostname[:port]");
			ctrl_shell_print_help_command("connect wss://hostname[:port]");
			ctrl_shell_printf("\nConnect to a broker using the provided transport and port.\n");
			ctrl_shell_printf("If no URL is provided, connects to mqtt://localhost:1883\n");
		}else if(!strcasecmp(command, "exit")){
			ctrl_shell_print_help_command("exit");
			ctrl_shell_printf("\nQuit the program\n");
		}else if(!strcasecmp(command, "help")){
			ctrl_shell_print_help_command("help <command>");
			ctrl_shell_printf("\nFind help on a command using '%shelp <command>%s'\n", ANSI_INPUT, ANSI_RESET);
			ctrl_shell_printf("Press tab multiple times to find currently available commands.\n");
		}else{
			ctrl_shell_printf("Unknown command '%s'\n", command);
		}
	}else{
		ctrl_shell_printf("This is the mosquitto_ctrl interactive shell, for controlling aspects of a mosquitto broker.\n");

		ctrl_shell_printf("Find help on a command using '%shelp <command>%s'\n", ANSI_INPUT, ANSI_RESET);
		ctrl_shell_printf("Press tab multiple times to find currently available commands.\n\n");

		ctrl_shell_printf("Example workflow:\n\n");
		ctrl_shell_printf("> auth\n");
		ctrl_shell_printf("username: admin\n");
		ctrl_shell_printf("password:\n");
		ctrl_shell_printf("> connect mqtt://localhost\n");
		ctrl_shell_printf("mqtt://localhost:1883> dynsec\n");
		ctrl_shell_printf("mqtt://localhost:1883|dynsec> createGroup newgroup\n");
		ctrl_shell_printf("OK\n\n");
	}
}


static void line_callback(char *line)
{
	char *saveptr = NULL;
	char *command;

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

	command = strtok_r(line, " ", &saveptr);

	if(!strcasecmp(command, "auth")){
		free(data.username);
		char *username_i = strtok_r(NULL, " ", &saveptr);
		if(username_i){
			data.username = strdup(username_i);
		}else{
			char promptbuf[50];
			snprintf(promptbuf, sizeof(promptbuf), "%susername:%s ", ANSI_INPUT, ANSI_RESET);
			data.username = readline(promptbuf);
		}

		char pwbuf[200];
		if(!ctrl_shell_get_password(pwbuf, sizeof(pwbuf))){
			ctrl_shell_printf("No password.\n");
			free(line);
			return;
		}
		free(data.password);
		data.password = strdup(pwbuf);
	}else if(!strcasecmp(command, "connect")){
		char *url = strtok_r(NULL, " ", &saveptr);
		if(url){
			if(!strncasecmp(url, "mqtt://", 7)) {
				url += 7;
				data.port = 1883;
				data.url_scheme = "mqtt";
			} else if(!strncasecmp(url, "mqtts://", 8)) {
#ifdef WITH_TLS
				url += 8;
				data.port = 8883;
				data.url_scheme = "mqtts";
#else
				ctrl_shell_printf("TLS support not available.\n");
				free(line);
				return;
#endif
			} else if(!strncasecmp(url, "ws://", 5)) {
				url += 5;
				data.port = 1883;
				data.transport = MOSQ_T_WEBSOCKETS;
				data.url_scheme = "ws";
			} else if(!strncasecmp(url, "wss://", 6)) {
#ifdef WITH_TLS
				url += 6;
				data.port = 8883;
				data.transport = MOSQ_T_WEBSOCKETS;
				data.url_scheme = "wss";
#else
				ctrl_shell_printf("TLS support not available.\n");
				free(line);
				return;
#endif
			}
			char *hostname_i = strtok_r(url, ":", &saveptr);
			char *port_i = strtok_r(NULL, ":", &saveptr);

			if(!hostname_i){
				ctrl_shell_printf("connect mqtt[s]://<hostname>:port\n");
				free(line);
				return;
			}
			free(data.hostname);
			data.hostname = strdup(hostname_i);
			if(port_i){
				data.port = atoi(port_i);
			}
		}else{
			if(data.hostname == NULL){
				data.hostname = strdup("localhost");
			}
			if(data.port == PORT_UNDEFINED){
				data.port = 1883;
			}
		}

		ctrl_shell__connect();
	}else if(!strcasecmp(command, "help")){
		print_help(&saveptr);
	}else if(!strcasecmp(command, "exit")){
		data.run = 0;
	}else{
		ctrl_shell_printf("Unknown command '%s'\n", command);
	}

	free(line);
}


void ctrl_shell__pre_connect_cleanup(void)
{
	completion_tree_free(commands_pre_connect);
	commands_pre_connect = NULL;
}


void ctrl_shell__pre_connect_init(void)
{
	command_tree_create();
	ctrl_shell_completion_commands_set(commands_pre_connect);
	ctrl_shell_line_callback_set(line_callback);
	snprintf(prompt, sizeof(prompt), "%s>%s ", ANSI_INPUT, ANSI_RESET);
}
#endif
