/*
Copyright (c) 2025 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR EDL-1.0

Contributors:
   Roger Light - initial implementation and documentation.
*/

/*
 * This is an example plugin showing how a plugin can choose how frequently it
 * receives a tick event. Note that this request is not a guarantee that the
 * tick will be called that frequently, only that it will not be called more
 * frequently.
 *
 * Setting to 0 means that a tick event will always be triggered for this
 * plugin when the broker is ready to do so.
 *
 * Compile with:
 *   gcc -I<path to mosquitto-repo/include> -fPIC -shared mosquitto_tick.c -o mosquitto_tick.so
 *
 * Use in config with the below, where the interval is in seconds:
 *
 *   plugin /path/to/mosquitto_tick.so
 *   plugin_opt_interval 1
 *
 * Note that this only works on Mosquitto 2.0 or later.
 */


#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <uthash.h>

#include "mosquitto.h"

#define PLUGIN_NAME "tick-interval"
#define PLUGIN_VERSION NULL

#ifndef UNUSED
#  define UNUSED(A) (void)(A)
#endif

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

struct plugin_data{
	mosquitto_plugin_id_t *pid;
	int interval;
};


static int tick_callback(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_tick *ed = event_data;
	struct plugin_data *data = userdata;

	UNUSED(event);

	mosquitto_log_printf(MOSQ_LOG_INFO, "Tick event for plugin with interval %d.", data->interval);
	ed->next_s = data->interval; /* We want the next tick to occur at the earliest in "interval" seconds */
	ed->next_ms = 0; /* And 0 milliseconds */

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **userdata, struct mosquitto_opt *opts, int opt_count)
{
	struct plugin_data *data;

	data = mosquitto_calloc(1, sizeof(struct plugin_data));
	if(!data){
		return MOSQ_ERR_NOMEM;
	}
	*userdata = data;
	data->interval = -1;
	data->pid = identifier;

	mosquitto_plugin_set_info(identifier, PLUGIN_NAME, PLUGIN_VERSION);

	for(int i=0; i<opt_count; i++){
		if(!strcmp(opts[i].key, "interval")){
			data->interval = atoi(opts[i].value);
		}else{
			mosquitto_log_printf(MOSQ_LOG_ERR, "Error: Unknown option '%s'.", opts[i].key);
			return MOSQ_ERR_INVAL;
		}
	}
	if(data->interval < 0){
		mosquitto_log_printf(MOSQ_LOG_ERR, "Error: interval must be >= 0.");
		return MOSQ_ERR_INVAL;
	}

	return mosquitto_callback_register(data->pid, MOSQ_EVT_TICK, tick_callback, NULL, data);
}


int mosquitto_plugin_cleanup(void *userdata, struct mosquitto_opt *opts, int opt_count)
{
	struct plugin_data *data = userdata;

	UNUSED(opts);
	UNUSED(opt_count);

	mosquitto_FREE(data);

	return MOSQ_ERR_SUCCESS;
}
