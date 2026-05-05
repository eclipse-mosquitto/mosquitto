#!/usr/bin/env python3

# Test whether a SUBSCRIBE to $SYS or $share succeeds

from mosq_test_helper import *

def do_test(port, proto_ver):
    connect_packet = mqtt_packets.gen_connect("subscribe-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe1_packet = mqtt_packets.gen_subscribe(mid, "$SYS/broker/missing", 0, proto_ver=proto_ver)
    suback1_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    mid = 2
    subscribe2_packet = mqtt_packets.gen_subscribe(mid, "$share/share/#", 0, proto_ver=proto_ver)
    suback2_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
    mosq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
    mosq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")
    sock.close()


if __name__ == '__main__':
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        do_test(port, proto_ver=4)
        do_test(port, proto_ver=5)
