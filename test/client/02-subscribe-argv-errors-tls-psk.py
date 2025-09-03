#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_sub --help' to see usage.\n"

    # Missing args for TLS-PSK related options
    argv_test('mosquitto_sub', ['--psk'], "Error: --psk argument given but no key specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--psk-identity'], "Error: --psk-identity argument given but no identity specified.\n\n" + helps, 1)