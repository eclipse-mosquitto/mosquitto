#include "libmosquitto_mock.hpp"

int mosquitto_connect_srv(struct mosquitto *mosq, const char *host,
			int keepalive, const char *bind_address)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_srv(mosq,
			host, keepalive, bind_address);
}
