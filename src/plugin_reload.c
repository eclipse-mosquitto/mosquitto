/*
Copyright (c) 2016-2025 Roger Light <roger@atchoo.org>

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

#include <string.h>

#include "mosquitto_broker_internal.h"
#include "mosquitto/broker.h"
#include "utlist.h"


static void plugin__handle_reload_single(struct mosquitto__security_options *opts)
{
	struct mosquitto_evt_reload event_data;
	struct mosquitto__callback *cb_base, *cb_next;

	memset(&event_data, 0, sizeof(event_data));

	// Using DL_FOREACH_SAFE here, as reload callbacks might unregister themself
	DL_FOREACH_SAFE(opts->plugin_callbacks.reload, cb_base, cb_next){
		cb_base->cb(MOSQ_EVT_RELOAD, &event_data, cb_base->userdata);
	}
}


void plugin__handle_reload(void)
{
	struct mosquitto__security_options *opts;

	/* Global plugins */
	plugin__handle_reload_single(&db.config->security_options);

	if(db.config->per_listener_settings){
		for(int i=0; i<db.config->listener_count; i++){
			opts = db.config->listeners[i].security_options;
			if(opts && opts->plugin_callbacks.reload){
				plugin__handle_reload_single(opts);
			}
		}
	}
}
