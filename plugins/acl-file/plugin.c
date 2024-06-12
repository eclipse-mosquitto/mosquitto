/*
Copyright (c) 2023 Cedalo Gmbh
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "mosquitto.h"
#include "acl_file.h"

#define PLUGIN_NAME "acl-file"

MOSQUITTO_PLUGIN_DECLARE_VERSION(5);

static mosquitto_plugin_id_t *mosq_pid = NULL;

static int handle_options(struct acl_file_data *data, struct mosquitto_opt *options, int option_count)
{
	for(int i=0; i<option_count; i++){
		if(!strcmp(options[i].key, "acl_file")){
			mosquitto_FREE(data->acl_file);
			data->acl_file = mosquitto_strdup(options[i].value);
			if(!data->acl_file){
				return MOSQ_ERR_NOMEM;
			}
		}else{
			mosquitto_log_printf(MOSQ_LOG_ERR, PLUGIN_NAME ": Error: Unknown option '%s'.", options[i].key);
			return MOSQ_ERR_INVAL;
		}
	}

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *options, int option_count)
{
	struct acl_file_data *data;
	int rc;

	UNUSED(options);
	UNUSED(option_count);

	data = mosquitto_calloc(1, sizeof(struct acl_file_data));
	if(!data){
		return MOSQ_ERR_NOMEM;
	}
	*user_data = data;

	mosq_pid = identifier;
	mosquitto_plugin_set_info(identifier, PLUGIN_NAME, NULL);

	rc = handle_options(data, options, option_count);
	if(rc) return rc;

	rc = acl__parse(data);
	if(rc) return rc;

	rc = mosquitto_callback_register(mosq_pid, MOSQ_EVT_ACL_CHECK, acl__check, NULL, data);
	if(rc) return rc;

	return MOSQ_ERR_SUCCESS;
}


int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *options, int option_count)
{
	struct acl_file_data *data = user_data;

	UNUSED(options);
	UNUSED(option_count);

	mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_ACL_CHECK, acl__check, NULL);

	return acl__cleanup(data);
}
