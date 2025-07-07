/*
Copyright (c) 2025 Cedalo GmbH

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
*/

#include <string.h>
#include <sqlite3.h>

#include "mosquitto.h"
#include "mosquitto/broker.h"
#include "persist_sqlite.h"
#include "util.h"

int persist_sqlite__will_add_cb(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_persist_will_msg *ed = event_data;
	struct mosquitto_sqlite *ms = userdata;
	int rc = MOSQ_ERR_SUCCESS;
	char *propties_json_str = NULL;
	UNUSED(event);

	if (!ed->data.clientid || !ed->data.topic){
		return MOSQ_ERR_INVAL;
	}
	
	if(ed->data.properties){
		propties_json_str = properties_to_json_str(ed->data.properties);
		if (!propties_json_str){
			return MOSQ_ERR_NOMEM;
		}
	}

	if(sqlite3_bind_text_from_c_str( ms->will_add_stmt, 1, ed->data.clientid) != SQLITE_OK
		 || sqlite3_bind_blob_optional(ms->will_add_stmt, 2, ed->data.payload, (int)ed->data.payloadlen) != SQLITE_OK
		 || sqlite3_bind_text_from_c_str(ms->will_add_stmt, 3, ed->data.topic) != SQLITE_OK
		 || sqlite3_bind_int64(ms->will_add_stmt, 4, (int64_t)ed->data.payloadlen) != SQLITE_OK
		 || sqlite3_bind_int(ms->will_add_stmt, 5, ed->data.qos) != SQLITE_OK
		 || sqlite3_bind_int(ms->will_add_stmt, 6, ed->data.retain) != SQLITE_OK
		 || sqlite3_bind_text_from_optional_c_str( ms->will_add_stmt, 7, propties_json_str) != SQLITE_OK) {
		rc = MOSQ_ERR_UNKNOWN;
	}
	rc = sqlite3_single_step_stmt(rc, ms, ms->will_add_stmt);
	sqlite3_reset(ms->will_add_stmt);
	mosquitto_free(propties_json_str);
	
	return rc;
}

int persist_sqlite__will_remove_cb(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_persist_will_msg *ed = event_data;
	struct mosquitto_sqlite *ms = userdata;
	int rc = MOSQ_ERR_SUCCESS;
	UNUSED(event);

	if(sqlite3_bind_text_from_c_str( ms->will_remove_stmt, 1, ed->data.clientid) != SQLITE_OK){
		rc = MOSQ_ERR_UNKNOWN;
	}
	rc = sqlite3_single_step_stmt(rc, ms, ms->will_remove_stmt);
	sqlite3_reset(ms->will_remove_stmt);
	
	return rc;
}
