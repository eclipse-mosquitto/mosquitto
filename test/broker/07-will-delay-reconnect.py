#!/usr/bin/env python3

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

    props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_SESSION_EXPIRY_INTERVAL, 60)
    will_props = mqtt5_props.gen_uint32_prop(mqtt5_props.PROP_WILL_DELAY_INTERVAL, 3)
    connect2a_packet = mosq_test.gen_connect("will-delay-reconnect-helper", proto_ver=5, will_topic="will/test", will_payload=b"will delay", will_properties=will_props, clean_session=False, properties=props)
    connack2a_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    connect2b_packet = mosq_test.gen_connect("will-delay-reconnect-helper", proto_ver=5, clean_session=False)
    connack2b_packet = mosq_test.gen_connack(rc=0, flags=1, proto_ver=5)

    subscribe_packet = mosq_test.gen_subscribe(mid, "will/test", 0, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid, 0, proto_ver=5)

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

        mosq_test.do_ping(sock1)
        rc = 0

        sock1.close()
        sock2.close()
    except mosq_test.TestError:
        pass
    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test()
