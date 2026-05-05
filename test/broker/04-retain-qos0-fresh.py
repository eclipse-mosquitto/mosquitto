#!/usr/bin/env python3

# Test whether a retained PUBLISH to a topic with QoS 0 is sent with
# retain=false to an already subscribed client.

from mosq_test_helper import *

def do_test(proto_ver):
    mid = 16
    connect_packet = mqtt_packets.gen_connect("retain-qos0-fresh", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    publish_packet = mqtt_packets.gen_publish("retain/qos0/fresh", qos=0, payload="retained message", retain=True, proto_ver=proto_ver)
    publish_fresh_packet = mqtt_packets.gen_publish("retain/qos0/fresh", qos=0, payload="retained message", proto_ver=proto_ver)
    subscribe_packet = mqtt_packets.gen_subscribe(mid, "retain/qos0/fresh", 0, proto_ver=proto_ver)
    suback_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)

    publish_packet_clear = mqtt_packets.gen_publish("retain/qos0/fresh", qos=0, payload=None, retain=True, proto_ver=proto_ver)
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        mosq_test.do_send_receive(sock, publish_packet, publish_fresh_packet, "publish")
        sock.send(publish_packet_clear)
        sock.close()


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
