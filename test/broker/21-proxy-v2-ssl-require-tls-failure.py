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
        f.write("proxy_protocol_v2_require_tls true\n")

def do_test(data):
    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    rc = 1

    expect_log = "Connection from 192.0.2.5:6275 rejected, client did not connect using TLS."
    try:
        sock = do_proxy_connect(port, PROXY_VER, PROXY_CMD_PROXY, PROXY_FAM_IPV4 | PROXY_PROTO_TCP, data)
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
        if rc != 0 or expect_log not in stde.decode('utf-8'):
            print(stde.decode('utf-8'))
            rc = 1
            raise ValueError(rc)

# No SSL at all
data = b"\xC0\x00\x02\x05" + b"\x00\x00\x00\x00" + b"\x18\x83" + b"\x00\x00"
do_test(data)
