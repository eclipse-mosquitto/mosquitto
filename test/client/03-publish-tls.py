#!/usr/bin/env python3

#

from mosq_test_helper import *

source_dir = Path(__file__).resolve().parent
ssl_dir = source_dir.parent / "ssl"

def do_test(address, insecure_option, expect_ssl_fail):
    rc = 1

    port = mosq_test.get_port()
    port = 8883

    env = {
        'XDG_CONFIG_HOME':'/tmp/missing',
        'SSLKEYLOGFILE':'/home/roger/keylog'
    }
    env = mosq_test.env_add_ld_library_path(env)
    cmd = [f'{mosq_test.get_build_root()}/client/mosquitto_pub',
            '--cafile', f"{ssl_dir}/all-ca.crt",
            '-d',
            '-h', address,
            '-p', str(port),
            '-t', '03/pub/tls/test',
            '-m', 'message',
            ]
    if insecure_option is not None:
        cmd.append(insecure_option)

    connect_packet = mosq_test.gen_connect("", clean_session=True)
    connack_packet = mosq_test.gen_connack(rc=0)
    publish_packet = mosq_test.gen_publish("03/pub/tls/test", qos=0, payload="message")

    broker = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    broker.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH, cafile=f"{ssl_dir}/all-ca.crt")
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(certfile=f"{ssl_dir}/server-san.crt", keyfile=f"{ssl_dir}/server-san.key")
    sbroker = context.wrap_socket(broker, server_side=True)
    sbroker.settimeout(20)
    sbroker.bind(('', port))
    sbroker.listen(5)

    try:
        pub = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

        (pub_sock, address) = sbroker.accept()
        pub_sock.settimeout(5)

        mosq_test.expect_packet(pub_sock, "connect", connect_packet)
        pub_sock.send(connack_packet)
        mosq_test.expect_packet(pub_sock, "publish", publish_packet)

        if expect_ssl_fail:
            raise mosq_test.TestError

        pub_terminate_rc = 0
        if mosq_test.wait_for_subprocess(pub):
            print("pub not terminated")
            pub_terminate_rc = 1
        (stdo, stde) = pub.communicate()

        rc = pub_terminate_rc
        pub_sock.close()
    except mosq_test.TestError:
        pass
    except ssl.SSLError as e:
        if expect_ssl_fail and e.reason == "SSLV3_ALERT_BAD_CERTIFICATE":
            rc = 0
            pass
        else:
            raise mosq_test.TestError
    except Exception as e:
        print(e)
    finally:
        broker.close()
        if rc:
            print(stde.decode('utf-8'))
            exit(rc)


do_test("127.0.0.1", None, False)
do_test(mosq_test.get_non_loopback_ip(), None, True)
do_test(mosq_test.get_non_loopback_ip(), "--insecure", False)
