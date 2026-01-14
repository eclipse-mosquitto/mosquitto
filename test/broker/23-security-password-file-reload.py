#!/usr/bin/env python3

from mosq_test_helper import *

def write_config_default(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write(f"password_file {port}.password\n")
        f.write("allow_anonymous true\n")


def write_config_plugin(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write(f"plugin {mosq_plugins.PASSWORD_FILE_PLUGIN_PATH}\n")
        f.write(f"plugin_opt_password_file {port}.password\n")
        f.write("allow_anonymous true\n")


def do_test(write_config_func):
    port = mosq_test.get_port()
    conf_file = os.path.basename(__file__).replace('.py', '.conf')
    write_config_func(conf_file, port)

    rc = 1
    connect_packet = mosq_test.gen_connect("password-change-test")
    connack_packet = mosq_test.gen_connack(rc=0)

    with open(f"{port}.password", "wt") as f:
        f.write("test:$7$1000$97ozvObcN5zP4MGzYUw4uRp8+mPQbThrHOX69vdHHNVwV4iZf2K2X23FS7weilZMKeV+9oLHdilybmpXcFApYg==$WlM0jUhsiQNQJe4IDt5K1rmtAdaenWGdntswJmDkp74W9pdrt/+RdIK3YaJ09o3pD1xbtokXq933bQh+CrjA4Q==\n")

    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    try:
        sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=port)
        sock.close()

        with open(f"{port}.password", "wt") as f:
            f.write("test:bad\n")

        mosq_test.reload_broker(broker)
        # Broker should terminate
        if mosq_test.wait_for_subprocess(broker) == 0 and broker.returncode == 3:
            rc = 0
    except mosq_test.TestError:
        pass
    except Exception as err:
        print(err)
    finally:
        os.remove(conf_file)
        os.remove(f"{port}.password")
        mosq_test.terminate_broker(broker)
        if rc:
            print(mosq_test.broker_log(broker))
            exit(rc)

do_test(write_config_default)
do_test(write_config_plugin)
