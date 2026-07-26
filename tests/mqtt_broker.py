#!/usr/bin/env python3
"""Run a command with a minimal local MQTT 3.1.1 broker on port 1883."""

import argparse
import os
import socket
import socketserver
import subprocess
import threading


def read_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise EOFError
        data.extend(chunk)
    return bytes(data)


def read_packet(sock):
    header = read_exact(sock, 1)[0]
    multiplier = 1
    remaining = 0
    while True:
        byte = read_exact(sock, 1)[0]
        remaining += (byte & 0x7f) * multiplier
        if not byte & 0x80:
            break
        multiplier *= 128
        if multiplier > 128**3:
            raise ValueError("invalid MQTT remaining length")
    return header, read_exact(sock, remaining)


def encode_length(length):
    encoded = bytearray()
    while True:
        byte = length % 128
        length //= 128
        if length:
            byte |= 0x80
        encoded.append(byte)
        if not length:
            return bytes(encoded)


def packet(packet_type, payload=b""):
    return bytes([packet_type]) + encode_length(len(payload)) + payload


def mqtt_string(data, offset=0):
    length = int.from_bytes(data[offset:offset + 2], "big")
    start = offset + 2
    return data[start:start + length], start + length


class Broker(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, address):
        super().__init__(address, Client)
        self.lock = threading.Lock()
        self.subscribers = {}

    def subscribe(self, topic, client):
        with self.lock:
            self.subscribers.setdefault(topic, set()).add(client)

    def remove(self, client):
        with self.lock:
            for clients in self.subscribers.values():
                clients.discard(client)

    def publish(self, topic, payload):
        with self.lock:
            clients = list(self.subscribers.get(topic, ()))
        body = len(topic).to_bytes(2, "big") + topic + payload
        for client in clients:
            client.send(packet(0x30, body))


class Client(socketserver.BaseRequestHandler):
    def setup(self):
        self.write_lock = threading.Lock()

    def send(self, data):
        try:
            with self.write_lock:
                self.request.sendall(data)
        except OSError:
            pass

    def handle(self):
        try:
            while True:
                header, body = read_packet(self.request)
                kind = header >> 4
                if kind == 1:  # CONNECT
                    self.send(packet(0x20, b"\x00\x00"))
                elif kind == 8:  # SUBSCRIBE
                    packet_id = body[:2]
                    offset = 2
                    acknowledgements = bytearray()
                    while offset < len(body):
                        topic, offset = mqtt_string(body, offset)
                        offset += 1  # requested QoS
                        self.server.subscribe(topic, self)
                        acknowledgements.append(0)
                    self.send(packet(0x90, packet_id + acknowledgements))
                elif kind == 3:  # PUBLISH
                    topic, offset = mqtt_string(body)
                    qos = (header >> 1) & 3
                    packet_id = body[offset:offset + 2] if qos else b""
                    if qos:
                        offset += 2
                    self.server.publish(topic, body[offset:])
                    if qos == 1:
                        self.send(packet(0x40, packet_id))
                elif kind == 12:  # PINGREQ
                    self.send(packet(0xd0))
                elif kind == 14:  # DISCONNECT
                    return
        except (EOFError, ConnectionError, OSError, ValueError):
            return
        finally:
            self.server.remove(self)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.command[:1] == ["--"]:
        args.command = args.command[1:]
    if not args.command:
        parser.error("a command is required after --")

    with Broker(("127.0.0.1", 0)) as broker:
        thread = threading.Thread(target=broker.serve_forever, daemon=True)
        thread.start()
        try:
            env = os.environ.copy()
            env["OPEN62541_TEST_MQTT_BROKER"] = (
                f"opc.mqtt://127.0.0.1:{broker.server_address[1]}"
            )
            return subprocess.run(args.command, check=False, env=env).returncode
        finally:
            broker.shutdown()
            thread.join()


if __name__ == "__main__":
    raise SystemExit(main())
