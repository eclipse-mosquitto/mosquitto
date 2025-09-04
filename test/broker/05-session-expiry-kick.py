#!/usr/bin/env python3

# MQTT v5. Test whether session expiry interval works correctly.

from mosq_test_helper import *

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("plugin c/kick_last_client.so\n")
        f.write("allow_anonymous true\n")
        f.write("log_type all\n")

def do_test():
    rc = 1

    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config(conf_file, port)
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    # Test the case of connect with session-expiry>0, kick, expiry for a crash
    props = mqtt5_props.gen_uint32_prop(mqtt5_props.SESSION_EXPIRY_INTERVAL, 1)
    connect_packet = mosq_test.gen_connect("05-session-expiry", clean_session=False, proto_ver=5, properties=props)
    connack_packet = mosq_test.gen_connack(flags=0, rc=0, proto_ver=5)

    try:
        sock = mosq_test.client_connect_only(port=port)
        sock.send(connect_packet)
        # Immediately disconnect, this should now be queued to expire, but the plugin should kick it first
        sock.close()

        time.sleep(2)

        # This should succeed if the broker is still online
        # The "session present" flag must *not* be set
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port, connack_error="connack 1")
        sock.close()

        rc = 0
    except mosq_test.TestError:
        pass
    finally:
        mosq_test.terminate_broker(broker)
        os.remove(conf_file)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)


do_test()
