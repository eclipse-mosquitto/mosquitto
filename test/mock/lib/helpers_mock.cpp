#include "libmosquitto_mock.hpp"

int mosquitto_subscribe_simple(struct mosquitto_message **messages,
		int msg_count, bool want_retained, const char *topic, int qos,
		const char *host, int port, const char *clientid, int keepalive,
		bool clean_session, const char *username, const char *password,
		const struct libmosquitto_will *will, const struct libmosquitto_tls *tls)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_simple(messages,
			msg_count, want_retained, topic, qos, host, port, clientid,
			keepalive, clean_session, username, password, will, tls);
}

int mosquitto_subscribe_callback(int (*callback)(struct mosquitto *,
		void *, const struct mosquitto_message *), void *userdata,
		const char *topic, int qos, const char *host, int port,
		const char *clientid, int keepalive, bool clean_session,
		const char *username, const char *password, const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls)
{
	return LibMosquittoMock::get_mock().mosquitto_subscribe_callback(
			callback, userdata, topic, qos, host, port, clientid,
			keepalive, clean_session, username, password, will, tls);

}
