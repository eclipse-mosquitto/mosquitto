#!/usr/bin/env python3

from mosq_test_helper import *
import http.client
import json
import os
import ssl

if os.environ.get('WITH_TLS') != 'yes' or os.environ.get('WITH_HTTP_API') != 'yes':
    exit(77)

def write_config(filename, mqtt_port, http_port):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write(f"listener {mqtt_port}\n")
        f.write(f"listener 0 {mqtt_port}.sock\n")
        f.write(f"certfile {ssl_dir}/server.crt\n")
        f.write(f"keyfile {ssl_dir}/server.key\n")

        f.write(f"listener {http_port}\n")
        f.write("protocol http_api\n")
        f.write(f"certfile {ssl_dir}/server.crt\n")
        f.write(f"keyfile {ssl_dir}/server.key\n")

mqtt_port, http_port = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, mqtt_port, http_port)

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=mqtt_port)

rc = 1

try:
    context = ssl.create_default_context()
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_verify_locations(cafile=f"{ssl_dir}/all-ca.crt")
    http_conn = http.client.HTTPSConnection(f"localhost:{http_port}", context=context)

    # Bad request type
    http_conn.request("POST", "/api/badrequest")
    response = http_conn.getresponse()
    if response.status != 405:
        raise ValueError(f"Error: /api/badrequest {response.status}")

    # Missing API
    http_conn.request("GET", "/api/missing")
    response = http_conn.getresponse()
    if response.status != 404:
        raise ValueError(f"Error: /api/missing {response.status}")

    # Listeners API
    http_conn.request("GET", "/api/v1/listeners")
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: /api/v1/listeners {response.status}")
    payload = json.loads(response.read().decode('utf-8'))
    expected_payload = {
        "listeners": [{
            "port": mqtt_port,
            "protocol": "mqtt",
            "tls": False,
            "mtls": False,
            "allow_anonymous": True
        }, {
            "path": f"{mqtt_port}.sock",
            "protocol": "mqtt",
            "tls": True,
            "mtls": False,
            "allow_anonymous": True
        }, {
            "port": http_port,
            "protocol": "httpapi",
            "tls": True,
            "mtls": False,
            "allow_anonymous": True
       }]
    }
    if payload != expected_payload:
        raise ValueError(f"Error: /api/v1/listeners payload {payload}")

    # systree API
    http_conn.request("GET", "/api/v1/systree")
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: /api/v1/systree {response.status}")
    payload = json.loads(response.read().decode('utf-8'))

    for topic in [
            '$SYS/broker/clients/maximum',
            '$SYS/broker/connections/socket/count',
            '$SYS/broker/heap/current',
            '$SYS/broker/heap/maximum',
            '$SYS/broker/messages/received',
            '$SYS/broker/bytes/received',
            '$SYS/broker/messages/stored',
            '$SYS/broker/retained messages/count',
            '$SYS/broker/store/messages/bytes',
            '$SYS/broker/uptime']:

        # Protect against values being slightly different by
        # setting to a known value
        # This read will fail if the key doesn't already exist
        if payload[topic] >= 0:
            payload[topic] = -1


    expected_payload = {
        '$SYS/broker/clients/total': 0,
        '$SYS/broker/clients/maximum': -1,
        '$SYS/broker/clients/disconnected': 0,
        '$SYS/broker/clients/connected': 0,
        '$SYS/broker/clients/expired': 0,
        '$SYS/broker/messages/stored': -1,
        '$SYS/broker/store/messages/bytes': -1,
        '$SYS/broker/subscriptions/count': 0,
        '$SYS/broker/shared_subscriptions/count': 0,
        '$SYS/broker/retained messages/count': -1,
        '$SYS/broker/heap/current': -1,
        '$SYS/broker/heap/maximum': -1,
        '$SYS/broker/messages/received': -1,
        '$SYS/broker/messages/sent': 0,
        '$SYS/broker/bytes/received': -1,
        '$SYS/broker/bytes/sent': 0,
        '$SYS/broker/publish/bytes/received': 0,
        '$SYS/broker/publish/bytes/sent': 0,
        '$SYS/broker/packet/out/count': 0,
        '$SYS/broker/packet/out/bytes': 0,
        '$SYS/broker/connections/socket/count': -1,
        '$SYS/broker/publish/messages/dropped': 0,
        '$SYS/broker/publish/messages/received': 0,
        '$SYS/broker/publish/messages/sent': 0,
        '$SYS/broker/uptime': -1
    }
    if payload != expected_payload:
        raise ValueError(f"Error: /api/v1/systree payload\n{payload}\n{expected_payload}")

    rc = 0
except mosq_test.TestError:
    pass
except Exception as e:
    print(e)
finally:
    os.remove(conf_file)
    try:
        os.remove(f"{mqtt_port}.sock")
    except FileNotFoundError:
        pass
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc != 0:
        print(stde.decode('utf-8'))
        rc = 1


exit(rc)
