#!/usr/bin/env python3

from mosq_test_helper import *

mosq_test.require_features(["INC_BRIDGE_SUPPORT", "WITH_TLS"])

source_dir = Path(__file__).resolve().parent
ssl_dir = source_dir.parent / "ssl"

def pub_helper(port):
    connect_packet = mosq_test.gen_connect("test-helper")
    connack_packet = mosq_test.gen_connack(rc=0)
    publish_packet = mosq_test.gen_publish("bridge/ssl/test", qos=0, payload="message")
    disconnect_packet = mosq_test.gen_disconnect()
    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port, connack_error="helper connack")
    sock.send(publish_packet)
    sock.send(disconnect_packet)
    sock.close()

def write_config(filename, address, port1, port2):
    with open(filename, 'w') as f:
        f.write(f"listener {port2}\n")
        f.write("allow_anonymous true\n")
        f.write("\n")
        f.write("connection bridge_test\n")
        f.write(f"address {address}:{port1}\n")
        f.write("topic bridge/# both 0\n")
        f.write("notifications false\n")
        f.write("restart_timeout 2\n")
        f.write("\n")
        f.write(f"bridge_cafile {ssl_dir}/all-ca.crt\n")
        f.write("bridge_insecure true\n")

def do_test(address):
    (port1, port2) = mosq_test.get_port(2)
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, address, port1, port2)

    rc = 1
    client_id = socket.gethostname()+".bridge_test"
    connect_packet = mosq_test.gen_connect(client_id, clean_session=False, proto_ver=128+4)
    connack_packet = mosq_test.gen_connack(rc=0)

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, "bridge/#", 0)
    suback_packet = mosq_test.gen_suback(mid, 0)

    publish_packet = mosq_test.gen_publish("bridge/ssl/test", qos=0, payload="message")

    ssock = mosq_test.listen_sock(port1, f"{ssl_dir}/all-ca.crt", f"{ssl_dir}/server-san.crt", f"{ssl_dir}/server-san.key")

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

    try:
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        mosq_test.expect_packet(bridge, "connect", connect_packet)
        bridge.send(connack_packet)

        mosq_test.expect_packet(bridge, "subscribe", subscribe_packet)
        bridge.send(suback_packet)

        pub_helper(port2)

        mosq_test.expect_packet(bridge, "publish", publish_packet)
        rc = 0

        bridge.close()
    except mosq_test.TestError:
        pass
    finally:
        os.remove(conf_file)
        try:
            bridge.close()
        except NameError:
            pass

        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        ssock.close()
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)

do_test("127.0.0.1")
do_test(mosq_test.get_non_loopback_ip()) # tests non-matching certificate hostname with bridge_insecure
