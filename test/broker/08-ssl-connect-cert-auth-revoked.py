#!/usr/bin/env python3

from mosq_test_helper import *

if sys.version < '2.7':
    print("WARNING: SSL not supported on Python 2.6")
    exit(0)

def write_config(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("listener %d\n" % (port2))
        f.write("allow_anonymous true\n")
        f.write("listener %d\n" % (port1))
        f.write("allow_anonymous true\n")
        f.write(f"cafile {ssl_dir}/all-ca.crt\n")
        f.write(f"certfile {ssl_dir}/server.crt\n")
        f.write(f"keyfile {ssl_dir}/server.key\n")
        f.write("require_certificate true\n")
        f.write(f"crlfile {ssl_dir}/crl.pem\n")

(port1, port2) = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port1, port2)

rc = 1
broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

ssl_eof = False
try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=f"{ssl_dir}/test-root-ca.crt")
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(certfile=f"{ssl_dir}/client-revoked.crt", keyfile=f"{ssl_dir}/client-revoked.key")
    ssock = context.wrap_socket(sock, server_hostname="localhost")
    ssock.settimeout(20)
    try:
        ssock.connect(("localhost", port1))
        try:
            ssock.read(1)
        except ssl.SSLEOFError:
            # Under load, sometimes the broker closes the connection after the
            # handshake has failed, but before we have chance to send our
            # payload and so we get an EOF.
            ssl_eof = True
        except ssl.SSLError as err:
            if err.reason == "SSLV3_ALERT_CERTIFICATE_REVOKED":
                rc = 0
            elif err.errno == 8 and "EOF occurred" in err.strerror:
                rc = 0
            else:
                broker.terminate()
                print(err.strerror)
                raise ValueError(err.errno) from err
    except ssl.SSLError as err:
        if err.errno == 1 and "certificate revoked" in err.strerror:
            rc = 0
        elif err.errno == 8 and "EOF occurred" in err.strerror:
            rc = 0
        else:
            broker.terminate()
            print(err.strerror)
            raise ValueError(err.errno)

except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    time.sleep(0.5)
    broker.terminate()
    if mosq_test.wait_for_subprocess(broker):
        print("broker not terminated")
        if rc == 0: rc=1
    (stdo, stde) = broker.communicate()
    if ssl_eof:
        if "certificate verify failed" in stde.decode('utf-8'):
            rc = 0
    if rc:
        print(stde.decode('utf-8'))

exit(rc)

