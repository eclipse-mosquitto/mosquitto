#!/usr/bin/env python3

# Test whether a will topic with invalid UTF-8 fails

from mosq_test_helper import *

def do_test(proto_ver):
    connect_packet = mqtt_packets.gen_connect("will-invalid-utf8", will_topic="will/invalid/utf8", proto_ver=proto_ver)

    b = list(struct.unpack("B"*len(connect_packet), connect_packet))
    b[40] = 0 # Topic should never have a 0x0000
    connect_packet = struct.pack("B"*len(b), *b)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        rc = 1
        try:
            sock = mosq_test.do_client_connect(connect_packet, b"", timeout=30, port=port)
            sock.close()
        except BrokenPipeError:
            rc = 0
        assert rc == 0


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
