#!/usr/bin/env python3

# Test $CONTROL/broker/v1 listListeners

from mosq_test_helper import *
import json
import shutil

mosq_test.require_features(["WITH_CONTROL"])

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("enable_control_api true\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port}\n")

def command_check(sock, command_payload, expected_response):
    command_packet = mosq_test.gen_publish(topic="$CONTROL/missing-endpoint/v1", qos=0, payload=json.dumps(command_payload))
    sock.send(command_packet)
    response = json.loads(mosq_test.read_publish(sock))
    if response != expected_response:
        print(expected_response)
        print(response)
        raise ValueError(response)


port = mosq_test.get_port(1)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

rc = 1
connect_packet = mosq_test.gen_connect("17-missing-endpoint")
connack_packet = mosq_test.gen_connack(rc=0)

mid = 2
subscribe_packet = mosq_test.gen_subscribe(mid, "$CONTROL/missing-endpoint/#", 0)
suback_packet = mosq_test.gen_suback(mid, 0)

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    cmd = {"commands":[{"command": "listListeners", "correlationData": "m3CtYVnySLCOwnHzITSeowvgla0InV4G"}]}
    response = {"error": "endpoint not available"}
    command_check(sock, cmd, response)
    mosq_test.do_ping(sock)

    rc = 0

    sock.close()
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))


exit(rc)
