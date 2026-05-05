#!/usr/bin/env python3

# Test whether "topic alias" works to the broker
# MQTT v5

from mosq_test_helper import *

def do_test():
    connect_packet = mqtt_packets.gen_connect("02-subpub-alias-unknown", proto_ver=5)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

    props = mqtt5_props.gen_uint16_prop(mqtt5_props.TOPIC_ALIAS, 3)
    publish1_packet = mqtt_packets.gen_publish("", qos=0, payload="message", proto_ver=5, properties=props)

    disconnect_packet = mqtt_packets.gen_disconnect(reason_code=mqtt5_rc.PROTOCOL_ERROR, proto_ver=5)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
        sock.send(publish1_packet)

        mosq_test.expect_packet(sock, "disconnect", disconnect_packet)

        sock.close()


if __name__ == '__main__':
    do_test()
