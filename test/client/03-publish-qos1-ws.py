#!/usr/bin/env python3

#

from mosq_test_helper import *

def write_config(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("allow_anonymous true\n")
        f.write(f"listener {port1}\n")
        f.write("protocol websockets\n")
        f.write(f"listener {port2}\n")

def do_test(proto_ver):
    rc = 1

    ports = mosq_test.get_port(2)
    conf_file = os.path.basename(__file__).replace('.py', '.conf')

    if proto_ver == 5:
        V = 'mqttv5'
    elif proto_ver == 4:
        V = 'mqttv311'
    else:
        V = 'mqttv31'

    env = {
            'LD_LIBRARY_PATH': mosq_test.get_build_root() + '/lib:'+ os.getenv("LD_LIBRARY_PATH", ""),
            'XDG_CONFIG_HOME':'/tmp/missing'
            }
    cmd = ['../../client/mosquitto_pub',
            '-p', str(ports[0]),
            '-q', '1',
            '-t', '03/pub/qos1/test',
            '-m', 'message',
            '-V', V,
            '--ws'
            ]

    mid = 1
    publish_packet = mosq_test.gen_publish("03/pub/qos1/test", qos=1, mid=mid, payload="message", proto_ver=proto_ver)
    if proto_ver == 5:
        puback_packet = mosq_test.gen_puback(mid, proto_ver=proto_ver, reason_code=mqtt5_rc.MQTT_RC_NO_MATCHING_SUBSCRIBERS)
    else:
        puback_packet = mosq_test.gen_puback(mid, proto_ver=proto_ver)

    write_config(conf_file, ports[0], ports[1])

    try:
        broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=ports[1], use_conf=True)
        sock = mosq_test.sub_helper(port=ports[1], topic="#", qos=1, proto_ver=proto_ver)

        pub = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
        pub_terminate_rc = 0
        if mosq_test.wait_for_subprocess(pub):
            print("pub not terminated")
            pub_terminate_rc = 1
        (stdo, stde) = pub.communicate()

        mosq_test.expect_packet(sock, "publish", publish_packet)
        rc = pub_terminate_rc
        sock.close()
    except mosq_test.TestError:
        pass
    except Exception as e:
        print(e)
    finally:
        broker.terminate()
        os.remove(conf_file)
        if mosq_test.wait_for_subprocess(broker):
            print("broker not terminated")
            if rc == 0: rc=1
        (stdo, stde) = broker.communicate()
        if rc:
            print(stde.decode('utf-8'))
            print("proto_ver=%d" % (proto_ver))
            exit(rc)


do_test(proto_ver=3)
do_test(proto_ver=4)
do_test(proto_ver=5)
