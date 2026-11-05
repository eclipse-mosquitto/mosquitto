#!/usr/bin/env python3

# Test retained delivery while a large outgoing PUBLISH is pending.

from mosq_test_helper import *

import socket
import mqtt5_props
def do_test(proto_ver):
    port = mosq_test.get_port()
    config = {
        "allow_anonymous": "true",
        "persistence": "false",
        "max_packet_size": 33554432,
    }

    connect_packet = mqtt_packets.gen_connect(
        "retain-qos0-busy-test", proto_ver=proto_ver
    )
    connack_properties = b""
    if proto_ver == 5:
        connack_properties = (
            mqtt5_props.gen_uint16_prop(
                mqtt5_props.TOPIC_ALIAS_MAXIMUM, 10
            )
            + mqtt5_props.gen_uint32_prop(
                mqtt5_props.MAXIMUM_PACKET_SIZE,
                config["max_packet_size"],
            )
            + mqtt5_props.gen_uint16_prop(
                mqtt5_props.RECEIVE_MAXIMUM, 20
            )
        )

    connack_packet = mqtt_packets.gen_connack(
        rc=0,
        proto_ver=proto_ver,
        properties=connack_properties,
        property_helper=False,
    )

    with MosquittoBroker(port=port, config=config):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            sock.settimeout(10)
            sock.connect(("127.0.0.1", port))
            mosq_test.do_send_receive(
                sock, connect_packet, connack_packet, "connack"
            )
            small_topic = "retain/qos0/busy/small"
            small_publish = mqtt_packets.gen_publish(
                small_topic,
                qos=0,
                payload="retained-small",
                retain=True,
                proto_ver=proto_ver,
            )

            subscribe_small = mqtt_packets.gen_subscribe(
                1, small_topic, 0, proto_ver=proto_ver
            )
            suback_small = mqtt_packets.gen_suback(
                1, 0, proto_ver=proto_ver
            )

            mosq_test.do_send(sock, small_publish)
            mosq_test.do_send_receive(
                sock, subscribe_small, suback_small, "small suback"
            )
            mosq_test.expect_packet(
                sock, "initial retained publish", small_publish
            )
            big_topic = "retain/qos0/busy/big"
            big_publish = mqtt_packets.gen_publish(
                big_topic,
                qos=0,
                payload="x" * (20 * 1024 * 1024),
                retain=True,
                proto_ver=proto_ver,
            )

            mosq_test.do_send(sock, big_publish)

            subscribe_big = mqtt_packets.gen_subscribe(
                2, big_topic, 0, proto_ver=proto_ver
            )
            suback_big = mqtt_packets.gen_suback(
                2, 0, proto_ver=proto_ver
            )
            mosq_test.do_send_receive(
                sock, subscribe_big, suback_big, "big suback"
            )

            first_byte = sock.recv(1, socket.MSG_PEEK)
            if first_byte != big_publish[:1]:
                raise AssertionError("Expected the large retained PUBLISH")
            observer_connect = mqtt_packets.gen_connect(
                "retain-qos0-busy-observer", proto_ver=proto_ver
            )
            with mosq_test.do_client_connect(
                observer_connect, connack_packet, port=port, timeout=10
            ) as observer:
                marker_topic = "retain/qos0/busy/marker"
                marker_subscribe = mqtt_packets.gen_subscribe(
                    1, marker_topic, 0, proto_ver=proto_ver
                )
                marker_suback = mqtt_packets.gen_suback(
                    1, 0, proto_ver=proto_ver
                )
                mosq_test.do_send_receive(
                    observer, marker_subscribe, marker_suback, "marker suback"
                )

                resubscribe_small = mqtt_packets.gen_subscribe(
                    3, small_topic, 0, proto_ver=proto_ver
                )
                resuback_small = mqtt_packets.gen_suback(
                    3, 0, proto_ver=proto_ver
                )
                marker_publish = mqtt_packets.gen_publish(
                    marker_topic,
                    qos=0,
                    payload="subscribe-processed",
                    proto_ver=proto_ver,
                )

                mosq_test.do_send(sock, resubscribe_small)
                mosq_test.do_send(sock, marker_publish)
                mosq_test.expect_packet(
                    observer, "processing marker", marker_publish
                )

                received = 0
                while received < len(big_publish):
                    chunk = sock.recv(min(65536, len(big_publish) - received))
                    if not chunk:
                        raise AssertionError("Connection closed during large PUBLISH")
                    if chunk != big_publish[received:received + len(chunk)]:
                        raise AssertionError("Large PUBLISH content mismatch")
                    received += len(chunk)

                mosq_test.expect_packet(
                    sock, "small resuback", resuback_small
                )
                mosq_test.expect_packet(
                    sock, "retained publish after resubscribe", small_publish
                )


if __name__ == "__main__":
    do_test(proto_ver=4)
    do_test(proto_ver=5)
