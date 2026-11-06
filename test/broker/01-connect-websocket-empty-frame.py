#!/usr/bin/env python3

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from mosquitto_broker import MosquittoBroker


mosq_test.require_features([
    "WITH_WEBSOCKETS",
    "WITH_WEBSOCKETS_BUILTIN",
])


def send_binary_frame(sock, payload):
    frame = sock._create_frame(sock.OPCODE_BINARY, bytearray(payload))
    sock._socket.sendall(frame)


port = mosq_test.get_port()
broker_config = BrokerConfig(
    listeners=[ListenerConfig(port=port, protocol="websockets")],
    allow_anonymous=False,
)
broker = MosquittoBroker(config=broker_config)

with broker:
    sock = mosq_test.client_connect_only(port=port, protocol="websockets")
    try:
        connect_packet = mqtt_packets.gen_connect("empty-frame")
        connack_packet = mqtt_packets.gen_connack(rc=5)

        # Leave four MQTT bytes pending. A zero-length WebSocket data frame
        # carries no MQTT data and must not advance the MQTT packet position.
        send_binary_frame(sock, connect_packet[:-4])
        send_binary_frame(sock, b"")
        send_binary_frame(sock, connect_packet[-4:])
        if sock.recv(len(connack_packet)) != connack_packet:
            raise mosq_test.TestError("missing connack after empty WebSocket frame")
        if not broker.is_running():
            raise mosq_test.TestError("broker exited after empty WebSocket frame")
    finally:
        sock.close()
