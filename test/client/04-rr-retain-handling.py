#!/usr/bin/env python3

#

from mosq_test_helper import *

def do_test():
    rc = 1

    port = mosq_test.get_port()

    env = {
        'XDG_CONFIG_HOME':'/tmp/missing'
    }
    env = mosq_test.env_add_ld_library_path(env)
    cmd = [mosq_test.get_client_path('mosquitto_rr'),
            '-p', str(port),
            '-q', '0',
            '-e', 'retain-handling',
            '-t', 'retain-handling',
            '-m', 'm',
            '-V', '5',
            ]

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(5)
    sock.bind(('', port))
    sock.listen(5)

    props = mqtt5_props.gen_uint16_prop(mqtt5_props.RECEIVE_MAXIMUM, 1)
    connect_packet = mosq_test.gen_connect("", proto_ver=5, properties=props)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    subscribe_packet_always = mosq_test.gen_subscribe(mid=1, topic="retain-handling", qos=0x00, proto_ver=5)
    subscribe_packet_new = mosq_test.gen_subscribe(mid=1, topic="retain-handling", qos=0x10, proto_ver=5)
    subscribe_packet_never = mosq_test.gen_subscribe(mid=1, topic="retain-handling", qos=0x20, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid=1, qos=0, proto_ver=5)

    props = mqtt5_props.gen_string_prop(mqtt5_props.RESPONSE_TOPIC, "retain-handling")
    publish_packet_in = mosq_test.gen_publish("retain-handling", qos=0, payload="m", proto_ver=5, properties=props)
    publish_packet_out = mosq_test.gen_publish("retain-handling", qos=0, payload="m", proto_ver=5)

    client_terminate_rc = 0

    try:
        for subscribe_packet, handling in [
                (subscribe_packet_always, 'always'),
                (subscribe_packet_new, 'new'),
                (subscribe_packet_never, 'never')]:

            client_cmd = cmd + ['--retain-handling', handling]
            client = subprocess.Popen(client_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)

            (conn, address) = sock.accept()
            conn.settimeout(5)

            mosq_test.expect_packet(conn, "connect", connect_packet)
            conn.send(connack_packet)

            mosq_test.expect_packet(conn, f"subscribe {handling}", subscribe_packet)
            conn.send(suback_packet)

            mosq_test.expect_packet(conn, "publish}", publish_packet_in)
            conn.send(publish_packet_out)

            if mosq_test.wait_for_subprocess(client):
                print("client not terminated")
                client_terminate_rc = 1
        sock.close()
    except mosq_test.TestError:
        pass
    except Exception as e:
        print(e)
    finally:
        sock.close()
        client.terminate()
        exit(client_terminate_rc)


do_test()
