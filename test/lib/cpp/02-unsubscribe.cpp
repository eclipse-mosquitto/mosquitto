#include <cassert>
#include <mosquitto/libmosquittopp.h>

static int run = -1;

class mosquittopp_test : public mosqpp::mosquittopp
{
public:
	mosquittopp_test(const char *id);

	void on_connect(int rc);
	void on_disconnect(int rc);
	void on_unsubscribe(int mid);
};

mosquittopp_test::mosquittopp_test(const char *id) : mosqpp::mosquittopp(id)
{
}


void mosquittopp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}else{
		unsubscribe(NULL, "unsubscribe/test");
	}
}


void mosquittopp_test::on_disconnect(int rc)
{
	run = rc;
}


void mosquittopp_test::on_unsubscribe(int mid)
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

	mosq = new mosquittopp_test("unsubscribe-test");

	mosq->connect("127.0.0.1", port, 60);

	while(run == -1){
		mosq->loop();
	}
	delete mosq;

	mosqpp::lib_cleanup();

	return run;
}
