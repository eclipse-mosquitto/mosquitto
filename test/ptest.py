#!/usr/bin/env python3

import json
import os
from pathlib import Path
import subprocess
import time
import sys

COLOUR_PASS = 34
COLOUR_SKIP = 242
COLOUR_FAIL = 124

class PTestCase():
    def __init__(self, path, ports, cmd, args=None):
        self.path = path
        self.ports = ports
        self.cmd = str(cmd)
        self.attempts = 0
        if args is not None:
            self.args = [self.cmd] + args
        else:
            self.args = [self.cmd]
        self.start_time = 0
        self.proc = None
        self.mosq_port = None
        self.runtime = 0

    def start(self):
        self.run_args = self.args.copy()
        for p in self.mosq_port:
            self.run_args.append(str(p))

        self.proc = subprocess.Popen(self.run_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self.path)
        self.start_time = time.time()

    def print_result(self, attempt, col):
        cmd = " ".join(self.run_args)
        if col == COLOUR_PASS:
            stat = "✓"
        elif col == COLOUR_SKIP:
            stat = "skip"
        elif col == COLOUR_FAIL:
            stat = "✗"
        else:
            stat = attempt

        if sys.stdout.isatty():
            ansi_col = f"\033[38:5:{col}m"
            ansi_reset = "\033[0m"
        else:
            ansi_col = ""
            ansi_reset = ""
        print(f"{self.runtime:0.3f}s : {stat} : {ansi_col}{self.path}/{cmd}{ansi_reset}")

    def print_timed_out(self, col):
        cmd = " ".join(self.run_args)
        if sys.stdout.isatty():
            ansi_col = f"\033[38:5:{col}m"
            ansi_reset = "\033[0m"
        else:
            ansi_col = ""
            ansi_reset = ""
        print(f"{self.runtime:0.3f}s : ⏳ : {ansi_col}{self.path}/{cmd}{ansi_reset}")

    def print_log(self):
        (stdo, stde) = self.proc.communicate()
        print(stdo.decode('utf-8'))
        print(stde.decode('utf-8'))


class PTest():
    def __init__(self, minport=1888, max_running=20):
        self.minport = minport
        self.max_running = 20
        self.tests = []

    def add_tests(self, test_list, path=".", label=""):
        for testdef in test_list:
            try:
                if isinstance(testdef[2], (list,)):
                    args = testdef[2]
                else:
                    args = [testdef[2]]
            except IndexError:
                args = None
            self.tests.append(PTestCase(path, testdef[0], testdef[1], args))

    def _next_test(self, ports):
        if len(self.tests) == 0 or len(ports) == 0:
            return

        test = self.tests.pop()
        test.mosq_port = []

        if len(ports) < test.ports:
            self.tests.insert(0, test)
            return None
        else:
            for i in range(0, test.ports):
                proc_port = ports.pop()
                test.mosq_port.append(proc_port)

            test.start()
            return test

    def run_tests(self, test_list):
        self.add_tests(test_list)
        self.run()

    def load_failed_tests(self):
        with open("failed-tests.json", "rt") as f:
            return json.loads(f.read())

    def run_failed_tests(self):
        test_list = self.load_failed_tests()
        self.add_tests(test_list)
        self.run()

    def run(self):
        ports = list(range(self.minport, self.minport+self.max_running+1))
        start_time = time.time()
        passed = 0
        retried = 0
        failed = 0
        skipped = 0

        failed_tests = []
        failed_tests_output = []
        running_tests = []
        retry_tests = []
        while len(self.tests) > 0 or len(running_tests) > 0 or len(retry_tests) > 0:
            if len(running_tests) <= self.max_running:
                t = self._next_test(ports)
                if t is None:
                    time.sleep(0.1)
                else:
                    running_tests.append(t)

            if len(running_tests) == 0 and len(self.tests) == 0 and len(retry_tests) > 0:
                # Only retry tests after everything else has finished to reduce load
                self.tests = retry_tests
                retry_tests = []

            for t in running_tests:
                t.proc.poll()
                t.runtime = time.time() - t.start_time
                if t.proc.returncode is not None:
                    running_tests.remove(t)

                    for portret in t.mosq_port:
                        ports.append(portret)
                    t.proc.terminate()
                    t.proc.wait()

                    if t.proc.returncode == 77:
                        skipped += 1
                        t.print_result(0, COLOUR_SKIP)
                        continue

                    if t.proc.returncode != 0 and t.attempts < 5:
                        t.print_result(t.attempts+1, 226-6*t.attempts)
                        retried += 1
                        t.attempts += 1
                        t.proc = None
                        t.mosq_port = None
                        retry_tests.append(t)
                        continue

                    if t.proc.returncode != 0:
                        t.print_result(0, COLOUR_FAIL)
                        failed = failed + 1
                        failed_tests.append(t.cmd)
                        failed_tests_output.append([t.ports]+t.args)
                        print(f"{t.cmd}:")
                        t.print_log()
                    else:
                        passed = passed + 1
                        t.print_result(0, COLOUR_PASS)
                elif t.runtime > 180: # 3 minutes max
                    t.print_timed_out(226-6*t.attempts)
                    t.proc.terminate()
                    t.proc.wait()
                    t.print_log()

        print("Passed: %d\nRetried: %d\nFailed: %d\nSkipped: %d\nTotal: %d\nTotal time: %0.2f" % (passed, retried, failed, skipped, passed+failed+skipped, time.time()-start_time))
        if failed > 0:
            print("Failing tests:")
            failed_tests.sort()
            for f in failed_tests:
                print(f)
            with open("failed-tests.json", "wt") as out:
                out.write(json.dumps(failed_tests_output))
            sys.exit(1)
        else:
            try:
                os.remove("failed-tests.json")
            except FileNotFoundError:
                pass
