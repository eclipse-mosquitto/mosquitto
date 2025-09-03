#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_pub --help' to see usage.\n"

    # Missing args
    argv_test('mosquitto_pub', ['--cafile'], "Error: --cafile argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--capath'], "Error: --capath argument given but no directory specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--cert'], "Error: --cert argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--ciphers'], "Error: --ciphers argument given but no ciphers specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--key'], "Error: --key argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--keyform'], "Error: --keyform argument given but no keyform specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--tls-alpn'], "Error: --tls-alpn argument given but no protocol specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--tls-engine'], "Error: --tls-engine argument given but no engine_id specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--tls-engine-kpass-sha1'], "Error: --tls-engine-kpass-sha1 argument given but no kpass sha1 specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--tls-version'], "Error: --tls-version argument given but no version specified.\n\n" + helps, 1)

    # Invalid combinations
    argv_test('mosquitto_pub', ['--cert', 'file'], "Error: Both certfile and keyfile must be provided if one of them is set.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--key', 'file'], "Error: Both certfile and keyfile must be provided if one of them is set.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--keyform', 'file'], "Error: If keyform is set, keyfile must be also specified.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--tls-engine-kpass-sha1', 'hash'], "Error: when using tls-engine-kpass-sha1, both tls-engine and keyform must also be provided.\n" + helps, 1)

    # Invalid values
    argv_test('mosquitto_pub', ['--tls-keylog', 'keylog', '-t','topic','-m','1', '--cafile', 'missing'], "Error: Problem setting TLS options: File not found.\n", 1)