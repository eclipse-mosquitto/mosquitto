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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <cjson/cJSON.h>

#include "handshake/handshake_service.grpc.pb.h"
#include "handshake/initial_agent_to_server_handshake_request.pb.h"
#include "handshake/initial_server_to_agent_handshake_response.pb.h"
#include "heartbeat/heartbeat_service.grpc.pb.h"
#include "heartbeat/heartbeat_request.pb.h"
#include "heartbeat/heartbeat_response.pb.h"
#include "informationmodel/information_model_service.grpc.pb.h"
#include "informationmodel/information_model_request.pb.h"
#include "informationmodel/information_model_response.pb.h"
#include "informationmodel/information_model.pb.h"
#include "informationmodel/information_model_updated_notification.pb.h"
#include "discovery/get_ungoverned_topics_service.grpc.pb.h"
#include "discovery/get_ungoverned_topics_request.pb.h"
#include "discovery/get_ungoverned_topics_response.pb.h"
#include "discovery/get_ungoverned_datapoint_service.grpc.pb.h"
#include "discovery/get_ungoverned_data_point_request.pb.h"
#include "discovery/get_ungoverned_data_point_response.pb.h"
#include "google/protobuf/empty.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "uuid.pb.h"

#include "pulse_client.h"
#include "pulse_information_model.h"
#include "pulse_discovery.h"

/* Global state for the Pulse client */
static std::shared_ptr<grpc::Channel> g_channel;
static std::unique_ptr<hivemq::pulse::grpc::HeartbeatService::Stub> g_heartbeat_stub;
static std::unique_ptr<hivemq::pulse::grpc::InformationModelService::Stub> g_infomodel_stub;
static std::unique_ptr<hivemq::pulse::grpc::UngovernedTopicsService::Stub> g_ungoverned_stub;
static std::unique_ptr<hivemq::pulse::grpc::UngovernedDataPointService::Stub> g_datapoint_stub;
static std::string g_auth_token;
static std::thread g_heartbeat_thread;
static std::thread g_notification_thread;
static std::thread g_ungoverned_thread;
static std::thread g_datapoint_thread;
static std::atomic<bool> g_running{false};
static std::mutex g_mutex;
static std::condition_variable g_cv;
static std::unique_ptr<grpc::ClientContext> g_notification_context;
static std::unique_ptr<grpc::ClientContext> g_ungoverned_context;
static std::unique_ptr<grpc::ClientContext> g_datapoint_context;
static int g_heartbeat_interval_seconds = 30; /* Default, will be updated from handshake */
static std::atomic<int64_t> g_request_id{0};

/* Stored UUIDs for later use */
static hivemq::pulse::grpc::UuidProto g_agent_uuid;
static hivemq::pulse::grpc::UuidProto g_project_uuid;

/* Information model storage */
static pulse_information_model_t *g_information_model = nullptr;
static std::atomic<bool> g_information_model_loaded{false};
static std::mutex g_infomodel_mutex;

/* Helper function to parse UUID string into UuidProto */
static bool parse_uuid(const char *uuid_str, hivemq::pulse::grpc::UuidProto *uuid_proto)
{
	if (!uuid_str || !uuid_proto) {
		return false;
	}

	/* UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
	 * We need to convert this to most_significant_bits and least_significant_bits */
	std::string uuid(uuid_str);

	/* Remove hyphens */
	std::string hex_str;
	for (char c : uuid) {
		if (c != '-') {
			hex_str += c;
		}
	}

	if (hex_str.length() != 32) {
		return false;
	}

	/* Parse as two 64-bit hex values */
	std::string msb_str = hex_str.substr(0, 16);
	std::string lsb_str = hex_str.substr(16, 16);

	try {
		uint64_t msb = std::stoull(msb_str, nullptr, 16);
		uint64_t lsb = std::stoull(lsb_str, nullptr, 16);

		uuid_proto->set_most_significant_bits(msb);
		uuid_proto->set_least_significant_bits(lsb);
		return true;
	} catch (...) {
		return false;
	}
}

/* Convert UuidProto to pulse_uuid_t */
static pulse_uuid_t uuid_proto_to_pulse(const hivemq::pulse::grpc::UuidProto &proto)
{
	pulse_uuid_t uuid;
	uuid.most_significant_bits = proto.most_significant_bits();
	uuid.least_significant_bits = proto.least_significant_bits();
	return uuid;
}

/* Duplicate a string, returning NULL for empty strings */
static char *strdup_or_null(const std::string &str)
{
	if (str.empty()) {
		return nullptr;
	}
	return strdup(str.c_str());
}

/* Convert protobuf timestamp to time_t */
static time_t timestamp_to_time_t(const google::protobuf::Timestamp &ts)
{
	return static_cast<time_t>(ts.seconds());
}

/* Free a pulse_node_t's allocated strings */
static void free_node(pulse_node_t *node)
{
	if (node) {
		free(node->type_identifier);
		free(node->name);
		free(node->description);
		free(node->properties);
		free(node->topic_level);
	}
}

/* Free a pulse_relation_t's allocated strings */
static void free_relation(pulse_relation_t *rel)
{
	if (rel) {
		free(rel->type_identifier);
		free(rel->properties);
	}
}

/* Free a pulse_tag_t's allocated strings */
static void free_tag(pulse_tag_t *tag)
{
	if (tag) {
		free(tag->name);
		free(tag->description);
		free(tag->topic);
		free(tag->data_type_id);
		free(tag->serialization_format_id);
	}
}

/* Free the entire information model */
static void free_information_model(pulse_information_model_t *model)
{
	if (!model) return;

	for (size_t i = 0; i < model->node_count; i++) {
		free_node(&model->nodes[i]);
	}
	free(model->nodes);

	for (size_t i = 0; i < model->relation_count; i++) {
		free_relation(&model->relations[i]);
	}
	free(model->relations);

	for (size_t i = 0; i < model->tag_count; i++) {
		free_tag(&model->tags[i]);
	}
	free(model->tags);

	free(model->name);
	free(model->description);
	free(model);
}

/* Convert NodeProto to pulse_node_t */
static void convert_node(const hivemq::pulse::grpc::NodeProto &proto, pulse_node_t *node)
{
	node->identifier = uuid_proto_to_pulse(proto.identifier());
	node->type_identifier = strdup_or_null(proto.type_identifier());
	node->name = strdup_or_null(proto.name());
	node->description = strdup_or_null(proto.description());
	node->properties = strdup_or_null(proto.properties());
	node->topic_level = strdup_or_null(proto.topic_level());
	node->computed = proto.computed();

	if (proto.has_metadata()) {
		node->metadata.created_timestamp = timestamp_to_time_t(proto.metadata().created_timestamp());
		node->metadata.updated_timestamp = timestamp_to_time_t(proto.metadata().updated_timestamp());
	} else {
		node->metadata.created_timestamp = 0;
		node->metadata.updated_timestamp = 0;
	}
}

/* Convert RelationProto to pulse_relation_t */
static void convert_relation(const hivemq::pulse::grpc::RelationProto &proto, pulse_relation_t *rel)
{
	rel->identifier = uuid_proto_to_pulse(proto.identifier());
	rel->type_identifier = strdup_or_null(proto.type_identifier());
	rel->source_node_identifier = uuid_proto_to_pulse(proto.source_node_identifier());
	rel->destination_node_identifier = uuid_proto_to_pulse(proto.destination_node_identifier());
	rel->properties = strdup_or_null(proto.properties());
	rel->computed = proto.computed();

	if (proto.has_metadata()) {
		rel->metadata.created_timestamp = timestamp_to_time_t(proto.metadata().created_timestamp());
		rel->metadata.updated_timestamp = timestamp_to_time_t(proto.metadata().updated_timestamp());
	} else {
		rel->metadata.created_timestamp = 0;
		rel->metadata.updated_timestamp = 0;
	}
}

/* Fetch and store the information model */
static int fetch_information_model(void)
{
	if (!g_infomodel_stub) {
		return -1;
	}

	/* Build the request - use project_id as information_model_identifier */
	hivemq::pulse::grpc::InformationModelRequestProto request;
	*request.mutable_information_model_identifier() = g_project_uuid;
	*request.mutable_project_id() = g_project_uuid;
	*request.mutable_agent_id() = g_agent_uuid;
	request.set_information_model_current_version(0); /* First fetch */

	hivemq::pulse::grpc::InformationModelResponseOrErrorProto response;
	grpc::ClientContext context;

	/* Add Authorization header */
	if (!g_auth_token.empty()) {
		context.AddMetadata("authorization", std::string("Bearer ") + g_auth_token);
	}

	grpc::Status status = g_infomodel_stub->GetInformationModel(&context, request, &response);

	if (!status.ok()) {
	fprintf(stderr, "Pulse: GetInformationModel failed: %s (code: %d)\n",
		        status.error_message().c_str(), static_cast<int>(status.error_code()));
		return -1;
	}

	if (response.has_error()) {
	fprintf(stderr, "Pulse: GetInformationModel error from server: %s\n",
		        response.error().message().c_str());
		return -1;
	}

	if (!response.has_response()) {
	fprintf(stderr, "Pulse: GetInformationModel returned empty response\n");
		return -1;
	}

	const auto &im_proto = response.response().information_model();

	/* Allocate and populate the C structure */
	std::lock_guard<std::mutex> lock(g_infomodel_mutex);

	/* Free existing model if any */
	if (g_information_model) {
		free_information_model(g_information_model);
		g_information_model = nullptr;
	}

	g_information_model = static_cast<pulse_information_model_t *>(
		calloc(1, sizeof(pulse_information_model_t)));
	if (!g_information_model) {
		return -1;
	}

	/* Basic info */
	g_information_model->identifier = uuid_proto_to_pulse(im_proto.identifier());
	g_information_model->name = strdup_or_null(im_proto.name());
	g_information_model->description = strdup_or_null(im_proto.description());
	g_information_model->version = im_proto.version();

	if (im_proto.has_metadata()) {
		g_information_model->metadata.implementation_version = im_proto.metadata().implementation_version();
		g_information_model->metadata.created_timestamp = timestamp_to_time_t(im_proto.metadata().created_timestamp());
		g_information_model->metadata.updated_timestamp = timestamp_to_time_t(im_proto.metadata().updated_timestamp());
	}

	/* Extract nodes and relations based on graph type */
	const google::protobuf::RepeatedPtrField<hivemq::pulse::grpc::NodeProto> *nodes_ptr = nullptr;
	const google::protobuf::RepeatedPtrField<hivemq::pulse::grpc::RelationProto> *relations_ptr = nullptr;

	switch (im_proto.graph_case()) {
	case hivemq::pulse::grpc::InformationModelProto::kCompleteGraph:
		nodes_ptr = &im_proto.complete_graph().nodes();
		relations_ptr = &im_proto.complete_graph().relations();
		break;
	case hivemq::pulse::grpc::InformationModelProto::kSubGraph:
		nodes_ptr = &im_proto.sub_graph().nodes();
		relations_ptr = &im_proto.sub_graph().relations();
		break;
	case hivemq::pulse::grpc::InformationModelProto::kCompleteGraphDelta:
	case hivemq::pulse::grpc::InformationModelProto::kSubGraphDelta:
		/* Deltas are not supported for initial fetch */
	fprintf(stderr, "Pulse: Warning: Received delta graph for initial information model fetch\n");
		break;
	default:
	fprintf(stderr, "Pulse: Warning: Unknown graph type in information model\n");
		break;
	}

	/* Convert nodes */
	if (nodes_ptr && nodes_ptr->size() > 0) {
		g_information_model->node_count = static_cast<size_t>(nodes_ptr->size());
		g_information_model->nodes = static_cast<pulse_node_t *>(
			calloc(g_information_model->node_count, sizeof(pulse_node_t)));
		if (g_information_model->nodes) {
			for (int i = 0; i < nodes_ptr->size(); i++) {
				convert_node((*nodes_ptr)[i], &g_information_model->nodes[i]);
			}
		}
	}

	/* Convert relations */
	if (relations_ptr && relations_ptr->size() > 0) {
		g_information_model->relation_count = static_cast<size_t>(relations_ptr->size());
		g_information_model->relations = static_cast<pulse_relation_t *>(
			calloc(g_information_model->relation_count, sizeof(pulse_relation_t)));
		if (g_information_model->relations) {
			for (int i = 0; i < relations_ptr->size(); i++) {
				convert_relation((*relations_ptr)[i], &g_information_model->relations[i]);
			}
		}
	}

	/* Extract tags from nodes with type "hivemq.node.tag" */
	size_t tag_count = 0;
	for (size_t i = 0; i < g_information_model->node_count; i++) {
		if (g_information_model->nodes[i].type_identifier &&
		    strcmp(g_information_model->nodes[i].type_identifier, "hivemq.node.tag") == 0) {
			tag_count++;
		}
	}

	if (tag_count > 0) {
		g_information_model->tags = static_cast<pulse_tag_t *>(
			calloc(tag_count, sizeof(pulse_tag_t)));
		if (g_information_model->tags) {
			size_t tag_idx = 0;
			for (size_t i = 0; i < g_information_model->node_count && tag_idx < tag_count; i++) {
				const pulse_node_t *node = &g_information_model->nodes[i];
				if (node->type_identifier &&
				    strcmp(node->type_identifier, "hivemq.node.tag") == 0) {
					pulse_tag_t *tag = &g_information_model->tags[tag_idx++];

					/* Copy basic fields from node */
					tag->identifier = node->identifier;
					tag->name = node->name ? strdup(node->name) : nullptr;
					tag->description = node->description ? strdup(node->description) : nullptr;

					/* Parse properties JSON to extract tag-specific fields */
					if (node->properties) {
						cJSON *props = cJSON_Parse(node->properties);
						if (props) {
							cJSON *topic = cJSON_GetObjectItem(props, "topic");
							if (topic && cJSON_IsString(topic)) {
								tag->topic = strdup(topic->valuestring);
							}

							cJSON *dataTypeId = cJSON_GetObjectItem(props, "dataTypeId");
							if (dataTypeId && cJSON_IsString(dataTypeId)) {
								tag->data_type_id = strdup(dataTypeId->valuestring);
							}

							cJSON *serializationId = cJSON_GetObjectItem(props, "dataSerializationFormatId");
							if (serializationId && cJSON_IsString(serializationId)) {
								tag->serialization_format_id = strdup(serializationId->valuestring);
							}

							/* Parse history settings if present */
							cJSON *history = cJSON_GetObjectItem(props, "history");
							if (history && cJSON_IsObject(history)) {
								tag->has_history = true;
								cJSON *maxBytes = cJSON_GetObjectItem(history, "maxBytes");
								if (maxBytes && cJSON_IsNumber(maxBytes)) {
									tag->history_max_bytes = static_cast<uint64_t>(maxBytes->valuedouble);
								}
								cJSON *maxAmount = cJSON_GetObjectItem(history, "maxAmount");
								if (maxAmount && cJSON_IsNumber(maxAmount)) {
									tag->history_max_amount = static_cast<uint64_t>(maxAmount->valuedouble);
								}
							}

							cJSON_Delete(props);
						}
					}
				}
			}
			g_information_model->tag_count = tag_idx;
		}
	}

	g_information_model_loaded = true;

	fprintf(stdout, "Pulse: Information model loaded: %s (version %lu, %zu nodes, %zu relations, %zu tags)\n",
	        g_information_model->name ? g_information_model->name : "(unnamed)",
	        (unsigned long)g_information_model->version,
	        g_information_model->node_count,
	        g_information_model->relation_count,
	        g_information_model->tag_count);

	return 0;
}

/* Called after fetch to dump the model (must be called without holding the lock) */
static void dump_information_model_debug(void)
{
	const char *debug_path = "/tmp/pulse_information_model.json";
	if (pulse_dump_information_model_to_json(debug_path) == 0) {
		fprintf(stdout, "Pulse: Debug: Information model dumped to %s\n", debug_path);
	}
}

/* Send a single heartbeat */
static int send_heartbeat(void)
{
	if (!g_heartbeat_stub) {
		return -1;
	}

	hivemq::pulse::grpc::HeartbeatRequestProto request;
	request.set_request_id(++g_request_id);

	/* Use current information model version */
	uint64_t current_version = 0;
	{
		std::lock_guard<std::mutex> lock(g_infomodel_mutex);
		if (g_information_model) {
			current_version = g_information_model->version;
		}
	}
	request.set_information_model_version(current_version);

	hivemq::pulse::grpc::HeartbeatResponseOrErrorProto response;
	grpc::ClientContext context;

	/* Add Authorization header */
	if (!g_auth_token.empty()) {
		context.AddMetadata("authorization", std::string("Bearer ") + g_auth_token);
	}

	grpc::Status status = g_heartbeat_stub->Heartbeat(&context, request, &response);

	if (!status.ok()) {
	fprintf(stderr, "Pulse: Heartbeat failed: %s (code: %d)\n",
		        status.error_message().c_str(), static_cast<int>(status.error_code()));
		return -1;
	}

	if (response.has_error()) {
	fprintf(stderr, "Pulse: Heartbeat error from server: %s\n",
		        response.error().message().c_str());
		return -1;
	}

	return 0;
}

/* Background thread function for sending heartbeats */
static void heartbeat_thread_func(void)
{
	while (g_running) {
		/* Wait for the heartbeat interval or until signaled to stop */
		std::unique_lock<std::mutex> lock(g_mutex);
		if (g_cv.wait_for(lock, std::chrono::seconds(g_heartbeat_interval_seconds),
		                  []{ return !g_running.load(); })) {
			/* We were signaled to stop */
			break;
		}
		lock.unlock();

		/* Send heartbeat */
		if (g_running) {
			if (send_heartbeat() == 0) {
				fprintf(stdout, "Pulse: Heartbeat sent successfully\n");
			}
		}
	}
}

/* Background thread function for listening to information model update notifications */
static void notification_thread_func(void)
{
	while (g_running) {
		/* Create a new context for this stream attempt */
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			g_notification_context = std::make_unique<grpc::ClientContext>();
			if (!g_auth_token.empty()) {
				g_notification_context->AddMetadata("authorization", std::string("Bearer ") + g_auth_token);
			}
		}

		/* Open the notification stream */
		google::protobuf::Empty empty_request;
		auto reader = g_infomodel_stub->OpenUnidirectionalInformationModelUpdatedNotificationStream(
			g_notification_context.get(), empty_request);

		fprintf(stdout, "Pulse: Information model update notification stream opened\n");

		/* Read notifications from the stream */
		hivemq::pulse::grpc::InformationModelUpdatedNotificationProto notification;
		while (g_running && reader->Read(&notification)) {
			uint64_t new_version = notification.information_model_current_version();
			uint64_t current_version = 0;

			{
				std::lock_guard<std::mutex> lock(g_infomodel_mutex);
				if (g_information_model) {
					current_version = g_information_model->version;
				}
			}

			if (new_version != current_version) {
				fprintf(stdout, "Pulse: Information model update notification received. "
				        "New version: %lu (current: %lu)\n",
				        (unsigned long)new_version, (unsigned long)current_version);

				/* Re-fetch the information model */
				if (fetch_information_model() == 0) {
					fprintf(stdout, "Pulse: Information model updated successfully\n");
					dump_information_model_debug();
				} else {
					fprintf(stderr, "Pulse: Warning: Failed to fetch updated information model\n");
				}
			}
		}

		/* Stream ended - check status */
		grpc::Status status = reader->Finish();
		if (!g_running) {
			/* We're shutting down, no need to reconnect */
			break;
		}

		if (!status.ok()) {
			fprintf(stderr, "Pulse: Information model notification stream ended: %s (code: %d)\n",
			        status.error_message().c_str(), static_cast<int>(status.error_code()));
		} else {
			fprintf(stdout, "Pulse: Information model notification stream closed by server\n");
		}

		/* Wait before reconnecting */
		if (g_running) {
			fprintf(stdout, "Pulse: Reconnecting notification stream in 5 seconds...\n");
			std::unique_lock<std::mutex> lock(g_mutex);
			if (g_cv.wait_for(lock, std::chrono::seconds(5), []{ return !g_running.load(); })) {
				break;
			}
		}
	}
}

/* Background thread function for handling ungoverned topics requests from server */
static void ungoverned_topics_thread_func(void)
{
	while (g_running) {
		/* Create a new context for this stream attempt */
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			g_ungoverned_context = std::make_unique<grpc::ClientContext>();
			if (!g_auth_token.empty()) {
				g_ungoverned_context->AddMetadata("authorization", std::string("Bearer ") + g_auth_token);
			}
		}

		/* Open the bidirectional stream */
		auto stream = g_ungoverned_stub->OpenBidirectionalGetUngovernedTopicsStream(
			g_ungoverned_context.get());

		fprintf(stdout, "Pulse: Ungoverned topics stream opened\n");

		/* Read requests from the server and respond */
		hivemq::pulse::grpc::GetUngovernedTopicsRequestProto request;
		while (g_running && stream->Read(&request)) {
			int64_t request_id = request.request_id();
			int64_t limit = request.limit();
			int64_t offset = request.offset();

			fprintf(stdout, "Pulse: Received ungoverned topics request (id=%ld, limit=%ld, offset=%ld)\n",
			        (long)request_id, (long)limit, (long)offset);

			/* Get the requested page of discovered topics */
			char **topics = nullptr;
			size_t count = 0;
			size_t total = 0;

			hivemq::pulse::grpc::GetUngovernedTopicsResponseOrErrorProto response;
			response.set_request_id(request_id);

			if (pulse_discovery_get_page(static_cast<size_t>(offset),
			                             static_cast<size_t>(limit),
			                             &topics, &count, &total) == 0) {
				/* Build success response */
				auto *resp = response.mutable_response();
				resp->set_total_count(static_cast<int32_t>(total));

				for (size_t i = 0; i < count; i++) {
					auto *topic_proto = resp->add_results();
					topic_proto->set_topic(topics[i]);
					/* Leave suggested_data_type_id and suggested_serialization_format_id empty for now */
				}

				pulse_discovery_free_page(topics, count);

				fprintf(stdout, "Pulse: Sending ungoverned topics response (count=%zu, total=%zu)\n",
				        count, total);
			} else {
				/* Build error response */
				auto *err = response.mutable_error();
				err->set_message("Failed to retrieve discovered topics");

				fprintf(stderr, "Pulse: Failed to get discovered topics for request %ld\n",
				        (long)request_id);
			}

			/* Send the response */
			if (!stream->Write(response)) {
				fprintf(stderr, "Pulse: Failed to write ungoverned topics response\n");
				break;
			}
		}

		/* Stream ended - check status */
		stream->WritesDone();
		grpc::Status status = stream->Finish();
		if (!g_running) {
			/* We're shutting down, no need to reconnect */
			break;
		}

		if (!status.ok()) {
			fprintf(stderr, "Pulse: Ungoverned topics stream ended: %s (code: %d)\n",
			        status.error_message().c_str(), static_cast<int>(status.error_code()));
		} else {
			fprintf(stdout, "Pulse: Ungoverned topics stream closed by server\n");
		}

		/* Wait before reconnecting */
		if (g_running) {
			fprintf(stdout, "Pulse: Reconnecting ungoverned topics stream in 5 seconds...\n");
			std::unique_lock<std::mutex> lock(g_mutex);
			if (g_cv.wait_for(lock, std::chrono::seconds(5), []{ return !g_running.load(); })) {
				break;
			}
		}
	}
}

/* Background thread function for handling ungoverned data point requests from server */
static void datapoint_thread_func(void)
{
	while (g_running) {
		/* Create a new context for this stream attempt */
		{
			std::lock_guard<std::mutex> lock(g_mutex);
			g_datapoint_context = std::make_unique<grpc::ClientContext>();
			if (!g_auth_token.empty()) {
				g_datapoint_context->AddMetadata("authorization", std::string("Bearer ") + g_auth_token);
			}
		}

		/* Open the bidirectional stream */
		auto stream = g_datapoint_stub->OpenBidirectionalGetUngovernedDataPointStream(
			g_datapoint_context.get());

		fprintf(stdout, "Pulse: Ungoverned data point stream opened\n");

		/* Read requests from the server and respond */
		hivemq::pulse::grpc::GetUngovernedDataPointRequestProto request;
		while (g_running && stream->Read(&request)) {
			int64_t request_id = request.request_id();
			std::string topic = request.topic();

			fprintf(stdout, "Pulse: Received data point request (id=%ld, topic='%s')\n",
			        (long)request_id, topic.c_str());

			/* Get the data point details */
			uint64_t count = 0;
			time_t first_seen = 0;
			time_t last_seen = 0;
			char *payload = nullptr;
			size_t payload_len = 0;

			hivemq::pulse::grpc::GetUngovernedDataPointResponseOrErrorProto response;
			response.set_request_id(request_id);

			if (pulse_discovery_get_details(topic.c_str(), &count, &first_seen, &last_seen,
			                                &payload, &payload_len) == 0) {
				/* Build success response */
				auto *resp = response.mutable_response();
				auto *dp = resp->mutable_ungoverned_data_point();

				dp->set_topic(topic);
				dp->set_count(static_cast<int32_t>(count));

				/* Set timestamps */
				auto *first_ts = dp->mutable_first_seen();
				first_ts->set_seconds(first_seen);
				first_ts->set_nanos(0);

				auto *last_ts = dp->mutable_last_seen();
				last_ts->set_seconds(last_seen);
				last_ts->set_nanos(0);

				/* Add data type suggestion with the payload sample */
				if (payload && payload_len > 0) {
					auto *suggestion = dp->add_data_type_suggestions();
					suggestion->set_data_type("unknown");
					suggestion->set_format("raw");
					suggestion->set_data(payload, payload_len);
					free(payload);
				}

				fprintf(stdout, "Pulse: Sending data point response (topic='%s', count=%lu)\n",
				        topic.c_str(), (unsigned long)count);
			} else {
				/* Build error response */
				auto *err = response.mutable_error();
				err->set_message("Topic not found in discovered topics");

				fprintf(stderr, "Pulse: Data point not found for topic '%s' (request %ld)\n",
				        topic.c_str(), (long)request_id);
			}

			/* Send the response */
			if (!stream->Write(response)) {
				fprintf(stderr, "Pulse: Failed to write data point response\n");
				break;
			}
		}

		/* Stream ended - check status */
		stream->WritesDone();
		grpc::Status status = stream->Finish();
		if (!g_running) {
			/* We're shutting down, no need to reconnect */
			break;
		}

		if (!status.ok()) {
			fprintf(stderr, "Pulse: Data point stream ended: %s (code: %d)\n",
			        status.error_message().c_str(), static_cast<int>(status.error_code()));
		} else {
			fprintf(stdout, "Pulse: Data point stream closed by server\n");
		}

		/* Wait before reconnecting */
		if (g_running) {
			fprintf(stdout, "Pulse: Reconnecting data point stream in 5 seconds...\n");
			std::unique_lock<std::mutex> lock(g_mutex);
			if (g_cv.wait_for(lock, std::chrono::seconds(5), []{ return !g_running.load(); })) {
				break;
			}
		}
	}
}

extern "C" int pulse_handshake(const char *server_address, const char *agent_id, const char *project_id, const char *auth_token)
{
	if (!server_address || !agent_id || !project_id) {
		return -1;
	}

	/* Store auth token for later use */
	if (auth_token) {
		g_auth_token = auth_token;
	}

	/* Create channel to the gRPC server */
	g_channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
	auto handshake_stub = hivemq::pulse::grpc::HandshakeService::NewStub(g_channel);

	/* Build the handshake request */
	hivemq::pulse::grpc::InitialAgentToServerHandshakeRequestProto request;
	request.set_agent_version(PULSE_AGENT_VERSION);
	request.set_application_version(PULSE_APPLICATION_VERSION);

	/* Set agent UUID and store for later use */
	auto *agent_uuid = request.mutable_agent_uuid();
	if (!parse_uuid(agent_id, agent_uuid)) {
	fprintf(stderr, "Pulse: Error: Invalid agent-id UUID format: %s\n", agent_id);
		return -1;
	}
	g_agent_uuid = *agent_uuid;

	/* Set project UUID and store for later use */
	auto *project_uuid = request.mutable_project_uuid();
	if (!parse_uuid(project_id, project_uuid)) {
	fprintf(stderr, "Pulse: Error: Invalid project-id UUID format: %s\n", project_id);
		return -1;
	}
	g_project_uuid = *project_uuid;

	/* Perform the handshake RPC */
	hivemq::pulse::grpc::InitialServerToAgentHandshakeResponseProto response;
	grpc::ClientContext context;

	/* Add Authorization header if auth token is provided */
	if (auth_token) {
		std::string bearer_token = std::string("Bearer ") + auth_token;
		context.AddMetadata("authorization", bearer_token);
	}

	grpc::Status status = handshake_stub->Handshake(&context, request, &response);

	if (!status.ok()) {
	fprintf(stderr, "Pulse: Handshake failed: %s (code: %d)\n",
		        status.error_message().c_str(), static_cast<int>(status.error_code()));
		return -1;
	}

	/* Check response status */
	if (response.status_code() != hivemq::pulse::grpc::InitialServerToAgentHandshakeResponseProto::SUCCESS) {
	fprintf(stderr, "Pulse: Handshake rejected by server: %s\n",
		        response.has_failed_message() ? response.failed_message().c_str() : "unknown reason");
		return -1;
	}

	fprintf(stdout, "Pulse: Handshake successful. Server version: %s\n",
	        response.server_version().c_str());

	if (!response.latest_agent_version().empty()) {
		if (response.latest_agent_version() != PULSE_AGENT_VERSION) {
			fprintf(stdout, "Pulse: Note: A newer agent version is available: %s\n",
			        response.latest_agent_version().c_str());
		}
	}

	/* Extract heartbeat interval from server_information */
	if (response.has_server_information()) {
		int interval = response.server_information().heartbeat_interval_seconds();
		if (interval > 0) {
			g_heartbeat_interval_seconds = interval;
			fprintf(stdout, "Pulse: Heartbeat interval: %d seconds\n", g_heartbeat_interval_seconds);
		}
	}

	/* Create service stubs */
	g_heartbeat_stub = hivemq::pulse::grpc::HeartbeatService::NewStub(g_channel);
	g_infomodel_stub = hivemq::pulse::grpc::InformationModelService::NewStub(g_channel);
	g_ungoverned_stub = hivemq::pulse::grpc::UngovernedTopicsService::NewStub(g_channel);
	g_datapoint_stub = hivemq::pulse::grpc::UngovernedDataPointService::NewStub(g_channel);

	/* Fetch the initial information model */
	if (fetch_information_model() != 0) {
		fprintf(stderr, "Pulse: Warning: Failed to fetch initial information model\n");
		/* Continue anyway - heartbeats will still work */
	} else {
		/* Dump for debugging */
		dump_information_model_debug();
	}

	/* Start background threads */
	g_running = true;
	g_heartbeat_thread = std::thread(heartbeat_thread_func);

	/* Start notification stream listener if information model was loaded */
	if (g_information_model_loaded) {
		g_notification_thread = std::thread(notification_thread_func);
	}

	/* Start ungoverned topics stream */
	g_ungoverned_thread = std::thread(ungoverned_topics_thread_func);

	/* Start data point stream */
	g_datapoint_thread = std::thread(datapoint_thread_func);

	return 0;
}

extern "C" void pulse_cleanup(void)
{
	/* Signal threads to stop */
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		g_running = false;
		/* Cancel streams if active */
		if (g_notification_context) {
			g_notification_context->TryCancel();
		}
		if (g_ungoverned_context) {
			g_ungoverned_context->TryCancel();
		}
		if (g_datapoint_context) {
			g_datapoint_context->TryCancel();
		}
	}
	g_cv.notify_all();

	/* Wait for heartbeat thread to finish */
	if (g_heartbeat_thread.joinable()) {
		g_heartbeat_thread.join();
	}

	/* Wait for notification thread to finish */
	if (g_notification_thread.joinable()) {
		g_notification_thread.join();
	}

	/* Wait for ungoverned topics thread to finish */
	if (g_ungoverned_thread.joinable()) {
		g_ungoverned_thread.join();
	}

	/* Wait for data point thread to finish */
	if (g_datapoint_thread.joinable()) {
		g_datapoint_thread.join();
	}

	/* Free information model */
	{
		std::lock_guard<std::mutex> lock(g_infomodel_mutex);
		if (g_information_model) {
			free_information_model(g_information_model);
			g_information_model = nullptr;
		}
		g_information_model_loaded = false;
	}

	/* Cleanup context and stubs */
	g_notification_context.reset();
	g_ungoverned_context.reset();
	g_datapoint_context.reset();
	g_datapoint_stub.reset();
	g_ungoverned_stub.reset();
	g_infomodel_stub.reset();
	g_heartbeat_stub.reset();
	g_channel.reset();
	g_auth_token.clear();
}

/* ========== C Accessor Functions for Information Model ========== */

extern "C" const pulse_information_model_t *pulse_get_information_model(void)
{
	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	return g_information_model;
}

extern "C" uint64_t pulse_get_information_model_version(void)
{
	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (g_information_model) {
		return g_information_model->version;
	}
	return 0;
}

extern "C" const pulse_node_t *pulse_find_node_by_id(const pulse_uuid_t *id)
{
	if (!id) return nullptr;

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return nullptr;

	for (size_t i = 0; i < g_information_model->node_count; i++) {
		if (pulse_uuid_equals(&g_information_model->nodes[i].identifier, id)) {
			return &g_information_model->nodes[i];
		}
	}
	return nullptr;
}

extern "C" const pulse_node_t *pulse_find_node_by_topic_level(const char *topic_level)
{
	if (!topic_level) return nullptr;

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return nullptr;

	for (size_t i = 0; i < g_information_model->node_count; i++) {
		if (g_information_model->nodes[i].topic_level &&
		    strcmp(g_information_model->nodes[i].topic_level, topic_level) == 0) {
			return &g_information_model->nodes[i];
		}
	}
	return nullptr;
}

extern "C" size_t pulse_get_tag_count(void)
{
	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return 0;
	return g_information_model->tag_count;
}

extern "C" const pulse_tag_t *pulse_get_tags(void)
{
	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return nullptr;
	return g_information_model->tags;
}

extern "C" const pulse_tag_t *pulse_find_tag_by_topic(const char *topic)
{
	if (!topic) return nullptr;

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return nullptr;

	for (size_t i = 0; i < g_information_model->tag_count; i++) {
		if (g_information_model->tags[i].topic &&
		    strcmp(g_information_model->tags[i].topic, topic) == 0) {
			return &g_information_model->tags[i];
		}
	}
	return nullptr;
}

extern "C" const pulse_tag_t *pulse_find_tag_by_id(const pulse_uuid_t *id)
{
	if (!id) return nullptr;

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) return nullptr;

	for (size_t i = 0; i < g_information_model->tag_count; i++) {
		if (pulse_uuid_equals(&g_information_model->tags[i].identifier, id)) {
			return &g_information_model->tags[i];
		}
	}
	return nullptr;
}

extern "C" const pulse_relation_t **pulse_get_outgoing_relations(const pulse_uuid_t *node_id, size_t *out_count)
{
	if (!node_id || !out_count) {
		if (out_count) *out_count = 0;
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) {
		*out_count = 0;
		return nullptr;
	}

	/* Count matching relations */
	size_t count = 0;
	for (size_t i = 0; i < g_information_model->relation_count; i++) {
		if (pulse_uuid_equals(&g_information_model->relations[i].source_node_identifier, node_id)) {
			count++;
		}
	}

	if (count == 0) {
		*out_count = 0;
		return nullptr;
	}

	/* Allocate and fill result array */
	const pulse_relation_t **result = static_cast<const pulse_relation_t **>(
		malloc(count * sizeof(pulse_relation_t *)));
	if (!result) {
		*out_count = 0;
		return nullptr;
	}

	size_t idx = 0;
	for (size_t i = 0; i < g_information_model->relation_count && idx < count; i++) {
		if (pulse_uuid_equals(&g_information_model->relations[i].source_node_identifier, node_id)) {
			result[idx++] = &g_information_model->relations[i];
		}
	}

	*out_count = count;
	return result;
}

extern "C" const pulse_relation_t **pulse_get_incoming_relations(const pulse_uuid_t *node_id, size_t *out_count)
{
	if (!node_id || !out_count) {
		if (out_count) *out_count = 0;
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) {
		*out_count = 0;
		return nullptr;
	}

	/* Count matching relations */
	size_t count = 0;
	for (size_t i = 0; i < g_information_model->relation_count; i++) {
		if (pulse_uuid_equals(&g_information_model->relations[i].destination_node_identifier, node_id)) {
			count++;
		}
	}

	if (count == 0) {
		*out_count = 0;
		return nullptr;
	}

	/* Allocate and fill result array */
	const pulse_relation_t **result = static_cast<const pulse_relation_t **>(
		malloc(count * sizeof(pulse_relation_t *)));
	if (!result) {
		*out_count = 0;
		return nullptr;
	}

	size_t idx = 0;
	for (size_t i = 0; i < g_information_model->relation_count && idx < count; i++) {
		if (pulse_uuid_equals(&g_information_model->relations[i].destination_node_identifier, node_id)) {
			result[idx++] = &g_information_model->relations[i];
		}
	}

	*out_count = count;
	return result;
}

extern "C" bool pulse_uuid_equals(const pulse_uuid_t *a, const pulse_uuid_t *b)
{
	if (!a || !b) return false;
	return a->most_significant_bits == b->most_significant_bits &&
	       a->least_significant_bits == b->least_significant_bits;
}

extern "C" void pulse_uuid_to_string(const pulse_uuid_t *uuid, char *out)
{
	if (!uuid || !out) return;

	/* Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
	snprintf(out, 37, "%08x-%04x-%04x-%04x-%012llx",
	         static_cast<uint32_t>(uuid->most_significant_bits >> 32),
	         static_cast<uint16_t>((uuid->most_significant_bits >> 16) & 0xFFFF),
	         static_cast<uint16_t>(uuid->most_significant_bits & 0xFFFF),
	         static_cast<uint16_t>(uuid->least_significant_bits >> 48),
	         static_cast<unsigned long long>(uuid->least_significant_bits & 0xFFFFFFFFFFFFULL));
}

/* Helper to create a cJSON UUID object */
static cJSON *uuid_to_json(const pulse_uuid_t *uuid)
{
	char uuid_str[37];
	pulse_uuid_to_string(uuid, uuid_str);
	return cJSON_CreateString(uuid_str);
}

/* Helper to create a cJSON node object */
static cJSON *node_to_json(const pulse_node_t *node)
{
	cJSON *obj = cJSON_CreateObject();
	if (!obj) return nullptr;

	cJSON_AddItemToObject(obj, "identifier", uuid_to_json(&node->identifier));
	if (node->type_identifier) cJSON_AddStringToObject(obj, "type_identifier", node->type_identifier);
	if (node->name) cJSON_AddStringToObject(obj, "name", node->name);
	if (node->description) cJSON_AddStringToObject(obj, "description", node->description);
	if (node->topic_level) cJSON_AddStringToObject(obj, "topic_level", node->topic_level);
	cJSON_AddBoolToObject(obj, "computed", node->computed);

	/* Properties as parsed JSON if valid, otherwise as string */
	if (node->properties) {
		cJSON *props = cJSON_Parse(node->properties);
		if (props) {
			cJSON_AddItemToObject(obj, "properties", props);
		} else {
			cJSON_AddStringToObject(obj, "properties", node->properties);
		}
	}

	/* Metadata */
	cJSON *metadata = cJSON_CreateObject();
	if (metadata) {
		cJSON_AddNumberToObject(metadata, "created_timestamp", static_cast<double>(node->metadata.created_timestamp));
		cJSON_AddNumberToObject(metadata, "updated_timestamp", static_cast<double>(node->metadata.updated_timestamp));
		cJSON_AddItemToObject(obj, "metadata", metadata);
	}

	return obj;
}

/* Helper to create a cJSON relation object */
static cJSON *relation_to_json(const pulse_relation_t *rel)
{
	cJSON *obj = cJSON_CreateObject();
	if (!obj) return nullptr;

	cJSON_AddItemToObject(obj, "identifier", uuid_to_json(&rel->identifier));
	if (rel->type_identifier) cJSON_AddStringToObject(obj, "type_identifier", rel->type_identifier);
	cJSON_AddItemToObject(obj, "source_node_identifier", uuid_to_json(&rel->source_node_identifier));
	cJSON_AddItemToObject(obj, "destination_node_identifier", uuid_to_json(&rel->destination_node_identifier));
	cJSON_AddBoolToObject(obj, "computed", rel->computed);

	/* Properties as parsed JSON if valid, otherwise as string */
	if (rel->properties) {
		cJSON *props = cJSON_Parse(rel->properties);
		if (props) {
			cJSON_AddItemToObject(obj, "properties", props);
		} else {
			cJSON_AddStringToObject(obj, "properties", rel->properties);
		}
	}

	/* Metadata */
	cJSON *metadata = cJSON_CreateObject();
	if (metadata) {
		cJSON_AddNumberToObject(metadata, "created_timestamp", static_cast<double>(rel->metadata.created_timestamp));
		cJSON_AddNumberToObject(metadata, "updated_timestamp", static_cast<double>(rel->metadata.updated_timestamp));
		cJSON_AddItemToObject(obj, "metadata", metadata);
	}

	return obj;
}

/* Helper to create a cJSON tag object */
static cJSON *tag_to_json(const pulse_tag_t *tag)
{
	cJSON *obj = cJSON_CreateObject();
	if (!obj) return nullptr;

	cJSON_AddItemToObject(obj, "identifier", uuid_to_json(&tag->identifier));
	if (tag->name) cJSON_AddStringToObject(obj, "name", tag->name);
	if (tag->description) cJSON_AddStringToObject(obj, "description", tag->description);
	if (tag->topic) cJSON_AddStringToObject(obj, "topic", tag->topic);
	if (tag->data_type_id) cJSON_AddStringToObject(obj, "data_type_id", tag->data_type_id);
	if (tag->serialization_format_id) cJSON_AddStringToObject(obj, "serialization_format_id", tag->serialization_format_id);

	if (tag->has_history) {
		cJSON *history = cJSON_CreateObject();
		if (history) {
			cJSON_AddNumberToObject(history, "max_bytes", static_cast<double>(tag->history_max_bytes));
			cJSON_AddNumberToObject(history, "max_amount", static_cast<double>(tag->history_max_amount));
			cJSON_AddItemToObject(obj, "history", history);
		}
	}

	return obj;
}

extern "C" int pulse_dump_information_model_to_json(const char *filepath)
{
	if (!filepath) return -1;

	std::lock_guard<std::mutex> lock(g_infomodel_mutex);
	if (!g_information_model) {
		fprintf(stderr, "Pulse: No information model loaded to dump\n");
		return -1;
	}

	/* Create root JSON object */
	cJSON *root = cJSON_CreateObject();
	if (!root) return -1;

	/* Basic info */
	cJSON_AddItemToObject(root, "identifier", uuid_to_json(&g_information_model->identifier));
	if (g_information_model->name) cJSON_AddStringToObject(root, "name", g_information_model->name);
	if (g_information_model->description) cJSON_AddStringToObject(root, "description", g_information_model->description);
	cJSON_AddNumberToObject(root, "version", static_cast<double>(g_information_model->version));

	/* Metadata */
	cJSON *metadata = cJSON_CreateObject();
	if (metadata) {
		cJSON_AddNumberToObject(metadata, "implementation_version",
		                        static_cast<double>(g_information_model->metadata.implementation_version));
		cJSON_AddNumberToObject(metadata, "created_timestamp",
		                        static_cast<double>(g_information_model->metadata.created_timestamp));
		cJSON_AddNumberToObject(metadata, "updated_timestamp",
		                        static_cast<double>(g_information_model->metadata.updated_timestamp));
		cJSON_AddItemToObject(root, "metadata", metadata);
	}

	/* Nodes array */
	cJSON *nodes = cJSON_CreateArray();
	if (nodes) {
		for (size_t i = 0; i < g_information_model->node_count; i++) {
			cJSON *node_json = node_to_json(&g_information_model->nodes[i]);
			if (node_json) {
				cJSON_AddItemToArray(nodes, node_json);
			}
		}
		cJSON_AddItemToObject(root, "nodes", nodes);
	}

	/* Relations array */
	cJSON *relations = cJSON_CreateArray();
	if (relations) {
		for (size_t i = 0; i < g_information_model->relation_count; i++) {
			cJSON *rel_json = relation_to_json(&g_information_model->relations[i]);
			if (rel_json) {
				cJSON_AddItemToArray(relations, rel_json);
			}
		}
		cJSON_AddItemToObject(root, "relations", relations);
	}

	/* Tags array */
	cJSON *tags_json = cJSON_CreateArray();
	if (tags_json) {
		for (size_t i = 0; i < g_information_model->tag_count; i++) {
			cJSON *tag_json = tag_to_json(&g_information_model->tags[i]);
			if (tag_json) {
				cJSON_AddItemToArray(tags_json, tag_json);
			}
		}
		cJSON_AddItemToObject(root, "tags", tags_json);
	}

	/* Write to file */
	char *json_str = cJSON_Print(root);
	cJSON_Delete(root);

	if (!json_str) {
		fprintf(stderr, "Pulse: Failed to serialize information model to JSON\n");
		return -1;
	}

	FILE *fp = fopen(filepath, "w");
	if (!fp) {
		fprintf(stderr, "Pulse: Failed to open file for writing: %s\n", filepath);
		free(json_str);
		return -1;
	}

	fputs(json_str, fp);
	fclose(fp);
	free(json_str);

	fprintf(stdout, "Pulse: Information model dumped to %s\n", filepath);
	return 0;
}
