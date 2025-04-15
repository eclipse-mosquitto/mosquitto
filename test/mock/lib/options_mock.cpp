#include "libmosquitto_mock.hpp"

int mosquitto_will_set(struct mosquitto *mosq, const char *topic,
		int payloadlen, const void *payload, int qos, bool retain)
{
	return LibMosquittoMock::get_mock().mosquitto_will_set(mosq,
			topic, payloadlen, payload, qos, retain);
}

int mosquitto_will_set_v5(struct mosquitto *mosq, const char *topic,
		int payloadlen, const void *payload, int qos, bool retain,
		mosquitto_property *properties)
{
	return LibMosquittoMock::get_mock().mosquitto_will_set_v5(mosq,
			topic, payloadlen, payload, qos, retain, properties);
}

int mosquitto_will_clear(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_will_clear(mosq);
}

int mosquitto_username_pw_set(struct mosquitto *mosq,
		const char *username, const char *password)
{
	return LibMosquittoMock::get_mock().mosquitto_username_pw_set(mosq,
			username, password);
}

int mosquitto_reconnect_delay_set(struct mosquitto *mosq,
		unsigned int reconnect_delay, unsigned int reconnect_delay_max,
		bool reconnect_exponential_backoff)
{
	return LibMosquittoMock::get_mock().mosquitto_reconnect_delay_set(mosq,
			reconnect_delay, reconnect_delay_max, reconnect_exponential_backoff);
}

int mosquitto_tls_set(struct mosquitto *mosq, const char *cafile,
		const char *capath, const char *certfile, const char *keyfile,
		int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	return LibMosquittoMock::get_mock().mosquitto_tls_set(mosq,
			cafile, capath, certfile, keyfile, pw_callback);
}

int mosquitto_tls_opts_set(struct mosquitto *mosq, int cert_reqs,
		const char *tls_version, const char *ciphers)
{
	return LibMosquittoMock::get_mock().mosquitto_tls_opts_set(mosq,
			cert_reqs, tls_version, ciphers);
}

int mosquitto_tls_insecure_set(struct mosquitto *mosq, bool value)
{
	return LibMosquittoMock::get_mock().mosquitto_tls_insecure_set(mosq,
			value);
}

int mosquitto_string_option(struct mosquitto *mosq,
		enum mosq_opt_t option, const char *value)
{
	return LibMosquittoMock::get_mock().mosquitto_string_option(mosq,
			option, value);
}

int mosquitto_tls_psk_set(struct mosquitto *mosq, const char *psk,
		const char *identity, const char *ciphers)
{
	return LibMosquittoMock::get_mock().mosquitto_tls_psk_set(mosq,
			psk, identity, ciphers);
}

int mosquitto_opts_set(struct mosquitto *mosq, enum mosq_opt_t option,
		void *value)
{
	return LibMosquittoMock::get_mock().mosquitto_opts_set(mosq,
			option, value);
}

int mosquitto_int_option(struct mosquitto *mosq, enum mosq_opt_t option,
		int value)
{
	return LibMosquittoMock::get_mock().mosquitto_int_option(mosq,
			option, value);
}

int mosquitto_void_option(struct mosquitto *mosq, enum mosq_opt_t option,
		void *value)
{
	return LibMosquittoMock::get_mock().mosquitto_void_option(mosq,
			option, value);
}

void mosquitto_user_data_set(struct mosquitto *mosq, void *userdata)
{
	LibMosquittoMock::get_mock().mosquitto_user_data_set(mosq, userdata);
}

void *mosquitto_userdata(struct mosquitto *mosq)
{
	return LibMosquittoMock::get_mock().mosquitto_userdata(mosq);
}
