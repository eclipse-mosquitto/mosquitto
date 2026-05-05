#!/usr/bin/env python3

# Test whether a client subscribed to a topic with retain-as-published set works as expected.
# MQTT v5

from mosq_test_helper import *

def do_test():
    connect_packet = mqtt_packets.gen_connect("02-subpub-qos0-rap", proto_ver=5)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

    mid = 530
    subscribe1_packet = mqtt_packets.gen_subscribe(mid, "02/subpub/rap/normal", 0, proto_ver=5)
    suback1_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=5)

    mid = 531
    subscribe2_packet = mqtt_packets.gen_subscribe(mid, "02/subpub/rap/rap", 0 | mqtt5_opts.MQTT_SUB_OPT_RETAIN_AS_PUBLISHED, proto_ver=5)
    suback2_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=5)

    publish1_packet = mqtt_packets.gen_publish("02/subpub/rap/normal", qos=0, retain=True, payload="message", proto_ver=5)
    publish2_packet = mqtt_packets.gen_publish("02/subpub/rap/rap", qos=0, retain=True, payload="message", proto_ver=5)

    publish1r_packet = mqtt_packets.gen_publish("02/subpub/rap/normal", qos=0, retain=False, payload="message", proto_ver=5)
    publish2r_packet = mqtt_packets.gen_publish("02/subpub/rap/rap", qos=0, retain=True, payload="message", proto_ver=5)

    mid = 1
    publish3_packet = mqtt_packets.gen_publish("02/subpub/rap/receive", qos=1, mid=mid, payload="success", proto_ver=5)


    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        mosq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback1")
        mosq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

        mosq_test.do_send_receive(sock, publish1_packet, publish1r_packet, "publish1")
        mosq_test.do_send_receive(sock, publish2_packet, publish2r_packet, "publish2")

        sock.close()


if __name__ == '__main__':
    do_test()
