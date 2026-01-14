#include <cassert>
#include <cstdlib>
#include <cstring>

#include <mosquitto/libmosquittopp.h>

static int run = -1;

class mosquittopp_test : public mosqpp::mosquittopp
{
public:
	mosquittopp_test(const char *id);

	void on_connect(int rc);
	void on_disconnect(int rc);
	void on_publish(int mid);
};

mosquittopp_test::mosquittopp_test(const char *id) : mosqpp::mosquittopp(id)
{
}


void mosquittopp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}else{
		publish(NULL, "pub/qos1/test", strlen("message"), "message", 1, false);
	}
}


void mosquittopp_test::on_disconnect(int rc)
{
	run = rc;
}


void mosquittopp_test::on_publish(int mid)
{
	assert(mid == 1);
	disconnect();
}


int main(int argc, char *argv[])
{
	mosquittopp_test *mosq;

	if(argc != 2){
		return 1;
	}
	int port = atoi(argv[1]);

	mosqpp::lib_init();

	mosq = new mosquittopp_test("publish-qos1-test");
	mosq->int_option(MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

	mosq->connect("127.0.0.1", port, 60);

	while(run == -1){
		mosq->loop(300, 1);
	}

	delete mosq;
	mosqpp::lib_cleanup();

	return run;
}

