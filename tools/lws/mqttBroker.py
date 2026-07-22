#!/usr/bin/env python3

"""Minimal MQTT 3.1.1 broker used by the LWS ConnectionManager tests."""

import argparse
import os
from pathlib import Path
import socketserver
import ssl
import subprocess
import threading


class BrokerState:
    def __init__(self):
        self.lock = threading.Lock()
        self.clients = set()

    def publish(self, topic, payload):
        packet = bytes([0x30]) + encode_remaining_length(2 + len(topic) + len(payload))
        packet += len(topic).to_bytes(2, "big") + topic + payload
        with self.lock:
            clients = list(self.clients)
        for client in clients:
            if topic in client.topics:
                try:
                    client.request.sendall(packet)
                except OSError:
                    pass


STATE = BrokerState()


def encode_remaining_length(length):
    result = bytearray()
    while True:
        digit = length % 128
        length //= 128
        if length:
            digit |= 0x80
        result.append(digit)
        if not length:
            return bytes(result)


def read_exact(stream, length):
    data = bytearray()
    while len(data) < length:
        chunk = stream.read(length - len(data))
        if not chunk:
            raise EOFError
        data.extend(chunk)
    return bytes(data)


def read_packet(stream):
    first = stream.read(1)
    if not first:
        raise EOFError
    multiplier = 1
    remaining = 0
    while True:
        digit = read_exact(stream, 1)[0]
        remaining += (digit & 0x7F) * multiplier
        if not digit & 0x80:
            break
        multiplier *= 128
        if multiplier > 128 * 128 * 128:
            raise ValueError("invalid MQTT remaining length")
    return first[0], read_exact(stream, remaining)


class MqttHandler(socketserver.StreamRequestHandler):
    def setup(self):
        super().setup()
        self.topics = set()
        with STATE.lock:
            STATE.clients.add(self)

    def finish(self):
        with STATE.lock:
            STATE.clients.discard(self)
        super().finish()

    def handle(self):
        try:
            while True:
                header, body = read_packet(self.rfile)
                packet_type = header >> 4
                if packet_type == 1:  # CONNECT
                    self.request.sendall(b"\x20\x02\x00\x00")
                elif packet_type == 3:  # PUBLISH, QoS 0
                    topic_len = int.from_bytes(body[:2], "big")
                    topic = body[2 : 2 + topic_len]
                    STATE.publish(topic, body[2 + topic_len :])
                elif packet_type == 8:  # SUBSCRIBE
                    packet_id = body[:2]
                    pos = 2
                    granted = bytearray()
                    while pos < len(body):
                        topic_len = int.from_bytes(body[pos : pos + 2], "big")
                        pos += 2
                        topic = body[pos : pos + topic_len]
                        pos += topic_len
                        pos += 1  # requested QoS
                        self.topics.add(topic)
                        granted.append(0)
                    response = packet_id + bytes(granted)
                    self.request.sendall(b"\x90" + encode_remaining_length(len(response)) + response)
                elif packet_type == 10:  # UNSUBSCRIBE
                    packet_id = body[:2]
                    pos = 2
                    while pos < len(body):
                        topic_len = int.from_bytes(body[pos : pos + 2], "big")
                        pos += 2
                        self.topics.discard(body[pos : pos + topic_len])
                        pos += topic_len
                    self.request.sendall(b"\xb0\x02" + packet_id)
                elif packet_type == 12:  # PINGREQ
                    self.request.sendall(b"\xd0\x00")
                elif packet_type == 14:  # DISCONNECT
                    return
        except (EOFError, OSError, ValueError):
            return


class ThreadingBroker(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def run_test(command):
    certificate_dir = Path(__file__).resolve().parents[2] / "tests/client/client_json"
    tls_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls_context.minimum_version = ssl.TLSVersion.TLSv1_2
    tls_context.load_cert_chain(certificate_dir / "server_cert.pem",
                                certificate_dir / "server_key.pem")
    mtls_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    mtls_context.minimum_version = ssl.TLSVersion.TLSv1_2
    mtls_context.load_cert_chain(certificate_dir / "server_cert.pem",
                                 certificate_dir / "server_key.pem")
    mtls_context.load_verify_locations(certificate_dir / "server_cert.pem")
    mtls_context.verify_mode = ssl.CERT_REQUIRED

    with (ThreadingBroker(("127.0.0.1", 0), MqttHandler) as broker,
          ThreadingBroker(("127.0.0.1", 0), MqttHandler) as tls_broker,
          ThreadingBroker(("127.0.0.1", 0), MqttHandler) as mtls_broker):
        tls_broker.socket = tls_context.wrap_socket(tls_broker.socket,
                                                    server_side=True)
        mtls_broker.socket = mtls_context.wrap_socket(mtls_broker.socket,
                                                      server_side=True)
        thread = threading.Thread(target=broker.serve_forever, daemon=True)
        tls_thread = threading.Thread(target=tls_broker.serve_forever, daemon=True)
        mtls_thread = threading.Thread(target=mtls_broker.serve_forever, daemon=True)
        thread.start()
        tls_thread.start()
        mtls_thread.start()
        env = os.environ.copy()
        env["OPEN62541_TEST_MQTT_PORT"] = str(broker.server_address[1])
        env["OPEN62541_TEST_MQTTS_PORT"] = str(tls_broker.server_address[1])
        env["OPEN62541_TEST_MQTTS_MTLS_PORT"] = str(mtls_broker.server_address[1])
        try:
            return subprocess.run(command, env=env, check=False).returncode
        finally:
            broker.shutdown()
            tls_broker.shutdown()
            mtls_broker.shutdown()
            thread.join()
            tls_thread.join()
            mtls_thread.join()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-test", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.run_test:
        parser.error("--run-test requires a command")
    return run_test(args.run_test)


if __name__ == "__main__":
    raise SystemExit(main())
