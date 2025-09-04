#!/usr/bin/env python3

from mosq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("plugin c/plugin_evt_client_offline.so\n")
        f.write("allow_anonymous true\n")


def do_test():
    rc = 1
    connect_packet = mosq_test.gen_connect("plugin-evt-subscribe", proto_ver=4, clean_session=False)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=4)

    publish_packet = mosq_test.gen_publish("evt/client/offline", qos=0, payload="plugin-evt-subscribe")

    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port, use_conf=True)

    try:
        sub_sock = mosq_test.sub_helper(port, '#')

        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        sock.close()

        mosq_test.expect_packet(sub_sock, "publish", publish_packet)
        rc = 0

        sub_sock.close()
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
