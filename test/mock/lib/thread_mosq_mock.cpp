#include "libmosquitto_mock.hpp"

int mosquitto_loop_start(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_start(mosq);
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
	return LibMosquittoMock::get_mock().mosquitto_loop_stop(mosq, force);
}

int mosquitto_threaded_set(struct mosquitto *mosq, bool threaded)
{
	return LibMosquittoMock::get_mock().mosquitto_threaded_set(mosq, threaded);
}
