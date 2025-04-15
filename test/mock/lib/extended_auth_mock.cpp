#include "libmosquitto_mock.hpp"

int mosquitto_ext_auth_continue(struct mosquitto *context,
		const char *auth_method, uint16_t auth_data_len, const void *auth_data,
		const mosquitto_property *input_props)
{
	return LibMosquittoMock::get_mock().mosquitto_ext_auth_continue(context,
			auth_method, auth_data_len, auth_data, input_props);
}
