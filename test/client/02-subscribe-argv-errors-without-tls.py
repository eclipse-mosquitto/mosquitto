#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_sub --help' to see usage.\n"

    # Usage and version, ignore actual text though.
    argv_test('mosquitto_sub', ['--help'], None, 1)
    argv_test('mosquitto_sub', ['--version'], None, 1)

    # Missing args
    argv_test('mosquitto_sub', ['-A'], "Error: -A argument given but no address specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-C'], "Error: -C argument given but no count specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-h'], "Error: -h argument given but no host specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-i'], "Error: -i argument given but no id specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-I'], "Error: -I argument given but no id prefix specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-k'], "Error: -k argument given but no keepalive specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L'], "Error: -L argument given but no URL specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-M'], "Error: -M argument given but max_inflight not specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-o'], "Error: -o argument given but no options file specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-p'], "Error: -p argument given but no port specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-P'], "Error: -P argument given but no password specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--proxy'], "Error: --proxy argument given but no proxy url specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--random-filter'], "Error: --random-filter argument given but no chance specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-q'], "Error: -q argument given but no QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-t'], "Error: -t argument given but no topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-u'], "Error: -u argument given but no username specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--unix'], "Error: --unix argument given but no socket path specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V'], "Error: --protocol-version argument given but no version specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-payload'], "Error: --will-payload argument given but no will payload specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-qos'], "Error: --will-qos argument given but no will QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-topic'], "Error: --will-topic argument given but no will topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-x'], "Error: -x argument given but no session expiry interval specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F'], "Error: -F argument given but no format specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-o'], "Error: -o argument given but no options file specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-T'], "Error: -T argument given but no topic filter specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-U'], "Error: -U argument given but no unsubscribe topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-W'], "Error: -W argument given but no timeout specified.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-payload', 'payload'], "Error: Will payload given, but no will topic given.\n" + helps, 1)
    # No -t or -U
    argv_test('mosquitto_sub', [], "Error: You must specify a topic to subscribe to (-t) or unsubscribe from (-U).\n" + helps, 1)

    # Invalid combinations
    argv_test('mosquitto_sub', ['-i', 'id', '-I', 'id-prefix'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-I', 'id-prefix', '-i', 'id'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)

    # Duplicate options
    argv_test('mosquitto_sub', ['-o', 'file1', '-o', 'file2'], "Error: Duplicate -o argument given.\n\n" + helps, 1)

    # Invalid output format
    argv_test('mosquitto_sub', ['-F', '%'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%0'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%-'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%1'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%.'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%.1'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '%Z'], "Error: Invalid format specifier 'Z'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '@'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '\\'], "Error: Incomplete escape specifier.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-F', '\\Z'], "Error: Invalid escape specifier 'Z'.\n" + helps, 1)

    # Invalid values
    argv_test('mosquitto_sub', ['-k', '-1'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-k', '65536'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-M', '0'], "Error: Maximum inflight messages must be greater than 0.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-p', '-1'], "Error: Invalid port given: -1\n" + helps, 1)
    argv_test('mosquitto_sub', ['-p', '65536'], "Error: Invalid port given: 65536\n" + helps, 1)
    argv_test('mosquitto_sub', ['-q', '-1'], "Error: Invalid QoS given: -1\n" + helps, 1)
    argv_test('mosquitto_sub', ['-q', '3'], "Error: Invalid QoS given: 3\n" + helps, 1)
    argv_test('mosquitto_sub', ['-C', '0'], "Error: Invalid message count \"0\".\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L', 'invalid://'], "Error: Unsupported URL scheme.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L', 'mqtt://localhost'], "Error: Invalid URL for -L argument specified - topic missing.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L', 'mqtts://localhost'], "Error: Invalid URL for -L argument specified - topic missing.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L', 'ws://localhost'], "Error: Invalid URL for -L argument specified - topic missing.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-L', 'wss://localhost'], "Error: Invalid URL for -L argument specified - topic missing.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'request-problem-information', '-1'], "Error: Property value (-1) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'request-problem-information', '256'], "Error: Property value (256) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'receive-maximum', '-1'], "Error: Property value (-1) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'receive-maximum', '65536'], "Error: Property value (65536) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '-1'], "Error: Property value (-1) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '4294967296'], "Error: Property value (4294967296) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'subscribe', 'subscription-identifier', '-1'], "Error: Property value (-1) out of range for property subscription-identifier.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'subscribe', 'subscription-identifier', '4294967296'], "Error: Property value (4294967296) out of range for property subscription-identifier.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'subscribe', 'topic-alias', '1'], "Error: topic-alias property not allowed for subscribe in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'auth', 'authentication-method', '1'], "Error: authentication-method property not supported for auth in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '5', '-D', 'puback', 'reason-string', '1'], "Error: reason-string property not supported for puback in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-t', '++'], "Error: Invalid subscription topic '++', are all '+' and '#' wildcards correct?\n" + helps, 1)
    argv_test('mosquitto_sub', ['-T', '++'], "Error: Invalid filter topic '++', are all '+' and '#' wildcards correct?\n" + helps, 1)
    argv_test('mosquitto_sub', ['-U', '++'], "Error: Invalid unsubscribe topic '++', are all '+' and '#' wildcards correct?\n" + helps, 1)
    argv_test('mosquitto_sub', ['-V', '0'], "Error: Invalid protocol version argument given.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-W', '0'], "Error: Invalid timeout \"0\".\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-qos', '-1'], "Error: Invalid will QoS -1.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-qos', '3'], "Error: Invalid will QoS 3.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--will-topic', '+'], "Error: Invalid will topic '+', does it contain '+' or '#'?\n" + helps, 1)
    argv_test('mosquitto_sub', ['-x', 'A'], "Error: session-expiry-interval not a number.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-x', '-2'], "Error: session-expiry-interval out of range.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['-x', '4294967296'], "Error: session-expiry-interval out of range.\n\n" + helps, 1)
    argv_test('mosquitto_sub', ['--retain-handling', 'invalid'], "Error: Unknown value 'invalid' for --retain-handling.\n\n" + helps, 1)

    # Unknown options
    argv_test('mosquitto_sub', ['--unknown'], "Error: Unknown option '--unknown'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-l'], "Error: Unknown option '-l'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-m'], "Error: Unknown option '-m'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-n'], "Error: Unknown option '-n'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-r'], "Error: Unknown option '-r'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['--repeat'], "Error: Unknown option '--repeat'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['--repeat-delay'], "Error: Unknown option '--repeat-delay'.\n" + helps, 1)
    argv_test('mosquitto_sub', ['-s'], "Error: Unknown option '-s'.\n" + helps, 1)