/*
Copyright (c) 2010-2021 Roger Light <roger@atchoo.org>

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

#ifndef MOSQUITTO_LIBMOSQUITTO_HELPERS_H
#define MOSQUITTO_LIBMOSQUITTO_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mosquitto/defs.h>
#include <mosquitto/mqtt_protocol.h>

/* =============================================================================
 *
 * Section: One line client helper functions
 *
 * =============================================================================
 */

struct libmosquitto_will {
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct libmosquitto_auth {
	char *username;
	char *password;
};

struct libmosquitto_tls {
	char *cafile;
	char *capath;
	char *certfile;
	char *keyfile;
	char *ciphers;
	char *tls_version;
	int (*pw_callback)(char *buf, int size, int rwflag, void *userdata);
	int cert_reqs;
};

/*
 * Function: mosquitto_subscribe_simple
 *
 * Helper function to make subscribing to a topic and retrieving some messages
 * very straightforward.
 *
 * This connects to a broker, subscribes to a topic, waits for msg_count
 * messages to be received, then returns after disconnecting cleanly.
 *
 * Parameters:
 *   messages - pointer to a "struct mosquitto_message *". The received
 *              messages will be returned here. On error, this will be set to
 *              NULL.
 *   msg_count - the number of messages to retrieve.
 *   want_retained - if set to true, stale retained messages will be treated as
 *                   normal messages with regards to msg_count. If set to
 *                   false, they will be ignored.
 *   topic - the subscription topic to use (wildcards are allowed).
 *   qos - the qos to use for the subscription.
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   clientid - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libmosquitto_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libmosquitto_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 *
 * Returns:
 *   MOSQ_ERR_SUCCESS - on success
 *   Greater than 0 - on error.
 */
libmosq_EXPORT int mosquitto_subscribe_simple(
		struct mosquitto_message **messages,
		int msg_count,
		bool want_retained,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *clientid,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls);


/*
 * Function: mosquitto_subscribe_callback
 *
 * Helper function to make subscribing to a topic and processing some messages
 * very straightforward.
 *
 * This connects to a broker, subscribes to a topic, then passes received
 * messages to a user provided callback. If the callback returns a 1, it then
 * disconnects cleanly and returns.
 *
 * Parameters:
 *   callback - a callback function in the following form:
 *              int callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
 *              Note that this is the same as the normal on_message callback,
 *              except that it returns an int.
 *   userdata - user provided pointer that will be passed to the callback.
 *   topic - the subscription topic to use (wildcards are allowed).
 *   qos - the qos to use for the subscription.
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   clientid - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libmosquitto_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libmosquitto_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 *
 * Returns:
 *   MOSQ_ERR_SUCCESS - on success
 *   Greater than 0 - on error.
 */
libmosq_EXPORT int mosquitto_subscribe_callback(
		int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *),
		void *userdata,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *clientid,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls);

/*
 * Function: mosquitto_publish_multiple
 *
 * Helper function to make publishing a number of messages very straightforward.
 *
 * This connects to a broker, sends the supplied messages, and returns after
 * disconnecting cleanly.
 *
 * Parameters:
 *   messages - an array of mosquitto_message structs
 *   message_count - the number of messages supplied
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   client_id - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libmosquitto_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libmosquitto_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 * Returns:
 *   MOSQ_ERR_SUCCESS - on success
 *   >0 - on error.
 */
libmosq_EXPORT int mosquitto_publish_multiple(
		struct mosquitto_message **messages,
		int message_count,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls);

/*
 * Function: mosquitto_publish_multiple
 *
 * Helper function to make publishing a single message very straightforward.
 *
 * This connects to a broker, sends the supplied messages, and returns after
 * disconnecting cleanly.
 *
 * Parameters:
 *   topic - the topic to post to
 *   payload - the payload
 *   payloadlen - the length, in bytes, of the payload
 *   qos - the QoS value to send with
 *   host - the broker to connect to.
 *   port - the network port the broker is listening on.
 *   client_id - the client id to use, or NULL if a random client id should be
 *               generated.
 *   keepalive - the MQTT keepalive value.
 *   clean_session - the MQTT clean session flag.
 *   username - the username string, or NULL for no username authentication.
 *   password - the password string, or NULL for an empty password.
 *   will - a libmosquitto_will struct containing will information, or NULL for
 *          no will.
 *   tls - a libmosquitto_tls struct containing TLS related parameters, or NULL
 *         for no use of TLS.
 *
 * Returns:
 *   MOSQ_ERR_SUCCESS - on success
 *   >0 - on error.
 */
libmosq_EXPORT int mosquitto_publish_single(
		const char *topic,
		const void *payload,
		int payloadlen,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls);

#ifdef __cplusplus
}
#endif

#endif
