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
	int i = 0;

	int port = atoi(argv[1]);

	mosquitto_lib_init();

	mosq = mosquitto_new("publish-helper-multiple-qos0-test", true, NULL);
	if(mosq == NULL){
		return 1;
	}

	struct mosquitto_message messages[5];
	struct mosquitto_message *message_ptrs[5];
	char buf[50];

	for (i = 0; i < 5; i++) {
		struct mosquitto_message *m = &messages[i];
		sprintf(buf, "publish-helper-multiple/qos0/test/%d", i);
		m->topic = strdup(buf);
		m->payload = "message";
		m->payloadlen = strlen("message");
		m->qos = 0;
		m->retain = 0;

		message_ptrs[i] = m;
	}

	rc = mosquitto_publish_multiple(
			message_ptrs,
			5,
			"localhost", port,
			"publish-helper-multiple-qos0-test", 60, true,
			NULL, NULL, NULL, NULL);

	mosquitto_lib_cleanup();
	return rc;
}
