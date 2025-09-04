#!/usr/bin/env python3

# Test whether a valid CONNECT results in the correct CONNACK packet.
from collections import deque
from mosq_test_helper import *
from os import walk

import atexit
import importlib
import json
import mosq_test
import platform
import socket

send = 1
recv = 2
disconnected_check = 3
connected_check = 4
publish = 5

vg_index = 1
vg_logfiles = []

@atexit.register
def test_cleanup():
    global vg_logfiles

    for f in vg_logfiles:
        try:
            if os.stat(f).st_size == 0:
                os.remove(f)
        except OSError:
            pass

class SingleMsg(object):
    __slots__ = 'action', 'message', 'comment'
    def __init__(self, action, message, comment=''):
        self.action = action
        self.message = message
        self.comment = comment

class MsgSequence(object):
    __slots__ = 'name', 'port', 'proto_ver', 'msgs', 'expect_disconnect', 'sock', 'client', 'clean_start', 'command'

    def __init__(self, name, port, default_connect=True, default_connack=True, proto_ver=4, clean_start=True, expect_disconnect=True, command=None):
        self.name = name
        self.msgs = deque()
        self.expect_disconnect = expect_disconnect
        self.port = port
        self.proto_ver = proto_ver
        self.clean_start = clean_start
        self.command = command
        self.sock = -1
        self.client = None
        if default_connect:
            self.add_recv(mosq_test.gen_connect("fuzzish", proto_ver=proto_ver), "default connect")
        if default_connack:
            properties = mqtt5_props.gen_uint16_prop(mqtt5_props.RECEIVE_MAXIMUM, 20)
            self.add_send(mosq_test.gen_connack(rc=0, proto_ver=proto_ver, properties=properties, property_helper=False), "default connack")

    def add_msg(self, message):
        try:
            c = message["comment"]
        except KeyError:
            c = ""
        if message["type"] == "send":
            self.add_send(parse_message(message["payload"]), c)
        elif message["type"] == "recv":
            self.add_recv(parse_message(message["payload"]), c)
        elif message["type"] == "publish":
            self.add_publish(message, c)

    def add_send(self, message, comment=""):
        self._add(send, message, comment)

    def add_recv(self, message, comment):
        self._add(recv, message, comment)

    def add_publish(self, message, comment):
        self._add(publish, message, comment)

    def add_connected_check(self):
        self._add(connected_check, b"")

    def add_disconnected_check(self):
        self._add(disconnected_check, b"")

    def run_client(self, server_sock, port):
        global vg_index
        global vg_logfiles

        env = mosq_test.env_add_ld_library_path()
        cmd = [
            Path(mosq_test.get_build_root(), 'test', 'lib', 'c', mosq_test.get_build_type(), 'fuzzish.exe'),
            str(port), str(self.proto_ver), str(self.clean_start)
        ]
        if os.environ.get('MOSQ_USE_VALGRIND') is not None:
            logfile = 'seq.'+str(vg_index)+'.vglog'
            cmd = ['/snap/bin/valgrind', '-q', '--trace-children=yes', '--leak-check=full', '--show-leak-kinds=all', '--log-file='+logfile] + cmd
            vg_logfiles.append(logfile)
            vg_index += 1

        if self.command is not None:
            cmd.append(self.command)
        self.client = subprocess.Popen(cmd, stderr=subprocess.PIPE, env=env)
        (self.sock, _) = server_sock.accept()

    def kill_client(self):
        self.sock.close()
        self.client.terminate()
        self.client.wait()
        if (platform.system() != 'Windows' and self.client.returncode != 0) \
                or (platform.system() == 'Windows' and self.client.returncode != 1):
            raise RuntimeError

    def _add(self, action, message, comment=""):
        msg = SingleMsg(action, message, comment)
        self.msgs.append(msg)

    def _connected_check(self):
        if not self._puback_check():
            raise ValueError("connection failed")

    def _send_message(self, msg):
        self.sock.send(msg.message)

    def _publish_message(self, msg):
        sock = mosq_test.client_connect_only(hostname="localhost", port=1888, timeout=2)
        sock.send(mosq_test.gen_connect("helper"))
        mosq_test.expect_packet(sock, "connack", mosq_test.gen_connack(rc=0))

        m = msg.message
        if m['qos'] == 0:
            sock.send(mosq_test.gen_publish(topic=m['topic'], payload=m['payload']))
        elif m['qos'] == 1:
            sock.send(mosq_test.gen_publish(mid=1, qos=1, topic=m['topic'], payload=m['payload']))
            mosq_test.expect_packet(sock, "helper puback", mosq_test.gen_puback(mid=1))
        elif m['qos'] == 2:
            sock.send(mosq_test.gen_publish(mid=1, qos=2, topic=m['topic'], payload=m['payload']))
            mosq_test.expect_packet(sock, "helper pubrec", mosq_test.gen_pubrec(mid=1))
            sock.send(mosq_test.gen_pubrel(mid=1))
            mosq_test.expect_packet(sock, "helper pubcomp", mosq_test.gen_pubcomp(mid=1))
        sock.close()

    def _recv_message(self, msg):
        data = self.sock.recv(len(msg.message))
        if data != msg.message:
            raise ValueError("Receive message %s | %s | %s" % (msg.comment, data, msg.message))


    def _puback_check(self):
        publish_packet = mosq_test.gen_publish(mid=65535, qos=1, topic="alive check", payload="payload", proto_ver=self.proto_ver)
        puback_packet = mosq_test.gen_puback(mid=65535, proto_ver=self.proto_ver)
        self.sock.send(publish_packet)
        packet = self.sock.recv(len(puback_packet))
        return packet == puback_packet


    def _disconnected_check(self):
        try:
            if self._puback_check() and self.expect_disconnect:
                raise ValueError("Still connected")
        except (ConnectionResetError, ConnectionAbortedError):
            if self.expect_disconnect:
                pass
            else:
                raise

    def _process_message(self, msg):
        if msg.action == send:
            self._send_message(msg)
        elif msg.action == recv:
            self._recv_message(msg)
        elif msg.action == publish:
            self._publish_message(msg)
        elif msg.action == disconnected_check:
            self._disconnected_check()
        elif msg.action == connected_check:
            self._connected_check()

    def process_next(self):
        msg = self.msgs.popleft()
        self._process_message(msg)

    def process_all(self):
        while len(self.msgs):
            self.process_next()
        if self.expect_disconnect:
            self._disconnected_check()
        else:
            self._connected_check()


def parse_message(message):
    b = bytes()
    parts = message.split(" ")
    for i in range(0, len(parts)):
        if len(parts[i]) == 0:
            continue
        elif parts[i][0] in ['i']:
            # General 8-bit unsigned decimal
            b += int(parts[i][1:]).to_bytes(length=1, byteorder='big', signed=False)
        elif parts[i][0] in ['H', 'k', 'm', 's']:
            # General 16-bit unsigned decimal
            # Or 'k' keepalive specific
            # Or 'm' mid specific
            # Or 's' string specific
            b += int(parts[i][1:]).to_bytes(length=2, byteorder='big', signed=False)
        elif parts[i][0] == "L":
            # 32-bit unsigned decimal
            b += int(parts[i][1:]).to_bytes(length=4, byteorder='big', signed=False)
        elif parts[i][0] == "'":
            s = parts[i][1:]
            while s[-1] != "'" and i < len(parts)-1:
                i += 1
                s += " " + parts[i]
            if s[-1] != "'":
                raise ValueError(f"message {message} has incomplete string type")
            b += bytes(s[0:-1].encode('utf-8'))
        elif parts[i][0] in ['v', 'r']:
            # General variable length integer
            # Or 'r' remaining length
            v = int(parts[i][1:])

            # This allows non-compliant values >=2^28
            while True:
                byte = v % 128
                v = v // 128

                if v > 0:
                    byte = byte | 0x80
                b += byte.to_bytes(length=1, byteorder='big', signed=False)
                if v == 0:
                    break
        else:
            # hex
            try:
                b += bytes.fromhex(parts[i])
            except ValueError:
                raise ValueError(f"message {message} has invalid hex bytes")

    return b


def do_test(hostname, port):
    data_path=Path(__file__).resolve().parent/"data"
    rc = 0
    sequences = []
    for (_, _, filenames) in walk(data_path):
        sequences.extend(filenames)
        break

    total = 0
    succeeded = 0
    test = None

    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.settimeout(10)
    server_sock.bind(('', port))
    server_sock.listen(5)

    for seq in sorted(sequences):
        if seq[-5:] != ".json":
            continue

        with open(data_path/seq, "r") as f:
            test_file = json.load(f)

        for g in test_file:
            group_name = g["group"]
            try:
                disabled = g["disable"]
                if disabled:
                    continue
            except KeyError:
                pass
            try:
                g_command = g["command"]
            except KeyError:
                g_command = None
            try:
                g_proto_ver = g["ver"]
            except KeyError:
                g_proto_ver = 4
            try:
                g_clean_start = g["clean_start"]
            except KeyError:
                g_clean_start = True
            try:
                g_connect = g["connect"]
            except KeyError:
                g_connect = True
            try:
                g_connack = g["connack"]
            except KeyError:
                g_connack = True
            try:
                g_expect_disconnect = g["expect_disconnect"]
            except KeyError:
                g_expect_disconnect = True

            try:
                group_msgs = g["group_msgs"]
            except KeyError:
                group_msgs = None

            tests = g["tests"]

            for t in tests:
                tname = group_name + " " + t["name"]
                try:
                    command = t["command"]
                except KeyError:
                    command = g_command
                try:
                    proto_ver = t["ver"]
                except KeyError:
                    proto_ver = g_proto_ver
                try:
                    clean_start = t["clean_start"]
                except KeyError:
                    clean_start = g_clean_start
                try:
                    connect = t["connect"]
                except KeyError:
                    connect = g_connect
                try:
                    connack = t["connack"]
                except KeyError:
                    connack = g_connack
                try:
                    expect_disconnect = t["expect_disconnect"]
                except KeyError:
                    expect_disconnect = g_expect_disconnect

                this_test = MsgSequence(tname, port,
                        proto_ver=proto_ver,
                        clean_start=clean_start,
                        expect_disconnect=expect_disconnect,
                        default_connect=connect,
                        default_connack=connack,
                        command=command)

                if group_msgs is not None:
                    for m in group_msgs:
                        this_test.add_msg(m)

                for m in t["msgs"]:
                    this_test.add_msg(m)

                this_test.run_client(server_sock, port)

                total += 1
                try:
                    this_test.process_all()
                    this_test.kill_client()
                    this_test = None
                    #print("\033[32m" + tname + "\033[0m")
                    succeeded += 1
                except (ValueError, ConnectionResetError, socket.timeout, mosq_test.TestError, RuntimeError) as e:
                    print("\033[31m" + tname + " failed: " + str(e) + "\033[0m")
                    rc = 1
                finally:
                    if this_test is not None:
                        try:
                            this_test.kill_client()
                        except RuntimeError:
                            pass

    print("%d tests total\n%d tests succeeded" % (total, succeeded))
    return rc

hostname = "localhost"
port = mosq_test.get_port()

rc = do_test(hostname=hostname, port=port)
exit(rc)
