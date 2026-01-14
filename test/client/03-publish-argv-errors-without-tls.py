#!/usr/bin/env python3

#

from mosq_test_helper import *

if __name__ == '__main__':
    helps = "\nUse 'mosquitto_pub --help' to see usage.\n"

    # Usage, version, ignore actual text though.
    argv_test('mosquitto_pub', ['--help'], None, 1)
    argv_test('mosquitto_pub', ['--version'], None, 1)

    # Missing args
    argv_test('mosquitto_pub', ['-A'], "Error: -A argument given but no address specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-f'], "Error: -f argument given but no file specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-h'], "Error: -h argument given but no host specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-i'], "Error: -i argument given but no id specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-I'], "Error: -I argument given but no id prefix specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-k'], "Error: -k argument given but no keepalive specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-L'], "Error: -L argument given but no URL specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-M'], "Error: -M argument given but max_inflight not specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-m'], "Error: -m argument given but no message specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-o'], "Error: -o argument given but no options file specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-p'], "Error: -p argument given but no port specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-P'], "Error: -P argument given but no password specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--proxy'], "Error: --proxy argument given but no proxy url specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-q'], "Error: -q argument given but no QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--repeat'], "Error: --repeat argument given but no count specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--repeat-delay'], "Error: --repeat-delay argument given but no time specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-t'], "Error: -t argument given but no topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-u'], "Error: -u argument given but no username specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--unix'], "Error: --unix argument given but no socket path specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V'], "Error: --protocol-version argument given but no version specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--will-payload'], "Error: --will-payload argument given but no will payload specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--will-qos'], "Error: --will-qos argument given but no will QoS specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--will-topic'], "Error: --will-topic argument given but no will topic specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-x'], "Error: -x argument given but no session expiry interval specified.\n\n" + helps, 1)

    argv_test('mosquitto_pub', ['-V', '5', '-D'], "Error: --property argument given but not enough arguments specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect'], "Error: --property argument given but not enough arguments specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'receive-maximum'], "Error: --property argument given but not enough arguments specified.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'invalid', 'receive-maximum', '1'], "Error: Invalid command invalid given in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'invalid', '1'], "Error: Invalid property name invalid given in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'will-delay-interval', '1'], "Error: will-delay-interval property not allowed for connect in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'user-property', 'key'], "Error: --property argument given but not enough arguments specified.\n\n" + helps, 1)

    # Invalid combinations
    argv_test('mosquitto_pub', ['-i', 'id', '-I', 'id-prefix'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-I', 'id-prefix', '-i', 'id'], "Error: -i and -I argument cannot be used together.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['--will-payload', 'payload'], "Error: Will payload given, but no will topic given.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--will-retain'], "Error: Will retain given, but no will topic given.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', 'mqttv5', '-x', '-1'], "Error: You must provide a client id if you are using an infinite session expiry interval.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', 'mqttv311', '-c'], "Error: You must provide a client id if you are using the -c option.\n" + helps, 1)


    # Mixed message types
    argv_test('mosquitto_pub', ['-m', 'message', '-f', 'file'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-m', 'message', '-l'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-l', '-m', 'message'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-l', '-n'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-l', '-s'], "Error: Only one type of message can be sent at once.\n\n" + helps, 1)

    # Invalid values
    argv_test('mosquitto_pub', ['-t', 'topic', '-f', 'missing'], "Error: Unable to read file \"missing\": No such file or directory.\nError loading input file \"missing\".\n", 1)
    argv_test('mosquitto_pub', ['-k', '-1'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-k', '65536'], "Error: Invalid keepalive given, it must be between 5 and 65535 inclusive.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-M', '0'], "Error: Maximum inflight messages must be greater than 0.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-p', '-1'], "Error: Invalid port given: -1\n" + helps, 1)
    argv_test('mosquitto_pub', ['-p', '65536'], "Error: Invalid port given: 65536\n" + helps, 1)
    argv_test('mosquitto_pub', ['-q', '-1'], "Error: Invalid QoS given: -1\n" + helps, 1)
    argv_test('mosquitto_pub', ['-q', '3'], "Error: Invalid QoS given: 3\n" + helps, 1)
    argv_test('mosquitto_pub', ['--repeat-delay', '-1'], "Error: --repeat-delay argument must be >=0.0.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-t', 'topic/+'], "Error: Invalid publish topic 'topic/+', does it contain '+' or '#'?\n" + helps, 1)
    argv_test('mosquitto_pub', ['-t', 'topic/#'], "Error: Invalid publish topic 'topic/#', does it contain '+' or '#'?\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'request-problem-information', '-1'], "Error: Property value (-1) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'request-problem-information', '256'], "Error: Property value (256) out of range for property request-problem-information.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'receive-maximum', '-1'], "Error: Property value (-1) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'receive-maximum', '65536'], "Error: Property value (65536) out of range for property receive-maximum.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '-1'], "Error: Property value (-1) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'session-expiry-interval', '4294967296'], "Error: Property value (4294967296) out of range for property session-expiry-interval.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'connect', 'subscription-identifier', '1'], "Error: subscription-identifier property not allowed for connect in --property argument.\n\n" + helps, 1)
    argv_test('mosquitto_pub', ['-V', '5', '-D', 'publish', 'subscription-identifier', '1'], "Error: subscription-identifier property not supported for publish in --property argument.\n\n" + helps, 1)

    # Unknown options
    argv_test('mosquitto_pub', ['--unknown'], "Error: Unknown option '--unknown'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-C', '1'], "Error: Unknown option '-C'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-e', 'response-topic'], "Error: Unknown option '-e'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-E'], "Error: Unknown option '-E'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-F', '%p'], "Error: Unknown option '-F'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-N'], "Error: Unknown option '-N'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--pretty'], "Error: Unknown option '--pretty'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-R'], "Error: Unknown option '-R'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--random-filter'], "Error: Unknown option '--random-filter'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--remove-retained'], "Error: Unknown option '--remove-retained'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--retain-as-published'], "Error: Unknown option '--retain-as-published'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--retain-handling', 'invalid'], "Error: Unknown option '--retain-handling'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['--retained-only'], "Error: Unknown option '--retained-only'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-T'], "Error: Unknown option '-T'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-U'], "Error: Unknown option '-U'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-v'], "Error: Unknown option '-v'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-W'], "Error: Unknown option '-W'.\n" + helps, 1)
    argv_test('mosquitto_pub', ['-w'], "Error: Unknown option '-w'.\n" + helps, 1)