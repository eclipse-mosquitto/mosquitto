#include "libmosquitto_mock.hpp"

void *mosquitto_ssl_get(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_ssl_get(mosq);
}
