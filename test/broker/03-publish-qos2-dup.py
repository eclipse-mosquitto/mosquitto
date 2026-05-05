#!/usr/bin/env python3

from mosq_test_helper import *

def do_test(proto_ver):
    connect_packet = mqtt_packets.gen_connect("03-pub-qos2-dup-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    publish_packet = mqtt_packets.gen_publish("topic", qos=2, mid=mid, payload="message", proto_ver=proto_ver, dup=1)
    pubrec_packet = mqtt_packets.gen_pubrec(mid, proto_ver=proto_ver)

    disconnect_packet = mqtt_packets.gen_disconnect(reason_code=130, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec 1")
        mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec 2")
        if proto_ver == 5:
            mosq_test.do_send_receive(sock, publish_packet, disconnect_packet, "disconnect")
        else:
            try:
                mosq_test.do_send_receive(sock, publish_packet, b"", "disconnect1")
            except BrokenPipeError:
                pass

        sock.close()


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
