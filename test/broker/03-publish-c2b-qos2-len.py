#!/usr/bin/env python3

# Check whether the broker handles a v5 PUBREL with all combinations
# of with/without reason code and properties.

from mosq_test_helper import *

def do_test(test, pubrel_packet):
    mid = 3265
    connect_packet = mqtt_packets.gen_connect("03-c2b-qos2-len", clean_session=False, proto_ver=5)
    connack_packet = mqtt_packets.gen_connack(flags=0, rc=0, proto_ver=5)

    mid = 1
    publish_packet = mqtt_packets.gen_publish("03/c2b/qos2/len/test", qos=2, mid=mid, payload="len-message", proto_ver=5)
    pubrec_packet = mqtt_packets.gen_pubrec(mid)
    pubcomp_packet = mqtt_packets.gen_pubcomp(mid)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)

        mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec")
        mosq_test.do_send_receive(sock, pubrel_packet, pubcomp_packet, "pubcomp")

        mosq_test.do_ping(sock)
        sock.close()


if __name__ == '__main__':
    # No reason code, no properties
    pubrel_packet = mqtt_packets.gen_pubrel(1)
    do_test("qos2 len 2", pubrel_packet)

    # Reason code, no properties
    pubrel_packet = mqtt_packets.gen_pubrel(1, proto_ver=5, reason_code=0x00)
    do_test("qos2 len 3", pubrel_packet)

    # Reason code, empty properties
    pubrel_packet = mqtt_packets.gen_pubrel(1, proto_ver=5, reason_code=0x00, properties="")
    do_test("qos2 len 4", pubrel_packet)

    # Reason code, one property
    props = mqtt5_props.gen_string_pair_prop(mqtt5_props.USER_PROPERTY, "key", "value")
    pubrel_packet = mqtt_packets.gen_pubrel(1, proto_ver=5, reason_code=0x00, properties=props)
    do_test("qos2 len >5", pubrel_packet)
