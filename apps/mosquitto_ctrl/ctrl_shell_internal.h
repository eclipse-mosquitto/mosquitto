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
#ifndef CTRL_SHELL_INTERNAL_H
#define CTRL_SHELL_INTERNAL_H

#ifdef WITH_EDITLINE

#ifdef __cplusplus
extern "C" {
#endif

#include <cjson/cJSON.h>
#include <editline/readline.h>

#include "ctrl_shell.h"
#include "mosquitto_ctrl.h"

struct ctrl_shell{
	char **subscription_list;
	int subscription_list_count;
	int run;
	struct mosquitto *mosq;
	rl_vcpfunc_t *line_callback;
	pthread_cond_t response_cond;
	pthread_mutex_t response_mutex;
	const char *request_topic;
	struct completion_tree_root *commands;
	void (*response_callback)(const char *command, cJSON *data, const char *payload);
	void (*mod_cleanup)(void);
	const char *url_scheme;
	char *username;
	char *password;
	char *clientid;
	char *tls_cafile;
	char *tls_capath;
	char *tls_certfile;
	char *tls_keyfile;
	char *hostname;
	int port;
	int connect_rc;
	int subscribe_rc;
	int publish_rc;
	int transport;
};

struct ctrl_shell__module{
	struct completion_tree_root *completion_commands;
	const char *request_topic;
	const char *response_topic;
	void (*response_callback)(const char *command, cJSON *data, const char *payload);
	void (*line_callback)(char *line);
	void (*on_subscribe)(void);
	void (*cleanup)(void);
};

extern struct ctrl_shell data;
extern char prompt[200];

void term__set_echo(bool echo);

int do_connect(void);

void ctrl_shell__pre_connect_init(void);
void ctrl_shell__pre_connect_cleanup(void);
void ctrl_shell__post_connect_init(void);
void ctrl_shell__post_connect_cleanup(void);
void ctrl_shell__load_module(void (*mod_init)(struct ctrl_shell__module *mod));

void ctrl_shell__connect_blocking(const char *hostname, int port);
void ctrl_shell__on_connect(struct mosquitto *mosq, void *userdata, int rc);
void ctrl_shell__on_subscribe(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
void ctrl_shell__on_publish(struct mosquitto *mosq, void *userdata, int mid, int reason_code, const mosquitto_property *props);
void ctrl_shell__on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg);

void ctrl_shell__broker_init(struct ctrl_shell__module *mod);
void ctrl_shell__dynsec_init(struct ctrl_shell__module *mod);

void ctrl_shell__main(struct mosq_config *config);

int ctrl_shell__connect(void);
void ctrl_shell__disconnect(void);

void ctrl_shell__output(const char *buf);

#ifdef __cplusplus
}
#endif

#endif
#endif
