#include "libmosquitto_mock.hpp"

int mosquitto_message_copy(struct mosquitto_message *dst,
		const struct mosquitto_message *src)
{
	return LibMosquittoMock::get_mock().mosquitto_message_copy(dst, src);
}

void mosquitto_message_free(struct mosquitto_message **message)
{
	LibMosquittoMock::get_mock().mosquitto_message_free(message);
}

void mosquitto_message_free_contents(struct mosquitto_message *message)
{
	LibMosquittoMock::get_mock().mosquitto_message_free_contents(message);
}
