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

#include "config.h"

#include <errno.h>
#include <string.h>

#ifndef WIN32
#  include <strings.h>
#endif

#include "mosquitto.h"
#include "mosquitto/mqtt_protocol.h"

const char *mosquitto_strerror(int mosq_errno)
{
	switch(mosq_errno){
		case MOSQ_ERR_QUOTA_EXCEEDED:
			return "Quota exceeded";
		case MOSQ_ERR_AUTH_DELAYED:
			return "Authentication delayed";
		case MOSQ_ERR_AUTH_CONTINUE:
			return "Continue with authentication";
		case MOSQ_ERR_NO_SUBSCRIBERS:
			return "No subscribers";
		case MOSQ_ERR_SUB_EXISTS:
			return "Subscription already exists";
		case MOSQ_ERR_CONN_PENDING:
			return "Connection pending";
		case MOSQ_ERR_SUCCESS:
			return "No error";
		case MOSQ_ERR_NOMEM:
			return "Out of memory";
		case MOSQ_ERR_PROTOCOL:
			return "A network protocol error occurred when communicating with the broker";
		case MOSQ_ERR_INVAL:
			return "Invalid input";
		case MOSQ_ERR_NO_CONN:
			return "The client is not currently connected";
		case MOSQ_ERR_CONN_REFUSED:
			return "The connection was refused";
		case MOSQ_ERR_NOT_FOUND:
			return "Message not found (internal error)";
		case MOSQ_ERR_CONN_LOST:
			return "The connection was lost";
		case MOSQ_ERR_TLS:
			return "A TLS error occurred";
		case MOSQ_ERR_PAYLOAD_SIZE:
			return "Payload too large";
		case MOSQ_ERR_NOT_SUPPORTED:
			return "This feature is not supported";
		case MOSQ_ERR_AUTH:
			return "Authorisation failed";
		case MOSQ_ERR_ACL_DENIED:
			return "Access denied by ACL";
		case MOSQ_ERR_UNKNOWN:
			return "Unknown error";
		case MOSQ_ERR_ERRNO:
			return strerror(errno);
		case MOSQ_ERR_EAI:
			return "Lookup error";
		case MOSQ_ERR_PROXY:
			return "Proxy error";
		case MOSQ_ERR_PLUGIN_DEFER:
			return "Plugin deferring result";
		case MOSQ_ERR_MALFORMED_UTF8:
			return "Malformed UTF-8";
		case MOSQ_ERR_KEEPALIVE:
			return "Keepalive exceeded";
		case MOSQ_ERR_LOOKUP:
			return "DNS Lookup failed";
		case MOSQ_ERR_MALFORMED_PACKET:
			return "Malformed packet";
		case MOSQ_ERR_DUPLICATE_PROPERTY:
			return "Duplicate property in property list";
		case MOSQ_ERR_TLS_HANDSHAKE:
			return "TLS handshake failed";
		case MOSQ_ERR_QOS_NOT_SUPPORTED:
			return "Requested QoS not supported on server";
		case MOSQ_ERR_OVERSIZE_PACKET:
			return "Packet larger than supported by the server";
		case MOSQ_ERR_OCSP:
			return "OCSP error";
		case MOSQ_ERR_TIMEOUT:
			return "Timeout";
		case MOSQ_ERR_ALREADY_EXISTS:
			return "Entry already exists";
		case MOSQ_ERR_PLUGIN_IGNORE:
			return "Ignore plugin";
		case MOSQ_ERR_HTTP_BAD_ORIGIN:
			return "Bad http origin";

		case MOSQ_ERR_UNSPECIFIED:
			return "Unspecified error";
		case MOSQ_ERR_IMPLEMENTATION_SPECIFIC:
			return "Implementaion specific error";
		case MOSQ_ERR_CLIENT_IDENTIFIER_NOT_VALID:
			return "Client identifier not valid";
		case MOSQ_ERR_BAD_USERNAME_OR_PASSWORD:
			return "Bad username or password";
		case MOSQ_ERR_SERVER_UNAVAILABLE:
			return "Server unavailable";
		case MOSQ_ERR_SERVER_BUSY:
			return "Server busy";
		case MOSQ_ERR_BANNED:
			return "Banned";
		case MOSQ_ERR_BAD_AUTHENTICATION_METHOD:
			return "Bad authentication method";
		case MOSQ_ERR_SESSION_TAKEN_OVER:
			return "Session taken over";
		case MOSQ_ERR_RECEIVE_MAXIMUM_EXCEEDED:
			return "Receive maximum exceeded";
		case MOSQ_ERR_TOPIC_ALIAS_INVALID:
			return "Topic alias invalid";
		case MOSQ_ERR_ADMINISTRATIVE_ACTION:
			return "Administrative action";
		case MOSQ_ERR_RETAIN_NOT_SUPPORTED:
			return "Retain not supported";
		case MOSQ_ERR_CONNECTION_RATE_EXCEEDED:
			return "Connection rate exceeded";
		default:
			if(mosq_errno >= 128) {
				// If mosq_errno is greater than 127,
				// a mqtt5_return_code error was used
				return mosquitto_reason_string(mosq_errno);
			} else {
				return "Unknown error";
			}
	}
}

const char *mosquitto_connack_string(int connack_code)
{
	switch(connack_code){
		case 0:
			return "Connection Accepted";
		case 1:
			return "Connection Refused: unacceptable protocol version";
		case 2:
			return "Connection Refused: identifier rejected";
		case 3:
			return "Connection Refused: broker unavailable";
		case 4:
			return "Connection Refused: bad user name or password";
		case 5:
			return "Connection Refused: not authorised";
		case 128:
			// The Server does not wish to reveal the reason for the failure, or none of the other Reason Codes apply.
			return "Connection Refused: Unspecified error";
		case 129:
			// Data within the CONNECT packet could not be correctly parsed.
			return "Connection Refused: Malformed Packet";
		case 130:
			// Data in the CONNECT packet does not conform to this specification.
			return "Connection Refused: Protocol Error";
		case 131:
			// The CONNECT is valid but is not accepted by this Server.
			return "Connection Refused: Implementation specific error";
		case 132:
			// The Server does not support the version of the MQTT protocol requested by the Client.
			return "Connection Refused: Unsupported Protocol Version";
		case 133:
			// The Client Identifier is a valid string but is not allowed by the Server.
			return "Connection Refused: Client Identifier not valid";
		case 134:
			// The Server does not accept the User Name or Password specified by the Client
			return "Connection Refused: Bad User Name or Password";
		case 135:
			// The Client is not authorized to connect.
			return "Connection Refused: Not authorized";
		case 136:
			// The MQTT Server is not available.
			return "Connection Refused: Server unavailable";
		case 137:
			// The Server is busy. Try again later.
			return "Connection Refused: Server busy";
		case 138:
			// This Client has been banned by administrative action. Contact the server administrator.
			return "Connection Refused: Banned";
		case 140:
			// The authentication method is not supported or does not match the authentication method currently in use.
			return "Connection Refused: Bad authentication method";
		case 144:
			// The Will Topic Name is not malformed, but is not accepted by this Server.
			return "Connection Refused: Topic Name invalid";
		case 149:
			// The CONNECT packet exceeded the maximum permissible size.
			return "Connection Refused: Packet too large";
		case 151:
			// An implementation or administrative imposed limit has been exceeded.
			return "Connection Refused: Quota exceeded";
		case 153:
			// The Will Payload does not match the specified Payload Format Indicator.
			return "Connection Refused: Payload format invalid";
		case 154:
			// The Server does not support retained messages, and Will Retain was set to 1.
			return "Connection Refused: Retain not supported";
		case 155:
			// The Server does not support the QoS set in Will QoS.
			return "Connection Refused: QoS not supported";
		case 156:
			// The Client should temporarily use another server.
			return "Connection Refused: Use another server";
		case 157:
			// The Client should permanently use another server.
			return "Connection Refused: Server moved";
		case 159:
			// The connection rate limit has been exceeded.
			return "Connection Refused: Connection rate exceeded";
		default:
			return "Connection Refused: unknown reason";
	}
}

const char *mosquitto_disconnect_string(int disconnect_code) {
	switch (disconnect_code) {
		case 0:
			// Close the connection normally. Do not send the Will Message.
			return "Disconnected: Normal disconnection";
		case 4:
			// The Client wishes to disconnect but requires that the Server also publishes its Will Message.
			return "Disconnected: Disconnect with Will Message";
		case 128:
			// The Connection is closed but the sender either does not wish to reveal the reason, or none of the other Reason Codes apply.
			return "Disconnected: Unspecified error";
		case 129:
			// The received packet does not conform to this specification.
			return "Disconnected: Malformed Packet";
		case 130:
			// An unexpected or out of order packet was received.
			return "Disconnected: Protocol Error";
		case 131:
			// The packet received is valid but cannot be processed by this implementation.
			return "Disconnected: Implementation specific error";
		case 135:
			// The request is not authorized.
			return "Disconnected: Not authorized";
		case 137:
			// The Server is busy and cannot continue processing requests from this Client.
			return "Disconnected: Server busy";
		case 139:
			// The Server is shutting down.
			return "Disconnected: Server shutting down";
		case 141:
			// The Connection is closed because no packet has been received for 1.5 times the Keepalive time.
			return "Disconnected: Keep Alive timeout";
		case 142:
			// Another Connection using the same ClientID has connected causing this Connection to be closed.
			return "Disconnected: Session taken over";
		case 143:
			// The Topic Filter is correctly formed, but is not accepted by this Sever.
			return "Disconnected: Topic Filter invalid";
		case 144:
			// The Topic Name is correctly formed, but is not accepted by this Client or Server.
			return "Disconnected: Topic Name invalid";
		case 147:
			// The Client or Server has received more than Receive Maximum publication for which it has not sent PUBACK or PUBCOMP.
			return "Disconnected: Receive Maximum exceeded";
		case 148:
			// The Client or Server has received a PUBLISH packet containing a Topic Alias which is greater than the Maximum Topic Alias it sent in the CONNECT or CONNACK packet.
			return "Disconnected: Topic Alias invalid";
		case 149:
			// The packet size is greater than Maximum Packet Size for this Client or Server.
			return "Disconnected: Packet too large";
		case 150:
			// The received data rate is too high.
			return "Disconnected: Message rate too high";
		case 151:
			// An implementation or administrative imposed limit has been exceeded.
			return "Disconnected: Quota exceeded";
		case 152:
			// The Connection is closed due to an administrative action.
			return "Disconnected: Administrative action";
		case 153:
			// The payload format does not match the one specified by the Payload Format Indicator.
			return "Disconnected: Payload format invalid";
		case 154:
			// The Server has does not support retained messages.
			return "Disconnected: Retain not supported";
		case 155:
			// The Client specified a QoS greater than the QoS specified in a Maximum QoS in the CONNACK.
			return "Disconnected: QoS not supported";
		case 156:
			// The Client should temporarily change its Server.
			return "Disconnected: Use another server";
		case 157:
			// The Server is moved and the Client should permanently change its server location.
			return "Disconnected: Server moved";
		case 158:
			// The Server does not support Shared Subscriptions.
			return "Disconnected: Shared Subscriptions not supported";
		case 159:
			// This connection is closed because the connection rate is too high.
			return "Disconnected: Connection rate exceeded";
		case 160:
			// The maximum connection time authorized for this connection has been exceeded.
			return "Disconnected: Maximum connect time exceeded";
		case 161:
			// The Server does not support Subscription Identifiers; the subscription is not accepted.
			return "Disconnected: Subscription Identifiers not supported";
		case 162:
			// The Server does not support Wildcard Subscriptions; the subscription is not accepted.
			return "Disconnected: Wildcard Subscriptions not supported";
		default:
			// Unknown reason
			return "Disconnected: unknown reason";
	}
}

const char *mosquitto_reason_string(int reason_code)
{
	switch(reason_code){
		case MQTT_RC_SUCCESS:
			return "Success";
		case MQTT_RC_GRANTED_QOS1:
			return "Granted QoS 1";
		case MQTT_RC_GRANTED_QOS2:
			return "Granted QoS 2";
		case MQTT_RC_DISCONNECT_WITH_WILL_MSG:
			return "Disconnect with Will Message";
		case MQTT_RC_NO_MATCHING_SUBSCRIBERS:
			return "No matching subscribers";
		case MQTT_RC_NO_SUBSCRIPTION_EXISTED:
			return "No subscription existed";
		case MQTT_RC_CONTINUE_AUTHENTICATION:
			return "Continue authentication";
		case MQTT_RC_REAUTHENTICATE:
			return "Re-authenticate";

		case MQTT_RC_UNSPECIFIED:
			return "Unspecified error";
		case MQTT_RC_MALFORMED_PACKET:
			return "Malformed Packet";
		case MQTT_RC_PROTOCOL_ERROR:
			return "Protocol Error";
		case MQTT_RC_IMPLEMENTATION_SPECIFIC:
			return "Implementation specific error";
		case MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION:
			return "Unsupported Protocol Version";
		case MQTT_RC_CLIENTID_NOT_VALID:
			return "Client Identifier not valid";
		case MQTT_RC_BAD_USERNAME_OR_PASSWORD:
			return "Bad User Name or Password";
		case MQTT_RC_NOT_AUTHORIZED:
			return "Not authorized";
		case MQTT_RC_SERVER_UNAVAILABLE:
			return "Server unavailable";
		case MQTT_RC_SERVER_BUSY:
			return "Server busy";
		case MQTT_RC_BANNED:
			return "Banned";
		case MQTT_RC_SERVER_SHUTTING_DOWN:
			return "Server shutting down";
		case MQTT_RC_BAD_AUTHENTICATION_METHOD:
			return "Bad authentication method";
		case MQTT_RC_KEEP_ALIVE_TIMEOUT:
			return "Keep Alive timeout";
		case MQTT_RC_SESSION_TAKEN_OVER:
			return "Session taken over";
		case MQTT_RC_TOPIC_FILTER_INVALID:
			return "Topic Filter invalid";
		case MQTT_RC_TOPIC_NAME_INVALID:
			return "Topic Name invalid";
		case MQTT_RC_PACKET_ID_IN_USE:
			return "Packet Identifier in use";
		case MQTT_RC_PACKET_ID_NOT_FOUND:
			return "Packet Identifier not found";
		case MQTT_RC_RECEIVE_MAXIMUM_EXCEEDED:
			return "Receive Maximum exceeded";
		case MQTT_RC_TOPIC_ALIAS_INVALID:
			return "Topic Alias invalid";
		case MQTT_RC_PACKET_TOO_LARGE:
			return "Packet too large";
		case MQTT_RC_MESSAGE_RATE_TOO_HIGH:
			return "Message rate too high";
		case MQTT_RC_QUOTA_EXCEEDED:
			return "Quota exceeded";
		case MQTT_RC_ADMINISTRATIVE_ACTION:
			return "Administrative action";
		case MQTT_RC_PAYLOAD_FORMAT_INVALID:
			return "Payload format invalid";
		case MQTT_RC_RETAIN_NOT_SUPPORTED:
			return "Retain not supported";
		case MQTT_RC_QOS_NOT_SUPPORTED:
			return "QoS not supported";
		case MQTT_RC_USE_ANOTHER_SERVER:
			return "Use another server";
		case MQTT_RC_SERVER_MOVED:
			return "Server moved";
		case MQTT_RC_SHARED_SUBS_NOT_SUPPORTED:
			return "Shared Subscriptions not supported";
		case MQTT_RC_CONNECTION_RATE_EXCEEDED:
			return "Connection rate exceeded";
		case MQTT_RC_MAXIMUM_CONNECT_TIME:
			return "Maximum connect time";
		case MQTT_RC_SUBSCRIPTION_IDS_NOT_SUPPORTED:
			return "Subscription identifiers not supported";
		case MQTT_RC_WILDCARD_SUBS_NOT_SUPPORTED:
			return "Wildcard Subscriptions not supported";
		default:
			return "Unknown reason";
	}
}


int mosquitto_string_to_command(const char *str, int *cmd)
{
	if(!strcasecmp(str, "connect")){
		*cmd = CMD_CONNECT;
	}else if(!strcasecmp(str, "connack")){
		*cmd = CMD_CONNACK;
	}else if(!strcasecmp(str, "publish")){
		*cmd = CMD_PUBLISH;
	}else if(!strcasecmp(str, "puback")){
		*cmd = CMD_PUBACK;
	}else if(!strcasecmp(str, "pubrec")){
		*cmd = CMD_PUBREC;
	}else if(!strcasecmp(str, "pubrel")){
		*cmd = CMD_PUBREL;
	}else if(!strcasecmp(str, "pubcomp")){
		*cmd = CMD_PUBCOMP;
	}else if(!strcasecmp(str, "subscribe")){
		*cmd = CMD_SUBSCRIBE;
	}else if(!strcasecmp(str, "suback")){
		*cmd = CMD_SUBACK;
	}else if(!strcasecmp(str, "unsubscribe")){
		*cmd = CMD_UNSUBSCRIBE;
	}else if(!strcasecmp(str, "unsuback")){
		*cmd = CMD_UNSUBACK;
	}else if(!strcasecmp(str, "disconnect")){
		*cmd = CMD_DISCONNECT;
	}else if(!strcasecmp(str, "auth")){
		*cmd = CMD_AUTH;
	}else if(!strcasecmp(str, "will")){
		*cmd = CMD_WILL;
	}else{
		*cmd = 0;
		return MOSQ_ERR_INVAL;
	}
	return MOSQ_ERR_SUCCESS;
}
