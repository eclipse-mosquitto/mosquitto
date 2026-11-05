#!/usr/bin/env python3

# Does a bridge respect the remote receive-maximum when the remote broker
# sends a PUBACK for the bridge connection state notification?
#
# The notification is sent directly on connect and is not tracked as an
# inflight message, so its PUBACK must not release send quota. If it does, the
# bridge sends more messages than the remote allows. Issue #3275.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig, MQTTBridgeConfig
from mosquitto_broker import MosquittoBroker

mosq_test.require_features(["INC_BRIDGE_SUPPORT"])


def recv_exact(sock, length):
    data = b""
    while len(data) < length:
        chunk = sock.recv(length - len(data))
        if len(chunk) == 0:
            raise BrokenPipeError("connection closed")
        data += chunk
    return data


def read_packet(sock):
    cmd, = struct.unpack("!B", recv_exact(sock, 1))
    rl = 0
    multiplier = 1
    while True:
        byte, = struct.unpack("!B", recv_exact(sock, 1))
        rl += (byte & 127) * multiplier
        multiplier *= 128
        if byte & 128 == 0:
            break
    return cmd, recv_exact(sock, rl)


def read_publish_v5(sock):
    cmd, body = read_packet(sock)
    if cmd & 0xF0 != 0x30:
        raise mosq_test.TestError(f"Expected PUBLISH, got command 0x{cmd:02X}")
    tlen, = struct.unpack("!H", body[0:2])
    topic = body[2:2+tlen].decode("utf-8")
    mid, = struct.unpack("!H", body[2+tlen:4+tlen])
    return topic, mid


def do_test():
    proto_ver = 5
    remote_clientid = "bridge-quota-test"
    notification_topic = f"$SYS/broker/connection/{remote_clientid}/state"

    properties = mqtt5_props.gen_uint16_prop(mqtt5_props.RECEIVE_MAXIMUM, 1)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver, properties=properties, property_helper=False)

    (port1, port2) = mosq_test.get_port(2)

    ssock = mosq_test.listen_sock(port1)

    broker_config = BrokerConfig(
        listeners = [ ListenerConfig(port=port2) ],
        bridges = [
            MQTTBridgeConfig(
                connection="bridge-quota-test",
                remote_clientid=remote_clientid,
                address=f"localhost:{port1}",
                topics=["bridge/# out 1"],
                cleansession=True,
                notifications=True,
                restart_timeout=5,
                try_private=False,
                bridge_attempt_unsubscribe=False,
                bridge_protocol_version="mqttv50",
                bridge_max_topic_alias=0,
            ),
        ],
        allow_anonymous=True,
    )
    broker = MosquittoBroker(config=broker_config)
    with broker:
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        cmd, _ = read_packet(bridge)
        if cmd != 0x10:
            raise mosq_test.TestError(f"Expected CONNECT, got command 0x{cmd:02X}")
        bridge.send(connack_packet)

        # Connection state notification, sent outside of the inflight queue
        topic, notification_mid = read_publish_v5(bridge)
        if topic != notification_topic:
            raise mosq_test.TestError(f"Expected notification, got {topic}")

        # Queue two messages for the bridge
        connect_packet = mqtt_packets.gen_connect("pub", proto_ver=proto_ver)
        local_connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)
        sock = mosq_test.do_client_connect(connect_packet, local_connack_packet, port=port2)
        for mid in (1, 2):
            publish_packet = mqtt_packets.gen_publish("bridge/quota", qos=1, mid=mid, payload="message", proto_ver=proto_ver)
            puback_packet = mqtt_packets.gen_puback(mid, proto_ver=proto_ver)
            mosq_test.do_send_receive(sock, publish_packet, puback_packet, f"local puback {mid}")

        # Receive-maximum is 1, so only the first message may be in flight
        topic, mid1 = read_publish_v5(bridge)
        if topic != "bridge/quota":
            raise mosq_test.TestError(f"Expected bridge/quota, got {topic}")

        # Acknowledging the notification must not allow the second message out
        bridge.send(mqtt_packets.gen_puback(notification_mid, proto_ver=proto_ver))
        bridge.settimeout(1)
        try:
            cmd, _ = read_packet(bridge)
            raise mosq_test.TestError(f"Receive-maximum exceeded, got command 0x{cmd:02X}")
        except socket.timeout:
            pass
        bridge.settimeout(20)

        # Acknowledging the first message releases the second
        bridge.send(mqtt_packets.gen_puback(mid1, proto_ver=proto_ver))
        topic, mid2 = read_publish_v5(bridge)
        if topic != "bridge/quota":
            raise mosq_test.TestError(f"Expected bridge/quota, got {topic}")
        bridge.send(mqtt_packets.gen_puback(mid2, proto_ver=proto_ver))

        sock.close()
        bridge.close()

    ssock.close()


do_test()
