#!/usr/bin/env python3

# Send the PROXY v2 address block in two TCP segments. The broker must join the
# segments in the correct order.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from matchers import Contains
from mosquitto_broker import MosquittoBroker
from proxy_helper import *
import json
import shutil
import socket

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
    data = b"\xC0\x00\x02\x05" + b"\xC6\x33\x64\x07" + b"\x18\x83" + b"\x00\x00"
    sock = do_proxy_v2_connect_split(port, PROXY_VER, PROXY_CMD_PROXY, PROXY_FAM_IPV4 | PROXY_PROTO_TCP, data, 4)
    mosq_test.do_send_receive(sock, connect_packet, connack_packet, "connack")
    mosq_test.do_ping(sock)
    sock.close()

broker.check_log(Contains("New client connected from 192.0.2.5:6275"))
