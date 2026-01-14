#!/usr/bin/env python3

from mosq_test_helper import *

def do_test(file, counts):
    stdout = f"SC: {counts[0]} " + \
        f"SS: {counts[1]} " + \
        f"MC: {counts[2]} " + \
        f"MS: {counts[3]} " + \
        f"  {counts[4]}\n"

    cmd = [
        mosquitto_db_dump_path,
        '--client-stats',
        Path(test_dir, "apps", "db_dump", "data", file)
    ]
    env = mosq_test.env_add_ld_library_path()

    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=1, encoding='utf-8', env=env)
    if res.stdout != stdout:
        print(res.stdout)
        print(stdout)
        raise mosq_test.TestError

do_test('v6-single-all.test-db', [1,27,1,111,'single-all'])
