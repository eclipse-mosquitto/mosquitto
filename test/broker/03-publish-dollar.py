#!/usr/bin/env python3

# Test whether a PUBLISH to a topic starting with $ succeeds

from mosq_test_helper import *

def do_test():
    mid = 19
    connect_packet = mqtt_packets.gen_connect("pub-dollar-test")
    connack_packet = mqtt_packets.gen_connack(rc=0)

    publish_packet = mqtt_packets.gen_publish("$test/test", qos=1, mid=mid, payload="message")
    puback_packet = mqtt_packets.gen_puback(mid)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")
        sock.close()


if __name__ == '__main__':
    do_test()
