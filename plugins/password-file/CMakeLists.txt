set(PLUGIN_NAME mosquitto_password_file)

set(SRCLIST
	password_check.c
	password_parse.c
	plugin.c
)

set(INCLIST ${mosquitto_SOURCE_DIR}/src)
set(LINKLIST libmosquitto_common)

add_mosquitto_plugin("${PLUGIN_NAME}" "${SRCLIST}" "${INCLIST}" "${LINKLIST}")
