#!/usr/bin/env python3

import argparse
import os
import subprocess
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs


class TestRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/":
            self.send_error(404, "Page not found")
            return

        body = b"<h1>Hello, World!</h1><p>This is a simple Python HTTP server.</p>"
        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        if self.path != "/post":
            self.send_error(404, "Endpoint not found")
            return

        content_length = int(self.headers.get("Content-Length", 0))
        post_data = self.rfile.read(content_length)
        parsed_data = parse_qs(post_data.decode("utf-8"))
        body = (f'{{"message": "Data received", "data": {parsed_data}}}').encode()

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format_string, *args):
        del format_string, args


def run_test(command):
    server = ThreadingHTTPServer(("127.0.0.1", 0), TestRequestHandler)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    environment = os.environ.copy()
    environment["OPEN62541_TEST_HTTP_PORT"] = str(server.server_port)
    try:
        return subprocess.run(command, env=environment, check=False).returncode
    finally:
        server.shutdown()
        server.server_close()
        server_thread.join()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--run-test", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    if args.run_test:
        return run_test(args.run_test)

    server = ThreadingHTTPServer(("127.0.0.1", args.port), TestRequestHandler)
    print(f"Server running on http://127.0.0.1:{server.server_port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
