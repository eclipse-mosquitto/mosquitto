#include "libmosquitto_mock.hpp"

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
			int payloadlen, const void *payload, int qos, bool retain)
{
	return LibMosquittoMock::get_mock().mosquitto_publish(mosq, mid, topic, payloadlen, static_cast<const char*>(payload), qos, retain);
}

int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic,
			int payloadlen, const void *payload, int qos, bool retain, const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_publish_v5(mosq, mid, topic, payloadlen, static_cast<const char*>(payload), qos, retain, properties);
}
