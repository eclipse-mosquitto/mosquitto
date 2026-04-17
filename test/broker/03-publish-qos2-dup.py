#!/usr/bin/env python3

from mosq_test_helper import *

def do_test(proto_ver):
    rc = 1
    connect_packet = mosq_test.gen_connect("03-pub-qos2-dup-test", proto_ver=proto_ver)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    publish_packet = mosq_test.gen_publish("topic", qos=2, mid=mid, payload="message", proto_ver=proto_ver, dup=1)
    pubrec_packet = mosq_test.gen_pubrec(mid, proto_ver=proto_ver)

    disconnect_packet = mosq_test.gen_disconnect(reason_code=130, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec 1")
        mosq_test.do_send_receive(sock, publish_packet, pubrec_packet, "pubrec 2")
        if proto_ver == 5:
            mosq_test.do_send_receive(sock, publish_packet, disconnect_packet, "disconnect")
            rc = 0
        else:
            try:
                mosq_test.do_send_receive(sock, publish_packet, b"", "disconnect1")
                rc = 0
            except BrokenPipeError:
                rc = 0

        sock.close()
    except Exception as e:
        print(e)
    except mosq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        if rc:
            print(mosq_test.broker_log(broker))
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


def all_tests():
    rc = do_test(proto_ver=4)
    if rc:
        return rc;
    rc = do_test(proto_ver=5)
    if rc:
        return rc;
    return 0

if __name__ == '__main__':
    all_tests()
