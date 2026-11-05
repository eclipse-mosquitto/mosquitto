#!/usr/bin/env python3

# Test whether a valid CONNECT results in the correct CONNACK packet using an
# SSL connection with client certificates required.

from mosq_test_helper import *

if sys.version < '2.7':
    print("WARNING: SSL not supported on Python 2.6")
    exit(0)

def write_config(filename, port1, port2):
    with open(filename, 'w') as f:
        f.write("port %d\n" % (port2))
        f.write("\n")
        f.write("listener %d\n" % (port1))
        f.write("cafile ../ssl/all-ca.crt\n")
        f.write("certfile ../ssl/server.crt\n")
        f.write("keyfile ../ssl/server.key\n")
        f.write("require_certificate true\n")

(port1, port2) = mosq_test.get_port(2)
conf_file = os.path.basename(__file__).replace('.py', '.conf')
write_config(conf_file, port1, port2)

rc = 1
broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port2, use_conf=True)

ssl_eof = False
try:
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile="../ssl/test-root-ca.crt")
    context.load_cert_chain(certfile="../ssl/client-expired.crt", keyfile="../ssl/client-expired.key")
    with socket.create_connection(("localhost", port1)) as sock:
        ssock = context.wrap_socket(sock, server_hostname="localhost", suppress_ragged_eofs=True)
        ssock.settimeout(None)
        try:
            ssock.read(1)
        except ssl.SSLEOFError:
            # Under load, sometimes the broker closes the connection after the
            # handshake has failed, but before we have chance to send our
            # payload and so we get an EOF.
            ssl_eof = True
        except ssl.SSLError as err:
            if err.reason in ["SSLV3_ALERT_CERTIFICATE_EXPIRED", "TLS_ALERT_CERTIFICATE_EXPIRED"]:
                rc = 0
            elif err.errno == 8 and "EOF occurred" in err.strerror:
                rc = 0
            else:
                broker.terminate()
                print(err.strerror)
                raise ValueError(err.errno) from err
except mosq_test.TestError:
    pass
finally:
    os.remove(conf_file)
    time.sleep(0.5)
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()

    if ssl_eof:
        if "certificate verify failed" in stde.decode('utf-8'):
            rc = 0
    if rc:
        print(stde.decode('utf-8'))

exit(rc)
