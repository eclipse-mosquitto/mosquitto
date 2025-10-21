#!/usr/bin/env python3

import inspect, os, sys

# From http://stackoverflow.com/questions/279237/python-import-a-module-from-a-folder
cmd_subfolder = os.path.realpath(
    os.path.abspath(
        os.path.join(os.path.split(inspect.getfile(inspect.currentframe()))[0], "../test/broker")
    )
)
if cmd_subfolder not in sys.path:
    sys.path.insert(0, cmd_subfolder)

from collections import deque
from os import mkdir,walk
from pathlib import Path
import json
import re
import shutil
import msg_sequence_test

def gen_packet_corpus(packet_type, input_path):
    try:
        mkdir("corpora")
    except FileExistsError:
        pass
    try:
        mkdir(f"corpora/{packet_type}")
    except FileExistsError:
        pass

    data_path=Path(input_path)
    rc = 0
    sequences = []
    for (_, _, filenames) in walk(data_path):
        sequences.extend(filenames)
        break

    written_packets = {}
    for seq in sorted(sequences):
        if seq[-5:] != ".json":
            continue

        with open(data_path/seq, "r") as f:
            test_file = json.load(f)

        for g in test_file:
            group_name = g["group"]
            tests = g["tests"]

            for t in tests:
                tname = group_name + " " + t["name"]

                fuzzi = 1
                for m in t["msgs"]:
                    if m["type"] == "send" or m["type"] == "recv":
                        fname = re.sub(r'[\/ \[\]\(\)]+', '-', tname) + str(fuzzi) + ".raw"
                        payload = m["payload"]

                        # No duplicates please
                        if payload not in written_packets:
                            written_packets[payload] = 1
                            with open(f"corpora/{packet_type}/{fname}", "wb") as f:
                                f.write(msg_sequence_test.parse_message(payload))
                            fuzzi += 1
    shutil.make_archive(f"corpora/{packet_type}_packet_seed_corpus", 'zip', f"corpora/{packet_type}")

gen_packet_corpus("broker", "../test/broker/data")
gen_packet_corpus("client", "../test/lib/data")
