#!/usr/bin/env python3

# Store two retained messages, then make the stored payloadlen of one of them
# larger than its payload. That message must be skipped on restore instead of
# reading past the end of the payload, and the other one restored as normal.

from mosq_test_helper import *
persist_help = persist_module()

port, _ = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
persist_help.write_config(conf_file, port)

rc = 1

persist_help.init(port)

proto_ver = 4
connect_packet = mqtt_packets.gen_connect("persist-payloadlen", proto_ver=proto_ver, clean_session=True)
connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=proto_ver)

publish_good_packet = mqtt_packets.gen_publish("test/good", qos=0, payload="good message", retain=True, proto_ver=proto_ver)
publish_bad_packet = mqtt_packets.gen_publish("test/bad", qos=0, payload="x", retain=True, proto_ver=proto_ver)

mid = 1
subscribe_packet = mqtt_packets.gen_subscribe(mid, "#", 0, proto_ver=proto_ver)
suback_packet = mqtt_packets.gen_suback(mid, qos=0, proto_ver=proto_ver)

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    sock.send(publish_good_packet)
    sock.send(publish_bad_packet)
    mosq_test.do_ping(sock)
    sock.close()

    (broker_terminate_rc, stde) = mosq_test.terminate_broker(broker)
    broker = None

    if persist_help.modify_base_msg_payloadlen(port, "test/bad", 1048576) != 1:
        raise ValueError("base message for test/bad not found")

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
    mosq_test.expect_packet(sock, "publish good", publish_good_packet)
    # Ping will fail if the bad message is received
    mosq_test.do_ping(sock)
    sock.close()

    (broker_terminate_rc, stde) = mosq_test.terminate_broker(broker)
    broker = None

    rc = broker_terminate_rc
finally:
    if broker is not None:
        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated (2)")
            if rc == 0: rc=1
    os.remove(conf_file)
    rc += persist_help.cleanup(port)

    if rc:
        print(mosq_test.broker_log(broker))


exit(rc)
