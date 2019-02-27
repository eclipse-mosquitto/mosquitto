#!/usr/bin/env python

# Test whether a client with a will delay handles correctly on the client reconnecting
# First connection is durable, second is clean session, and without a will, so the will should not be received.
# MQTT 5

from mosq_test_helper import *


def do_test():
    rc = 1
    keepalive = 60

    mid = 1
    connect1_packet = mosq_test.gen_connect("will-qos0-test", keepalive=keepalive, proto_ver=5)
    connack1_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_WILL_DELAY_INTERVAL, 3)
    connect2a_packet = mosq_test.gen_connect("will-helper", keepalive=keepalive, proto_ver=5, will_topic="will/test", will_payload="will delay", will_properties=props, clean_session=False)
    connack2a_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    connect2b_packet = mosq_test.gen_connect("will-helper", keepalive=keepalive, proto_ver=5, clean_session=True)
    connack2b_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    subscribe_packet = mosq_test.gen_subscribe(mid, "will/test", 0, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid, 0, proto_ver=5)

    pingreq_packet = mosq_test.gen_pingreq()
    pingresp_packet = mosq_test.gen_pingresp()

    port = mosq_test.get_port()
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock1 = mosq_test.do_client_connect(connect1_packet, connack1_packet, timeout=30, port=port)
        mosq_test.do_send_receive(sock1, subscribe_packet, suback_packet, "suback")

        sock2 = mosq_test.do_client_connect(connect2a_packet, connack2a_packet, timeout=30, port=port)
        sock2.close()

        time.sleep(1)
        sock2 = mosq_test.do_client_connect(connect2b_packet, connack2b_packet, timeout=30, port=port)
        time.sleep(3)

        # The client2 has reconnected within the original will delay interval, which has now
        # passed, but it should have been deleted anyway. Disconnect and see
        # whether we get the old will. We should not.
        sock2.close()

        mosq_test.do_send_receive(sock1, pingreq_packet, pingresp_packet, "pingresp")
        rc = 0

        sock1.close()
        sock2.close()
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde)
            exit(rc)


do_test()
