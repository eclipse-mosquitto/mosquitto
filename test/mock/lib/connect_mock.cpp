#include "libmosquitto_mock.hpp"

int mosquitto_connect(struct mosquitto *mosq, const char *host,
		int port, int keepalive)
{
	return LibMosquittoMock::get_mock().mosquitto_connect(mosq, host,
		port, keepalive);
}

int mosquitto_connect_bind(struct mosquitto *mosq, const char *host,
		int port, int keepalive, const char *bind_address)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_bind(mosq, host,
		port, keepalive, bind_address);
}

int mosquitto_connect_bind_v5(struct mosquitto *mosq, const char *host,
		int port, int keepalive, const char *bind_address,
		const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_bind_v5(mosq, host,
		port, keepalive, bind_address, properties);
}

int mosquitto_connect_async(struct mosquitto *mosq, const char *host,
		int port, int keepalive)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_async(mosq, host,
		port, keepalive);
}

int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host,
		int port, int keepalive, const char *bind_address)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_bind_async(mosq, host,
		port, keepalive, bind_address);
}

int mosquitto_reconnect_async(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_reconnect_async(mosq);
}

int mosquitto_reconnect(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_reconnect(mosq);
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect(mosq);
}

int mosquitto_disconnect_v5(struct mosquitto *mosq, int reason_code,
		const mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect_v5(mosq,
			reason_code, properties);
}
