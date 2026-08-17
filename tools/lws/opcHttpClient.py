#!/usr/bin/env python3
# Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)

import argparse
import gzip
import http.client
import shutil
import socket
import struct
import subprocess
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


def request_body(request_handle):
    payload = bytearray.fromhex(
        "0100a601000000000000000000004101000000000000ffffffff00000000"
        "000000ffffffffffffffffffffffff"
    )[4:]
    struct.pack_into("<q", payload, 2,
                     time.time_ns() // 100 + 116444736000000000)
    struct.pack_into("<I", payload, 10, request_handle)
    # OPC UA Binary over HTTPS carries the service TypeId followed directly
    # by the encoded service structure (the same service payload as UATCP,
    # without an additional ExtensionObject envelope).
    return bytes.fromhex("0100a601") + payload


def validate_response(payload, request_handle):
    if payload[:4] != struct.pack("<BBH", 0x01, 0, 425):
        service_result = (
            struct.unpack_from("<I", payload, 16)[0]
            if len(payload) >= 20
            else None
        )
        raise RuntimeError(
            "unexpected OPC UA response type: "
            f"{payload[:24].hex()} (serviceResult={service_result})"
        )
    if len(payload) < 16:
        raise RuntimeError("truncated OPC UA Binary service response")
    if struct.unpack_from("<I", payload, 12)[0] != request_handle:
        raise RuntimeError("OPC UA request handle was not preserved")


def post_request(port, request_handle, headers=None, body=None):
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=3)
    request_headers = {"Content-Type": "application/octet-stream"}
    if headers:
        request_headers.update(headers)
    try:
        connection.request(
            "POST", "/ua", body=body or request_body(request_handle),
            headers=request_headers
        )
        response = connection.getresponse()
        payload = response.read()
        return response.status, {
            name.lower(): value for name, value in response.getheaders()
        }, payload
    finally:
        connection.close()


def receive_raw_response(raw):
    response = bytearray()
    header_end = -1
    expected = None
    while expected is None or len(response) < expected:
        chunk = raw.recv(4096)
        if not chunk:
            break
        response.extend(chunk)
        if header_end < 0:
            header_end = response.find(b"\r\n\r\n")
            if header_end >= 0:
                header = bytes(response[:header_end]).lower()
                content_length = 0
                for line in header.split(b"\r\n"):
                    if line.startswith(b"content-length:"):
                        content_length = int(line.split(b":", 1)[1].strip())
                expected = header_end + 4 + content_length
    return bytes(response)


def exercise_raw_socket(port):
    handle = 654
    body = request_body(handle)
    request = (
        b"POST /ua HTTP/1.1\r\nHost: localhost\r\n"
        b"Content-Type: application/octet-stream\r\n"
        + f"Content-Length: {len(body)}\r\nConnection: close\r\n\r\n".encode()
        + body
    )
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw:
        for length in (1, 2, 3, 5, 8, 13):
            if not request:
                break
            raw.sendall(request[:length])
            request = request[length:]
        raw.sendall(request)
        response = receive_raw_response(raw)
    header, payload = response.split(b"\r\n\r\n", 1)
    if not header.startswith(b"HTTP/1.1 200"):
        raise RuntimeError(f"raw HTTP request failed: {header[:80]!r}")
    validate_response(payload, handle)

    unsupported = (
        b"POST /ua HTTP/1.1\r\nHost: localhost\r\n"
        b"Content-Type: application/octet-stream\r\n"
        b"Content-Encoding: br\r\nContent-Length: 1\r\n"
        b"Connection: close\r\n\r\nx"
    )
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw:
        raw.sendall(unsupported)
        response = receive_raw_response(raw)
    if not response.startswith(b"HTTP/1.1 415"):
        raise RuntimeError(f"unsupported coding was not rejected: {response[:80]!r}")

    # Broken and malformed carriers must not poison the listener or siblings.
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw:
        raw.sendall(b"POST /ua HTTP/1.1\r\nHost: localhost\r\n")
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw:
        raw.sendall(
            b"POST /ua HTTP/1.1\r\nHost: localhost\r\n"
            b"Transfer-Encoding: chunked\r\n\r\nz\r\n"
        )
    status, _, payload = post_request(port, 655)
    if status != 200:
        raise RuntimeError(f"listener did not survive broken carriers: {status}")
    validate_response(payload, 655)


def exercise_curl(port, temp_dir):
    if not shutil.which("curl"):
        return
    handle = 777
    request_path = Path(temp_dir) / "request.bin"
    response_path = Path(temp_dir) / "response.bin"
    headers_path = Path(temp_dir) / "headers.txt"
    request_path.write_bytes(request_body(handle))
    subprocess.run(
        ["curl", "--http1.1", "--fail", "--silent", "--show-error",
         "--output", str(response_path), "--dump-header", str(headers_path),
         "--header", "Content-Type: application/octet-stream",
         "--data-binary", f"@{request_path}",
         f"http://127.0.0.1:{port}/ua"],
        check=True,
    )
    if not headers_path.read_bytes().startswith(b"HTTP/1.1 200"):
        raise RuntimeError("curl did not receive HTTP 200")
    validate_response(response_path.read_bytes(), handle)


def exercise_load(port):
    # Reuse one carrier long enough to expose keep-alive lifecycle leaks.
    connection = http.client.HTTPConnection("127.0.0.1", port, timeout=3)
    try:
        for handle in range(1000, 1050):
            connection.request(
                "POST", "/ua", body=request_body(handle),
                headers={"Content-Type": "application/octet-stream"},
            )
            response = connection.getresponse()
            payload = response.read()
            if response.status != 200:
                raise RuntimeError(f"keep-alive status {response.status}")
            validate_response(payload, handle)
    finally:
        connection.close()

    # Independent carriers fail or complete without sharing request state.
    def one_request(handle):
        status, _, payload = post_request(port, handle)
        if status != 200:
            raise RuntimeError(f"parallel status {status}")
        validate_response(payload, handle)

    with ThreadPoolExecutor(max_workers=8) as executor:
        list(executor.map(one_request, range(2000, 2032)))


def run_python_client(command):
    native = subprocess.run(command, check=False)
    if native.returncode:
        return native.returncode

    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    with tempfile.TemporaryDirectory() as temp_dir:
        marker = Path(temp_dir) / "complete"
        server = subprocess.Popen(
            [*command, "--python-http-server", str(port), str(marker)]
        )
        try:
            for attempt in range(100):
                try:
                    status, headers, payload = post_request(port, 321)
                    if status != 200:
                        raise RuntimeError(f"unexpected HTTP status {status}")
                    if headers.get("content-type", "") != "application/octet-stream":
                        raise RuntimeError("unexpected Content-Type")
                    validate_response(payload, 321)
                    break
                except (ConnectionError, OSError):
                    if attempt == 99:
                        raise
                    time.sleep(0.02)

            # Compression is strict when compiled in and explicitly reported as
            # unsupported otherwise, so both build configurations are covered.
            compressed = gzip.compress(request_body(444))
            status, headers, payload = post_request(
                port, 444,
                {"Content-Encoding": "gzip", "Accept-Encoding": "gzip"},
                compressed,
            )
            if status == 200:
                if headers.get("content-encoding", "") == "gzip":
                    payload = gzip.decompress(payload)
                validate_response(payload, 444)
            elif status != 415:
                raise RuntimeError(f"unexpected gzip status {status}")

            status, _, payload = post_request(
                port, 445, {"Accept-Encoding": "br, identity;q=0"}
            )
            if status != 406 or payload:
                raise RuntimeError(
                    f"unacceptable response coding was not rejected: {status}"
                )

            exercise_raw_socket(port)
            exercise_curl(port, temp_dir)
            exercise_load(port)
            marker.touch()
            return server.wait(timeout=10)
        finally:
            if server.poll() is None:
                server.terminate()
                server.wait(timeout=10)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-test", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    return run_python_client(args.run_test)


if __name__ == "__main__":
    raise SystemExit(main())
