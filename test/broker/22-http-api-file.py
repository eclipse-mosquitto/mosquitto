#!/usr/bin/env python3

from mosq_test_helper import *
import http.client
import json

def write_config(filename, mqtt_port, http_port):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write(f"listener {mqtt_port}\n")
        f.write(f"listener {http_port} 127.0.0.1\n")
        f.write("protocol http_api\n")
        f.write(f"http_dir ./\n")

def write_index():
    with open("index.html", 'w') as f:
        f.write("<html></html>")

mqtt_port, http_port = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, mqtt_port, http_port)
write_index()

broker = mosq_test.start_broker(filename=os.path.basename(__file__), use_conf=True, port=mqtt_port)

rc = 1

try:
    http_conn = http.client.HTTPConnection(f"localhost:{http_port}")

    # Bad request
    http_conn.request("POST", "/post")
    response = http_conn.getresponse()
    if response.status != 405:
        raise ValueError(f"Error: /post {response.status}")

    # Bad request
    http_conn.request("PUT", "/put")
    response = http_conn.getresponse()
    if response.status != 405:
        raise ValueError(f"Error: /put {response.status}")

    # Missing file
    http_conn.request("GET", "/missing")
    response = http_conn.getresponse()
    if response.status != 404:
        raise ValueError(f"Error: /api/missing {response.status}")

    # File not in dir
    http_conn.request("GET", "../../../../../../../../etc/passwd")
    response = http_conn.getresponse()
    if response.status != 404:
        raise ValueError(f"Error: ../../../../../../../../etc/passwd {response.status}")

    # Present file
    http_conn.request("GET", "/index.html")
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: /index.html {response.status}")

    # Root
    http_conn.request("GET", "/")
    response = http_conn.getresponse()
    if response.status != 200:
        raise ValueError(f"Error: / {response.status}")
    payload = response.read().decode('utf-8')
    if payload != "<html></html>":
        raise ValueError(f"Error: / {payload}")


    rc = 0
except mosq_test.TestError:
    pass
except Exception as e:
    print(e)
finally:
    os.remove(conf_file)
    os.remove("index.html")
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    if rc:
        print(mosq_test.broker_log(broker))
        rc = 1


exit(rc)
