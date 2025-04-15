#include "libmosquitto_mock.hpp"

void mosquitto_connect_callback_set(struct mosquitto *mosq,
		void (*on_connect)(struct mosquitto *, void *, int))
{
	return LibMosquittoMock::get_mock().mosquitto_connect_callback_set(mosq, on_connect);
}

void mosquitto_connect_with_flags_callback_set(struct mosquitto *mosq,
		void (*on_connect)(struct mosquitto *, void *, int, int))
{
	return LibMosquittoMock::get_mock().mosquitto_connect_with_flags_callback_set(mosq, on_connect);
}

void mosquitto_connect_v5_callback_set(struct mosquitto *mosq,
		void (*on_connect)(struct mosquitto *, void *, int, int, const mosquitto_property *))
{
	return LibMosquittoMock::get_mock().mosquitto_connect_v5_callback_set(mosq, on_connect);
}

void mosquitto_pre_connect_callback_set(struct mosquitto *mosq,
		void (*on_pre_connect)(struct mosquitto *, void *))
{
	return LibMosquittoMock::get_mock().mosquitto_pre_connect_callback_set(mosq, on_pre_connect);
}

void mosquitto_disconnect_callback_set(struct mosquitto *mosq,
		void (*on_disconnect)(struct mosquitto *, void *, int))
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect_callback_set(mosq, on_disconnect);
}

void mosquitto_disconnect_v5_callback_set(struct mosquitto *mosq,
		void (*on_disconnect)(struct mosquitto *, void *, int, const mosquitto_property *))
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect_v5_callback_set(mosq, on_disconnect);
}

void mosquitto_publish_callback_set(struct mosquitto *mosq,
		void (*on_publish)(struct mosquitto *, void *, int))
{
	return LibMosquittoMock::get_mock().mosquitto_publish_callback_set(mosq, on_publish);
}

void mosquitto_publish_v5_callback_set(struct mosquitto *mosq,
		void (*on_publish)(struct mosquitto *, void *, int, int, const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_publish_v5_callback_set(mosq, on_publish);
}

void mosquitto_message_callback_set(struct mosquitto *mosq,
		void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *))
{
	return LibMosquittoMock::get_mock().mosquitto_message_callback_set(mosq, on_message);
}

void mosquitto_message_v5_callback_set(struct mosquitto *mosq,
		void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *,
		const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_message_v5_callback_set(mosq, on_message);
}

void mosquitto_subscribe_callback_set(struct mosquitto *mosq,
		void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *))
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_callback_set(mosq, on_subscribe);
}

void mosquitto_subscribe_v5_callback_set(struct mosquitto *mosq,
		void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *,
		const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_v5_callback_set(mosq, on_subscribe);
}

void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq,
		void (*on_unsubscribe)(struct mosquitto *, void *, int))
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
}

void mosquitto_unsubscribe_v5_callback_set(struct mosquitto *mosq,
		void (*on_unsubscribe)(struct mosquitto *, void *, int, const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_v5_callback_set(mosq, on_unsubscribe);
}

void mosquitto_unsubscribe2_v5_callback_set(struct mosquitto *mosq,
		void (*on_unsubscribe)(struct mosquitto *, void *, int, int, const int *,
		const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe2_v5_callback_set(mosq, on_unsubscribe);
}

void mosquitto_log_callback_set(struct mosquitto *mosq,
		void (*on_log)(struct mosquitto *, void *, int, const char *))
{
	return LibMosquittoMock::get_mock().mosquitto_log_callback_set(mosq, on_log);
}

void mosquitto_ext_auth_callback_set(struct mosquitto *mosq,
		int (*on_ext_auth)(struct mosquitto *, void *, const char *, uint16_t, const void *,
		const mosquitto_property *props))
{
	return LibMosquittoMock::get_mock().mosquitto_ext_auth_callback_set(mosq, on_ext_auth);
}
