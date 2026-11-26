"""Helpers shared by the tests that reload TLS listeners."""

import hashlib
import os
import re
import shutil
import socket
import ssl
from pathlib import Path

import mosq_test
import mqtt_packets

ssl_dir = Path(__file__).resolve().parent / "ssl"

_PEM_CERT = re.compile(r"-----BEGIN CERTIFICATE-----.+?-----END CERTIFICATE-----", re.DOTALL)

# How the broker refusing a TLS client shows up on the client side. With
# TLS 1.3 the client certificate is only checked after the client has finished
# its side of the handshake, so the refusal may only surface when reading the
# CONNACK.
TLS_REFUSED_ERRORS = (ssl.SSLError, BrokenPipeError, ConnectionResetError, ConnectionAbortedError)


def replace_file(source, dest):
    """Atomically replace dest with the contents of source."""
    tmp = f"{dest}.new"
    shutil.copyfile(source, tmp)
    os.replace(tmp, dest)


def pem_certs(text):
    """Return the PEM certificate blocks in text."""
    return _PEM_CERT.findall(text)


def cert_fingerprint(pem):
    """Return the SHA-256 fingerprint of a PEM certificate block."""
    return hashlib.sha256(ssl.PEM_cert_to_DER_cert(pem)).hexdigest()


def cert_file_fingerprint(path):
    """Return the SHA-256 fingerprint of the first certificate in a PEM file.

    The fixtures may have a text dump of the certificate before the PEM block.
    """
    with open(path) as f:
        certs = pem_certs(f.read())
    if not certs:
        raise ValueError(f"No PEM certificate block found in {path}")
    return cert_fingerprint(certs[0])


def tls_client_context(cafile="test-root-ca.crt", cert="client"):
    """Return a client context trusting cafile and, unless cert is None,
    presenting the client certificate cert.crt with the key cert.key."""
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=str(ssl_dir / cafile))
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    if cert is not None:
        context.load_cert_chain(certfile=str(ssl_dir / f"{cert}.crt"),
                                keyfile=str(ssl_dir / f"{cert}.key"))
    return context


def tls_connect(port, client_id, cert="client", keepalive=60):
    """Connect using the client certificate cert, complete an MQTT CONNECT and
    return the connected socket."""
    sock = socket.create_connection(("localhost", port), timeout=20)
    try:
        ssock = tls_client_context(cert=cert).wrap_socket(sock, server_hostname="localhost")
    except BaseException:
        sock.close()
        raise
    try:
        mosq_test.do_send_receive(ssock, mqtt_packets.gen_connect(client_id, keepalive=keepalive),
                                  mqtt_packets.gen_connack(rc=0), "connack")
    except BaseException:
        ssock.close()
        raise
    return ssock


def try_tls_connect(port, client_id, cert="client"):
    """Attempt to connect using the client certificate cert.

    Returns None if the broker accepted the client, otherwise a short
    description of how it refused.
    """
    try:
        tls_connect(port, client_id, cert).close()
        return None
    except TLS_REFUSED_ERRORS as err:
        return f"{type(err).__name__}: {err}"


def expect_tls_accepted(port, client_id, when, cert="client"):
    reason = try_tls_connect(port, client_id, cert)
    if reason is not None:
        raise mosq_test.TestError(f"{cert}.crt was refused {when}: {reason}")


def expect_tls_refused(port, client_id, when, cert="client"):
    if try_tls_connect(port, client_id, cert) is None:
        raise mosq_test.TestError(f"{cert}.crt was accepted {when}")
