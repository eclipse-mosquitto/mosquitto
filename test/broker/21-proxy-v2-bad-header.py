#!/usr/bin/env python3

from mosq_test_helper import *
from proxy_v2_helper import *
import json
import shutil
import socket

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("log_type all\n")
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write("enable_proxy_protocol_v2 true\n")

connect_packet = mosq_test.gen_connect("proxy-test", keepalive=42, clean_session=False, proto_ver=5)
connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

def do_test(header):
    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    rc = 1

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(("localhost", port))
        sock.send(header)
        try:
            data = sock.recv(10)
            if len(data) == 0:
                rc = 0
        except ConnectionResetError:
            rc = 0
        sock.close()
    except mosq_test.TestError:
        pass
    finally:
        os.remove(conf_file)
        broker.terminate()
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        (stdo, stde) = broker.communicate()
        if rc != 0:
            print(stde.decode('utf-8'))
            rc = 1
            raise ValueError(rc)

# Bad magic
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0b" + b"\x21\x01\x00\x0c" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Bad version
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x31\x01\x00\x0c" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Bad command
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x23\x01\x00\x0c" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Bad family (proxy command with unspecified family)
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x00\x00\x0c" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Short length IPv4
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x11\x00\x0b" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Short length IPv6
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x21\x00\x0b" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00"
do_test(proxy_header)

# Short SSL TLV
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x11\x00\x10" + b"\x00\x00\x00\x00" + b"\x00\x00\x00\x00" + b"\x00\x00" + b"\x00\x00" + b"\x20" + b"\x00\x01" + b"\x21\x00"
do_test(proxy_header)

# Too long SSL TLV for overall length
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x11\x00\x10" + b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00" \
    + b"\x20" \
    + b"\x00\x19" \
    + b"\x05" \
    + b"\x00\x00\x00\x00" \
    + b"\x21" \
    + b"\x00\x07" \
    + b"\x54\x4C\x53\x76\x31\x2E\x33" \
    + b"\x23" \
    + b"\x00\x08" \
    + b"\x70\x71\x72\x73\x74\x75\x76"
do_test(proxy_header)

# Too long SSL sub TLV for overall length
proxy_header = b"\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a" + b"\x21\x11\x00\x28" + b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00" \
    + b"\x20" \
    + b"\x00\x19" \
    + b"\x05" \
    + b"\x00\x00\x00\x00" \
    + b"\x21" \
    + b"\x00\x07" \
    + b"\x54\x4C\x53\x76\x31\x2E\x33" \
    + b"\x23" \
    + b"\x00\x0A" \
    + b"\x70\x71\x72\x73\x74\x75\x76"
do_test(proxy_header)
