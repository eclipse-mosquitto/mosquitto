//#include <stdbool.h>
#include <cstdio>
//#include <stdlib.h>
#include <cstring>
#include <mosquittopp.h>

#define WILL_MAX UINT16_MAX

static int run = -1;

class mosquittopp_test : public mosqpp::mosquittopp
{
	public:
		mosquittopp_test(const char *id);
};

mosquittopp_test::mosquittopp_test(const char *id) : mosqpp::mosquittopp(id)
{
}

int main(int argc, char *argv[])
{
	struct mosquittopp_test *mosq;
	int rc;
	char *payload;

	int port = atoi(argv[1]);

	payload = (char *)malloc(WILL_MAX+1);
	if(!payload){
		return 1;
	}
	memset(payload, 'A', WILL_MAX+1);

	mosqpp::lib_init();

	mosq = new mosquittopp_test("01-will-set");

	/* One byte too long, so must be rejected */
	rc = mosq->will_set("topic/on/unexpected/disconnect", WILL_MAX+1, payload, 1, true);
	if(rc != MOSQ_ERR_PAYLOAD_SIZE){
		fprintf(stderr, "will_set(%d) returned %d, expected MOSQ_ERR_PAYLOAD_SIZE\n", WILL_MAX+1, rc);
		free(payload);
		delete mosq;
		mosqpp::lib_cleanup();
		return 1;
	}

	/* WILL_MAX length should be accepted */
	rc = mosq->will_set("topic/on/unexpected/disconnect", WILL_MAX, payload, 1, true);
	free(payload);
	payload = NULL;
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "will_set(%d) returned %d, expected MOSQ_ERR_SUCCESS\n", WILL_MAX, rc);
		delete mosq;
		mosqpp::lib_cleanup();
		return 1;
	}

	mosq->connect("localhost", port, 60);

	while(run == -1){
		mosq->loop();
	}
	delete mosq;

	delete mosq;
	mosqpp::lib_cleanup();

	return run;
}
