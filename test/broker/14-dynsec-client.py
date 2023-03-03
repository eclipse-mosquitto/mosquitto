#!/usr/bin/env python3

from mosq_test_helper import *
import json
import shutil

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write(f"plugin {mosq_test.get_build_root()}/plugins/dynamic-security/mosquitto_dynamic_security.so\n")
        f.write("plugin_opt_config_file %d/dynamic-security.json\n" % (port))

def command_check(sock, command_payload, expected_response):
    command_packet = mosq_test.gen_publish(topic="$CONTROL/dynamic-security/v1", qos=0, payload=json.dumps(command_payload))
    sock.send(command_packet)
    response = json.loads(mosq_test.read_publish(sock))
    if response != expected_response:
        print(expected_response)
        print(response)
        raise ValueError(response)



port = mosq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

add_client_command = { "commands": [{
            "command": "createClient", "username": "user_one",
            "password": "password", "clientid": "cid",
            "textname": "Name", "textdescription": "Description",
            "rolename": "", "correlationData": "2" }]
}
add_client_response = {'responses': [{'command': 'createClient', 'correlationData': '2'}]}
add_client_repeat_response = {'responses':[{"command":"createClient","error":"Client already exists", "correlationData":"2"}]}

list_clients_command = { "commands": [{
            "command": "listClients", "verbose": False, "correlationData": "10"}]
}
list_clients_response = {'responses': [{"command": "listClients", "data":{"totalCount":2, "clients":["admin", "user_one"]},"correlationData":"10"}]}

list_clients_verbose_command = { "commands": [{
            "command": "listClients", "verbose": True, "correlationData": "20"}]
}
list_clients_verbose_response = {'responses':[{"command": "listClients", "data":{"totalCount":2, "clients":[
    {'username': 'admin', 'textname': 'Dynsec admin user', 'roles': [{'rolename': 'admin'}], 'groups': [], 'connections': [{'address': '127.0.0.1'}]},
    {"username":"user_one", "clientid":"cid", "textname":"Name", "textdescription":"Description",
        "roles":[], "groups":[], 'connections': []}]}, "correlationData":"20"}]}


get_client_command = { "commands": [{
    "command": "getClient", "username": "user_one", "correlationData": "42"}]}
get_client_response = {'responses':[{'command': 'getClient', 'data': {'client': {'username': 'user_one', 'clientid': 'cid',
    'textname': 'Name', 'textdescription': 'Description', 'groups': [], 'connections': [], 'roles': []}}, "correlationData":"42"}]}

set_client_password_command = {"commands": [{
    "command": "setClientPassword", "username": "user_one", "password": "password"}]}
set_client_password_response = {"responses": [{"command":"setClientPassword"}]}

delete_client_command = { "commands": [{
            "command": "deleteClient", "username": "user_one"}]}
delete_client_response = {'responses':[{'command': 'deleteClient'}]}


rc = 1
connect_packet = mosq_test.gen_connect("ctrl-test", username="admin", password="admin")
connack_packet = mosq_test.gen_connack(rc=0)

anon_connect_packet = mosq_test.gen_connect("anon-helper")
anon_connack_packet = mosq_test.gen_connack(rc=0)

mid = 2
subscribe_packet = mosq_test.gen_subscribe(mid, "$CONTROL/dynamic-security/#", 1)
suback_packet = mosq_test.gen_suback(mid, 1)

try:
    os.mkdir(str(port))
    shutil.copyfile(str(Path(__file__).resolve().parent / "dynamic-security-init.json"), "%d/dynamic-security.json" % (port))
except FileExistsError:
    pass

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    # The anon user is used to ensure that when the commands are run they are also valid if an anon user is present.
    anon_sock = mosq_test.do_client_connect(anon_connect_packet, anon_connack_packet, timeout=5, port=port)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    # Add client
    command_check(sock, add_client_command, add_client_response)

    # List clients non-verbose
    command_check(sock, list_clients_command, list_clients_response)

    # List clients verbose
    command_check(sock, list_clients_verbose_command, list_clients_verbose_response)

    # Kill broker and restart, checking whether our changes were saved.
    broker.terminate()
    broker_terminate_rc = 0
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        broker_terminate_rc = 1
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    # Get client
    command_check(sock, get_client_command, get_client_response)

    # List clients non-verbose
    command_check(sock, list_clients_command, list_clients_response)

    # List clients verbose
    command_check(sock, list_clients_verbose_command, list_clients_verbose_response)

    # Add duplicate client
    command_check(sock, add_client_command, add_client_repeat_response)

    # Set client password
    command_check(sock, set_client_password_command, set_client_password_response)

    # Delete client
    command_check(sock, delete_client_command, delete_client_response)

    rc = broker_terminate_rc

    sock.close()
    anon_sock.close()
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    try:
        os.remove(f"{port}/dynamic-security.json")
    except FileNotFoundError:
        pass
    os.rmdir(f"{port}")
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))


exit(rc)
