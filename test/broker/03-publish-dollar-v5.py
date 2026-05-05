#!/usr/bin/env python3

# Test whether a PUBLISH to $ topics QoS 1 results in the expected PUBACK packet.

from mosq_test_helper import *

mid = 1
def helper(port, topic, reason_code):
    global mid

    connect_packet = mqtt_packets.gen_connect("03-publish-dollar-v5", proto_ver=5)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)
    publish_packet = mqtt_packets.gen_publish(topic, qos=1, mid=mid, payload="message", proto_ver=5)

    if reason_code == 0:
        puback_packet = mqtt_packets.gen_puback(mid, proto_ver=5)
    else:
        puback_packet = mqtt_packets.gen_puback(mid, proto_ver=5, reason_code=reason_code)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
    mosq_test.do_send_receive(sock, publish_packet, puback_packet, "puback%d"%(mid))
    mid += 1


def do_test():
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        helper(port, "$SYS/broker/uptime", mqtt5_rc.NOT_AUTHORIZED)
        helper(port, "$SYS/broker/connection/me", mqtt5_rc.NOT_AUTHORIZED)
        helper(port, "$SYS/broker/connection/me/state", mqtt5_rc.NO_MATCHING_SUBSCRIBERS)
        helper(port, "$share/share/03/publish/dollar/v5/topic", mqtt5_rc.NOT_AUTHORIZED)


if __name__ == '__main__':
    do_test()
