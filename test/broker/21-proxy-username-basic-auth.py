#!/usr/bin/env python3

# Test whether a client on a PROXY protocol listener that sets
# use_identity_as_username or use_subject_as_username only skips the password
# check when its username is the certificate CN from the PROXY v2 header.

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig
from matchers import Contains
from mosquitto_broker import MosquittoBroker
from proxy_helper import *
import struct

mosq_test.require_features(["WITH_TLS", "WITH_WEBSOCKETS", "WITH_WEBSOCKETS_BUILTIN"])

V1_HEADER = b"PROXY TCP4 192.0.2.5 127.0.0.1 6275 1234\r\n"
V2_ADDRESSES = b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00"

def v2_ssl_tlv(common_name):
    # Client flags 0x05: connected over TLS and presented a certificate.
    # Verify 0: the certificate was verified.
    value = b"\x05" + b"\x00\x00\x00\x00" + b"\x21\x00\x07TLSv1.3"
    value += b"\x22" + struct.pack("!H", len(common_name)) + common_name
    return b"\x20" + struct.pack("!H", len(value)) + value

def write_pwfile(filename):
    with open(filename, 'w') as f:
        # Username "admin", password test
        f.write('admin:$6$njERlZMi/7DzNB9E$iiavfuXvUm8iyDZArTy7smTxh07GXXOrOsqxfW6gkOYVXHGk+W+i/8d3xDxrMwEPygEBhoA8A/gjQC0N2M4Lkw==\n')

def do_test(proxy_version, proxy_data, listener_options, username, password, rc_v4, rc_v5, expect_log=None):
    pw_file = os.path.basename(__file__).replace('.py', '.pwfile')

    for proto_ver in [4, 5]:
        write_pwfile(pw_file)
        connect_packet = mqtt_packets.gen_connect("proxy-test", keepalive=42, username=username, password=password, proto_ver=proto_ver)
        if proto_ver == 5:
            properties = b"" if rc_v5 == 0 else None
            connack_packet = mqtt_packets.gen_connack(rc=rc_v5, proto_ver=proto_ver, properties=properties)
        else:
            connack_packet = mqtt_packets.gen_connack(rc=rc_v4, proto_ver=proto_ver)

        port = mosq_test.get_port()
        broker_config = BrokerConfig(
            listeners=[ ListenerConfig(port=port, enable_proxy_protocol=proxy_version, **listener_options) ],
            allow_anonymous=False,
            password_file=pw_file,
            log_type="all",
        )
        broker = MosquittoBroker(config=broker_config)
        broker.add_extra_file(pw_file)
        with broker:
            if proxy_version == 1:
                sock = do_proxy_v1_connect(port, proxy_data)
            else:
                sock = do_proxy_v2_connect(port, PROXY_VER, PROXY_CMD_PROXY, PROXY_FAM_IPV4 | PROXY_PROTO_TCP, proxy_data)
            mosq_test.do_send_receive(sock, connect_packet, connack_packet, "connack")
            if rc_v4 == 0:
                mosq_test.do_ping(sock)
            sock.close()
        if expect_log is not None:
            broker.check_log(Contains(expect_log))


v2_identity = {"use_identity_as_username": True, "require_certificate": False}
v2_identity_cert = {"use_identity_as_username": True, "require_certificate": True}

# PROXY v2 without a certificate: the username comes from the CONNECT packet,
# so the password is checked.
do_test(2, V2_ADDRESSES, v2_identity, "admin", None, 5, mqtt5_rc.NOT_AUTHORIZED)
do_test(2, V2_ADDRESSES, v2_identity, "admin", "wrong", 5, mqtt5_rc.NOT_AUTHORIZED)
do_test(2, V2_ADDRESSES, v2_identity, "admin", "test", 0, 0, "u'admin'")

# PROXY v2 with a certificate that require_certificate does not ask for: the
# CONNECT username still applies.
do_test(2, V2_ADDRESSES + v2_ssl_tlv(b"ppppppp"), v2_identity, "admin", None, 5, mqtt5_rc.NOT_AUTHORIZED)

# PROXY v1 carries no certificate, so the username always comes from the
# CONNECT packet.
for options in [
        {"use_subject_as_username": True, "require_certificate": True},
        {"use_subject_as_username": True, "require_certificate": False},
        {"use_identity_as_username": True, "require_certificate": False},
    ]:
    do_test(1, V1_HEADER, options, "admin", None, 5, mqtt5_rc.NOT_AUTHORIZED)
    do_test(1, V1_HEADER, options, "admin", "test", 0, 0, "u'admin'")

# PROXY v2 with a verified certificate: the CN is the username and no password
# is needed. The CONNECT username and password are ignored.
do_test(2, V2_ADDRESSES + v2_ssl_tlv(b"ppppppp"), v2_identity_cert, None, None, 0, 0, "u'ppppppp'")
do_test(2, V2_ADDRESSES + v2_ssl_tlv(b"ppppppp"), v2_identity_cert, "admin", "wrong", 0, 0, "u'ppppppp'")

# PROXY v2 with a verified certificate and an empty CN: there is no identity.
do_test(2, V2_ADDRESSES + v2_ssl_tlv(b""), v2_identity_cert, "admin", None, 4, mqtt5_rc.BAD_USERNAME_OR_PASSWORD)
