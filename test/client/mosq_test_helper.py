import inspect, os, sys

# From http://stackoverflow.com/questions/279237/python-import-a-module-from-a-folder
cmd_subfolder = os.path.realpath(os.path.abspath(os.path.join(os.path.split(inspect.getfile( inspect.currentframe() ))[0],"..")))
if cmd_subfolder not in sys.path:
    sys.path.insert(0, cmd_subfolder)

import mosq_test
import mqtt5_opts
import mqtt5_props
import mqtt5_rc

import socket
import ssl
import struct
import subprocess
import time
import errno

from pathlib import Path

source_dir = Path(__file__).resolve().parent

def argv_test(cmd, args, stderr_expected, rc_expected):
    rc = 1

    port = mosq_test.get_port()

    env = {
        'XDG_CONFIG_HOME':'/tmp/missing'
    }
    env = mosq_test.env_add_ld_library_path(env)
    cmd = [mosq_test.get_client_path(cmd)] + args

    client = subprocess.run(cmd, capture_output=True, encoding='utf-8', env=env)
    stde = client.stderr.strip()
    if client.returncode != rc_expected:
        raise mosq_test.TestError(f"Return code of {cmd}: {client.returncode} != {rc_expected}, stderr: {stde}")
    if stderr_expected is not None and stde != stderr_expected.strip():
        raise mosq_test.TestError(f"Error log not as expected, got:\n{stde}\nExpected:\n{stderr_expected}")