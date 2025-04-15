#include "libmosquitto_mock.hpp"

int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
	return LibMosquittoMock::get_mock().mosquitto_loop(mosq, timeout, max_packets);
}

int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_forever(mosq, timeout, max_packets);
}

int mosquitto_loop_misc(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_misc(mosq);
}

int mosquitto_loop_read(struct mosquitto *mosq, int max_packets)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_read(mosq, max_packets);
}

int mosquitto_loop_write(struct mosquitto *mosq, int max_packets)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_write(mosq, max_packets);
}
