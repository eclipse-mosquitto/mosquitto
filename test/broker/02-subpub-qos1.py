#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

from mosq_test_helper import *

def do_test(port, proto_ver):
    mid = 530
    connect_packet = mqtt_packets.gen_connect("subpub-qos1-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    subscribe_packet = mqtt_packets.gen_subscribe(mid, "subpub/qos1", 1, proto_ver=proto_ver)
    suback_packet = mqtt_packets.gen_suback(mid, 1, proto_ver=proto_ver)

    mid = 300
    publish_packet = mqtt_packets.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=proto_ver)
    puback_packet = mqtt_packets.gen_puback(mid, proto_ver=proto_ver)

    mid = 1
    publish_packet2 = mqtt_packets.gen_publish("subpub/qos1", qos=1, mid=mid, payload="message", proto_ver=proto_ver)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
    sock.send(publish_packet)
    mosq_test.receive_unordered(sock, puback_packet, publish_packet2, "puback/publish2")
    sock.close()


if __name__ == '__main__':
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        do_test(port, proto_ver=4)
        do_test(port, proto_ver=5)
