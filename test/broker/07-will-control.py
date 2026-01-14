#!/usr/bin/env python3

# Test whether a client setting a will with $CONTROL in is denied

from mosq_test_helper import *


def do_test(start_broker, proto_ver):
    rc = 1
    mid = 1
    connect_packet = mosq_test.gen_connect("will", will_topic="$CONTROL/dynamic-security/v1", will_payload=b"will-message", proto_ver=proto_ver)

    port = mosq_test.get_port()
    if start_broker:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.client_connect_only(port=port)
        sock.send(connect_packet)
        d = sock.recv(1)
        if d == b"":
            rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    except Exception as e:
        print(e)
    finally:
        if start_broker:
            mosq_test.terminate_broker(broker)
            broker.wait()
            if rc:
                print(mosq_test.broker_log(broker))
                exit(rc)
        else:
            return rc


def all_tests(start_broker=False):
    rc = do_test(start_broker, proto_ver=4)
    if rc:
        return rc;
    rc = do_test(start_broker, proto_ver=5)
    if rc:
        return rc;
    return 0

if __name__ == '__main__':
    all_tests(True)
