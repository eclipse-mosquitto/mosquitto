#include "libmosquitto_mock.hpp"

void mosquitto_connect_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_connect on_connect)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_callback_set(mosq, on_connect);
}

void mosquitto_connect_with_flags_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_connect_with_flags on_connect)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_with_flags_callback_set(mosq, on_connect);
}

void mosquitto_connect_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_connect_v5 on_connect)
{
	return LibMosquittoMock::get_mock().mosquitto_connect_v5_callback_set(mosq, on_connect);
}

void mosquitto_pre_connect_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_pre_connect on_pre_connect)
{
	return LibMosquittoMock::get_mock().mosquitto_pre_connect_callback_set(mosq, on_pre_connect);
}

void mosquitto_disconnect_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_disconnect on_disconnect)
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect_callback_set(mosq, on_disconnect);
}

void mosquitto_disconnect_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_disconnect_v5 on_disconnect)
{
	return LibMosquittoMock::get_mock().mosquitto_disconnect_v5_callback_set(mosq, on_disconnect);
}

void mosquitto_publish_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_publish on_publish)
{
	return LibMosquittoMock::get_mock().mosquitto_publish_callback_set(mosq, on_publish);
}

void mosquitto_publish_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_publish_v5 on_publish)
{
	return LibMosquittoMock::get_mock().mosquitto_publish_v5_callback_set(mosq, on_publish);
}

void mosquitto_message_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_message on_message)
{
	return LibMosquittoMock::get_mock().mosquitto_message_callback_set(mosq, on_message);
}

void mosquitto_message_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_message_v5 on_message)
{
	return LibMosquittoMock::get_mock().mosquitto_message_v5_callback_set(mosq, on_message);
}

void mosquitto_subscribe_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_subscribe on_subscribe)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_callback_set(mosq, on_subscribe);
}

void mosquitto_subscribe_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_subscribe_v5 on_subscribe)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_v5_callback_set(mosq, on_subscribe);
}

void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_unsubscribe on_unsubscribe)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
}

void mosquitto_unsubscribe_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_unsubscribe_v5 on_unsubscribe)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe_v5_callback_set(mosq, on_unsubscribe);
}

void mosquitto_unsubscribe2_v5_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_unsubscribe2_v5 on_unsubscribe)
{
	return LibMosquittoMock::get_mock().mosquitto_unsubscribe2_v5_callback_set(mosq, on_unsubscribe);
}

void mosquitto_log_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_log on_log)
{
	return LibMosquittoMock::get_mock().mosquitto_log_callback_set(mosq, on_log);
}

void mosquitto_ext_auth_callback_set(struct mosquitto *mosq, LIBMOSQ_CB_ext_auth on_ext_auth)
{
	return LibMosquittoMock::get_mock().mosquitto_ext_auth_callback_set(mosq, on_ext_auth);
}
