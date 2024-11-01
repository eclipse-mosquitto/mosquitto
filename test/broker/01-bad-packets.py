#!/usr/bin/env python3

# tests bad packets

from mosq_test_helper import *

rc = 0
keepalive = 10
port = mosq_test.get_port()

def do_test(clientid, send, expected):
    global rc
    connect_packet = mosq_test.gen_connect(clientid, proto_ver=5, keepalive=keepalive)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        sock.send(send)

    except mosq_test.TestError:
        pass

    finally:
        broker.terminate()
        broker.wait()
        (stdo, stde) = broker.communicate()
        stde_string = stde.decode('utf-8')
        if expected not in stde_string:
           print(clientid, 'test failed! Expected', expected)
           print(stde_string)
           rc += 1;

pingreq_packet = mosq_test.gen_pingreq()
do_test('pingreq', pingreq_packet, 'Sending PINGRESP to pingreq')
do_test('reserved-command', b"\x0a\xc0t", 'Client reserved-command disconnected due to protocol error.')
do_test('reserved-flags', b"\xc1\x00", 'Client reserved-flags disconnected due to malformed packet.')
do_test('illegal-remaining', b"\xc0\x02\0x33\0x33", 'Client illegal-remaining disconnected due to malformed packet.')


exit(rc)
