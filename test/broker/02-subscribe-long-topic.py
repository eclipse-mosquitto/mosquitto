#!/usr/bin/env python3

# Test whether a SUBSCRIBE to a topic with 65535 hierarchy characters fails
# This needs checking with MOSQ_USE_VALGRIND=1 to detect memory failures
# https://github.com/eclipse/mosquitto/issues/1412

from mosq_test_helper import *

def do_test(start_broker, proto_ver):
    rc = 1
    mid = 1
    connect_packet = mosq_test.gen_connect("subscribe-long-test", proto_ver=proto_ver)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    subscribe_packet = mosq_test.gen_subscribe(mid, "/"*65535, 0, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = None
    if start_broker:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        if proto_ver == 4:
            try:
                mosq_test.do_send_receive(sock, subscribe_packet, b"", "suback")
            except BrokenPipeError:
                rc = 0
        else:
            disconnect_packet = mosq_test.gen_disconnect(proto_ver=5, reason_code = mqtt5_rc.MALFORMED_PACKET)
            mosq_test.do_send_receive(sock, subscribe_packet, disconnect_packet, "suback")
            rc = 0

        sock.close()
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
