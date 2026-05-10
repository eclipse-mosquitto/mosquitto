#!/usr/bin/env python3

# Does a bridge honour retain-available

from mosq_test_helper import *

from broker_config import BrokerConfig, ListenerConfig, MQTTBridgeConfig
from mosquitto_broker import MosquittoBroker

mosq_test.require_features(["INC_BRIDGE_SUPPORT"])

def do_test():
    hostname = socket.gethostname()
    client_id = hostname+".bridge_sample"
    connect_packet_retain = mqtt_packets.gen_connect(client_id, clean_session=False, proto_ver=5, will_topic=f"$SYS/broker/connection/{hostname}.bridge_sample/state", will_payload=b"0", will_qos=1, will_retain=True)
    connect_packet_no_retain = mqtt_packets.gen_connect(client_id, clean_session=False, proto_ver=5, will_topic=f"$SYS/broker/connection/{hostname}.bridge_sample/state", will_payload=b"0", will_qos=1, will_retain=False)

    props = mqtt5_props.gen_byte_prop(mqtt5_props.RETAIN_AVAILABLE, 0)
    connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5, properties=props)

    mid = 1
    opts = mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL | mqtt5_opts.MQTT_SUB_OPT_RETAIN_AS_PUBLISHED

    subscribe_packet = mqtt_packets.gen_subscribe(mid, "bridge with space/#", 1 | opts, proto_ver=5)
    suback_packet = mqtt_packets.gen_suback(mid, 1, proto_ver=5)

    publish_packet = mqtt_packets.gen_publish("bridge with space/retain/test", qos=0, retain=True, payload="message", proto_ver=5)


    helper_connect_packet = mqtt_packets.gen_connect("helper", clean_session=True, proto_ver=5)
    helper_connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)
    helper_publish_packet = mqtt_packets.gen_publish("bridge with space/retain/test", qos=0, retain=True, payload="message", proto_ver=5)

    (port1, port2) = mosq_test.get_port(2)

    ssock = mosq_test.listen_sock(port1)

    broker_config = BrokerConfig(
        listeners = [ ListenerConfig(port=port2) ],
        bridges = [
            MQTTBridgeConfig(
                connection="bridge_sample",
                address=f"localhost:{port1}",
                bridge_protocol_version="mqttv50",
                topics=["\"bridge with space/#\" both 1"],
                bridge_max_topic_alias=0,
                restart_timeout=2,
            ),
        ],
        allow_anonymous=True,
        retain_available=False,
    )
    broker = MosquittoBroker(config=broker_config)
    with broker:
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

do_test()
