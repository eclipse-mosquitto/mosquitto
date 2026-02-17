/*
Copyright (c) 2024 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
*/

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>

#include <cjson/cJSON.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include "mosquitto.h"
#include "mosquitto/broker.h"
#include "mosquitto/broker_plugin.h"

#include "pulse_client.h"
#include "pulse_discovery.h"

/* Declare plugin version */
MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

/* Plugin state */
struct pulse_plugin_data {
	char *connection_string;
	char *agent_id;
	char *project_id;
	char *grpc_server;
	int discovery_max_entries;
	bool initialized;
};

static mosquitto_plugin_id_t *plg_id = NULL;
static struct pulse_plugin_data plugin_data;

/* Forward declarations */
static int pulse_message_callback(int event, void *event_data, void *userdata);
static int pulse_tick_callback(int event, void *event_data, void *userdata);

/*
 * Parse a Pulse connection string (JWT token) and extract:
 * - sub -> agent_id
 * - project -> project_id
 * - uri -> grpc_server (strips tcp:// or grpc:// prefix)
 */
static int parse_connection_string(struct pulse_plugin_data *data, const char *jwt)
{
	const char *first_dot, *second_dot;
	char *payload_b64 = NULL;
	char *payload_b64_padded = NULL;
	unsigned char *payload_json = NULL;
	int payload_len = 0;
	cJSON *json = NULL;
	cJSON *item;
	const char *str_val;
	int rc = -1;
	size_t payload_b64_len;
	int padding_needed;

	if (!jwt) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: NULL connection string.");
		return -1;
	}

	/* Store the full connection string for use as auth token */
	free(data->connection_string);
	data->connection_string = strdup(jwt);
	if (!data->connection_string) {
		return -1;
	}

	/* JWT format: header.payload.signature - extract payload */
	first_dot = strchr(jwt, '.');
	if (!first_dot) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Invalid JWT format - no first dot.");
		return -1;
	}
	second_dot = strchr(first_dot + 1, '.');
	if (!second_dot) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Invalid JWT format - no second dot.");
		return -1;
	}

	/* Extract the payload (between first and second dot) */
	payload_b64_len = (size_t)(second_dot - first_dot - 1);
	payload_b64 = (char *)malloc(payload_b64_len + 1);
	if (!payload_b64) {
		return -1;
	}
	memcpy(payload_b64, first_dot + 1, payload_b64_len);
	payload_b64[payload_b64_len] = '\0';

	/* Base64url to Base64: replace - with + and _ with / */
	for (size_t i = 0; i < payload_b64_len; i++) {
		if (payload_b64[i] == '-') {
			payload_b64[i] = '+';
		} else if (payload_b64[i] == '_') {
			payload_b64[i] = '/';
		}
	}

	/* Add padding if necessary */
	padding_needed = (4 - (payload_b64_len % 4)) % 4;
	payload_b64_padded = (char *)malloc(payload_b64_len + padding_needed + 1);
	if (!payload_b64_padded) {
		free(payload_b64);
		return -1;
	}
	memcpy(payload_b64_padded, payload_b64, payload_b64_len);
	for (int i = 0; i < padding_needed; i++) {
		payload_b64_padded[payload_b64_len + i] = '=';
	}
	payload_b64_padded[payload_b64_len + padding_needed] = '\0';
	free(payload_b64);

	/* Decode base64 using OpenSSL */
	BIO *bio = BIO_new_mem_buf(payload_b64_padded, -1);
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	bio = BIO_push(b64, bio);

	/* Allocate buffer for decoded data */
	payload_json = (unsigned char *)malloc(payload_b64_len + padding_needed + 1);
	if (!payload_json) {
		BIO_free_all(bio);
		free(payload_b64_padded);
		return -1;
	}

	payload_len = BIO_read(bio, payload_json, (int)(payload_b64_len + padding_needed));
	BIO_free_all(bio);
	free(payload_b64_padded);

	if (payload_len <= 0) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Failed to decode JWT payload.");
		free(payload_json);
		return -1;
	}
	payload_json[payload_len] = '\0';

	/* Parse JSON payload */
	json = cJSON_ParseWithLength((const char *)payload_json, payload_len);
	if (!json) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Invalid JSON in JWT payload.");
		free(payload_json);
		return -1;
	}
	free(payload_json);

	/* Extract "sub" -> agent_id */
	item = cJSON_GetObjectItem(json, "sub");
	str_val = cJSON_GetStringValue(item);
	if (str_val) {
		free(data->agent_id);
		data->agent_id = strdup(str_val);
		if (!data->agent_id) {
			rc = -1;
			goto cleanup;
		}
	} else {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: JWT missing 'sub' field.");
		goto cleanup;
	}

	/* Extract "project" -> project_id */
	item = cJSON_GetObjectItem(json, "project");
	str_val = cJSON_GetStringValue(item);
	if (str_val) {
		free(data->project_id);
		data->project_id = strdup(str_val);
		if (!data->project_id) {
			rc = -1;
			goto cleanup;
		}
	} else {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: JWT missing 'project' field.");
		goto cleanup;
	}

	/* Extract "uri" -> grpc_server (strip tcp:// or grpc:// prefix) */
	item = cJSON_GetObjectItem(json, "uri");
	str_val = cJSON_GetStringValue(item);
	if (str_val) {
		const char *server = str_val;
		/* Strip protocol prefix if present */
		if (strncmp(server, "tcp://", 6) == 0) {
			server += 6;
		} else if (strncmp(server, "grpc://", 7) == 0) {
			server += 7;
		} else if (strncmp(server, "https://", 8) == 0) {
			server += 8;
		} else if (strncmp(server, "http://", 7) == 0) {
			server += 7;
		}
		free(data->grpc_server);
		data->grpc_server = strdup(server);
		if (!data->grpc_server) {
			rc = -1;
			goto cleanup;
		}
	} else {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: JWT missing 'uri' field.");
		goto cleanup;
	}

	rc = 0;

cleanup:
	cJSON_Delete(json);
	return rc;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **userdata, struct mosquitto_opt *options, int option_count)
{
	int i;
	int rc;

	memset(&plugin_data, 0, sizeof(struct pulse_plugin_data));
	plugin_data.discovery_max_entries = 1000; /* Default */

	/* Parse plugin options */
	for (i = 0; i < option_count; i++) {
		if (!strcasecmp(options[i].key, "connection_string")) {
			if (parse_connection_string(&plugin_data, options[i].value) != 0) {
				mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Failed to parse connection_string.");
				return MOSQ_ERR_INVAL;
			}
		} else if (!strcasecmp(options[i].key, "agent_id")) {
			free(plugin_data.agent_id);
			plugin_data.agent_id = strdup(options[i].value);
		} else if (!strcasecmp(options[i].key, "project_id")) {
			free(plugin_data.project_id);
			plugin_data.project_id = strdup(options[i].value);
		} else if (!strcasecmp(options[i].key, "grpc_server")) {
			free(plugin_data.grpc_server);
			plugin_data.grpc_server = strdup(options[i].value);
		} else if (!strcasecmp(options[i].key, "discovery_max")) {
			plugin_data.discovery_max_entries = atoi(options[i].value);
		}
	}

	/* Validate required configuration */
	if (!plugin_data.agent_id || !plugin_data.project_id) {
		mosquitto_log_printf(MOSQ_LOG_WARNING,
			"Pulse: Missing agent_id or project_id. Plugin will not be activated. "
			"Provide plugin_opt_connection_string or individual plugin_opt_agent_id/plugin_opt_project_id.");
		return MOSQ_ERR_SUCCESS;
	}

	plg_id = identifier;
	mosquitto_plugin_set_info(identifier, "pulse", NULL);

	/* Initialize discovery module with max entries config */
	pulse_discovery_init(plugin_data.discovery_max_entries);

	/* Perform gRPC handshake with retry */
	const char *server = plugin_data.grpc_server ? plugin_data.grpc_server : "localhost:50051";
	mosquitto_log_printf(MOSQ_LOG_INFO, "Pulse: Connecting to server at %s", server);

	int retry_count = 0;
	while ((rc = pulse_handshake(server, plugin_data.agent_id, plugin_data.project_id, plugin_data.connection_string)) != 0) {
		retry_count++;
		if (retry_count > 3) {
			mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Handshake failed after %d retries, giving up.", retry_count);
			return MOSQ_ERR_UNKNOWN;
		}
		mosquitto_log_printf(MOSQ_LOG_WARNING, "Pulse: Handshake failed, retrying in 10 seconds... (attempt %d)", retry_count);
		/* Note: In plugin context we can't block for 10 seconds, so we fail after retries */
	}

	plugin_data.initialized = true;

	/* Register message callback for topic discovery */
	rc = mosquitto_callback_register(plg_id, MOSQ_EVT_MESSAGE_IN, pulse_message_callback, NULL, &plugin_data);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Failed to register message callback: %d", rc);
		pulse_cleanup();
		return rc;
	}

	/* Register tick callback for periodic stats logging */
	rc = mosquitto_callback_register(plg_id, MOSQ_EVT_TICK, pulse_tick_callback, NULL, &plugin_data);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "Pulse: Failed to register tick callback: %d", rc);
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_MESSAGE_IN, pulse_message_callback, NULL);
		pulse_cleanup();
		return rc;
	}

	mosquitto_log_printf(MOSQ_LOG_INFO, "Pulse: Plugin initialized successfully.");
	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_cleanup(void *userdata, struct mosquitto_opt *options, int option_count)
{
	(void)userdata;
	(void)options;
	(void)option_count;

	if (plugin_data.initialized) {
		/* Unregister callbacks */
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_MESSAGE_IN, pulse_message_callback, NULL);
		mosquitto_callback_unregister(plg_id, MOSQ_EVT_TICK, pulse_tick_callback, NULL);

		/* Cleanup gRPC client */
		pulse_cleanup();

		/* Cleanup discovery module */
		pulse_discovery_cleanup();
	}

	/* Free configuration strings */
	free(plugin_data.connection_string);
	free(plugin_data.agent_id);
	free(plugin_data.project_id);
	free(plugin_data.grpc_server);

	memset(&plugin_data, 0, sizeof(struct pulse_plugin_data));

	mosquitto_log_printf(MOSQ_LOG_INFO, "Pulse: Plugin cleanup complete.");
	return MOSQ_ERR_SUCCESS;
}


/*
 * Message callback - called for each incoming PUBLISH message.
 * Used for topic discovery.
 */
static int pulse_message_callback(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_message *msg = (struct mosquitto_evt_message *)event_data;

	(void)event;
	(void)userdata;

	if (msg && msg->topic) {
		pulse_discovery_process_message(msg->topic, msg->payload, msg->payloadlen);
	}

	return MOSQ_ERR_SUCCESS;
}


/*
 * Tick callback - called periodically by the broker.
 * Used for logging discovery stats.
 */
static int pulse_tick_callback(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_tick *tick = (struct mosquitto_evt_tick *)event_data;
	static time_t last_stats_log = 0;

	(void)event;
	(void)userdata;

	/* Log stats every 5 seconds */
	if (tick->now_s - last_stats_log >= 5) {
		size_t count = pulse_discovery_get_count();
		mosquitto_log_printf(MOSQ_LOG_INFO, "Pulse: Discovered topics count: %zu", count);
		last_stats_log = tick->now_s;
	}

	return MOSQ_ERR_SUCCESS;
}
