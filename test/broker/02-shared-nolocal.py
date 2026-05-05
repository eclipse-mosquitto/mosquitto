#!/usr/bin/env python3

# No local option is not allowed on shared subscriptions

from mosq_test_helper import *

def do_test():
    mid = 1

    connect1_packet = mqtt_packets.gen_connect("02-shared-nolocal-client1", proto_ver=5)
    connack1_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

    subscribe_packet = mqtt_packets.gen_subscribe(mid, "$share/sharename/subpub/qos1", 1 | mqtt5_opts.MQTT_SUB_OPT_NO_LOCAL, proto_ver=5)
    disconnect_packet = mqtt_packets.gen_disconnect(reason_code=mqtt5_rc.PROTOCOL_ERROR, proto_ver=5)

    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)

    with broker:
        sock = mosq_test.do_client_connect(connect1_packet, connack1_packet, timeout=20, port=port)
        mosq_test.do_send_receive(sock, subscribe_packet, disconnect_packet, "disconnect")
        sock.close()

if __name__ == '__main__':
    do_test()
