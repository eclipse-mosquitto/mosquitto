#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_pub --help' to see usage.\n"

    # Missing args
    argv_test('mosquitto_pub', ['--psk'], "Error: --psk argument given but no key specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--psk-identity'], "Error: --psk-identity argument given but no identity specified.\n\n" + helps, 1)

    # Invalid combinations
    argv_test('mosquitto_pub', ['--cafile', 'file', '--psk', 'key'], "Error: Only one of --psk or --cafile/--capath may be used at once.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--capath', 'dir', '--psk', 'key'], "Error: Only one of --psk or --cafile/--capath may be used at once.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--psk', 'key'], "Error: --psk-identity required if --psk used.\n" + helps, 1)