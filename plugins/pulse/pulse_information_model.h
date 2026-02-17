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

#ifndef PULSE_INFORMATION_MODEL_H
#define PULSE_INFORMATION_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * UUID representation for Pulse entities.
 */
typedef struct pulse_uuid {
	uint64_t most_significant_bits;
	uint64_t least_significant_bits;
} pulse_uuid_t;

/**
 * Node metadata.
 */
typedef struct pulse_node_metadata {
	time_t created_timestamp;
	time_t updated_timestamp;
} pulse_node_metadata_t;

/**
 * A node in the information model graph.
 */
typedef struct pulse_node {
	pulse_uuid_t identifier;
	char *type_identifier;    /* Dynamically typed node type */
	char *name;
	char *description;        /* May be NULL */
	char *properties;         /* JSON string, may be NULL */
	char *topic_level;        /* May be NULL */
	pulse_node_metadata_t metadata;
	bool computed;
} pulse_node_t;

/**
 * Relation metadata.
 */
typedef struct pulse_relation_metadata {
	time_t created_timestamp;
	time_t updated_timestamp;
} pulse_relation_metadata_t;

/**
 * A relation (edge) in the information model graph.
 */
typedef struct pulse_relation {
	pulse_uuid_t identifier;
	char *type_identifier;    /* Dynamically typed relation type */
	pulse_uuid_t source_node_identifier;
	pulse_uuid_t destination_node_identifier;
	char *properties;         /* JSON string, may be NULL */
	pulse_relation_metadata_t metadata;
	bool computed;
} pulse_relation_t;

/**
 * Information model metadata.
 */
typedef struct pulse_information_model_metadata {
	uint64_t implementation_version;
	time_t created_timestamp;
	time_t updated_timestamp;
} pulse_information_model_metadata_t;

/**
 * A tag represents an MQTT data point with topic and type information.
 * This is a first-class object extracted from nodes with type_identifier "hivemq.node.tag".
 */
typedef struct pulse_tag {
	pulse_uuid_t identifier;
	char *name;
	char *description;              /* May be NULL */
	char *topic;                    /* MQTT topic */
	char *data_type_id;             /* e.g., "hivemq.datatypes.generic.decimal" */
	char *serialization_format_id;  /* e.g., "hivemq.serializations.generic.utf8" */

	/* Optional history/retention settings */
	bool has_history;
	uint64_t history_max_bytes;
	uint64_t history_max_amount;
} pulse_tag_t;

/**
 * The complete information model.
 */
typedef struct pulse_information_model {
	pulse_uuid_t identifier;
	char *name;
	char *description;        /* May be NULL */
	uint64_t version;
	pulse_information_model_metadata_t metadata;

	/* Graph data */
	pulse_node_t *nodes;
	size_t node_count;

	pulse_relation_t *relations;
	size_t relation_count;

	/* First-class tag objects (extracted from nodes with type "hivemq.node.tag") */
	pulse_tag_t *tags;
	size_t tag_count;
} pulse_information_model_t;

/**
 * Get the current information model.
 * Returns NULL if no information model has been loaded.
 * The returned pointer is owned by the pulse client and must not be freed.
 */
const pulse_information_model_t *pulse_get_information_model(void);

/**
 * Get the current information model version.
 * Returns 0 if no information model has been loaded.
 */
uint64_t pulse_get_information_model_version(void);

/**
 * Find a node by its identifier.
 * Returns NULL if not found.
 */
const pulse_node_t *pulse_find_node_by_id(const pulse_uuid_t *id);

/**
 * Find a node by its topic level.
 * Returns NULL if not found.
 */
const pulse_node_t *pulse_find_node_by_topic_level(const char *topic_level);

/**
 * Get the number of tags in the information model.
 * Returns 0 if no information model is loaded.
 */
size_t pulse_get_tag_count(void);

/**
 * Get all tags in the information model.
 * Returns NULL if no information model is loaded.
 * The returned pointer is owned by the pulse client and must not be freed.
 */
const pulse_tag_t *pulse_get_tags(void);

/**
 * Find a tag by its MQTT topic.
 * Returns NULL if not found.
 */
const pulse_tag_t *pulse_find_tag_by_topic(const char *topic);

/**
 * Find a tag by its identifier.
 * Returns NULL if not found.
 */
const pulse_tag_t *pulse_find_tag_by_id(const pulse_uuid_t *id);

/**
 * Get all relations where the given node is the source.
 * Caller must free the returned array (but not the relations themselves).
 * Returns NULL if none found, sets out_count to 0.
 */
const pulse_relation_t **pulse_get_outgoing_relations(const pulse_uuid_t *node_id, size_t *out_count);

/**
 * Get all relations where the given node is the destination.
 * Caller must free the returned array (but not the relations themselves).
 * Returns NULL if none found, sets out_count to 0.
 */
const pulse_relation_t **pulse_get_incoming_relations(const pulse_uuid_t *node_id, size_t *out_count);

/**
 * Compare two UUIDs for equality.
 * Returns true if equal, false otherwise.
 */
bool pulse_uuid_equals(const pulse_uuid_t *a, const pulse_uuid_t *b);

/**
 * Convert a UUID to a string representation.
 * The output buffer must be at least 37 bytes (36 chars + null terminator).
 * Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 */
void pulse_uuid_to_string(const pulse_uuid_t *uuid, char *out);

/**
 * Dump the current information model to a JSON file for debugging.
 * Creates a file with the complete information model structure.
 *
 * @param filepath Path to the output JSON file
 * @return 0 on success, non-zero on failure
 */
int pulse_dump_information_model_to_json(const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* PULSE_INFORMATION_MODEL_H */
