#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

from mosq_test_helper import *

def do_test(port, proto_ver):
    mid = 530
    connect_packet = mqtt_packets.gen_connect("subpub-qos2-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    subscribe_packet = mqtt_packets.gen_subscribe(mid, "subpub/qos2", 2, proto_ver=proto_ver)
    suback_packet = mqtt_packets.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 301
    publish_packet = mqtt_packets.gen_publish("subpub/qos2", qos=2, mid=mid, payload="message", proto_ver=proto_ver)
    pubrec_packet = mqtt_packets.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet = mqtt_packets.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet = mqtt_packets.gen_pubcomp(mid, proto_ver=proto_ver)

    mid = 1
    publish_packet2 = mqtt_packets.gen_publish("subpub/qos2", qos=2, mid=mid, payload="message", proto_ver=proto_ver)
    pubrec_packet2 = mqtt_packets.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_packet2 = mqtt_packets.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_packet2 = mqtt_packets.gen_pubcomp(mid, proto_ver=proto_ver)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
    mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec")
    sock.send(pubrel_packet)
    mosq_test.receive_unordered(sock, pubcomp_packet, publish_packet2, "pubcomp/publish2")
    mosq_test.do_send_receive(sock, pubrec_packet2, pubrel_packet2, "pubrel2")
    sock.send(pubcomp_packet2)
    # Broker side of flow complete so can quit here.
    sock.close()


if __name__ == '__main__':
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        do_test(port, proto_ver=4)
        do_test(port, proto_ver=5)
