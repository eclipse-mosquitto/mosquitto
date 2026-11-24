#!/usr/bin/env python3

# Test whether mosquitto_rr exits correctly when its -W timeout expires
# without a response having been received.

from mosq_test_helper import *
import platform

mosq_test.require_features(["WITH_BROKER"])

if platform.system() == 'Windows':
    # -W is not supported
    exit(0)

MOSQ_ERR_TIMEOUT = 27

def do_test(port, proto_ver):
    if proto_ver == 5:
        V = 'mqttv5'
    elif proto_ver == 4:
        V = 'mqttv311'
    else:
        V = 'mqttv31'

    env = {
        'XDG_CONFIG_HOME':'/tmp/missing'
    }
    env = mosq_test.env_add_ld_library_path(env)
    cmd = [mosq_paths.mosquitto_rr,
            '-p', str(port),
            '-q', '1',
            '-t', '04/rr/timeout/request',
            '-e', '04/rr/timeout/response',
            '-m', 'request',
            '-V', V,
            '-W', '1'
            ]

    rr = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    if mosq_test.wait_for_subprocess(rr, timeout=5):
        raise mosq_test.TestError("rr not terminated")
    (stdo, stde) = rr.communicate()
    if rr.returncode != MOSQ_ERR_TIMEOUT:
        raise mosq_test.TestError(f"rr return code {rr.returncode}, expected {MOSQ_ERR_TIMEOUT}")
    if stde.decode('utf-8') != "Timed out\n":
        raise mosq_test.TestError(f"rr stderr '{stde.decode('utf-8')}', expected 'Timed out'")
    if stdo.decode('utf-8') != "":
        raise mosq_test.TestError(f"rr stdout '{stdo.decode('utf-8')}', expected ''")


if __name__ == '__main__':
    port = mosq_test.get_port()
    broker = MosquittoBroker(port=port)
    with broker:
        do_test(port, proto_ver=3)
        do_test(port, proto_ver=4)
        do_test(port, proto_ver=5)
