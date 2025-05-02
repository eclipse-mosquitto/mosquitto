#include <mosquitto/embedded_broker.h>
#include "mosquitto_broker_internal.h"

extern int g_run;

void signal__setup(void) {}
void signal__flag_check(void) {}

void mosquitto_broker_stop()
{
	g_run = false;
}
