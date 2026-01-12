#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <mosquitto/broker.h>
#include <mosquitto/broker_plugin.h>

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

static mosquitto_plugin_id_t *plg_id;


int callback_message_in(int event, void *event_data, void *user_data)
{
	struct mosquitto_evt_message *ed = event_data;

	(void)user_data;

	if(event != MOSQ_EVT_MESSAGE_IN){
		abort();
	}

	if(!strcmp(ed->topic, "first-publish")){
		mosquitto_plugin_publish_copy(
				"second-publish",
				strlen("second-payload"),
				"second-payload",
				1,
				false,
				NULL);
	}else if(!strcmp(ed->topic, "second-publish")){
		mosquitto_plugin_publish_copy(
				"third-publish",
				strlen("third-payload"),
				"third-payload",
				1,
				false,
				NULL);
	}

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *opts, int opt_count)
{
	(void)user_data;
	(void)opts;
	(void)opt_count;

	plg_id = identifier;

	mosquitto_callback_register(plg_id, MOSQ_EVT_MESSAGE_IN, callback_message_in, NULL, NULL);

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *opts, int opt_count)
{
	(void)user_data;
	(void)opts;
	(void)opt_count;

	mosquitto_callback_unregister(plg_id, MOSQ_EVT_MESSAGE_IN, callback_message_in, NULL);

	return MOSQ_ERR_SUCCESS;
}
