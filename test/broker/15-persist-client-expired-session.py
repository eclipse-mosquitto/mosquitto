#!/usr/bin/env python3

# Connect a client, add a subscription, disconnect, send a message with a
# different client, restore, reconnect, check it is received.

from mosq_test_helper import *

persist_help = persist_module()

port = mosq_test.get_port()

num_messages = 100


def do_test(
    test_case_name: str, additional_config_entries: dict, message_expiry_interval: int
):
    conf_file = os.path.basename(__file__).replace(".py", f"_{port}.conf")
    persist_help.write_config(
        conf_file,
        port,
        additional_config_entries=additional_config_entries,
    )
    persist_help.init(port)

    client_id = "test-expired-session-subscriber"
    username = "test-session-expiry"

    qos = 1
    topic = "client-msg/test"
    source_id = "test-expired-session-publisher"
    proto_ver = 5

    connect_packet = mosq_test.gen_connect(
        client_id,
        username=username,
        proto_ver=proto_ver,
        clean_session=False,
        session_expiry=60,
    )
    connack_packet = mosq_test.gen_connack(rc=0, proto_ver=proto_ver)

    mid = 1
    subscribe_packet = mosq_test.gen_subscribe(mid, topic, qos, proto_ver=proto_ver)
    suback_packet = mosq_test.gen_suback(mid, qos=qos, proto_ver=proto_ver)

    connect2_packet = mosq_test.gen_connect(
        source_id, username=username, proto_ver=proto_ver
    )

    rc = 1

    broker = mosq_test.start_broker(filename=conf_file, use_conf=True, port=port)

    con = None
    try:
        sock = mosq_test.do_client_connect(
            connect_packet, connack_packet, timeout=5, port=port
        )
        mosq_test.do_send_receive(sock, subscribe_packet, suback_packet, "suback")
        sock.close()

        sock = mosq_test.do_client_connect(
            connect2_packet, connack_packet, timeout=5, port=port
        )
        props = (
            mqtt5_props.gen_uint32_prop(
                mqtt5_props.PROP_MESSAGE_EXPIRY_INTERVAL, message_expiry_interval
            )
            if message_expiry_interval > 0
            else b""
        )
        for i in range(num_messages):
            payload = f"queued message {i:3}"
            mid = 10 + i
            publish_packet = mosq_test.gen_publish(
                topic,
                mid=mid,
                qos=qos,
                payload=payload.encode("UTF-8"),
                proto_ver=proto_ver,
                properties=props,
            )
            puback_packet = mosq_test.gen_puback(mid=mid, proto_ver=proto_ver)
            mosq_test.do_send_receive(sock, publish_packet, puback_packet, "puback")
        sock.close()

        # Terminate the broker
        (broker_terminate_rc, stde) = mosq_test.terminate_broker(broker)
        broker = None

        persist_help.check_counts(
            port,
            clients=1,
            client_msgs_out=num_messages,
            base_msgs=num_messages,
            subscriptions=1,
        )

        # Check client
        persist_help.check_client(
            port,
            client_id,
            username=username,
            will_delay_time=0,
            session_expiry_time=60,
            listener_port=port,
            max_packet_size=0,
            max_qos=2,
            retain_available=1,
            session_expiry_interval=60,
            will_delay_interval=0,
        )

        # Check subscription
        persist_help.check_subscription(port, client_id, topic, qos, 0)

        # Check stored message
        for i in range(num_messages):
            payload = f"queued message {i:3}"
            payload_b = payload.encode("UTF-8")
            mid = 10 + i
            store_id = persist_help.check_base_msg(
                port,
                message_expiry_interval,
                topic,
                payload_b,
                source_id,
                username,
                len(payload_b),
                mid,
                port,
                qos,
                retain=0,
                idx=i,
            )

            # Check client msg
            subscriber_mid = 1 + i
            cmsg_id = 1 + i
            persist_help.check_client_msg(
                port,
                client_id,
                cmsg_id,
                store_id,
                0,
                persist_help.dir_out,
                subscriber_mid,
                qos,
                0,
                persist_help.ms_queued,
            )

        # Put session expiry_time into the past
        assert persist_help.modify_client(port, client_id, sub_expiry_time=120) == 1

        # Restart broker
        broker = mosq_test.start_broker(filename=conf_file, use_conf=True, port=port)

        # Connect client again, it should have a session, but all queued messages should be dropped
        sock = mosq_test.do_client_connect(
            connect_packet,
            connack_packet,
            timeout=5,
            port=port,
        )

        # Send ping and wait for the PINGRESP to make sure the broker will not send a queued message instead
        mosq_test.do_ping(sock)
        sock.close()

        (broker_terminate_rc, stde) = mosq_test.terminate_broker(broker)
        broker = None

        persist_help.check_counts(
            port,
            clients=1,
            client_msgs_out=0,
            base_msgs=0,
            subscriptions=0,
        )

        rc = broker_terminate_rc
    finally:
        if broker is not None:
            broker.terminate()
            if mosq_test.wait_for_subprocess(broker):
                if rc == 0:
                    rc = 1
            (_, stde) = broker.communicate()
        os.remove(conf_file)
        rc += persist_help.cleanup(port)

        print(f"{test_case_name}")
        if rc:
            print(stde.decode("utf-8"))
        assert rc == 0, f"rc: {rc}"


memory_queue_config = {
    "log_type": "all",
    "max_queued_messages": num_messages,
}


do_test(
    "memory queue, message expiry interval: 0",
    additional_config_entries=memory_queue_config,
    message_expiry_interval=0,
)
do_test(
    "memory queue, message expiry interval: 120",
    additional_config_entries=memory_queue_config,
    message_expiry_interval=120,
)
