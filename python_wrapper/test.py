from threading import Thread
import time
from IPython import embed

import openua


class UAServer(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.server = openua.Server()
        self.status = None

    def run(self):
        print("start server")
        self.status = self.server.run()
        print("server stopped")

    def stop(self):
        print("trying to stop server")
        self.server.stop()


if __name__ == "__main__":
    s = UAServer()
    s.start()
    try:
        embed()
    finally:
        s.stop()
        time.sleep(0.1)
        print("Stopped with status: ", s.status)


