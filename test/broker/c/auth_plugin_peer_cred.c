#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <mosquitto.h>
#include <mosquitto/broker.h>
#include <mosquitto/broker_plugin.h>

static mosquitto_plugin_id_t *plg_id;

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

static int basic_auth_callback(int event, void *event_data, void *user_data)
{
	struct mosquitto_evt_basic_auth *ed = event_data;
	uid_t uid = 0;
	gid_t gid = 0;
	pid_t pid = 0;
	int rc;

	(void)event;
	(void)user_data;

	/* All out parameters are optional. */
	rc = mosquitto_client_peer_cred(ed->client, NULL, NULL, NULL);
	if(rc == MOSQ_ERR_NOT_SUPPORTED){
		/* Not a Unix socket client, refuse. */
		return MOSQ_ERR_AUTH;
	}else if(rc != MOSQ_ERR_SUCCESS){
		return MOSQ_ERR_AUTH;
	}

	rc = mosquitto_client_peer_cred(ed->client, &uid, &gid, &pid);
	if(rc == MOSQ_ERR_SUCCESS && uid == getuid() && gid == getgid() && pid > 0){
		return MOSQ_ERR_SUCCESS;
	}
	return MOSQ_ERR_AUTH;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *auth_opts, int auth_opt_count)
{
	(void)user_data;
	(void)auth_opts;
	(void)auth_opt_count;

	plg_id = identifier;

	return mosquitto_callback_register(plg_id, MOSQ_EVT_BASIC_AUTH, basic_auth_callback, NULL, NULL);
}


int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *auth_opts, int auth_opt_count)
{
	(void)user_data;
	(void)auth_opts;
	(void)auth_opt_count;

	return mosquitto_callback_unregister(plg_id, MOSQ_EVT_BASIC_AUTH, basic_auth_callback, NULL);
}
