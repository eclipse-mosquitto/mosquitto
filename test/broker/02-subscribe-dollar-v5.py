#!/usr/bin/env python3

# Test whether a SUBSCRIBE to $SYS or $share succeeds

from mosq_test_helper import *

def do_test(start_broker, proto_ver):
    rc = 1
    connect_packet = mqtt_packets.gen_connect("subscribe-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe1_packet = mqtt_packets.gen_subscribe(mid, "$SYS/broker/missing", 0, proto_ver=proto_ver)
    suback1_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    mid = 2
    subscribe2_packet = mqtt_packets.gen_subscribe(mid, "$share/share/#", 0, proto_ver=proto_ver)
    suback2_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    port = mosq_test.get_port()
    if start_broker:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
        mosq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

        rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    finally:
        if start_broker:
            mosq_test.terminate_broker(broker)
            if mosq_test.wait_for_subprocess(broker):
                print("broker not terminated")
                if rc == 0: rc=1
            if rc:
                print(mosq_test.broker_log(broker))
        if rc:
            exit(rc)


def all_tests(start_broker=False):
    do_test(start_broker, proto_ver=4)
    do_test(start_broker, proto_ver=5)
    exit(0)

if __name__ == '__main__':
    all_tests(True)
