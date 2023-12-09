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

int ctrl_shell__connect(void)
{
	if(data.port == PORT_UNDEFINED){
		data.port = 1883;
	}
	if(data.mosq){
		mosquitto_destroy(data.mosq);
	}
	data.mosq = mosquitto_new(data.clientid, true, NULL);
	if(!strcmp(data.url_scheme, "mqtts") || !strcmp(data.url_scheme, "wss")){
		mosquitto_int_option(data.mosq, MOSQ_OPT_TLS_USE_OS_CERTS, 1);
	}
	if(data.transport == MOSQ_T_WEBSOCKETS){
		mosquitto_int_option(data.mosq, MOSQ_OPT_TRANSPORT, data.transport);
	}
	if(data.username && data.password){
		mosquitto_username_pw_set(data.mosq, data.username, data.password);
	}
	if(data.tls_cafile || data.tls_capath || data.tls_certfile || data.tls_keyfile){
		int rc = mosquitto_tls_set(data.mosq, data.tls_cafile, data.tls_capath, data.tls_certfile, data.tls_keyfile, NULL);
		if(rc){
			if(rc == MOSQ_ERR_INVAL){
				ctrl_shell_printf("%sError setting TLS options: File not found.%s\n", ANSI_ERROR, ANSI_RESET);
			}else{
				ctrl_shell_printf("%sError setting TLS options: %s.%s\n", ANSI_ERROR, mosquitto_strerror(rc), ANSI_RESET);
			}
		}
	}
	mosquitto_int_option(data.mosq, MOSQ_OPT_PROTOCOL_VERSION, 5);

	mosquitto_connect_callback_set(data.mosq, ctrl_shell__on_connect);
	mosquitto_subscribe_callback_set(data.mosq, ctrl_shell__on_subscribe);
	mosquitto_publish_v5_callback_set(data.mosq, ctrl_shell__on_publish);
	mosquitto_message_callback_set(data.mosq, ctrl_shell__on_message);
	ctrl_shell__connect_blocking(data.hostname, data.port);
	if(data.connect_rc){
		ctrl_shell_printf("%sUnable to connect: %s%s\n", ANSI_ERROR, mosquitto_reason_string(data.connect_rc), ANSI_RESET);
		return 1;
	}
	ctrl_shell__post_connect_init();

	return 0;
}

void ctrl_shell__disconnect(void)
{
	if(!data.mosq) return;

	mosquitto_disconnect(data.mosq);
	mosquitto_loop_stop(data.mosq, false);
	mosquitto_destroy(data.mosq);
	data.mosq = NULL;

	for(int i=0; i<data.subscription_list_count; i++){
		free(data.subscription_list[i]);
	}
	free(data.subscription_list);
	data.subscription_list = NULL;
	data.subscription_list_count = 0;

	ctrl_shell__post_connect_cleanup();
	ctrl_shell__pre_connect_init();
}
#endif
