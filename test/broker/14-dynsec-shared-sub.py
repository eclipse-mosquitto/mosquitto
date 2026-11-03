#!/usr/bin/env python3

# Check that subscribe and unsubscribe rules also work for shared
# subscriptions. For "$share/<name>/<filter>", the rules must be checked
# against <filter>, not against the full string.

from mosq_test_helper import *
from dynsec_helper import *
import json
import shutil

mosq_test.require_features(["WITH_CONTROL", "WITH_PLUGINS", "WITH_PLUGIN_DYNAMIC_SECURITY", "WITH_TLS"])

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous false\n")
        f.write(f"plugin {mosq_paths.plugin_dynamic_security}\n")
        f.write(f"plugin_opt_config_file {Path(str(port), 'dynamic-security.json')}\n")


port = mosq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

setup_command = {"commands":[
    { "command": "createClient", "username": "user_one", "password": "password", "clientid": "cid" },
    { "command": "createRole", "rolename": "myrole", "allowwildcardsubs": True },
    { "command": "addRoleACL", "rolename": "myrole", "acltype": "subscribePattern", "topic": "secret/#", "allow": False },
    { "command": "addRoleACL", "rolename": "myrole", "acltype": "subscribePattern", "topic": "public/#", "allow": True },
    { "command": "addRoleACL", "rolename": "myrole", "acltype": "unsubscribePattern", "topic": "secret/#", "allow": False },
    { "command": "addClientRole", "username": "user_one", "rolename": "myrole" },
    { "command": "setDefaultACLAccess", "acls": [{ "acltype": "subscribe", "allow": True }] }
    ]}

setup_response = {'responses': [
    {'command': 'createClient'},
    {'command': 'createRole'},
    {'command': 'addRoleACL'},
    {'command': 'addRoleACL'},
    {'command': 'addRoleACL'},
    {'command': 'addClientRole'},
    {'command': 'setDefaultACLAccess'}
    ]}

default_deny_command = {"commands":[
    { "command": "setDefaultACLAccess", "acls": [{ "acltype": "subscribe", "allow": False }] }
    ]}
default_deny_response = {'responses': [{'command': 'setDefaultACLAccess'}]}

rc = 1
connect_packet_admin = mqtt_packets.gen_connect("ctrl-test", username="admin", password="admin")
connack_packet_admin = mqtt_packets.gen_connack(rc=0)

mid = 2
subscribe_packet_admin = mqtt_packets.gen_subscribe(mid, "$CONTROL/dynamic-security/#", 1)
suback_packet_admin = mqtt_packets.gen_suback(mid, 1)

connect_packet = mqtt_packets.gen_connect("cid", username="user_one", password="password", proto_ver=5)
connack_packet = mqtt_packets.gen_connack(rc=0, proto_ver=5)

def check_subscribe(sock, mid, topic, allowed):
    subscribe_packet = mqtt_packets.gen_subscribe(mid, topic, 0, proto_ver=5)
    if allowed:
        suback_packet = mqtt_packets.gen_suback(mid, 0, proto_ver=5)
    else:
        suback_packet = mqtt_packets.gen_suback(mid, mqtt5_rc.NOT_AUTHORIZED, proto_ver=5)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, f"suback {topic}")

def check_unsubscribe(sock, mid, topic, allowed):
    unsubscribe_packet = mqtt_packets.gen_unsubscribe(mid, topic, proto_ver=5)
    if allowed:
        unsuback_packet = mqtt_packets.gen_unsuback(mid, mqtt5_rc.NO_SUBSCRIPTION_EXISTED, proto_ver=5)
    else:
        unsuback_packet = mqtt_packets.gen_unsuback(mid, mqtt5_rc.NOT_AUTHORIZED, proto_ver=5)
    mosq_test.do_send_receive(sock, unsubscribe_packet, unsuback_packet, f"unsuback {topic}")

try:
    os.mkdir(str(port))
    shutil.copyfile(str(Path(__file__).resolve().parent / "dynamic-security-init.json"), "%d/dynamic-security.json" % (port))
except FileExistsError:
    pass

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = mosq_test.do_client_connect(connect_packet_admin, connack_packet_admin, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet_admin, suback_packet_admin, "suback")

    command_check(sock, setup_command, setup_response)

    csock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)

    # Default: subscribe is allowed. "secret/#" is denied.
    check_subscribe(csock, 3, "secret/x", False)
    check_subscribe(csock, 4, "$share/g/secret/x", False)
    check_subscribe(csock, 5, "$share/g/secret/#", False)
    check_subscribe(csock, 6, "other/x", True)
    check_subscribe(csock, 7, "$share/g/other/x", True)

    # Unsubscribe from "secret/#" is denied.
    check_unsubscribe(csock, 8, "secret/x", False)
    check_unsubscribe(csock, 9, "$share/g/secret/x", False)

    # Default: subscribe is denied. "public/#" is allowed.
    command_check(sock, default_deny_command, default_deny_response)
    check_subscribe(csock, 10, "public/x", True)
    check_subscribe(csock, 11, "$share/g/public/x", True)
    check_subscribe(csock, 12, "$share/g/other/x", False)

    csock.close()
    rc = 0

    sock.close()
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    try:
        os.remove(f"{port}/dynamic-security.json")
    except FileNotFoundError:
        pass
    shutil.rmtree(f"{port}")
    mosq_test.terminate_broker(broker)
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    if rc:
        print(mosq_test.broker_log(broker))


exit(rc)
