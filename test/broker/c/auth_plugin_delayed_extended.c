#include <mosquitto/broker.h>
#include <mosquitto/defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

static int tick_callback(int event, void *event_data, void *user_data);
static int ext_auth_start_callback(int event, void *event_data, void *user_data);
static int ext_auth_continue_callback(int event, void *event_data, void *user_data);
static int stash_auth_info(struct mosquitto_evt_extended_auth *ed);

static mosquitto_plugin_id_t *plg_id;

static char *clientid = NULL;
static char *authmethod = NULL;
static void *authdata = NULL;
static uint16_t data_len = 0;
static int step = -1;
static int auth_delay = -1;

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *auth_opts, int auth_opt_count)
{
	(void)user_data;
	(void)auth_opts;
	(void)auth_opt_count;

	plg_id = identifier;

	mosquitto_callback_register(plg_id, MOSQ_EVT_TICK, tick_callback, NULL, NULL);
	mosquitto_callback_register(plg_id, MOSQ_EVT_EXT_AUTH_START, ext_auth_start_callback, NULL, NULL);
	mosquitto_callback_register(plg_id, MOSQ_EVT_EXT_AUTH_CONTINUE, ext_auth_continue_callback, NULL, NULL);

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *auth_opts, int auth_opt_count)
{
	(void)user_data;
	(void)auth_opts;
	(void)auth_opt_count;

	free(authmethod);
	free(authdata);
	free(clientid);

	mosquitto_callback_unregister(plg_id, MOSQ_EVT_EXT_AUTH_CONTINUE, ext_auth_continue_callback, NULL);
	mosquitto_callback_unregister(plg_id, MOSQ_EVT_EXT_AUTH_START, ext_auth_start_callback, NULL);
	mosquitto_callback_unregister(plg_id, MOSQ_EVT_TICK, tick_callback, NULL);

	return MOSQ_ERR_SUCCESS;
}


static int tick_callback(int event, void *event_data, void *user_data)
{
	struct mosquitto_evt_tick *ed = event_data;

	(void)user_data;

	if(event != MOSQ_EVT_TICK){
		abort();
	}

	if(auth_delay == 0){
		if(step == 1){
			if(clientid && authmethod && authdata
					&& !strcmp(authmethod, "delayed-auth") && !memcmp(authdata, "good-start", data_len)){
				void *respdata = strdup("good-middle");
				mosquitto_complete_extended_auth(clientid, MOSQ_ERR_AUTH_CONTINUE, respdata, strlen("good-middle"));
			}else{
				mosquitto_complete_extended_auth(clientid, MOSQ_ERR_AUTH, NULL, 0);
			}
		}else if(step == 2){
			if(clientid && authmethod && authdata
					&& !strcmp(authmethod, "delayed-auth") && !memcmp(authdata, "good-finish", data_len)){
				mosquitto_complete_extended_auth(clientid, MOSQ_ERR_SUCCESS, NULL, 0);
			}else{
				mosquitto_complete_extended_auth(clientid, MOSQ_ERR_AUTH, NULL, 0);
			}
			step = -1;
		}
		free(authmethod);
		free(authdata);
		free(clientid);
		authmethod = NULL;
		authdata = NULL;
		clientid = NULL;
	}else if(auth_delay > 0){
		auth_delay--;
	}

	/* fast turn around for quick testing */
	ed->next_ms = 100;

	return MOSQ_ERR_SUCCESS;
}


static int ext_auth_start_callback(int event, void *event_data, void *user_data)
{
	struct mosquitto_evt_extended_auth *ed = event_data;

	(void)event;
	(void)user_data;

	int rc = stash_auth_info(ed);
	if(rc){
		return rc;
	}

	/* Delay for arbitrary 10 ticks */
	auth_delay = 10;
	step = 1;

	return MOSQ_ERR_AUTH_DELAYED;
}


static int ext_auth_continue_callback(int event, void *event_data, void *user_data)
{
	struct mosquitto_evt_extended_auth *ed = event_data;

	(void)event;
	(void)user_data;

	int rc = stash_auth_info(ed);
	if(rc){
		return rc;
	}

	/* Delay for arbitrary 10 ticks */
	auth_delay = 10;
	step = 2;

	return MOSQ_ERR_AUTH_DELAYED;
}


static int stash_auth_info(struct mosquitto_evt_extended_auth *ed)
{
	free(authmethod);
	free(authdata);
	free(clientid);

	if(ed->auth_method){
		authmethod = strdup(ed->auth_method);
	}
	if(ed->data_in){
		data_len = ed->data_in_len;
		authdata = malloc(ed->data_in_len);
		if(!authdata){
			return MOSQ_ERR_NOMEM;
		}
		memcpy(authdata, ed->data_in, ed->data_in_len);
	}
	clientid = strdup(mosquitto_client_id(ed->client));

	return MOSQ_ERR_SUCCESS;
}


