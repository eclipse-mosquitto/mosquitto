#ifndef JSON_HELP_H
#define JSON_HELP_H
/*
Copyright (c) 2020-2021 Roger Light <roger@atchoo.org>

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
#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* "optional==false" can also be taken to mean "only return success if the key exists and is valid" */
int json_get_bool(cJSON *json, const char *name, bool *value, bool optional, bool default_value);
int json_get_int(cJSON *json, const char *name, int *value, bool optional, int default_value);
int json_get_int64(cJSON *json, const char *name, int64_t *value, bool optional, int64_t default_value);
int json_get_string(cJSON *json, const char *name, const char **value, bool optional);

cJSON *cJSON_AddIntToObject(cJSON * const object, const char * const name, long long number);
cJSON *cJSON_AddUIntToObject(cJSON * const object, const char * const name, unsigned long long number);
cJSON *cJSON_CreateInt(int num);

#define CJSON_IS_VALID_STRING(A) ((A) != NULL && cJSON_IsString(A) && (A)->valuestring)

#ifdef __cplusplus
}
#endif

#endif
