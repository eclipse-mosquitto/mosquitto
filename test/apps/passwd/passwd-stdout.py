#!/usr/bin/env python3

# Test parsing of command line args and errors. Does not test arg functionality.

from mosq_test_helper import *

def do_test(args, rc_expected, response=None, input=None):
    proc = subprocess.run([mosquitto_passwd_path]
                    + args,
                    capture_output=True, encoding='utf-8', timeout=2, input=input,
                    env=mosq_test.env_add_ld_library_path())

    if response is not None:
        if proc.stdout[0:len(response)] != response:
            print(len(proc.stdout))
            print(len(response))
            print(proc.stdout[0:len(response)])
            print(response)
            raise ValueError(proc.stdout)

    if proc.returncode != rc_expected:
        print(proc.returncode)
        raise ValueError(args)

resp = "Password: \nReenter password: \nstdout:$7$1000$"
do_test(["-c", "-", "stdout"], 0, response=resp, input="pw\npw\n")

resp = "stdout:$7$1000$"
do_test(["-b", "-c", "-", "stdout", "pw"], 0, response=resp)

exit(0)
