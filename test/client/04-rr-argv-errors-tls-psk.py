#!/usr/bin/env python3

#

from mosq_test_helper import *

mosq_test.require_features(["WITH_CLIENTS", "WITH_TLS", "WITH_TLS_PSK"])

def do_test(args, stderr_expected, rc_expected):
    rc = 1

    port = mosq_test.get_port()

    env = {
        'XDG_CONFIG_HOME':'/tmp/missing'
    }
    env = mosq_test.env_add_ld_library_path(env)

    cmd = [mosq_paths.mosquitto_rr] + args

    sub = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    sub.wait()
    (stdo, stde) = sub.communicate()
    if sub.returncode != rc_expected:
        raise mosq_test.TestError(sub.returncode)
    if stderr_expected is not None and stderr_expected not in stde.decode('utf-8'):
        raise mosq_test.TestError(stde)


if __name__ == '__main__':
    # Missing args for TLS-PSK related options
    do_test(['--psk'], "Error: --psk argument given but no key specified.", 1)
    do_test(['--psk-identity'], "Error: --psk-identity argument given but no identity specified.", 1)
