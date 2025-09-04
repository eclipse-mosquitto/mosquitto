#!/usr/bin/env python3

# Test whether a client sends a correct UNSUBSCRIBE packet.

from mosq_test_helper import *

def do_test(conn, data):
    connect_packet = mosq_test.gen_connect("unsubscribe-test")
    connack_packet = mosq_test.gen_connack(rc=0)

    disconnect_packet = mosq_test.gen_disconnect()

    mid = 1
    unsubscribe_packet = mosq_test.gen_unsubscribe(mid, "unsubscribe/test")
    unsuback_packet = mosq_test.gen_unsuback(mid)

    mosq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")
    mosq_test.do_receive_send(conn, unsubscribe_packet, unsuback_packet, "unsubscribe")
    mosq_test.expect_packet(conn, "disconnect", disconnect_packet)


mosq_test.client_test(Path(mosq_test.get_build_root(), "test", "lib", "c", mosq_test.get_build_type(), "02-unsubscribe.exe"), [], do_test, None)
mosq_test.client_test(Path(mosq_test.get_build_root(), "test", "lib", "cpp", mosq_test.get_build_type(), "02-unsubscribe.exe"), [], do_test, None)
