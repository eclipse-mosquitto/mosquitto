#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.

from mosq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write(f"listener {port}\n")
        f.write("allow_anonymous true\n")
        f.write("allow_duplicate_messages false\n")

def do_test(start_broker, proto_ver):
    rc = 1
    connect_packet = mosq_test.gen_connect("subpub-qos1-test", proto_ver=proto_ver)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe1_packet = mosq_test.gen_subscribe(mid, "subpub/+/topic", 0, proto_ver=proto_ver)
    suback1_packet = mosq_test.gen_suback(mid, 0, proto_ver=proto_ver)

    mid = 2
    subscribe2_packet = mosq_test.gen_subscribe(mid, "subpub/topic/+", 0, proto_ver=proto_ver)
    suback2_packet = mosq_test.gen_suback(mid, 0, proto_ver=proto_ver)

    publish_packet = mosq_test.gen_publish("subpub/topic/topic", qos=0, payload="message", proto_ver=proto_ver)

    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)

    if start_broker:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)

        mosq_test.do_send_receive(sock, subscribe1_packet, suback1_packet, "suback 1")
        mosq_test.do_send_receive(sock, subscribe2_packet, suback2_packet, "suback 2")

        sock.send(publish_packet)
        mosq_test.expect_packet(sock, "publish 1", publish_packet)
        mosq_test.do_ping(sock)
        rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    finally:
        if start_broker:
            broker.terminate()
            if mosq_test.wait_for_subprocess(broker):
                print("broker not terminated")
                if rc == 0: rc=1
            os.remove(conf_file)
            if rc:
                (stdo, stde) = broker.communicate()
                print(stde.decode('utf-8'))
                print("proto_ver=%d" % (proto_ver))
                exit(rc)
        else:
            return rc


def all_tests(start_broker=False):
    rc = do_test(start_broker, proto_ver=4)
    if rc:
        return rc;

if __name__ == '__main__':
    all_tests(True)
