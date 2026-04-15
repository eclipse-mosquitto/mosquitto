#!/usr/bin/env python3

# Test that SIGHUP reloads cafile so that CA cert rotations take effect.
#
# The broker is started with cafile pointing to all-ca.crt (which includes
# the signing CA).  A symlink is used as the cafile so we can atomically
# switch it to a completely different CA (test-fake-root-ca.crt) while the
# broker is running, then send SIGHUP.
#
# After the reload the broker must use the new CA store: client.crt (signed
# by the original CA chain) must be rejected because the new cafile no longer
# trusts its issuer.

from mosq_test_helper import *
from broker_config import BrokerConfig, ListenerConfig

mosq_test.require_features(["WITH_TLS"])


def connect_expect_success(port):
    """Connect with client.crt; expect CONNACK rc=0."""
    connect_packet = mqtt_packets.gen_connect("ca-reload-ok")
    connack_packet = mqtt_packets.gen_connack(rc=0)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH,
                                     cafile=str(ssl_dir / "test-root-ca.crt"))
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(certfile=str(ssl_dir / "client.crt"),
                        keyfile=str(ssl_dir / "client.key"))
    ssock = ctx.wrap_socket(sock, server_hostname="localhost")
    ssock.settimeout(20)
    ssock.connect(("localhost", port))
    mosq_test.do_send_receive(ssock, connect_packet, connack_packet, "connack")
    ssock.close()


def connect_expect_rejection(port):
    """Connect with client.crt; expect rejection after CA rotation."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH,
                                     cafile=str(ssl_dir / "test-root-ca.crt"))
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(certfile=str(ssl_dir / "client.crt"),
                        keyfile=str(ssl_dir / "client.key"))
    ssock = ctx.wrap_socket(sock, server_hostname="localhost")
    ssock.settimeout(20)
    try:
        ssock.connect(("localhost", port))
        try:
            ssock.read(1)
        except (ssl.SSLEOFError, ssl.SSLError):
            return  # expected
        raise mosq_test.TestError(
            "client.crt was accepted after CA rotation to an unrelated CA")
    except ssl.SSLError as err:
        if "certificate" in err.strerror.lower() or "EOF" in err.strerror or err.errno == 8:
            return  # expected rejection
        raise
    except TimeoutError:
        raise mosq_test.TestError(
            "client.crt connection timed out instead of being rejected after CA rotation")


# A symlink in the cwd serves as the cafile so it can be retargeted atomically.
cafile_symlink = os.path.basename(__file__).replace('.py', '.cafile.crt')
try:
    os.unlink(cafile_symlink)
except FileNotFoundError:
    pass
os.symlink(str(ssl_dir / "all-ca.crt"), cafile_symlink)

port = mosq_test.get_port()
broker_config = BrokerConfig(
    listeners=[
        ListenerConfig(
            port=port,
            cafile=cafile_symlink,
            certfile=ssl_dir / "server.crt",
            keyfile=ssl_dir / "server.key",
            require_certificate=True,
        ),
    ],
    allow_anonymous=True,
)

with MosquittoBroker(config=broker_config) as broker:
    broker.add_extra_file(cafile_symlink)

    # Baseline: client.crt accepted with original CA.
    connect_expect_success(port)

    # Rotate cafile symlink to a completely unrelated CA and reload.
    new_link = cafile_symlink + ".new"
    os.symlink(str(ssl_dir / "test-fake-root-ca.crt"), new_link)
    os.replace(new_link, cafile_symlink)  # atomic swap

    broker.reload()
    time.sleep(0.5)

    # After reload with the unrelated CA: client.crt must now be rejected.
    connect_expect_rejection(port)
