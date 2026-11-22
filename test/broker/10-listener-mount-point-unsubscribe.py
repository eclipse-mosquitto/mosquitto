#!/usr/bin/env python3

# Check that UNSUBSCRIBE on a listener with a mount point removes the
# subscription that was made with the same topic filter.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from mosquitto_broker import MosquittoBroker

def helper(port, proto_ver, payload):
    connect_packet = mqtt_packets.gen_connect("10-listener-mount-unsub-helper", proto_ver=proto_ver)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

    publish_packet = mqtt_packets.gen_publish("10/listener/mount/unsub", qos=0, payload=payload, proto_ver=proto_ver)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port, connack_error="helper connack")
    sock.send(publish_packet)
    sock.close()


def do_test(proto_ver):
    mid = 1

    # Subscriber on the listener with a mount point
    connect_packet1 = mqtt_packets.gen_connect("test1", proto_ver=proto_ver)
    connack_packet1 = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)
    subscribe_packet1 = mqtt_packets.gen_subscribe(mid, "10/listener/mount/unsub", 0, proto_ver=proto_ver)
    suback_packet1 = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)
    unsubscribe_packet1 = mqtt_packets.gen_unsubscribe(mid+1, "10/listener/mount/unsub", proto_ver=proto_ver)
    unsuback_packet1 = mqtt_packets.gen_unsuback(mid+1, proto_ver=proto_ver)
    publish_packet1 = mqtt_packets.gen_publish("10/listener/mount/unsub", qos=0, payload="before", proto_ver=proto_ver)
    pingreq_packet = mqtt_packets.gen_pingreq()
    pingresp_packet = mqtt_packets.gen_pingresp()

    # Subscriber on the listener without a mount point, used to know when the
    # broker has finished processing a publish.
    connect_packet2 = mqtt_packets.gen_connect("test2", proto_ver=proto_ver)
    connack_packet2 = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)
    subscribe_packet2 = mqtt_packets.gen_subscribe(mid, "#", 0, proto_ver=proto_ver)
    suback_packet2 = mqtt_packets.gen_suback(mid, 0, proto_ver=proto_ver)
    publish_before2 = mqtt_packets.gen_publish("mount/10/listener/mount/unsub", qos=0, payload="before", proto_ver=proto_ver)
    publish_after2 = mqtt_packets.gen_publish("mount/10/listener/mount/unsub", qos=0, payload="after", proto_ver=proto_ver)

    (port1, port2) = mosq_test.get_port(2)
    broker_config = BrokerConfig(
        listeners = [
            ListenerConfig(port=port1),
            ListenerConfig(
                port=port2,
                mount_point="mount/",
            )
        ],
        allow_anonymous=True,
    )
    broker = MosquittoBroker(config=broker_config)
    with broker:
        sock1 = mosq_test.do_client_connect(connect_packet1, connack_packet1, timeout=20, port=port2)
        mosq_test.do_send_receive(sock1, subscribe_packet1, suback_packet1, "suback1")

        sock2 = mosq_test.do_client_connect(connect_packet2, connack_packet2, timeout=20, port=port1)
        mosq_test.do_send_receive(sock2, subscribe_packet2, suback_packet2, "suback2")

        # The subscription is active before the unsubscribe
        helper(port2, proto_ver, "before")
        mosq_test.expect_packet(sock1, "publish before", publish_packet1)
        mosq_test.expect_packet(sock2, "publish before 2", publish_before2)

        mosq_test.do_send_receive(sock1, unsubscribe_packet1, unsuback_packet1, "unsuback1")

        # Nothing should be delivered to sock1 after the unsubscribe
        helper(port2, proto_ver, "after")
        mosq_test.expect_packet(sock2, "publish after 2", publish_after2)
        mosq_test.do_send_receive(sock1, pingreq_packet, pingresp_packet, "pingresp")

        sock1.close()
        sock2.close()


do_test(proto_ver=4)
do_test(proto_ver=5)
