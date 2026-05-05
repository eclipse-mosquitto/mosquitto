#!/usr/bin/env python3

# Test whether a client subscribed to a topic does not receive its own message
# sent to that topic if no local is set.
# MQTT v5

from mosq_test_helper import *

def do_test():
    connect_packet = mqtt_packets.gen_connect("02-subpub-qos1-nolocal", proto_ver=5)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

    mid = 530
    subscribe_packet = mqtt_packets.gen_subscribe(mid, "02/subpub/qos1/nolocal/qos1", 1 | mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL, proto_ver=5)
    suback_packet = mqtt_packets.gen_suback(mid, 1, proto_ver=5)

    mid = 531
    subscribe2_packet = mqtt_packets.gen_subscribe(mid, "02/subpub/qos1/nolocal/receive", 1, proto_ver=5)
    suback2_packet = mqtt_packets.gen_suback(mid, 1, proto_ver=5)

    mid = 300
    publish_packet = mqtt_packets.gen_publish("02/subpub/qos1/nolocal/qos1", qos=1, mid=mid, payload="message", proto_ver=5)
    puback_packet = mqtt_packets.gen_puback(mid, proto_ver=5)

    mid = 301
    publish2_packet = mqtt_packets.gen_publish("02/subpub/qos1/nolocal/receive", qos=1, mid=mid, payload="success", proto_ver=5)
    puback2_packet = mqtt_packets.gen_puback(mid, proto_ver=5)

    mid = 1
    publish3_packet = mqtt_packets.gen_publish("02/subpub/qos1/nolocal/receive", qos=1, mid=mid, payload="success", proto_ver=5)


    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        mosq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback2")

        mosq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")
        sock.send(publish2_packet)

        mosq_test.receive_unordered(sock, puback2_packet, publish3_packet, "puback2/publish3")

        sock.close()



if __name__ == '__main__':
    do_test()
