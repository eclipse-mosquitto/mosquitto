#!/usr/bin/env python3

# Test whether a PUBREC with reason code >= 0x80 is handled correctly

from mosq_test_helper import *

def helper(port):
    connect_packet = mosq_test.gen_connect("test-helper", keepalive=60)
    connack_packet = mosq_test.gen_connack(rc=0)

    mid = 1
    publish_1_packet = mosq_test.gen_publish("qos2/pubrec/rejected", qos=2, mid=mid, payload="rejected-message")
    pubrec_1_packet = mosq_test.gen_pubrec(mid)
    pubrel_1_packet = mosq_test.gen_pubrel(mid)
    pubcomp_1_packet = mosq_test.gen_pubcomp(mid)

    mid = 2
    publish_2_packet = mosq_test.gen_publish("qos2/pubrec/accepted", qos=2, mid=mid, payload="accepted-message")
    pubrec_2_packet = mosq_test.gen_pubrec(mid)
    pubrel_2_packet = mosq_test.gen_pubrel(mid)
    pubcomp_2_packet = mosq_test.gen_pubcomp(mid)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, connack_error="helper connack", port=port)

    mosq_test.do_send_receive(sock, publish_1_packet, pubrec_1_packet, "helper pubrec")
    mosq_test.do_send_receive(sock, pubrel_1_packet, pubcomp_1_packet, "helper pubcomp")

    mosq_test.do_send_receive(sock, publish_2_packet, pubrec_2_packet, "helper pubrec")
    mosq_test.do_send_receive(sock, pubrel_2_packet, pubcomp_2_packet, "helper pubcomp")
    sock.close()


def do_test(proto_ver):
    rc = 1
    keepalive = 60
    connect_packet = mosq_test.gen_connect("pub-qo2-timeout-test", keepalive=keepalive, proto_ver=proto_ver)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, "qos2/pubrec/+", 2, proto_ver=proto_ver)
    suback_packet = mosq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 1
    publish_1_packet = mosq_test.gen_publish("qos2/pubrec/rejected", qos=2, mid=mid, payload="rejected-message", proto_ver=proto_ver)
    pubrec_1_packet = mosq_test.gen_pubrec(mid, proto_ver=proto_ver, reason_code=0x80)

    mid = 2
    publish_2_packet = mosq_test.gen_publish("qos2/pubrec/accepted", qos=2, mid=mid, payload="accepted-message", proto_ver=proto_ver)
    pubrec_2_packet = mosq_test.gen_pubrec(mid, proto_ver=proto_ver)
    pubrel_2_packet = mosq_test.gen_pubrel(mid, proto_ver=proto_ver)
    pubcomp_2_packet = mosq_test.gen_pubcomp(mid, proto_ver=proto_ver)

    port = mosq_test.get_port()
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        helper(port)

        # Should have now received a publish command
        mosq_test.expect_packet(sock, "publish 1", publish_1_packet)
        sock.send(pubrec_1_packet)

        mosq_test.expect_packet(sock, "publish 2", publish_2_packet)
        mosq_test.do_send_receive(sock, pubrec_2_packet, pubrel_2_packet, "pubrel 2")
        sock.send(pubcomp_2_packet)
        rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(proto_ver=5)
exit(0)
