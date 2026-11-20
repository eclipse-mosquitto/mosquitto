#!/usr/bin/env python3

# Check that a PROXY v2 header that arrives over more than one read is
# reassembled correctly.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from matchers import Contains
from mosquitto_broker import MosquittoBroker
from proxy_helper import *
import socket
import time

mosq_test.require_features(["WITH_WEBSOCKETS", "WITH_WEBSOCKETS_BUILTIN"])

connect_packet = mqtt_packets.gen_connect("proxy-test", keepalive=42, clean_session=False, proto_ver=5)
connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

port = mosq_test.get_port()
broker_config = BrokerConfig(
    listeners = [
        ListenerConfig(
            port=port,
            enable_proxy_protocol=2
        )
    ],
    allow_anonymous=True,
    log_type="all",
)
broker = MosquittoBroker(config=broker_config)
with broker:
    data = b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00"
    proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a"
    proxy_header += bytes([PROXY_VER | PROXY_CMD_PROXY, PROXY_FAM_IPV4 | PROXY_PROTO_TCP, 0, len(data)])

    # Send the address block in two parts, so the broker has to read it in two goes
    sock = do_connect(port, proxy_header + data[0:6])
    time.sleep(0.5)
    sock.send(data[6:])
    mosq_test.do_send_receive(sock, connect_packet, connack_packet, "connack")
    mosq_test.do_ping(sock)
    sock.close()

broker.check_log(Contains("New client connected from 192.0.2.5:6275"))
