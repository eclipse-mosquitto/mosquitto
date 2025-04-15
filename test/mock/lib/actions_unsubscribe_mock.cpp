#include "libmosquitto_mock.hpp"

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe(mosq, mid, sub);
}

int mosquitto_unsubscribe_v5(struct mosquitto *mosq, int *mid, const char *sub,
		const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_v5(mosq, mid, sub, properties);
}

int mosquitto_unsubscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count,
			char *const *const sub, const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_multiple(mosq,
			mid, sub_count, sub, properties);
}
