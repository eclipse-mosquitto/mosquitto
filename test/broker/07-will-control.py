#!/usr/bin/env python3

# Test whether a client setting a will with $CONTROL in is denied

from mosq_test_helper import *


def do_test(proto_ver):
    mid = 1
    connect_packet = mqtt_packets.gen_connect("will", will_topic="$CONTROL/dynamic-security/v1", will_payload=b"will-message", proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        sock = mosq_test.client_connect_only(port=port)
        sock.send(connect_packet)
        d = sock.recv(1)
        if d != b"":
            raise ValueError(d)
        sock.close()


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
