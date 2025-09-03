#!/usr/bin/env python3

# Test parsing of command line args and errors. Does not test arg functionality.

from mosq_test_helper import *

def do_test(args, rc_expected, response=None, input=None):
    proc = subprocess.run([mosquitto_signal_path]
                    + args,
                    capture_output=True, encoding='utf-8', timeout=2, input=input)

    if response is not None:
        if proc.stderr != response:
            print(len(proc.stderr))
            print(len(response))
            raise ValueError(proc.stderr)

    if proc.returncode != rc_expected:
        print(proc.returncode)
        raise ValueError(args)

do_test([], 1) # For the usage message
do_test(["--help"], 1)
do_test(["--invalid"], 1, response="Error: One of -a or -p must be used.\n")
do_test(["-p"], 1, response="Error: -p argument given but process ID missing.\n")
do_test(["-p", "0"], 1, response="Error: Process ID must be >0.\n")
do_test(["-p", "1"], 1, response="Error: No signal given.\n")
do_test(["-a"], 1, response="Error: No signal given.\n")
do_test(["-p", "1", "invalid"], 1, response="Error: Unknown signal 'invalid'.\n")
do_test(["-p", "1", "config-reload"], 0)
do_test(["-p", "1", "log-rotate"], 0)
do_test(["-p", "1", "shutdown"], 0)
do_test(["-p", "1", "tree-print"], 0)
do_test(["-p", "1", "xtreport"], 0)
do_test(["-a", "config-reload"], 0)
exit(0)
