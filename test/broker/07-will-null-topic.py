#!/usr/bin/env python3

import struct

from mosq_test_helper import *

def do_test(start_broker, proto_ver):
    rc = 1
    connect_packet = mosq_test.gen_connect("will-null-topic", will_topic="", will_payload=struct.pack("!4sB7s", b"will", 0, b"message"), proto_ver=proto_ver)

    if proto_ver == 5:
        connack_packet = mosq_test.gen_connack(rc=mqtt5_rc.PROTOCOL_ERROR, proto_ver=5)
    else:
        connack_packet = b""

    port = mosq_test.get_port()
    broker = None
    if start_broker:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=30, port=port)
        sock.close()
    except BrokenPipeError:
        rc = 0
    finally:
        if broker:
            mosq_test.terminate_broker(broker)
            if mosq_test.wait_for_subprocess(broker):
                print("broker not terminated")
                if rc == 0: rc=1
            if rc:
                print(mosq_test.broker_log(broker))
                print("proto_ver=%d" % (proto_ver))
    return rc


def all_tests(start_broker=False):
    rc = do_test(start_broker, proto_ver=4)
    if rc:
        return rc
    rc = do_test(start_broker, proto_ver=5)
    if rc:
        return rc
    return 0

if __name__ == '__main__':
    sys.exit(all_tests(True))
