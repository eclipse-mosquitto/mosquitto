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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "acl_file.h"
#include "mosquitto_broker_internal.h"
#include "mosquitto/mqtt_protocol.h"
#include "send_mosq.h"
#include "util_mosq.h"


int acl_file__init(void)
{
	int rc;

	/* Load acl data if required. */
	if(db.config->per_listener_settings){
		for(int i=0; i<db.config->listener_count; i++){
			if(db.config->listeners[i].security_options->acl_data.acl_file){
				rc = acl__parse(&db.config->listeners[i].security_options->acl_data);
				if(rc){
					log__printf(NULL, MOSQ_LOG_ERR, "Error opening acl file \"%s\".", db.config->listeners[i].security_options->acl_file);
					return rc;
				}
				if(db.config->listeners[i].security_options->plugin_count == 0){
					config__plugin_add_secopt(db.config->listeners[i].security_options->pid, db.config->listeners[i].security_options);
				}

				mosquitto_callback_register(db.config->listeners[i].security_options->pid,
						MOSQ_EVT_ACL_CHECK, acl__check, NULL, &db.config->listeners[i].security_options->acl_data.acl_file);
			}
		}
	}else{
		if(db.config->security_options.acl_data.acl_file){
			rc = acl__parse(&db.config->security_options.acl_data);
			if(rc){
				log__printf(NULL, MOSQ_LOG_ERR, "Error opening acl file \"%s\".", db.config->security_options.acl_data.acl_file);
				return rc;
			}
			if(db.config->security_options.plugin_count == 0){
				config__plugin_add_secopt(db.config->security_options.pid, &db.config->security_options);
			}

			mosquitto_callback_register(db.config->security_options.pid,
					MOSQ_EVT_ACL_CHECK, acl__check, NULL, &db.config->security_options.acl_data);
		}
	}

	return MOSQ_ERR_SUCCESS;
}

int acl_file__cleanup(void)
{
	if(db.config->per_listener_settings){
		for(int i=0; i<db.config->listener_count; i++){
			if(db.config->listeners[i].security_options->pid){
				mosquitto_callback_unregister(db.config->listeners[i].security_options->pid,
						MOSQ_EVT_ACL_CHECK, acl__check, NULL);

				acl__cleanup(&db.config->listeners[i].security_options->acl_data);
				mosquitto_FREE(db.config->listeners[i].security_options->pid->plugin_name);
				mosquitto_FREE(db.config->listeners[i].security_options->pid->config.security_options);
				mosquitto_FREE(db.config->listeners[i].security_options->pid);
			}
		}
	}else{
		if(db.config->security_options.pid){
			mosquitto_callback_unregister(db.config->security_options.pid,
					MOSQ_EVT_ACL_CHECK, acl__check, NULL);

			acl__cleanup(&db.config->security_options.acl_data);
			mosquitto_FREE(db.config->security_options.pid->plugin_name);
			mosquitto_FREE(db.config->security_options.pid->config.security_options);
			mosquitto_FREE(db.config->security_options.pid);
		}
	}
	return MOSQ_ERR_SUCCESS;
}
