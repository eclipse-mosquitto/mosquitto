#!/usr/bin/env python3

# Test whether a retained PUBLISH is cleared when a zero length retained
# message is published to a topic.

from mosq_test_helper import *


def do_test(proto_ver):
    connect_packet = mqtt_packets.gen_connect("retain-qos0-clear-test", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    publish_packet = mqtt_packets.gen_publish("retain/qos0/clear/test", qos=0, payload="retained message", retain=True, proto_ver=proto_ver)
    retain_clear_packet = mqtt_packets.gen_publish("retain/qos0/clear/test", qos=0, payload=None, retain=True, proto_ver=proto_ver)
    mid_sub = 592
    subscribe_packet = mqtt_packets.gen_subscribe(mid_sub, "retain/qos0/clear/test", 0, proto_ver=proto_ver)
    suback_packet = mqtt_packets.gen_suback(mid_sub, 0, proto_ver=proto_ver)

    mid_unsub = 593
    unsubscribe_packet = mqtt_packets.gen_unsubscribe(mid_unsub, "retain/qos0/clear/test", proto_ver=proto_ver)
    unsuback_packet = mqtt_packets.gen_unsuback(mid_unsub, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=4, port=port)
        # Send retained message
        sock.send(publish_packet)
        # Subscribe to topic, we should get the retained message back.
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        mosq_test.expect_packet(sock, "publish", publish_packet)
        # Now unsubscribe from the topic before we clear the retained
        # message.
        mosq_test.do_send_receive(sock, unsubscribe_packet, unsuback_packet, "unsuback")

        # Now clear the retained message.
        sock.send(retain_clear_packet)

        # Subscribe to topic, we shouldn't get anything back apart
        # from the SUBACK.
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        # If we do get something back, it should be before this ping, so if
        # this succeeds then we're ok.
        mosq_test.do_ping(sock)
        sock.close()


if __name__ == '__main__':
    do_test(proto_ver=4)
    do_test(proto_ver=5)
