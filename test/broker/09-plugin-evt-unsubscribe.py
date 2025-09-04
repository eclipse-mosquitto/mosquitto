#!/usr/bin/env python3

from mosq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write(f"plugin {mosq_plugins.gen_test_plugin_path('plugin_evt_unsubscribe')}\n")
        f.write("allow_anonymous true\n")


def do_test():
    rc = 1
    connect_packet = mosq_test.gen_connect("plugin-evt-unsubscribe", proto_ver=5)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, "unsubscribe-topic", 1, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid, 1, proto_ver=5)

    mid = 2
    unsubscribe_packet = mosq_test.gen_unsubscribe(mid, "unsubscribe-topic", proto_ver=5)
    unsuback_packet = mosq_test.gen_unsuback(mid, mqtt5_rc.NO_SUBSCRIPTION_EXISTED, proto_ver=5)

    publish_packet = mosq_test.gen_publish("unsubscribe-topic", qos=0, payload="message1", proto_ver=5)

    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port, use_conf=True)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=20, port=port)

        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        mosq_test.do_send_receive(sock, unsubscribe_packet, unsuback_packet, "unsuback")
        mosq_test.do_send_receive(sock, publish_packet, publish_packet, "publish")

        rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    finally:
        os.remove(conf_file)
        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)


do_test()
