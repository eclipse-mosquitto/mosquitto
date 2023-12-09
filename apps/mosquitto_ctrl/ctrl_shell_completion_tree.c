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

void completion_tree_arg_list_args_free(struct completion_tree_arg_list *arg_list)
{
	struct completion_tree_arg *arg, *next;

	if(!arg_list) return;

	arg = arg_list->args;
	while(arg){
		next = arg->next;
		free(arg);
		arg = next;
	}
	arg_list->args = NULL;
}

void completion_tree_arg_list_free(struct completion_tree_arg_list *arg_list)
{
	if(!arg_list) return;
	if(arg_list->is_shared) return;

	completion_tree_arg_list_args_free(arg_list);
	free(arg_list);
}

void completion_tree_cmd_free(struct completion_tree_cmd *cmd)
{
	if(!cmd) return;

	for(int i=0; i<cmd->arg_list_count; i++){
		completion_tree_arg_list_free(cmd->arg_lists[i]);
	}
	free(cmd->arg_lists);
	free(cmd);
}

void completion_tree_free(struct completion_tree_root *tree)
{
	struct completion_tree_cmd *cmd, *next;

	if(!tree) return;

	cmd = tree->commands;
	while(cmd){
		next = cmd->next;
		completion_tree_cmd_free(cmd);
		cmd = next;
	}
	free(tree);
}

struct completion_tree_cmd *completion_tree_cmd_add(struct completion_tree_root *root, const char *name)
{
	struct completion_tree_cmd *new_node;

	new_node = calloc(1, sizeof(struct completion_tree_cmd) + strlen(name) + 1);
	if(!new_node) return NULL;

	strcpy(new_node->name, name);

	new_node->next = root->commands;
	root->commands = new_node;

	return new_node;
}

struct completion_tree_arg_list *completion_tree_cmd_new_arg_list(void)
{
	return calloc(1, sizeof(struct completion_tree_arg_list));
}

void completion_tree_cmd_append_arg_list(struct completion_tree_cmd *cmd, struct completion_tree_arg_list *new_list)
{
	struct completion_tree_arg_list **arg_list;

	arg_list = realloc(cmd->arg_lists, (size_t)(cmd->arg_list_count+1)*sizeof(struct completion_tree_arg_list *));
	if(!arg_list) return;

	cmd->arg_lists = arg_list;

	cmd->arg_lists[cmd->arg_list_count] = new_list;
	cmd->arg_list_count++;
}

struct completion_tree_arg_list *completion_tree_cmd_add_arg_list(struct completion_tree_cmd *cmd)
{
	if(!cmd) return NULL;

	struct completion_tree_arg_list *new_list;
	new_list = completion_tree_cmd_new_arg_list();
	if(!new_list) return NULL;

	completion_tree_cmd_append_arg_list(cmd, new_list);

	return new_list;
}

void completion_tree_arg_list_add_arg(struct completion_tree_arg_list *arg_list, const char *name)
{
	if(!arg_list || !name) return;

	struct completion_tree_arg *new_node;

	new_node = calloc(1, sizeof(struct completion_tree_arg) + strlen(name) + 1);
	if(!new_node) return;

	strcpy(new_node->name, name);

	new_node->next = arg_list->args;
	arg_list->args = new_node;
}
#endif
