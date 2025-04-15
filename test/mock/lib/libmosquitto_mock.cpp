#include "libmosquitto_mock.hpp"

LibMosquittoMock::LibMosquittoMock() {};
LibMosquittoMock::~LibMosquittoMock() {};

int mosquitto_lib_version(int *major, int *minor, int *revision)
{
	return LibMosquittoMock::get_mock().mosquitto_lib_version(
			major, minor, revision);
}

int mosquitto_lib_init()
{
	return LibMosquittoMock::get_mock().mosquitto_lib_init();
}

int mosquitto_lib_cleanup()
{
	return LibMosquittoMock::get_mock().mosquitto_lib_cleanup();
}

struct mosquitto *mosquitto_new(const char *id, bool clean_start, void *userdata)
{
	return LibMosquittoMock::get_mock().mosquitto_new(id, clean_start, userdata);
}

int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_start,
			void *userdata)
{
	return LibMosquittoMock::get_mock().mosquitto_reinitialise(mosq, id, clean_start, userdata);
}

void mosquitto_destroy(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_destroy(mosq);
}

int mosquitto_socket(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_socket(mosq);
}

bool mosquitto_want_write(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_want_write(mosq);
}
