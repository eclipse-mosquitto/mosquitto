#include "libmosquitto_mock.hpp"

int mosquitto_socks5_set(struct mosquitto *mosq, const char *host,
			int port, const char *username, const char *password)
{
	return LibMosquittoMock::get_mock().mosquitto_socks5_set(mosq,
			host, port, username, password);
}
