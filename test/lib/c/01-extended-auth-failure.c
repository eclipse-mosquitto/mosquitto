#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <mosquitto/mqtt_protocol.h>

static int run = -1;


static void on_connect(struct mosquitto *mosq, void *obj, int rc, int flags, const mosquitto_property *properties)
{
	(void)obj;
	(void)rc;
	(void)flags;
	(void)properties;

	/* FIXME - should verify flags and all properties here. */
	if(rc){
		exit(1);
	}else{
		mosquitto_disconnect(mosq);
	}
}


static int on_ext_auth(struct mosquitto *mosq, void *obj, const char *auth_method, uint16_t auth_data_len, const void *auth_data, const mosquitto_property *properties)
{
	(void)mosq;
	(void)obj;
	(void)auth_method;
	(void)auth_data;
	(void)auth_data_len;
	(void)properties;

	return MOSQ_ERR_AUTH;
}


static void on_disconnect(struct mosquitto *mosq, void *obj, int rc, const mosquitto_property *properties)
{
	(void)mosq;
	(void)obj;
	(void)properties;

	/* FIXME - should verify flags and all properties here. */
	run = rc;
}


int main(int argc, char *argv[])
{
	int rc;
	struct mosquitto *mosq;
	int port;
	mosquitto_property *props = NULL;

	if(argc < 2){
		return 1;
	}
	port = atoi(argv[1]);

	mosquitto_lib_init();

	mosq = mosquitto_new("01-extended-auth", true, NULL);
	if(mosq == NULL){
		return 1;
	}
	mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, 5);
	mosquitto_connect_v5_callback_set(mosq, on_connect);
	mosquitto_ext_auth_callback_set(mosq, on_ext_auth);
	mosquitto_disconnect_v5_callback_set(mosq, on_disconnect);

	mosquitto_property_add_int32(&props, MQTT_PROP_MAXIMUM_PACKET_SIZE, 1000);
	rc = mosquitto_connect_bind_v5(mosq, "127.0.0.1", port, 60, NULL, props);
	mosquitto_property_free_all(&props);
	if(rc != MOSQ_ERR_SUCCESS){
		return rc;
	}

	while(run == -1){
		mosquitto_loop(mosq, -1, 1);
	}

	mosquitto_destroy(mosq);

	mosquitto_lib_cleanup();
	return run;
}
