#!/usr/bin/env python3

# Test $CONTROL/broker/v1 listListeners

from mosq_test_helper import *
import json
import shutil

def write_config(filename, ports):
    with open(filename, 'w') as f:
        f.write("enable_control_api true\n")
        f.write("allow_anonymous true\n")
        f.write("listener %d\n" % (ports[0]))
        f.write("protocol mqtt\n")
        f.write("listener %d\n" % (ports[1]))
        f.write("protocol websockets\n")
        f.write("listener %d\n" % (ports[2]))
        f.write("protocol mqtt\n")
        f.write("certfile ../ssl/server.crt\n")
        f.write("keyfile ../ssl/server.key\n")
        f.write("listener %d\n" % (ports[3]))
        f.write("protocol websockets\n")
        f.write("certfile ../ssl/server.crt\n")
        f.write("keyfile ../ssl/server.key\n")
        f.write("listener 0 17-list-listeners-mqtt.sock\n")
        f.write("protocol mqtt\n")
        f.write("listener 0 17-list-listeners-websockets.sock\n")
        f.write("protocol websockets\n")

def command_check(sock, command_payload, expected_response):
    command_packet = mosq_test.gen_publish(topic="$CONTROL/broker/v1", qos=0, payload=json.dumps(command_payload))
    sock.send(command_packet)
    response = json.loads(mosq_test.read_publish(sock))
    if response != expected_response:
        print(expected_response)
        print(response)
        raise ValueError(response)

def invalid_command_check(sock, command_payload, cmd_name, error_msg):
    command_packet = mosq_test.gen_publish(topic="$CONTROL/broker/v1", qos=0, payload=command_payload)
    sock.send(command_packet)
    response = json.loads(mosq_test.read_publish(sock))
    expected_response = {'responses': [{'command': cmd_name, 'error': error_msg}]}
    if response != expected_response:
        print(expected_response)
        print(response)
        raise ValueError(response)



ports = mosq_test.get_port(4)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, ports)

cmd_success = {"commands":[{"command": "listListeners", "correlationData": "m3CtYVnySLCOwnHzITSeowvgla0InV4G"}]}

response_success = {'responses': [{'command': 'listListeners', "correlationData": "m3CtYVnySLCOwnHzITSeowvgla0InV4G", 'data':{
    'listeners':[
        {
            'port': ports[0],
            'protocol': 'mqtt',
            'tls': False
        },{
            'port': ports[1],
            'protocol': 'mqtt+websockets',
            'tls': False
        },{
            'port': ports[2],
            'protocol': 'mqtt',
            'tls': True
        },{
            'port': ports[3],
            'protocol': 'mqtt+websockets',
            'tls': True
        },{
            'port': 0,
            'protocol': 'mqtt',
            'socket-path': '17-list-listeners-mqtt.sock',
            'tls': False
        },{
            'port': 0,
            'protocol': 'mqtt+websockets',
            'socket-path': '17-list-listeners-websockets.sock',
            'tls': False
        }
    ]}}]}

rc = 1
connect_packet = mosq_test.gen_connect("17-list-listeners")
connack_packet = mosq_test.gen_connack(rc=0)

mid = 2
subscribe_packet = mosq_test.gen_subscribe(mid, "$CONTROL/broker/#", 0)
suback_packet = mosq_test.gen_suback(mid, 0)

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=ports[0])

try:
    sock = mosq_test.do_client_connect(connect_packet, connack_packet, port=ports[0])
    mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")

    invalid_command_check(sock, "not valid json", "Unknown command", "Payload not valid JSON")
    invalid_command_check(sock, "{}", "Unknown command", "Invalid/missing commands")

    cmd = {"commands":["command"]}
    invalid_command_check(sock, json.dumps(cmd), "Unknown command", "Command not an object")

    cmd = {"commands":[{}]}
    invalid_command_check(sock, json.dumps(cmd), "Unknown command", "Missing command")

    cmd = {"commands":[{"command": "unknown command"}]}
    invalid_command_check(sock, json.dumps(cmd), "unknown command", "Unknown command")

    cmd = {"commands":[{"command": "listListeners", "correlationData": True}]}
    invalid_command_check(sock, json.dumps(cmd), "listListeners", "Invalid correlationData data type.")

    command_check(sock, cmd_success, response_success)
    mosq_test.do_ping(sock)

    rc = 0

    sock.close()
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    for f in ["17-list-listeners-mqtt.sock", "17-list-listeners-websockets.sock"]:
        try:
            os.remove(f)
        except FileNotFoundError:
            pass
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde.decode('utf-8'))


exit(rc)
