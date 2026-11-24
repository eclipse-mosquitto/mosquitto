#!/usr/bin/env python3

# Test whether mosquitto_sub exits correctly when its -W timeout expires, or
# when it receives SIGTERM / SIGINT. The signals must lead to a clean
# disconnect that requests the will be sent, or to an immediate exit if no
# CONNACK has been received yet.

from mosq_test_helper import *
import platform
import signal

mosq_test.require_features(["WITH_BROKER"])

if platform.system() == 'Windows':
    # Signals are not available
    exit(0)

MOSQ_ERR_TIMEOUT = 27
will_topic = "02/sub/signals/will"
will_payload = "will-payload"


def start_sub(port, proto_ver, extra_args):
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
    cmd = [mosq_paths.mosquitto_sub,
            '-p', str(port),
            '-q', '1',
            '-t', '02/sub/signals/test',
            '-V', V,
            '--will-topic', will_topic,
            '--will-payload', will_payload,
            ] + extra_args
    return subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)


def check_sub(sub, rc_expected, stderr_expected):
    if mosq_test.wait_for_subprocess(sub, timeout=5):
        raise mosq_test.TestError("sub not terminated")
    (stdo, stde) = sub.communicate()
    if sub.returncode != rc_expected:
        raise mosq_test.TestError(f"sub return code {sub.returncode}, expected {rc_expected}")
    if stde.decode('utf-8') != stderr_expected:
        raise mosq_test.TestError(f"sub stderr '{stde.decode('utf-8')}', expected '{stderr_expected}'")
    if stdo.decode('utf-8') != "":
        raise mosq_test.TestError(f"sub stdout '{stdo.decode('utf-8')}', expected ''")


def do_test(port, proto_ver, sig):
    # sig is None to test -W, otherwise the signal to send
    helper = mosq_test.sub_helper(port=port, topic=will_topic)
    try:
        if sig is None:
            sub = start_sub(port, proto_ver, ['-W', '1'])
            check_sub(sub, MOSQ_ERR_TIMEOUT, "Timed out\n")
        else:
            sub = start_sub(port, proto_ver, [])
            time.sleep(0.5)
            sub.send_signal(sig)
            check_sub(sub, 0, "")

        if proto_ver == 5:
            # The disconnect uses "disconnect with will message", so the will
            # must have been published.
            will_packet = mqtt_packets.gen_publish(will_topic, qos=0, payload=will_payload)
            mosq_test.expect_packet(helper, "will", will_packet)
        # This also checks that no will was received for MQTT v3.x
        mosq_test.do_ping(helper)
    finally:
        helper.close()


def do_test_no_connack(port, sig):
    # A server that accepts the connection but never sends a CONNACK. The
    # client must exit immediately on timeout/signal.
    ssock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ssock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    ssock.bind(('', port))
    ssock.listen(5)
    try:
        if sig is None:
            sub = start_sub(port, 4, ['-W', '1'])
        else:
            sub = start_sub(port, 4, [])
            time.sleep(0.5)
            sub.send_signal(sig)
        check_sub(sub, 255, "")
    finally:
        ssock.close()


if __name__ == '__main__':
    (port, port_no_connack) = mosq_test.get_port(2)
    broker = MosquittoBroker(port=port)
    with broker:
        for sig in [None, signal.SIGTERM, signal.SIGINT]:
            do_test(port, proto_ver=3, sig=sig)
            do_test(port, proto_ver=4, sig=sig)
            do_test(port, proto_ver=5, sig=sig)

    do_test_no_connack(port_no_connack, None)
    do_test_no_connack(port_no_connack, signal.SIGTERM)
    do_test_no_connack(port_no_connack, signal.SIGINT)
