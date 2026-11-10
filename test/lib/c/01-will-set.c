#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#define WILL_MAX UINT16_MAX

static int run = -1;
int main(int argc, char *argv[])
{
	int rc;
	struct mosquitto *mosq;

	int port = atoi(argv[1]);

	char *payload = malloc(WILL_MAX+1);
	if(!payload){
		return 1;
	}
	memset(payload, 'A', WILL_MAX+1);

	mosquitto_lib_init();

	mosq = mosquitto_new("01-will-set", true, NULL);
	if(mosq == NULL){
		free(payload);
		mosquitto_lib_cleanup();
		return 1;
	}

	/* One byte too long, so must be rejected. */
	rc = mosquitto_will_set(mosq, "topic/on/unexpected/disconnect", WILL_MAX+1, payload, 1, true);
	if(rc != MOSQ_ERR_PAYLOAD_SIZE){
		fprintf(stderr, "will_set(%d) returned %d, expected MOSQ_ERR_PAYLOAD_SIZE\n", WILL_MAX+1, rc);
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return 1;
	}

	/* WILL_MAX length should be accepted */
	rc = mosquitto_will_set(mosq, "topic/on/unexpected/disconnect", WILL_MAX, payload, 1, true);
	free(payload);
	payload = NULL;
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "will_set(%d) returned %d, expected MOSQ_ERR_SUCCESS\n", WILL_MAX, rc);
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return 1;
	}

	rc = mosquitto_connect(mosq, "localhost", port, 60);

	while(run == -1){
		mosquitto_loop(mosq, -1, 1);
	}
	mosquitto_destroy(mosq);

	mosquitto_lib_cleanup();
	return run;
}
