#!/usr/bin/env python3

from mosq_test_helper import *

def write_config1(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write(f"listener {port2}\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("allow_anonymous true\n")

def write_config2(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("auto_id_prefix new-\n")
        f.write(f"listener {port2}\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("allow_anonymous true\n")

def write_config3(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write(f"listener {port2}\n")
        f.write("listener_auto_id_prefix port2-\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("allow_anonymous true\n")

def write_config4(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write(f"listener {port2}\n")
        f.write("listener_auto_id_prefix port2-\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("listener_auto_id_prefix port1-\n")
        f.write("allow_anonymous true\n")

def write_config5(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("auto_id_prefix global-\n")
        f.write(f"listener {port2}\n")
        f.write("listener_auto_id_prefix port2-\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("listener_auto_id_prefix port1-\n")
        f.write("allow_anonymous true\n")

def write_config6(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("auto_id_prefix global-\n")
        f.write(f"listener {port2}\n")
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("listener_auto_id_prefix port1-\n")
        f.write("allow_anonymous true\n")


def do_test(config_func, client_port, auto_id):
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    config_func(conf_file, port1, port2)

    rc = 1
    connect_packet = mosq_test.gen_connect("", proto_ver=5)
    props = mqtt5_props.gen_string_prop(mqtt5_props.ASSIGNED_CLIENT_IDENTIFIER, f"{auto_id}xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=5, properties=props)
    # Remove the "xxxx" part - this means the front part of the packet
    # is correct (so remaining length etc. is correct), but we don't
    # need to match against the random id.
    connack_packet = connack_packet[:-44]

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port1, use_conf=True)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=client_port)
        sock.close()
        rc = 0
    except mosq_test.TestError:
        pass
    finally:
        mosq_test.terminate_broker(broker)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        os.remove(conf_file)
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)


(port1, port2) = mosq_test.get_port(2)

do_test(config_func=write_config1, client_port=port1, auto_id="auto-")
do_test(config_func=write_config1, client_port=port2, auto_id="auto-")
do_test(config_func=write_config2, client_port=port1, auto_id="new-")
do_test(config_func=write_config2, client_port=port2, auto_id="new-")
do_test(config_func=write_config3, client_port=port1, auto_id="auto-")
do_test(config_func=write_config3, client_port=port2, auto_id="port2-")
do_test(config_func=write_config4, client_port=port1, auto_id="port1-")
do_test(config_func=write_config4, client_port=port2, auto_id="port2-")
do_test(config_func=write_config5, client_port=port1, auto_id="port1-")
do_test(config_func=write_config5, client_port=port2, auto_id="port2-")
do_test(config_func=write_config6, client_port=port1, auto_id="port1-")
do_test(config_func=write_config6, client_port=port2, auto_id="global-")

exit(0)
