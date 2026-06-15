#!/usr/bin/env python3

# Test that after a SIGHUP:
#   1. New TLS connections with a valid certificate are still accepted.
#   2. Revoked certificates are still rejected.
#      (A missing CRL would allow revoked certs to connect.)
#   3. Repeated SIGHUPs do not degrade behaviour.

from mosq_test_helper import *
from broker_config import BrokerConfig, ListenerConfig

mosq_test.require_features(["WITH_TLS"])

SIGHUP_COUNT = 3  # exercise multiple reloads



def connect_valid(port):
    """Connect with a valid client certificate; expect CONNACK rc=0."""
    connect_packet = mqtt_packets.gen_connect("hup-reload-valid")
    connack_packet = mqtt_packets.gen_connack(rc=0)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH,
                                         cafile=str(ssl_dir / "test-root-ca.crt"))
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(certfile=str(ssl_dir / "client.crt"),
                            keyfile=str(ssl_dir / "client.key"))
    ssock = context.wrap_socket(sock, server_hostname="localhost")
    ssock.settimeout(20)
    ssock.connect(("localhost", port))
    mosq_test.do_send_receive(ssock, connect_packet, connack_packet, "connack")
    ssock.close()


def connect_revoked_expect_rejection(port):
    """Connect with a revoked certificate; expect the TLS handshake or
    the subsequent read to fail with a certificate-revoked error."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH,
                                         cafile=str(ssl_dir / "test-root-ca.crt"))
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(certfile=str(ssl_dir / "client-revoked.crt"),
                            keyfile=str(ssl_dir / "client-revoked.key"))
    ssock = context.wrap_socket(sock, server_hostname="localhost")
    ssock.settimeout(20)
    try:
        ssock.connect(("localhost", port))
        # The broker may close the connection after the handshake
        try:
            ssock.read(1)
        except (ssl.SSLEOFError, ssl.SSLError):
            return  # expected
        raise mosq_test.TestError("Revoked certificate was accepted after SIGHUP")
    except ssl.SSLError as err:
        if "revoked" in err.strerror or "EOF" in err.strerror or err.errno == 8:
            return  # expected rejection
        raise


port = mosq_test.get_port()
broker_config = BrokerConfig(
    listeners=[
        ListenerConfig(
            port=port,
            cafile=ssl_dir / "all-ca.crt",
            certfile=ssl_dir / "server.crt",
            keyfile=ssl_dir / "server.key",
            require_certificate=True,
            crlfile=ssl_dir / "crl.pem",
        ),
    ],
    allow_anonymous=True,
)

with MosquittoBroker(config=broker_config) as broker:
    # Baseline: verify TLS works before any reload
    connect_valid(port)

    for i in range(SIGHUP_COUNT):
        broker.reload()
        time.sleep(0.5)  # give broker time to reload

        # After each SIGHUP a new handshake must succeed
        connect_valid(port)

        # CRL must still be active: revoked cert must be rejected
        connect_revoked_expect_rejection(port)
