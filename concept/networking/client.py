import socket


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("localhost", 4840))
    try:
        while True:
            data = bytes(input(), encoding="utf-8")
            sock.send(data)
    except KeyboardInterrupt:
        pass
    sock.close()


if __name__ == "__main__":
    main()
