#!/usr/bin/env python3

from mosq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("plugin c/plugin_evt_message_in_publish.so\n")
        f.write("allow_anonymous true\n")


def do_test():
    rc = 1
    connect_packet = mosq_test.gen_connect("plugin-evt-message-in-publish", proto_ver=5)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, "#", 2, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid, 2, proto_ver=5)

    publish_packet1 = mosq_test.gen_publish(topic="first-publish", qos=0, proto_ver=5)
    publish_packet2 = mosq_test.gen_publish(topic="second-publish", payload="second-payload", qos=1, mid=1, proto_ver=5)
    publish_packet3 = mosq_test.gen_publish(topic="third-publish", payload="third-payload", qos=1, mid=2, proto_ver=5)

    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port, use_conf=True)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)

        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

        sock.send(publish_packet1)

        mosq_test.expect_packet(sock, "publish 1", publish_packet1)
        mosq_test.expect_packet(sock, "publish 2", publish_packet2)
        mosq_test.expect_packet(sock, "publish 3", publish_packet3)
        mosq_test.do_ping(sock)

        rc = 0

        sock.close()
    except mosq_test.TestError:
        pass
    except Exception as e:
        print(e)
    finally:
        os.remove(conf_file)
        broker.terminate()
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test()
