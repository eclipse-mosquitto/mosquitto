#!/usr/bin/env python3

# Connect a client, add a subscription, disconnect, send a message with a
# different client, restore, reconnect, check it is received.

from mosq_test_helper import *
from persist_module_helper import *

persist_help = persist_module()

port = mosq_test.get_port()

proto_ver = 5
qos = 1
topic = "test-will-msg"
username = "test-will-msg"

subscriber_id = "test-will-subscriber"
publisher_id = "test-will-publisher"


def do_test(
    session_expiry: int,
    will_qos: int,
    will_retain: bool,
):
    mid = 1

    def check_will_received(do_subscribe: bool, expect_will_publish=bool):
        nonlocal mid
        # Reconnect client, it should have a session
        subscriber_sock = connect_client(
            port,
            subscriber_id,
            username,
            proto_ver,
            session_expiry=60,
            session_present=True,
            subscribe_topic=topic if do_subscribe else None,
        )
        if expect_will_publish:
            will_publish = mosq_test.gen_publish(
                topic,
                will_qos,
                will_payload,
                will_retain and do_subscribe,
                mid=mid,
                proto_ver=proto_ver,
                properties=will_properties,
            )
            if will_qos > 0:
                pub_ack = mosq_test.gen_puback(mid=1, proto_ver=proto_ver)
                mosq_test.do_receive_send(subscriber_sock, will_publish, pub_ack)
            else:
                mosq_test.expect_packet(subscriber_sock, "will message", will_publish)
            mid += 1
        # Always send a ping. Either to make sure a potential puback is processed by the server
        # or to make sure the server has not send an unexpected publish
        mosq_test.do_ping(subscriber_sock)
        subscriber_sock.close()

    print(
        f" {'persistent' if session_expiry > 0 else 'non persistent'} client"
        f" with will qos = {will_qos}"
        f" and will_retain = {will_retain}"
    )
    expect_will_received = will_qos > 0

    conf_file = os.path.basename(__file__).replace(".py", f"_{port}.conf")
    persist_help.write_config(
        conf_file,
        port,
        additional_config_entries={"plugin_opt_flush_period": 0, "log_type": "all"},
    )
    persist_help.init(port)

    rc = 1

    will_payload = b"My simple will message"
    will_properties = b""

    broker = mosq_test.start_broker(filename=conf_file, use_conf=True, port=port)
    stde = None
    stde2 = None
    try:
        subscriber_sock = connect_client(
            port,
            subscriber_id,
            username,
            proto_ver,
            session_expiry=60,
            subscribe_topic=topic,
        )  # .close()

        publisher_sock = connect_client(
            port,
            publisher_id,
            username,
            proto_ver,
            session_expiry=session_expiry,
            will_topic=topic,
            will_qos=will_qos,
            will_retain=will_retain,
            will_payload=will_payload,
        )
        # Send an additional ping to make sure the commit to the DB has happened
        mosq_test.do_ping(publisher_sock)

        # Kill the broker
        broker.kill()
        (_, stde) = broker.communicate()
        broker = None

        client_msg_counts = {subscriber_id: 0}
        if session_expiry > 0:
            client_msg_counts[publisher_id] = None
        check_db(
            persist_help,
            port,
            username,
            subscription_topic=topic,
            client_msg_counts=client_msg_counts,
            publisher_id=publisher_id,
            num_published_msgs=0,
            check_session_expiry_time=False,
        )
        persist_help.check_will(
            port,
            publisher_id,
            will_payload,
            topic,
            will_qos,
            will_retain,
            None,
        )

        # Restart broker
        broker = mosq_test.start_broker(filename=conf_file, use_conf=True, port=port)

        check_will_received(do_subscribe=False, expect_will_publish=will_qos > 0)
        check_will_received(do_subscribe=True, expect_will_publish=will_retain)

        (broker_terminate_rc, stde2) = mosq_test.terminate_broker(broker)
        broker = None

        rc = broker_terminate_rc
    finally:
        if broker is not None:
            broker.terminate()
            if mosq_test.wait_for_subprocess(broker):
                if rc == 0:
                    rc = 1
            (_, stde3) = broker.communicate()
            if not stde:
                stde = stde3
            else:
                stde2 = stde3
        os.remove(conf_file)
        rc += persist_help.cleanup(port)

        if rc:
            if stde:
                print(stde.decode("utf-8"))
            if stde2:
                print("Broker after restart")
                print(stde2.decode("utf-8"))
        # assert rc == 0, f"rc: {rc}"


do_test(session_expiry=0, will_qos=0, will_retain=0)

do_test(session_expiry=0, will_qos=1, will_retain=0)

do_test(session_expiry=0, will_qos=0, will_retain=1)

do_test(session_expiry=0, will_qos=1, will_retain=1)

do_test(session_expiry=60, will_qos=0, will_retain=0)

do_test(session_expiry=60, will_qos=1, will_retain=0)

do_test(session_expiry=60, will_qos=0, will_retain=1)

do_test(session_expiry=60, will_qos=1, will_retain=1)
