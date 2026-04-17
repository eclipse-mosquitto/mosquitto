#!/usr/bin/env python3

# Test whether an anonymous connection is correctly denied.

from mosq_test_helper import *

def write_config1(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config2(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous true\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config3(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous true\n")

def write_config4(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config5(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous true\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config6(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous true\n")

def write_config7(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous false\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config8(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous false\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous true\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous false\n")

def write_config9(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous false\n")
        f.write("listener %d\n" % (port1))
        f.write("listener_allow_anonymous false\n")
        f.write("listener %d\n" % (port2))
        f.write("listener_allow_anonymous true\n")


def do_test(write_config, expect_success1, expect_success2):
    port1, port2 = mosq_test.get_port(2)
    if write_config is not None:
        conf_file = os.path.basename(__file__).replace('.py', '.conf')
        write_config(conf_file, port1, port2)

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port1)

    try:
        for proto_ver in [4, 5]:
            rc = 1
            connect_packet = mosq_test.gen_connect(f"connect-anon-test-{proto_ver}-{expect_success1}-{expect_success2}", proto_ver=proto_ver)

            if proto_ver == 5:
                connack_packet_success = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)
                connack_packet_rejected = mosq_test.gen_connack(rc=mqtt5_rc.NOT_AUTHORIZED, proto_ver=proto_ver, properties=None)
            else:
                connack_packet_success = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)
                connack_packet_rejected = mosq_test.gen_connack(rc=5, proto_ver=proto_ver)


            if expect_success1:
                sock = mosq_test.do_client_connect(connect_packet, connack_packet_success, port=port1)
            else:
                sock = mosq_test.do_client_connect(connect_packet, connack_packet_rejected, port=port1)
            sock.close()

            if expect_success2:
                sock = mosq_test.do_client_connect(connect_packet, connack_packet_success, port=port2)
            else:
                sock = mosq_test.do_client_connect(connect_packet, connack_packet_rejected, port=port2)
            sock.close()
            rc = 0
    except mosq_test.TestError:
        pass
    finally:
        if write_config is not None:
            os.remove(conf_file)
        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        if rc:
            print(mosq_test.broker_log(broker))
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(write_config=write_config1, expect_success1=False, expect_success2=False)
do_test(write_config=write_config2, expect_success1=True, expect_success2=False)
do_test(write_config=write_config3, expect_success1=False, expect_success2=True)
do_test(write_config=write_config4, expect_success1=False, expect_success2=False)
do_test(write_config=write_config5, expect_success1=True, expect_success2=False)
do_test(write_config=write_config6, expect_success1=False, expect_success2=True)
do_test(write_config=write_config7, expect_success1=False, expect_success2=False)
do_test(write_config=write_config8, expect_success1=True, expect_success2=False)
do_test(write_config=write_config9, expect_success1=False, expect_success2=True)
exit(0)
