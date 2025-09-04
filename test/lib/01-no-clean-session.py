#!/usr/bin/env python3

# Test whether a client produces a correct connect with clean session not set.

# The client should connect to port 1888 with keepalive=60, clean session not
# set, and client id 01-no-clean-session.

from mosq_test_helper import *

def do_test(conn, data):
    connect_packet = mosq_test.gen_connect("01-no-clean-session", clean_session=False)

    mosq_test.expect_packet(conn, "connect", connect_packet)


mosq_test.client_test(Path(mosq_test.get_build_root(), "test", "lib", "c", mosq_test.get_build_type(), "01-no-clean-session.exe"), [], do_test, None)
mosq_test.client_test(Path(mosq_test.get_build_root(), "test", "lib", "cpp", mosq_test.get_build_type(), "01-no-clean-session.exe"), [], do_test, None)
