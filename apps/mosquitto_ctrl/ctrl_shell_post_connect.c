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

#ifdef WITH_EDITLINE

#define UNUSED(A) (void)(A)

static struct completion_tree_root *commands_post_connect = NULL;

static void command_tree_create(void)
{
	if(commands_post_connect) return;

	commands_post_connect = calloc(1, sizeof(struct completion_tree_root));

	completion_tree_cmd_add(commands_post_connect, "broker");
	completion_tree_cmd_add(commands_post_connect, "disconnect");
	completion_tree_cmd_add(commands_post_connect, "dynsec");
	completion_tree_cmd_add(commands_post_connect, "help");
	completion_tree_cmd_add(commands_post_connect, "exit");
}

static void print_help(char **saveptr)
{
	char *command = strtok_r(NULL, " ", saveptr);
	if(command){
		if(!strcasecmp(command, "dynsec")){
			ctrl_shell_print_help_command("dynsec");
			ctrl_shell_printf("\nStart the dynamic-security control mode.\n");
		}else if(!strcasecmp(command, "broker")){
			ctrl_shell_print_help_command("broker");
			ctrl_shell_printf("\nStart the broker control mode.\n");
		}else{
			ctrl_shell_print_help_final(command, NULL);
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

struct module_data{
	const char *name;
	void (*mod_init)(struct ctrl_shell__module *mod);
};

const struct module_data mod_data[] = {
	{ "broker", ctrl_shell__broker_init },
	{ "dynsec", ctrl_shell__dynsec_init },
};

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
	bool found = false;

	for(size_t i=0; i<sizeof(mod_data)/sizeof(struct module_data); i++){
		if(!strcasecmp(command, mod_data[i].name)){
			snprintf(prompt, sizeof(prompt), "%s%s://%s:%d%s|%s%s%s>%s ",
					ANSI_URL, data.url_scheme, data.hostname, data.port, ANSI_RESET,
					ANSI_MODULE, mod_data[i].name, ANSI_INPUT, ANSI_RESET);
			ctrl_shell__load_module(mod_data[i].mod_init);
			found = true;
			break;
		}
	}
	if(!strcasecmp(command, "help")){
		found = true;
		print_help(&saveptr);
	}
	if(found == false){
		if(!ctrl_shell_callback_final(line)){
			ctrl_shell_printf("Unknown command '%s'\n", command);
		}
	}

	free(line);
}


void ctrl_shell__post_connect_cleanup(void)
{
	completion_tree_free(commands_post_connect);
	commands_post_connect = NULL;
}


void ctrl_shell__post_connect_init(void)
{
	command_tree_create();

	ctrl_shell_completion_commands_set(commands_post_connect);
	ctrl_shell_line_callback_set(line_callback);

	snprintf(prompt, sizeof(prompt), "%s%s://%s:%d%s>%s ", ANSI_URL, data.url_scheme, data.hostname, data.port, ANSI_INPUT, ANSI_RESET);
}
#endif
