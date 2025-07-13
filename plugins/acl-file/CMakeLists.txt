set(PLUGIN_NAME mosquitto_acl_file)

set(SRCLIST
	acl_check.c
	acl_parse.c
	plugin.c
)

set(INCLIST ${mosquitto_SOURCE_DIR}/src)
set(LINKLIST libmosquitto_common)

add_mosquitto_plugin("${PLUGIN_NAME}" "${SRCLIST}" "${INCLIST}" "${LINKLIST}")
