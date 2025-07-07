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

static inline int sqlite3_bind_text_from_c_str(sqlite3_stmt* stmt, int col_index, const char* str)
{
	return sqlite3_bind_text(stmt, col_index, str, (int)strlen(str), SQLITE_STATIC);
}

static inline int sqlite3_bind_text_from_optional_c_str(sqlite3_stmt* stmt, int col_index, const char* str)
{
	return str
		? sqlite3_bind_text_from_c_str(stmt, col_index, str)
		: sqlite3_bind_null(stmt, col_index);
}

static inline int sqlite3_bind_blob_optional(sqlite3_stmt* stmt, int col_index, const void* ptr, int blob_len)
{
	return ptr
		?  sqlite3_bind_blob(stmt, col_index, ptr, blob_len, SQLITE_STATIC)
		: sqlite3_bind_null(stmt, col_index);
}

static inline int sqlite3_single_step_stmt(int rc, struct mosquitto_sqlite* ms, sqlite3_stmt* stmt)
{
	if (rc != MOSQ_ERR_SUCCESS){
		return rc;
	}
	ms->event_count++;
	return sqlite3_step(stmt) == SQLITE_DONE ? MOSQ_ERR_SUCCESS : MOSQ_ERR_UNKNOWN;
}

static inline char *properties_to_json_str(const mosquitto_property *properties)
{
	cJSON *array;
	char *json_str;

	array = mosquitto_properties_to_json(properties);
	if(!array) return NULL;

	json_str = cJSON_PrintUnformatted(array);
	cJSON_Delete(array);
	return json_str;
}


