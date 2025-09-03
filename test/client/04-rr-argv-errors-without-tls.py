#!/usr/bin/env python3

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_rr --help' to see usage.\n"

    # Usage, version, ignore actual text though.
    argv_test('mosquitto_rr', ['--help'], None, 1)
    argv_test('mosquitto_rr', ['--version'], None, 1)

    # Missing args
    argv_test('mosquitto_rr', ['-A'], "Error: -A argument given but no address specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-e'], "Error: -e argument given but no response topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-h'], "Error: -h argument given but no host specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-i'], "Error: -i argument given but no id specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-I'], "Error: -I argument given but no id prefix specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-k'], "Error: -k argument given but no keepalive specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-L'], "Error: -L argument given but no URL specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-M'], "Error: -M argument given but max_inflight not specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-m'], "Error: -m argument given but no message specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-o'], "Error: -o argument given but no options file specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-p'], "Error: -p argument given but no port specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-P'], "Error: -P argument given but no password specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--proxy'], "Error: --proxy argument given but no proxy url specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-q'], "Error: -q argument given but no QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-t'], "Error: -t argument given but no topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-u'], "Error: -u argument given but no username specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--unix'], "Error: --unix argument given but no socket path specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V'], "Error: --protocol-version argument given but no version specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-payload'], "Error: --will-payload argument given but no will payload specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-qos'], "Error: --will-qos argument given but no will QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-topic'], "Error: --will-topic argument given but no will topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-x'], "Error: -x argument given but no session expiry interval specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F'], "Error: -F argument given but no format specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-o'], "Error: -o argument given but no options file specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-W'], "Error: -W argument given but no timeout specified.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-payload', 'payload'], "Error: Will payload given, but no will topic given.\n" + helps, 1)
    # No -t or -U
    argv_test('mosquitto_rr', [], "Error: All of topic, message, and response topic must be supplied.\n" + helps, 1)

    # Invalid combinations
    argv_test('mosquitto_rr', ['-i', 'id', '-I', 'id-prefix'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-I', 'id-prefix', '-i', 'id'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)

    # Invalid output format
    argv_test('mosquitto_rr', ['-F', '%'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%0'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%-'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%1'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%.'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%.1'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '%Z'], "Error: Invalid format specifier 'Z'.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '@'], "Error: Incomplete format specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '\\'], "Error: Incomplete escape specifier.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-F', '\\Z'], "Error: Invalid escape specifier 'Z'.\n" + helps, 1)

    # Invalid values
    argv_test('mosquitto_rr', ['-e', 'topic/+'], "Error: Invalid response topic 'topic/+', does it contain '+' or '#'?\n" + helps, 1)
    argv_test('mosquitto_rr', ['-k', '-1'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-k', '65536'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-M', '0'], "Error: Maximum inflight messages must be greater than 0.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-p', '-1'], "Error: Invalid port given: -1\n" + helps, 1)
    argv_test('mosquitto_rr', ['-p', '65536'], "Error: Invalid port given: 65536\n" + helps, 1)
    argv_test('mosquitto_rr', ['-q', '-1'], "Error: Invalid QoS given: -1\n" + helps, 1)
    argv_test('mosquitto_rr', ['-q', '3'], "Error: Invalid QoS given: 3\n" + helps, 1)
    argv_test('mosquitto_rr', ['-L', 'invalid://'], "Error: Unsupported URL scheme.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-L', 'mqtt://localhost'], "Error: Invalid URL for -L argument specified - topic missing.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'request-problem-information', '-1'], "Error: Property value (-1) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'request-problem-information', '256'], "Error: Property value (256) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'receive-maximum', '-1'], "Error: Property value (-1) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'receive-maximum', '65536'], "Error: Property value (65536) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '-1'], "Error: Property value (-1) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '4294967296'], "Error: Property value (4294967296) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'subscribe', 'subscription-identifier', '-1'], "Error: Property value (-1) out of range for property subscription-identifier.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'subscribe', 'subscription-identifier', '4294967296'], "Error: Property value (4294967296) out of range for property subscription-identifier.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '5', '-D', 'subscribe', 'topic-alias', '1'], "Error: topic-alias property not allowed for subscribe in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-V', '0'], "Error: Invalid protocol version argument given.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-W', '0'], "Error: Invalid timeout \"0\".\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-qos', '-1'], "Error: Invalid will QoS -1.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-qos', '3'], "Error: Invalid will QoS 3.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--will-topic', '+'], "Error: Invalid will topic '+', does it contain '+' or '#'?\n" + helps, 1)
    argv_test('mosquitto_rr', ['-x', 'A'], "Error: session-expiry-interval not a number.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-x', '-2'], "Error: session-expiry-interval out of range.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['-x', '4294967296'], "Error: session-expiry-interval out of range.\n\n" + helps, 1)
    argv_test('mosquitto_rr', ['--retain-handling', 'invalid'], "Error: Unknown value 'invalid' for --retain-handling.\n\n" + helps, 1)

    # Mixed message types
    argv_test('mosquitto_rr', ['-m', 'message', '-f', 'file'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)

    # Unknown options
    argv_test('mosquitto_rr', ['--unknown'], "Error: Unknown option '--unknown'.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-l'], "Error: Unknown option '-l'.\n" + helps, 1)
    argv_test('mosquitto_rr', ['-r'], "Error: Unknown option '-r'.\n" + helps, 1)
    argv_test('mosquitto_rr', ['--repeat'], "Error: Unknown option '--repeat'.\n" + helps, 1)
    argv_test('mosquitto_rr', ['--repeat-delay'], "Error: Unknown option '--repeat-delay'.\n" + helps, 1)