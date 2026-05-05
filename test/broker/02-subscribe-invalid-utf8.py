#!/usr/bin/env python3

# Test whether a SUBSCRIBE to a topic with an invalid UTF-8 topic fails

from mosq_test_helper import *

def do_test(port, proto_ver):
    mid = 53
    connect_packet = mqtt_packets.gen_connect("subscribe-invalid-utf8", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    subscribe_packet = mqtt_packets.gen_subscribe(mid, "invalid/utf8", 0, proto_ver=proto_ver)
    b = list(struct.unpack("B"*len(subscribe_packet), subscribe_packet))
    b[13] = 0 # Topic should never have a 0x0000
    subscribe_packet = struct.pack("B"*len(b), *b)


    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
    if proto_ver == 4:
        try:
            mosq_test.do_send_receive(sock, subscribe_packet, b"", "suback")
        except BrokenPipeError:
            pass
    else:
        disconnect_packet = mqtt_packets.gen_disconnect(proto_ver=5, reason_code = mqtt5_rc.MALFORMED_PACKET)
        mosq_test.do_send_receive(sock, subscribe_packet, disconnect_packet, "suback")

    sock.close()


if __name__ == '__main__':
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        do_test(port, proto_ver=4)
        do_test(port, proto_ver=5)
