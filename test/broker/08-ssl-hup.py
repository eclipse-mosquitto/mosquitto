#!/usr/bin/env python3

# Test that SIGHUP reloads the TLS configuration of listeners.
#
# Usage: 08-ssl-hup.py <scenario> [ports...]
#
# Each scenario is run as a separate test:
#
# cafile, capath
#   The listener points at a private copy of a CA bundle or directory that
#   is rotated to an unrelated CA and restored. Reloading has to replace the
#   trust store rather than extend it, so after the rotation client.crt must
#   be refused, and after the restore it must be accepted again. A missing
#   capath must leave the previous context in use, while a dangling symlink in
#   capath, e.g. left behind after removing a CA without rehashing, must not
#   stop it from being loaded.

from mosq_test_helper import *
from broker_config import BrokerConfig, ListenerConfig
from tls_helper import expect_tls_accepted, expect_tls_refused, replace_file
import shutil

mosq_test.require_features(["WITH_TLS"])

RELOAD_ERROR = "keeping the previous TLS configuration"


# --- cafile, capath --------------------------------------------------------

def subject_hash(cert):
    result = subprocess.run(['openssl', 'x509', '-hash', '-noout', '-in', str(cert)],
                            capture_output=True, timeout=10)
    if result.returncode != 0:
        raise mosq_test.TestError(
            f"openssl could not hash {cert}: {result.stderr.decode()}")
    return result.stdout.decode().strip()


def set_capath(path, ca_names):
    """Replace the contents of the capath directory with the given CA certs."""
    for entry in os.listdir(path):
        os.remove(os.path.join(path, entry))
    for name in ca_names:
        source = ssl_dir / name
        shutil.copyfile(source, os.path.join(path, f"{subject_hash(source)}.0"))
    os.symlink("removed-ca.crt", os.path.join(path, "00000000.0"))


def run_rotation_test(trust_store, trusted_cas, unrelated_cas):
    is_cafile = trust_store == "cafile"
    store_path = basename + ".store"
    saved_path = store_path + ".saved"

    if is_cafile:
        replace_file(ssl_dir / trusted_cas[0], store_path)
    else:
        shutil.rmtree(store_path, ignore_errors=True)
        shutil.rmtree(saved_path, ignore_errors=True)
        os.mkdir(store_path)
        set_capath(store_path, trusted_cas)

    port = mosq_test.get_port()
    listener_args = {
        "port": port,
        "certfile": ssl_dir / "server.crt",
        "keyfile": ssl_dir / "server.key",
        "require_certificate": True,
    }
    listener_args["cafile" if is_cafile else "capath"] = store_path
    broker_config = BrokerConfig(
        listeners=[ListenerConfig(**listener_args)],
        allow_anonymous=True,
    )

    with MosquittoBroker(config=broker_config) as broker:
        if is_cafile:
            broker.add_extra_file(store_path)

        prefix = trust_store + "-rotation"
        expect_tls_accepted(port, f"{prefix}-before", "before any reload")

        if not is_cafile:
            os.rename(store_path, saved_path)
            try:
                broker.reload_and_wait()
                expect_tls_accepted(port, f"{prefix}-missing",
                                    "after reloading a missing capath")
            finally:
                os.rename(saved_path, store_path)
            broker.reload_and_wait()
            expect_tls_accepted(port, f"{prefix}-repaired", "after capath was restored")

        if is_cafile:
            replace_file(ssl_dir / unrelated_cas[0], store_path)
        else:
            set_capath(store_path, unrelated_cas)
        broker.reload_and_wait()
        expect_tls_refused(port, f"{prefix}-rotated",
                           f"after {trust_store} was rotated to an unrelated CA")

        if is_cafile:
            replace_file(ssl_dir / trusted_cas[0], store_path)
        else:
            set_capath(store_path, trusted_cas)
        broker.reload_and_wait()
        expect_tls_accepted(port, f"{prefix}-restored",
                            f"after the original {trust_store} was restored")

    if not is_cafile and broker.get_log().count(RELOAD_ERROR) != 1:
        raise mosq_test.TestError("Expected one failed capath reload to be logged")

    if not is_cafile:
        shutil.rmtree(store_path, ignore_errors=True)


def test_cafile():
    # A hierarchy of CA certs in a single file.
    run_rotation_test("cafile", ["all-ca.crt"], ["test-fake-root-ca.crt"])


def test_capath():
    # A hierarchy of CA certs in a directory of individual files.
    run_rotation_test("capath", ["test-root-ca.crt", "test-signing-ca.crt"], ["test-fake-root-ca.crt"])


SCENARIOS = {
    "cafile": test_cafile,
    "capath": test_capath,
}

if len(sys.argv) < 2 or sys.argv[1] not in SCENARIOS:
    print(f"usage: {sys.argv[0]} {{{','.join(SCENARIOS)}}} [ports...]")
    sys.exit(1)
# Leave only the ports for mosq_test.get_port().
scenario = sys.argv.pop(1)
# Private file names include the scenario, so that scenarios running in
# parallel in the same directory don't share them.
basename = f"{Path(__file__).stem}-{scenario}"
SCENARIOS[scenario]()
