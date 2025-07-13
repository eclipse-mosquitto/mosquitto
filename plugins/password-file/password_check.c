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

#include <uthash.h>

#include "mosquitto.h"
#include "password_file.h"

int password_file__check(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_basic_auth *ed = event_data;
	struct password_file_data *data = userdata;
	struct mosquitto__unpwd *u;

	UNUSED(event);

	if(ed->username == NULL){
		return MOSQ_ERR_PLUGIN_IGNORE;
	}

	// FIXME if(ed->client->bridge) return MOSQ_ERR_SUCCESS;

	HASH_FIND(hh, data->unpwd, ed->username, strlen(ed->username), u);
	if(u){
		if(u->pw){
			if(ed->password){
				return mosquitto_pw_verify(u->pw, ed->password);
			}else{
				return MOSQ_ERR_AUTH;
			}
		}else{
			return MOSQ_ERR_SUCCESS;
		}
	}

	return MOSQ_ERR_AUTH;
}
