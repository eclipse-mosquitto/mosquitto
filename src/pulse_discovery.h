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

#ifndef PULSE_DISCOVERY_H
#define PULSE_DISCOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A discovered topic that is not part of the information model.
 */
typedef struct pulse_discovered_topic {
	char *topic;                    /* MQTT topic */
	void *payload_sample;           /* Sample of last received payload */
	size_t payload_sample_len;      /* Length of payload sample */
	uint64_t message_count;         /* Number of messages received on this topic */
	time_t first_seen;              /* Timestamp of first message */
	time_t last_seen;               /* Timestamp of last message */
	struct pulse_discovered_topic *next; /* For linked list / hash table */
} pulse_discovered_topic_t;

/**
 * Process an incoming message for discovery.
 * If the topic is not governed by a tag in the information model,
 * it will be tracked as a discovered topic.
 *
 * @param topic The MQTT topic
 * @param payload The message payload (may be NULL)
 * @param payload_len Length of the payload
 * @return true if the topic is governed (has a matching tag), false if it was discovered
 */
bool pulse_discovery_process_message(const char *topic, const void *payload, size_t payload_len);

/**
 * Check if a topic is governed (has a matching tag in the information model).
 *
 * @param topic The MQTT topic to check
 * @return true if governed, false otherwise
 */
bool pulse_discovery_is_governed(const char *topic);

/**
 * Get the number of discovered (ungoverned) topics.
 */
size_t pulse_discovery_get_count(void);

/**
 * Get all discovered topics as a linked list.
 * The returned pointer is owned by the discovery module and must not be freed.
 */
const pulse_discovered_topic_t *pulse_discovery_get_all(void);

/**
 * Find a discovered topic by name.
 * Returns NULL if not found.
 */
const pulse_discovered_topic_t *pulse_discovery_find(const char *topic);

/**
 * Clear all discovered topics.
 */
void pulse_discovery_clear(void);

/**
 * Start the discovery stats logging thread.
 */
void pulse_discovery_start(void);

/**
 * Cleanup and free all discovery resources.
 */
void pulse_discovery_cleanup(void);

/**
 * Get details of a specific discovered topic.
 *
 * @param topic The topic to look up
 * @param out_count Pointer to store message count
 * @param out_first_seen Pointer to store first seen timestamp
 * @param out_last_seen Pointer to store last seen timestamp
 * @param out_payload Pointer to store payload sample (caller must free)
 * @param out_payload_len Pointer to store payload length
 * @return 0 on success, -1 if not found
 */
int pulse_discovery_get_details(const char *topic,
                                uint64_t *out_count,
                                time_t *out_first_seen,
                                time_t *out_last_seen,
                                char **out_payload,
                                size_t *out_payload_len);

/**
 * Get a page of discovered topics.
 * Caller must free the returned array with pulse_discovery_free_page().
 *
 * @param offset Starting index
 * @param limit Maximum number of topics to return
 * @param out_topics Pointer to store allocated array of topic strings
 * @param out_count Pointer to store actual count returned
 * @param out_total Pointer to store total count of discovered topics
 * @return 0 on success, non-zero on failure
 */
int pulse_discovery_get_page(size_t offset, size_t limit,
                             char ***out_topics, size_t *out_count, size_t *out_total);

/**
 * Free a page of topics returned by pulse_discovery_get_page().
 */
void pulse_discovery_free_page(char **topics, size_t count);

/**
 * Dump all discovered topics to a JSON file.
 *
 * @param filepath Path to the output JSON file
 * @return 0 on success, non-zero on failure
 */
int pulse_discovery_dump_to_json(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* PULSE_DISCOVERY_H */
