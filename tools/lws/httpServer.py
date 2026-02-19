from http.server import SimpleHTTPRequestHandler, HTTPServer
import urllib.parse
import threading
import socket

PORT = 8000

# Create a basic request handler
class MyHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(b"<h1>Hello, World!</h1><p>This is a simple Python HTTP server.</p>")
        else:
            self.send_error(404, "Page not found")

    def do_POST(self):
        if self.path == "/post":
            # Get content length (size of data)
            content_length = int(self.headers.get("Content-Length", 0))
            post_data = self.rfile.read(content_length)  # Read request body
            parsed_data = urllib.parse.parse_qs(post_data.decode("utf-8"))

            # Respond with received data
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            response = f'{{"message": "Data received", "data": {parsed_data}}}'
            self.wfile.write(response.encode("utf-8"))
        else:
            self.send_error(404, "Endpoint not found")

class HTTPServerV6(HTTPServer):
    address_family = socket.AF_INET6

def run_server(server_cls, host, port):
    httpd = server_cls((host, port), MyHandler)
    httpd.allow_reuse_address = True
    print(f"Server running on http://{host if ':' not in host else f'[{host}]'}:{port}")
    httpd.serve_forever()

if __name__ == "__main__":
    # IPv4-Server (127.0.0.1)
    t4 = threading.Thread(target=run_server, args=(HTTPServer, "127.0.0.1", PORT), daemon=True)
    t4.start()

    # IPv6-Server (::1)
    try:
        t6 = threading.Thread(target=run_server, args=(HTTPServerV6, "::1", PORT), daemon=True)
        t6.start()
    except OSError as e:
        print(f"IPv6 not available: {e}")

    t4.join()
