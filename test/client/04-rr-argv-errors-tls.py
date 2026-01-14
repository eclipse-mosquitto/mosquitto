#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_rr --help' to see usage.\n"

    # Missing args for TLS related options
    argv_test('mosquitto_rr', ['--cafile'], "Error: --cafile argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--capath'], "Error: --capath argument given but no directory specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--cert'], "Error: --cert argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--ciphers'], "Error: --ciphers argument given but no ciphers specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--key'], "Error: --key argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--keyform'], "Error: --keyform argument given but no keyform specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--tls-alpn'], "Error: --tls-alpn argument given but no protocol specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--tls-engine'], "Error: --tls-engine argument given but no engine_id specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--tls-engine-kpass-sha1'], "Error: --tls-engine-kpass-sha1 argument given but no kpass sha1 specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--tls-version'], "Error: --tls-version argument given but no version specified.\n\n" + helps, 1)
