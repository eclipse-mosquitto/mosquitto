#!/usr/bin/env python3

# Test whether a client sends a correct PUBLISH to a topic with QoS 0, using
# the mosquitto_publish_multiple helper function.

# The client should connect to port 1888 with keepalive=60, clean session set,
# and client id publish-helper-multiple-qos0-test
# The test will send a CONNACK message to the client with rc=0. Upon receiving
# the CONNACK and verifying that rc=0, the client should send five PUBLISH messages
# to topic "publish-helper-multiple/qos0/test/$i" with payload "message" and QoS=0.
# If rc!=0, the client should exit with an error.
# After sending the PUBLISH messages, the client should send a DISCONNECT message.

from mosq_test_helper import *

port = mosq_test.get_lib_port()

rc = 1
keepalive = 60
connect_packet = mosq_test.gen_connect("publish-helper-multiple-qos0-test", keepalive=keepalive, clean_session=True)
connack_packet = mosq_test.gen_connack(rc=0)

publish_packets = []
for i in range(0, 5):
    publish_packets.append(mosq_test.gen_publish("publish-helper-multiple/qos0/test/%d" % i, qos=0, payload="message"))

disconnect_packet = mosq_test.gen_disconnect()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.settimeout(10)
sock.bind(('', port))
sock.listen(5)

client_args = sys.argv[1:]
env = dict(os.environ)
env['LD_LIBRARY_PATH'] = '../../lib:../../lib/cpp'
try:
    pp = env['PYTHONPATH']
except KeyError:
    pp = ''
env['PYTHONPATH'] = '../../lib/python:'+pp
client = mosq_test.start_client(filename=sys.argv[1].replace('/', '-'), cmd=client_args, env=env, port=port)

try:
    (conn, address) = sock.accept()
    conn.settimeout(10)

    mosq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")

    for i in range(0, 2):
        mosq_test.expect_packet(conn, "publish", publish_packets[i])

    # Disconnect client. It should reconnect and send the remainder
    # of the messages
    conn.close()
    (conn, address) = sock.accept()

    mosq_test.do_receive_send(conn, connect_packet, connack_packet, "connect")

    for i in range(2, 5):
        mosq_test.expect_packet(conn, "publish", publish_packets[i])

    mosq_test.expect_packet(conn, "disconnect", disconnect_packet)
    rc = 0

    conn.close()
except mosq_test.TestError:
    pass
finally:
    client.terminate()
    client.wait()
    if rc:
        (stdo, stde) = client.communicate()
        print(stde)
    sock.close()

exit(rc)
