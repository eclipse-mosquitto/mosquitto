#!/usr/bin/env python3

# Does a bridge resend a QoS=1 message correctly after a disconnect?

from mosq_test_helper import *

def write_config(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write(f"listener {port2}\n")
        f.write("allow_anonymous true\n")
        f.write("connection bridge1\n")
        f.write(f"address 127.0.0.1:{port1}\n")
        f.write("topic room1/# both 2 sensor/ myhouse/\n")
        f.write("topic tst/ba both 2\n")
        f.write("topic # both 2\n")
        f.write("keepalive_interval 600\n")
        f.write("remote_clientid mosquitto\n")
        f.write("bridge_protocol_version mqttv50\n")
        f.write("notifications false\n")

def do_test(proto_ver):
    (port1, port2) = mosq_test.get_port(2)
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port1, port2)

    rc = 1
    keepalive = 600
    client_id = "mosquitto"
    properties = mqtt5_props.gen_uint16_prop(mqtt5_props.TOPIC_ALIAS_MAXIMUM, 10)
    properties += mqtt5_props.gen_uint16_prop(mqtt5_props.RECEIVE_MAXIMUM, 20)
    connect_packet = mosq_test.gen_connect(client_id, keepalive=keepalive, clean_session=False, proto_ver=proto_ver, properties=properties)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    if proto_ver == 5:
        opts = mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL | mqtt5_opts.MQTT_SUB_OPT_RETAIN_AS_PUBLISHED
    else:
        opts = 0

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, "myhouse/room1/#", 2 | opts, proto_ver=proto_ver)
    suback_packet = mosq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 2
    subscribe_packet2 = mosq_test.gen_subscribe(mid, "tst/ba", 2 | opts, proto_ver=proto_ver)
    suback_packet2= mosq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    mid = 3
    subscribe_packet3 = mosq_test.gen_subscribe(mid, "#", 2 | opts, proto_ver=proto_ver)
    suback_packet3 = mosq_test.gen_suback(mid, 2, proto_ver=proto_ver)

    ssock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ssock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    ssock.settimeout(40)
    ssock.bind(('', port1))
    ssock.listen(5)

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

    try:
        (bridge, address) = ssock.accept()
        bridge.settimeout(20)

        mosq_test.expect_packet(bridge, "connect", connect_packet)
        bridge.send(connack_packet)

        mosq_test.expect_packet(bridge, "subscribe1", subscribe_packet)
        bridge.send(suback_packet)

        mosq_test.expect_packet(bridge, "subscribe2", subscribe_packet2)
        bridge.send(suback_packet2)

        mosq_test.expect_packet(bridge, "subscribe3", subscribe_packet3)
        bridge.send(suback_packet3)

        try:
            bridge.send(bytes.fromhex("320c00062b2b2b2b2b2b00040033"))
            #bridge.send(bytes.fromhex("320c00062b2b2b2b2b2b00040033"))
            #bridge.send(bytes.fromhex("320c00062b2b2b2b2b2b00040033"))
            bridge.send(bytes.fromhex("C000")) # PING
            d = bridge.recv(1)
            if len(d) == 0:
                rc = 0
        except (ConnectionResetError, BrokenPipeError, mosq_test.TestError):
            #expected behaviour
            rc = 0

        bridge.close()
    except mosq_test.TestError:
        pass
    except Exception as e:
        print(e)
    finally:
        os.remove(conf_file)
        try:
            bridge.close()
        except NameError:
            pass

        mosq_test.terminate_broker(broker)
        broker.wait()
        ssock.close()
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)


do_test(proto_ver=5)

exit(0)
