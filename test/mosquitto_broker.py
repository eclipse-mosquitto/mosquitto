"""
"""

from subprocess import Popen
from typing import Optional, Union

import logging
import os

import mosq_test

class MosquittoBroker:
    def __init__(
        self,
        port: Optional[int] = None,
        use_conf=False,
        config_file_name=None,
        env=None,
        termination_timeout = 10,
    ):
        assert port is not None
        self._port = port
        self._use_conf = use_conf

        if config_file_name is None or config_file_name == "":
            config_file_name = f"{str(self._port)}.conf"
        self._config_file_name = config_file_name
        self._process :Optional[Popen]= None
        self.env = env
        self._termination_timeout = termination_timeout

    def __str__(self):
        return f"{self.__class__.__name__}:{self._port}"

    @property
    def port(self):
        return self._port

    @property
    def process(self):
        if self._process is None:
            raise RuntimeError("process not started yet")
        return self._process

    def start(self, expect_fail=False, check_port=True, start_timeout=0.1):
        logging.info(f"Starting {self}")
        self._process = mosq_test.start_broker(
            use_conf=self._use_conf,
            filename = self._config_file_name,
            port=self._port,
            env=self.env,
            expect_fail=expect_fail,
            check_port=check_port,
            timeout=start_timeout
        )
        logging.info(f"{self} "+ "Started" if self.is_running() else f"Terminated rc={self._process.returncode}")

    def is_running(self):
        return self._process and self._process.poll() is None

    def get_log(self):
        return mosq_test.broker_log(self._process)

    def terminate(self):
        if self._process:
            mosq_test.terminate_broker(self._process)

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, ex_type, value, tb):
        self.stop(ex_type)

    def stop(self, ex_type=None):
        if self._process:
            timed_out, _ = mosq_test.terminate_broker(self._process)
            logging.info(f"Stopping {self}")
            logging.info(f"Stopped {self}")
            if ex_type is not None or timed_out or self._process.returncode != 0:
                print(f"\n{self} log:")
                print(self.get_log())
            else:
                logging.debug(f"\n{self} log:")
                logging.debug(self.get_log())
            if timed_out:
                raise RuntimeError(f"{self} timed out when shutting down")
            if self._process.returncode != 0:
                raise RuntimeError(f"{self} exited with {self._process.returncode}")
