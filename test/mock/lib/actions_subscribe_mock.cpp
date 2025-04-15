#include "libmosquitto_mock.hpp"

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe(mosq, mid, sub, qos);
}

int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, int qos,
			int options, const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_v5(mosq, mid, sub, qos, options, properties);
}

int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count,
			char *const *const sub, int qos, int options, const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_multiple(mosq, mid, sub_count, sub, qos, options, properties);
}
