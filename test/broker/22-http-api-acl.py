#!/usr/bin/env python3

from mosq_test_helper import *
import http.client
import json
import os
import re

if os.environ.get('WITH_HTTP_API') != 'yes':
    exit(77)

def write_config(filename, mqtt_port, http_port):
    with open(filename, 'w') as f:
        f.write(f"allow_anonymous true\n")
        f.write(f"listener {mqtt_port}\n")

        f.write(f"listener {http_port}\n")
        f.write("protocol http_api\n")
        f.write(f"plugin {mosq_test.get_build_root()}/plugins/acl-file/mosquitto_acl_file.so\n")
        f.write(f"plugin_opt_acl_file {http_port}.acl\n")

mqtt_port, http_port = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, mqtt_port, http_port)

with open(f"{http_port}.acl", "wt") as f:
    f.write("topic read /api/v1/version")

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=mqtt_port)

rc = 1

try:
    http_conn = http.client.HTTPConnection(f"localhost:{http_port}")

    # systree API
    http_conn.request("GET", "/api/v1/systree")
    response = http_conn.getresponse()
    if response.status != 401:
        raise ValueError(f"/api/v1/systree {response.status}")
    payload = response.read().decode('utf-8')
    if payload != "Not authorised\n":
        raise ValueError(f"Error: {payload}")

    # Version API
    http_conn.request("GET", "/api/v1/version")
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: /api/v1/version {response.status}")
    payload = response.read().decode('utf-8')
    if not re.match(r'^\d+\.\d+\.\d+.*$', payload):
        raise ValueError(f"Error: /api/v1/version\n{payload}")


    rc = 0
except mosq_test.TestError:
    pass
except Exception as e:
    print(e)
finally:
    os.remove(conf_file)
    os.remove(f"{http_port}.acl")
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc != 0:
        print(stde.decode('utf-8'))
        rc = 1


exit(rc)
