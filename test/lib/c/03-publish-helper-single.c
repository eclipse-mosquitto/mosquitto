#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

static int run = -1;

int main(int argc, char *argv[])
{
	int rc;
	struct mosquitto *mosq;

	int port = atoi(argv[1]);

	mosquitto_lib_init();

	mosq = mosquitto_new("publish-helper-single-qos0-test", true, NULL);
	if(mosq == NULL){
		return 1;
	}

	mosquitto_publish_single(
			"publish-helper-single/qos0/test",
			"message", strlen("message"),
			0, "localhost", port,
			"publish-helper-single-qos0-test", 60, true,
			NULL, NULL, NULL, NULL);

	mosquitto_lib_cleanup();
	return run;
}
