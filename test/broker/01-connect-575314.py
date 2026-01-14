#!/usr/bin/env python3

# Check for performance of processing user-property on CONNECT

from mosq_test_helper import *
from psutil import Process as ProcessInfo

def do_test():
    num_connects = 1000
    num_props = 5000
    
    rc = 1
    props = mqtt5_props.gen_string_pair_prop(mqtt5_props.USER_PROPERTY, "key", "value")
    for i in range(0, num_props):
        props += mqtt5_props.gen_string_pair_prop(mqtt5_props.USER_PROPERTY, "key", "value")
    connect_packet_slow = mosq_test.gen_connect("connect-user-property", proto_ver=5, properties=props)
    connect_packet_fast = mosq_test.gen_connect("a"*65000, proto_ver=5)
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5)

    port = mosq_test.get_port()
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

    try:
        broker_info = ProcessInfo(broker.pid)
        cpu_user_start = broker_info.cpu_times().user

        for i in range(num_connects):
            sock = mosq_test.do_client_connect(connect_packet_slow, connack_packet, port=port)
            sock.close()
        cpu_user_end = broker_info.cpu_times().user
        cpu_user_with_props = cpu_user_end - cpu_user_start
        cpu_user_start = cpu_user_end

        for i in range(num_connects):
            sock = mosq_test.do_client_connect(connect_packet_fast, connack_packet, port=port)
            sock.close()
        cpu_user_end = broker_info.cpu_times().user
        cpu_user_without_props = cpu_user_end - cpu_user_start

        # 20 is chosen as a factor that works in plain mode and running under
        # valgrind. The slow performance manifests as a factor of >100. Fast is <10.
        if cpu_user_with_props / cpu_user_without_props < 10.0:
            rc = 0
        else:
            print(f"CPU usage ratio with/without properties is {cpu_user_with_props / cpu_user_without_props}")
    except mosq_test.TestError:
        pass
    finally:
        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)


do_test()
exit(0)
