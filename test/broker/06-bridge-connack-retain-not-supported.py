#!/usr/bin/env python3

# Does a bridge honour retain-available

from mosq_test_helper import *

mosq_test.require_features(["INC_BRIDGE_SUPPORT"])

def write_config(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port2))
        f.write("allow_anonymous true\n")
        f.write("retain_available false\n")
        f.write("\n")
        f.write("connection bridge_sample\n")
        f.write("address localhost:%d\n" % (port1))
        f.write("bridge_protocol_version mqttv50\n")
        f.write("topic \"bridge with space/#\" both 1\n")
        f.write("bridge_max_topic_alias 0\n")
        f.write("restart_timeout 2\n")

def do_test():
    (port1, port2) = mosq_test.get_port(2)
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port1, port2)

    rc = 1
    hostname = socket.gethostname()
    client_id = hostname+".bridge_sample"
    connect_packet_retain = mosq_test.gen_connect(client_id, clean_session=False, proto_ver=5, will_topic=f"$SYS/broker/connection/{hostname}.bridge_sample/state", will_payload=b"0", will_qos=1, will_retain=True)
    connect_packet_no_retain = mosq_test.gen_connect(client_id, clean_session=False, proto_ver=5, will_topic=f"$SYS/broker/connection/{hostname}.bridge_sample/state", will_payload=b"0", will_qos=1, will_retain=False)

    props = mqtt5_props.gen_byte_prop(mqtt5_props.RETAIN_AVAILABLE, 0)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5, properties=props)

    mid = 1
    opts = mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL | mqtt5_opts.MQTT_SUB_OPT_RETAIN_AS_PUBLISHED

    subscribe_packet = mosq_test.gen_subscribe(mid, "bridge with space/#", 1 | opts, proto_ver=5)
    suback_packet = mosq_test.gen_suback(mid, 1, proto_ver=5)

    publish_packet = mosq_test.gen_publish("bridge with space/retain/test", qos=0, retain=True, payload="message", proto_ver=5)


    helper_connect_packet = mosq_test.gen_connect("helper", clean_session=True, proto_ver=5)
    helper_connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)
    helper_publish_packet = mosq_test.gen_publish("bridge with space/retain/test", qos=0, retain=True, payload="message", proto_ver=5)

    ssock = mosq_test.listen_sock(port1)

    try:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

        # Connect with a will with retain set, get refused
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)
        mosq_test.expect_packet(bridge, "connect", connect_packet_retain)
        bridge.send(connack_packet)
        bridge.close()

        # Now retry without retain
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)
        mosq_test.expect_packet(bridge, "connect", connect_packet_no_retain)
        bridge.send(connack_packet)
        bridge.close()

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

do_test()

exit(0)
