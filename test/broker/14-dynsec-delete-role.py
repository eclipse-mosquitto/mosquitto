#!/usr/bin/env python3

# Check that deleting a role cleans up all references to it, no matter whether
# the role was assigned to a group/client on creation or via a modify command.

from mosq_test_helper import *
import json
import shutil

mosq_test.require_features(["WITH_CONTROL", "WITH_PLUGINS", "WITH_PLUGIN_DYNAMIC_SECURITY", "WITH_TLS"])

def write_config(filename, port):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port))
        f.write("allow_anonymous true\n")
        f.write(f"plugin {mosq_paths.plugin_dynamic_security}\n")
        f.write("plugin_opt_config_file %d/dynamic-security.json\n" % (port))

port = mosq_test.get_port()
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port)

command_topic="$CONTROL/dynamic-security/v1"

def role_command(command, rolename, correlation):
    cmd = {"command": command, "rolename": rolename, "correlationData": correlation}
    if command == "createRole":
        cmd["acls"] = []
    return {"commands": [cmd]}

def role_response(command, correlation):
    return {'responses': [{'command': command, 'correlationData': correlation}]}

create_roles = ["role_one", "role_two", "role_three", "role_four"]

# Group with roles assigned on creation
create_group_with_roles_command = { "commands": [{
    "command": "createGroup", "groupname": "group_create",
    "textname": "Name", "textdescription": "Description",
    "roles":[{'rolename':'role_one', 'priority':5}, {'rolename':'role_two'}],
    "correlationData": "10" }]
}
create_group_with_roles_response = {'responses': [{'command': 'createGroup', 'correlationData': '10'}]}

# Group with roles assigned via modifyGroup
create_group_modify_command = { "commands": [{
    "command": "createGroup", "groupname": "group_modify",
    "textname": "Name", "textdescription": "Description",
    "correlationData": "11" }]
}
create_group_modify_response = {'responses': [{'command': 'createGroup', 'correlationData': '11'}]}

modify_group_command1 = { "commands": [{
    "command": "modifyGroup", "groupname": "group_modify",
    "roles":[{'rolename':'role_three'}],
    "correlationData": "12" }]
}
modify_group_response1 = {'responses': [{'command': 'modifyGroup', 'correlationData': '12'}]}

# Replace the roles, so role_three must no longer reference the group
modify_group_command2 = { "commands": [{
    "command": "modifyGroup", "groupname": "group_modify",
    "roles":[{'rolename':'role_four'}],
    "correlationData": "13" }]
}
modify_group_response2 = {'responses': [{'command': 'modifyGroup', 'correlationData': '13'}]}

# Client with roles assigned on creation
create_client_command = { "commands": [{
    "command": "createClient", "username": "user_one",
    "password": "password", "clientid": "cid",
    "textname": "Name", "textdescription": "Description",
    "roles":[{'rolename':'role_one', 'priority':5}, {'rolename':'role_two'}],
    "correlationData": "14" }]
}
create_client_response = {'responses': [{'command': 'createClient', 'correlationData': '14'}]}

def get_group_command(groupname):
    return {"commands": [{"command": "getGroup", "groupname": groupname}]}

def get_group_response(groupname, roles):
    return {'responses':[{'command': 'getGroup', 'data': {'group': {'groupname': groupname,
        'textname': 'Name', 'textdescription': 'Description',
        'clients': [],
        'roles': roles}}}]}

get_client_command = {"commands": [{"command": "getClient", "username": "user_one"}]}

def get_client_response(roles):
    return {'responses':[{'command': 'getClient', 'data': {'client': {'username': 'user_one', 'clientid': 'cid',
        'textname': 'Name', 'textdescription': 'Description',
        'roles': roles,
        'groups': [],
        'connections': []
        }}}]}

delete_group_command = { "commands": [{"command": "deleteGroup", "groupname": "group_modify", "correlationData": "20"}]}
delete_group_response = {'responses': [{'command': 'deleteGroup', 'correlationData': '20'}]}

delete_client_command = { "commands": [{"command": "deleteClient", "username": "user_one", "correlationData": "21"}]}
delete_client_response = {'responses': [{'command': 'deleteClient', 'correlationData': '21'}]}


rc = 1
connect_packet = mqtt_packets.gen_connect("ctrl-test", username="admin", password="admin")
connack_packet = mqtt_packets.gen_connack(rc=0)

mid = 2
subscribe_packet = mqtt_packets.gen_subscribe(mid, "$CONTROL/dynamic-security/#", 1)
suback_packet = mqtt_packets.gen_suback(mid, 1)

try:
    os.mkdir(str(port))
    shutil.copyfile(str(Path(__file__).resolve().parent / "dynamic-security-init.json"), "%d/dynamic-security.json" % (port))
except FileExistsError:
    pass

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

try:
    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    # Create roles
    for i, rolename in enumerate(create_roles):
        mosq_test.do_send_receive_command(sock, command_topic,
                role_command("createRole", rolename, str(i)),
                role_response("createRole", str(i)), msg=f"create {rolename}")

    # Create group with roles, group with roles added by modify, client with roles
    mosq_test.do_send_receive_command(sock, command_topic, create_group_with_roles_command, create_group_with_roles_response)
    mosq_test.do_send_receive_command(sock, command_topic, create_group_modify_command, create_group_modify_response)
    mosq_test.do_send_receive_command(sock, command_topic, modify_group_command1, modify_group_response1)
    mosq_test.do_send_receive_command(sock, command_topic, modify_group_command2, modify_group_response2)
    mosq_test.do_send_receive_command(sock, command_topic, create_client_command, create_client_response)

    # Check the initial state
    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_create"),
            get_group_response("group_create", [{'rolename':'role_one', 'priority':5}, {'rolename':'role_two'}]), msg="get group_create 1")
    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_modify"),
            get_group_response("group_modify", [{'rolename':'role_four'}]), msg="get group_modify 1")
    mosq_test.do_send_receive_command(sock, command_topic, get_client_command,
            get_client_response([{'rolename':'role_one', 'priority':5}, {'rolename':'role_two'}]), msg="get client 1")

    # Delete the roles that were assigned on creation
    mosq_test.do_send_receive_command(sock, command_topic,
            role_command("deleteRole", "role_one", "30"), role_response("deleteRole", "30"), msg="delete role_one")

    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_create"),
            get_group_response("group_create", [{'rolename':'role_two'}]), msg="get group_create 2")
    mosq_test.do_send_receive_command(sock, command_topic, get_client_command,
            get_client_response([{'rolename':'role_two'}]), msg="get client 2")

    mosq_test.do_send_receive_command(sock, command_topic,
            role_command("deleteRole", "role_two", "31"), role_response("deleteRole", "31"), msg="delete role_two")

    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_create"),
            get_group_response("group_create", []), msg="get group_create 3")
    mosq_test.do_send_receive_command(sock, command_topic, get_client_command,
            get_client_response([]), msg="get client 3")

    # Delete the role that was assigned by modifyGroup
    mosq_test.do_send_receive_command(sock, command_topic,
            role_command("deleteRole", "role_four", "32"), role_response("deleteRole", "32"), msg="delete role_four")

    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_modify"),
            get_group_response("group_modify", []), msg="get group_modify 2")

    # role_three was replaced in group_modify and then the group is deleted, so
    # deleting role_three must not touch the deleted group.
    mosq_test.do_send_receive_command(sock, command_topic, delete_group_command, delete_group_response)
    mosq_test.do_send_receive_command(sock, command_topic,
            role_command("deleteRole", "role_three", "33"), role_response("deleteRole", "33"), msg="delete role_three")

    # Deleting a client must not leave references behind in its roles
    mosq_test.do_send_receive_command(sock, command_topic, delete_client_command, delete_client_response)

    # Kill broker and restart, checking the config was saved and is loadable.
    broker.terminate()
    broker_terminate_rc = 0
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        broker_terminate_rc = 1
    broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=port)

    sock = mosq_test.do_client_connect(connect_packet, connack_packet, timeout=5, port=port)
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    mosq_test.do_send_receive_command(sock, command_topic, get_group_command("group_create"),
            get_group_response("group_create", []), msg="get group_create 4")

    rc = broker_terminate_rc

    sock.close()
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    try:
        os.remove(f"{port}/dynamic-security.json")
        pass
    except FileNotFoundError:
        pass
    os.rmdir(f"{port}")
    mosq_test.terminate_broker(broker)
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    if rc:
        print(mosq_test.broker_log(broker))

exit(rc)
