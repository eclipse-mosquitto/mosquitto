#!/usr/bin/env python3

# Test whether a retained PUBLISH to a topic with QoS 0 is actually retained.

from mosq_test_helper import *


def do_test(proto_ver):
    mid = 16
    connect_packet = mqtt_packets.gen_connect("retain-qos0-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    publish_packet = mqtt_packets.gen_publish("retain/qos0/test", qos=0, payload="retained message", retain=True, proto_ver=proto_ver)
    subscribe_packet = mqtt_packets.gen_subscribe(mid, "retain/qos0/test", 0, proto_ver=proto_ver)
    suback_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        sock.send(publish_packet)
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        mosq_test.expect_packet(sock, "publish", publish_packet)
        sock.close()


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
