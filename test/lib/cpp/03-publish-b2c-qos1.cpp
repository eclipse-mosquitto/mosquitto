#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <mosquitto/libmosquittopp.h>

class mosquittopp_test : public mosqpp::mosquittopp
{
public:
	mosquittopp_test(const char *id);

	void on_connect(int rc);
	void on_message(const struct mosquitto_message *msg);
};

mosquittopp_test::mosquittopp_test(const char *id) : mosqpp::mosquittopp(id)
{
}


void mosquittopp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}
}


void mosquittopp_test::on_message(const struct mosquitto_message *msg)
{
	if(msg->mid != 123){
		printf("Invalid mid (%d)\n", msg->mid);
		exit(1);
	}
	if(msg->qos != 1){
		printf("Invalid qos (%d)\n", msg->qos);
		exit(1);
	}
	if(strcmp(msg->topic, "pub/qos1/receive")){
		printf("Invalid topic (%s)\n", msg->topic);
		exit(1);
	}
	if(strcmp((char *)msg->payload, "message")){
		printf("Invalid payload (%s)\n", (char *)msg->payload);
		exit(1);
	}
	if(msg->payloadlen != 7){
		printf("Invalid payloadlen (%d)\n", msg->payloadlen);
		exit(1);
	}
	if(msg->retain != false){
		printf("Invalid retain (%d)\n", msg->retain);
		exit(1);
	}

	exit(0);
}


int main(int argc, char *argv[])
{
	mosquittopp_test *mosq;
	int rc = 1;

	if(argc != 2){
		return 1;
	}
	int port = atoi(argv[1]);

	mosqpp::lib_init();

	mosq = new mosquittopp_test("publish-qos1-test");

	mosq->connect("127.0.0.1", port, 60);

	while(1){
		if(mosq->loop()){
			rc = 0;
			break;
		}
	}

	delete mosq;
	mosqpp::lib_cleanup();

	return rc;
}

