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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <uthash.h>
#include <cjson/cJSON.h>

#include "mosquitto_broker_internal.h"
#include "pulse_discovery.h"
#include "pulse_information_model.h"

/* Maximum payload sample size to store (to avoid memory bloat) */
#define MAX_PAYLOAD_SAMPLE_SIZE 1024

/* Hash table entry for discovered topics */
typedef struct discovery_entry {
	char *topic;                    /* Key for hash table */
	void *payload_sample;
	size_t payload_sample_len;
	uint64_t message_count;
	time_t first_seen;
	time_t last_seen;
	UT_hash_handle hh;
} discovery_entry_t;

/* Global hash table of discovered topics */
static discovery_entry_t *g_discovered_topics = NULL;
static pthread_mutex_t g_discovery_mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t g_discovery_count = 0;

/* Stats logging thread */
static pthread_t g_stats_thread;
static volatile bool g_stats_running = false;

/* Free a single discovery entry */
static void free_discovery_entry(discovery_entry_t *entry)
{
	if (entry) {
		mosquitto_FREE(entry->topic);
		mosquitto_FREE(entry->payload_sample);
		mosquitto_FREE(entry);
	}
}

/* Stats logging thread function */
static void *discovery_stats_thread(void *arg)
{
	(void)arg;

	while (g_stats_running) {
		sleep(5);
		if (!g_stats_running) break;

		pthread_mutex_lock(&g_discovery_mutex);
		size_t count = g_discovery_count;
		pthread_mutex_unlock(&g_discovery_mutex);

		log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Discovered topics count: %zu", count);
	}

	return NULL;
}

bool pulse_discovery_is_governed(const char *topic)
{
	if (!topic) return false;

	/* Check if there's a tag with this exact topic */
	const pulse_tag_t *tag = pulse_find_tag_by_topic(topic);
	return (tag != NULL);
}

bool pulse_discovery_process_message(const char *topic, const void *payload, size_t payload_len)
{
	if (!topic) return false;

	/* First, check if this topic is governed by a tag */
	if (pulse_discovery_is_governed(topic)) {
		return true; /* Governed - no discovery needed */
	}

	/* Topic is not governed - track it as discovered */
	pthread_mutex_lock(&g_discovery_mutex);

	discovery_entry_t *entry = NULL;
	HASH_FIND_STR(g_discovered_topics, topic, entry);

	time_t now = time(NULL);

	if (entry) {
		/* Update existing entry */
		entry->message_count++;
		entry->last_seen = now;

		/* Update payload sample */
		mosquitto_FREE(entry->payload_sample);
		entry->payload_sample = NULL;
		entry->payload_sample_len = 0;

		if (payload && payload_len > 0) {
			size_t sample_len = (payload_len > MAX_PAYLOAD_SAMPLE_SIZE) ? MAX_PAYLOAD_SAMPLE_SIZE : payload_len;
			entry->payload_sample = mosquitto_malloc(sample_len + 1);
			if (entry->payload_sample) {
				memcpy(entry->payload_sample, payload, sample_len);
				((char *)entry->payload_sample)[sample_len] = '\0'; /* Null terminate for safety */
				entry->payload_sample_len = sample_len;
			}
		}
	} else {
		/* Check if we've reached the maximum number of discovered topics */
		int max_entries = db.config->pulse_discovery_max_entries;
		if (max_entries > 0 && (int)g_discovery_count >= max_entries) {
			/* Limit reached - don't add new entries */
			pthread_mutex_unlock(&g_discovery_mutex);
			return false;
		}

		/* Create new entry */
		entry = mosquitto_calloc(1, sizeof(discovery_entry_t));
		if (!entry) {
			pthread_mutex_unlock(&g_discovery_mutex);
			return false;
		}

		entry->topic = mosquitto_strdup(topic);
		if (!entry->topic) {
			free_discovery_entry(entry);
			pthread_mutex_unlock(&g_discovery_mutex);
			return false;
		}

		entry->message_count = 1;
		entry->first_seen = now;
		entry->last_seen = now;

		/* Store payload sample */
		if (payload && payload_len > 0) {
			size_t sample_len = (payload_len > MAX_PAYLOAD_SAMPLE_SIZE) ? MAX_PAYLOAD_SAMPLE_SIZE : payload_len;
			entry->payload_sample = mosquitto_malloc(sample_len + 1);
			if (entry->payload_sample) {
				memcpy(entry->payload_sample, payload, sample_len);
				((char *)entry->payload_sample)[sample_len] = '\0';
				entry->payload_sample_len = sample_len;
			}
		}

		HASH_ADD_KEYPTR(hh, g_discovered_topics, entry->topic, strlen(entry->topic), entry);
		g_discovery_count++;

		/* Log discovery of new topic */
		log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Discovered ungoverned topic: %s", topic);
	}

	pthread_mutex_unlock(&g_discovery_mutex);
	return false; /* Not governed */
}

size_t pulse_discovery_get_count(void)
{
	pthread_mutex_lock(&g_discovery_mutex);
	size_t count = g_discovery_count;
	pthread_mutex_unlock(&g_discovery_mutex);
	return count;
}

const pulse_discovered_topic_t *pulse_discovery_get_all(void)
{
	/* Note: This returns the internal hash table which uses different struct layout.
	 * For a proper implementation, we'd need to build a linked list copy.
	 * For now, return NULL and use pulse_discovery_find or iteration. */
	return NULL;
}

const pulse_discovered_topic_t *pulse_discovery_find(const char *topic)
{
	if (!topic) return NULL;

	pthread_mutex_lock(&g_discovery_mutex);
	discovery_entry_t *entry = NULL;
	HASH_FIND_STR(g_discovered_topics, topic, entry);
	pthread_mutex_unlock(&g_discovery_mutex);

	/* Cast is safe because the structs have compatible layout for the public fields */
	return (const pulse_discovered_topic_t *)entry;
}

void pulse_discovery_clear(void)
{
	pthread_mutex_lock(&g_discovery_mutex);

	discovery_entry_t *entry, *tmp;
	HASH_ITER(hh, g_discovered_topics, entry, tmp) {
		HASH_DEL(g_discovered_topics, entry);
		free_discovery_entry(entry);
	}
	g_discovery_count = 0;

	pthread_mutex_unlock(&g_discovery_mutex);

	log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Cleared all discovered topics");
}

void pulse_discovery_start(void)
{
	if (g_stats_running) return;

	g_stats_running = true;
	if (pthread_create(&g_stats_thread, NULL, discovery_stats_thread, NULL) != 0) {
		log__printf(NULL, MOSQ_LOG_ERR, "Pulse: Failed to create discovery stats thread");
		g_stats_running = false;
	} else {
		log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Discovery stats thread started");
	}
}

void pulse_discovery_cleanup(void)
{
	/* Stop the stats thread */
	if (g_stats_running) {
		g_stats_running = false;
		pthread_join(g_stats_thread, NULL);
		log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Discovery stats thread stopped");
	}

	pulse_discovery_clear();
}

int pulse_discovery_get_details(const char *topic,
                                uint64_t *out_count,
                                time_t *out_first_seen,
                                time_t *out_last_seen,
                                char **out_payload,
                                size_t *out_payload_len)
{
	if (!topic) return -1;

	pthread_mutex_lock(&g_discovery_mutex);

	discovery_entry_t *entry = NULL;
	HASH_FIND_STR(g_discovered_topics, topic, entry);

	if (!entry) {
		pthread_mutex_unlock(&g_discovery_mutex);
		return -1;
	}

	if (out_count) *out_count = entry->message_count;
	if (out_first_seen) *out_first_seen = entry->first_seen;
	if (out_last_seen) *out_last_seen = entry->last_seen;

	if (out_payload && out_payload_len) {
		if (entry->payload_sample && entry->payload_sample_len > 0) {
			*out_payload = (char *)malloc(entry->payload_sample_len + 1);
			if (*out_payload) {
				memcpy(*out_payload, entry->payload_sample, entry->payload_sample_len);
				(*out_payload)[entry->payload_sample_len] = '\0';
				*out_payload_len = entry->payload_sample_len;
			} else {
				*out_payload_len = 0;
			}
		} else {
			*out_payload = NULL;
			*out_payload_len = 0;
		}
	}

	pthread_mutex_unlock(&g_discovery_mutex);
	return 0;
}

int pulse_discovery_get_page(size_t offset, size_t limit,
                             char ***out_topics, size_t *out_count, size_t *out_total)
{
	if (!out_topics || !out_count || !out_total) return -1;

	*out_topics = NULL;
	*out_count = 0;
	*out_total = 0;

	pthread_mutex_lock(&g_discovery_mutex);

	*out_total = g_discovery_count;

	if (g_discovery_count == 0 || offset >= g_discovery_count) {
		pthread_mutex_unlock(&g_discovery_mutex);
		return 0;
	}

	/* Calculate how many to return */
	size_t available = g_discovery_count - offset;
	size_t count = (available < limit) ? available : limit;

	if (count == 0) {
		pthread_mutex_unlock(&g_discovery_mutex);
		return 0;
	}

	/* Allocate array of topic strings */
	char **topics = (char **)malloc(count * sizeof(char *));
	if (!topics) {
		pthread_mutex_unlock(&g_discovery_mutex);
		return -1;
	}

	/* Iterate through hash table with offset and limit */
	discovery_entry_t *entry, *tmp;
	size_t idx = 0;
	size_t result_idx = 0;

	HASH_ITER(hh, g_discovered_topics, entry, tmp) {
		if (idx >= offset && result_idx < count) {
			topics[result_idx] = strdup(entry->topic);
			if (!topics[result_idx]) {
				/* Cleanup on failure */
				for (size_t i = 0; i < result_idx; i++) {
					free(topics[i]);
				}
				free(topics);
				pthread_mutex_unlock(&g_discovery_mutex);
				return -1;
			}
			result_idx++;
		}
		idx++;
		if (result_idx >= count) break;
	}

	pthread_mutex_unlock(&g_discovery_mutex);

	*out_topics = topics;
	*out_count = result_idx;
	return 0;
}

void pulse_discovery_free_page(char **topics, size_t count)
{
	if (!topics) return;
	for (size_t i = 0; i < count; i++) {
		free(topics[i]);
	}
	free(topics);
}

int pulse_discovery_dump_to_json(const char *filepath)
{
	if (!filepath) return -1;

	pthread_mutex_lock(&g_discovery_mutex);

	cJSON *root = cJSON_CreateObject();
	if (!root) {
		pthread_mutex_unlock(&g_discovery_mutex);
		return -1;
	}

	cJSON_AddNumberToObject(root, "discovered_count", (double)g_discovery_count);

	cJSON *topics = cJSON_CreateArray();
	if (topics) {
		discovery_entry_t *entry, *tmp;
		HASH_ITER(hh, g_discovered_topics, entry, tmp) {
			cJSON *topic_obj = cJSON_CreateObject();
			if (topic_obj) {
				cJSON_AddStringToObject(topic_obj, "topic", entry->topic);
				cJSON_AddNumberToObject(topic_obj, "message_count", (double)entry->message_count);
				cJSON_AddNumberToObject(topic_obj, "first_seen", (double)entry->first_seen);
				cJSON_AddNumberToObject(topic_obj, "last_seen", (double)entry->last_seen);

				if (entry->payload_sample && entry->payload_sample_len > 0) {
					/* Try to add as string if it looks like text */
					cJSON_AddStringToObject(topic_obj, "payload_sample", (const char *)entry->payload_sample);
					cJSON_AddNumberToObject(topic_obj, "payload_sample_len", (double)entry->payload_sample_len);
				}

				cJSON_AddItemToArray(topics, topic_obj);
			}
		}
		cJSON_AddItemToObject(root, "topics", topics);
	}

	pthread_mutex_unlock(&g_discovery_mutex);

	char *json_str = cJSON_Print(root);
	cJSON_Delete(root);

	if (!json_str) {
		return -1;
	}

	FILE *fp = fopen(filepath, "w");
	if (!fp) {
		free(json_str);
		return -1;
	}

	fputs(json_str, fp);
	fclose(fp);
	free(json_str);

	log__printf(NULL, MOSQ_LOG_INFO, "Pulse: Discovered topics dumped to %s", filepath);
	return 0;
}
