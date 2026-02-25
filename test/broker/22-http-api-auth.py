#!/usr/bin/env python3

from mosq_test_helper import *
import base64
import http.client
import json
import os
import re

if os.environ.get('WITH_TLS') != 'yes' or os.environ.get('WITH_HTTP_API') != 'yes':
    exit(77)

def write_config(filename, mqtt_port, http_port):
    with open(filename, 'w') as f:
        f.write(f"listener {mqtt_port}\n")

        f.write(f"listener {http_port}\n")
        f.write("protocol http_api\n")
        f.write(f"plugin {mosq_test.get_build_root()}/plugins/password-file/mosquitto_password_file.so\n")
        f.write("plugin_opt_password_file %s/%s\n" % (Path(__file__).resolve().parent, filename.replace('.conf', '.pwfile')))

mqtt_port, http_port = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, mqtt_port, http_port)

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=mqtt_port)

rc = 1

try:
    http_conn = http.client.HTTPConnection(f"localhost:{http_port}")

    # No auth
    http_conn.request("GET", "/api/v1/version")
    response = http_conn.getresponse()
    if response.status != 401:
        raise ValueError(f"Error: /api/v1/version {response.status}")
    payload = response.read().decode('utf-8')
    if payload != "Not authorised\n":
        raise ValueError(f"Error: {payload}")

    # Bad auth
    credentials = "user:invalid"
    encoded_credentials = base64.b64encode(credentials.encode()).decode()
    headers = {
        "Authorization": f"Basic {encoded_credentials}"
    }
    http_conn.request("GET", "/api/v1/version", headers=headers)
    response = http_conn.getresponse()
    if response.status != 401:
        raise ValueError(f"Error: /api/v1/version {response.status}")
    payload = response.read().decode('utf-8')
    if payload != "Not authorised\n":
        raise ValueError(f"Error: {payload}")

    # Good auth
    credentials = "user:password"
    encoded_credentials = base64.b64encode(credentials.encode()).decode()
    headers = {
        "Authorization": f"Basic {encoded_credentials}"
    }
    http_conn.request("GET", "/api/v1/version", headers=headers)
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: /api/v1/version {response.status}")

    rc = 0
except mosq_test.TestError:
    pass
except Exception as e:
    print(e)
finally:
    os.remove(conf_file)
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if rc != 0:
        print(stde.decode('utf-8'))
        rc = 1


exit(rc)
