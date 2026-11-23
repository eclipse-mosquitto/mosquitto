#!/usr/bin/env python3

# Test whether a client that connected through a PROXY v2 listener with
# use_identity_as_username, and whose username is its certificate CN, stays
# connected when a SIGHUP is received.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from mosquitto_broker import MosquittoBroker
from proxy_helper import *

mosq_test.require_features(["WITH_TLS", "WITH_WEBSOCKETS", "WITH_WEBSOCKETS_BUILTIN"])

def write_pwfile(filename):
    with open(filename, 'w') as f:
        # Username "admin", password test
        f.write('admin:$6$njERlZMi/7DzNB9E$iiavfuXvUm8iyDZArTy7smTxh07GXXOrOsqxfW6gkOYVXHGk+W+i/8d3xDxrMwEPygEBhoA8A/gjQC0N2M4Lkw==\n')

def do_test(data):
    pw_file = os.path.basename(__file__).replace('.py', '.pwfile')
    write_pwfile(pw_file)

    connect_packet = mqtt_packets.gen_connect("proxy-test", keepalive=42)
    connack_packet = mqtt_packets.gen_connack(rc=0)

    port = mosq_test.get_port()
    broker_config = BrokerConfig(
        listeners=[
            ListenerConfig(
                port=port,
                enable_proxy_protocol=2,
                require_certificate=True,
                use_identity_as_username=True,
            )
        ],
        allow_anonymous=False,
        password_file=pw_file,
    )
    broker = MosquittoBroker(config=broker_config)
    broker.add_extra_file(pw_file)
    with broker:
        sock = do_proxy_v2_connect(port, PROXY_VER, PROXY_CMD_PROXY, PROXY_FAM_IPV4 | PROXY_PROTO_TCP, data)
        mosq_test.do_send_receive(sock, connect_packet, connack_packet, "connack")

        broker.reload()
        time.sleep(1)

        # This will fail if we've been disconnected
        mosq_test.do_ping(sock)
        sock.close()

# Verified certificate with CN "ppppppp"
data = b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00" \
    + b"\x20" \
    + b"\x00\x19" \
    + b"\x05" \
    + b"\x00\x00\x00\x00" \
    + b"\x21" \
    + b"\x00\x07" \
    + b"\x54\x4C\x53\x76\x31\x2E\x33" \
    + b"\x22" \
    + b"\x00\x07" \
    + b"\x70\x70\x70\x70\x70\x70\x70"
do_test(data)
