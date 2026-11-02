#!/usr/bin/env python3

# Test mosquitto_client_peer_cred(): a plugin accepts clients connected through
# a Unix socket listener (identified by their peer credentials) and refuses
# clients connected via TCP.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig, PluginConfig
from mosquitto_broker import MosquittoBroker

mosq_test.require_features(["WITH_UNIX_SOCKETS"])

try:
    s = socket.AF_UNIX
except AttributeError:
    # Not supported on Windows
    exit(77)


def do_test():
    connect_packet = mqtt_packets.gen_connect("peer-cred")
    connack_ok = mqtt_packets.gen_connack(rc=0)
    connack_refused = mqtt_packets.gen_connack(rc=5)

    port = mosq_test.get_port()
    sockpath = f"{port}.sock"
    broker_config = BrokerConfig(
        listeners = [
            ListenerConfig(port=port),
            ListenerConfig(port=0, address=sockpath),
        ],
        plugins = [ PluginConfig(path=mosq_paths.test_plugin('auth_plugin_peer_cred')) ],
        allow_anonymous=False,
    )
    broker = MosquittoBroker(config=broker_config)
    broker.add_extra_file(sockpath)
    with broker:
        # Unix socket client is identified by its credentials
        sock = mosq_test.do_client_connect_unix(connect_packet, connack_ok, path=sockpath)
        sock.close()

        # TCP client has no peer credentials and is refused
        sock = mosq_test.do_client_connect(connect_packet, connack_refused, port=port)
        sock.close()

do_test()
