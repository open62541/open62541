from __future__ import annotations
import logging
import datetime
import random
import select
import socket
import sys
from abc import ABC, abstractmethod
from typing import Callable, List

FORMAT = '%(asctime)-24s %(levelname)-8s %(module)-16s - %(message)s'
logging.getLogger().setLevel(logging.ERROR)
logging.basicConfig(format=FORMAT)


LOGGER = logging.getLogger(__name__)


class Socket(ABC):

    def __init__(self) -> None:
        super().__init__()
        self._data_callback: SocketDataCallback = None

    def data_callback(self, data: bytes or None) -> None:
        if self._data_callback:
            return self._data_callback(self, data)
        else:
            LOGGER.warning(f"No data callback set. Discarding data")

    def set_data_callback(self, data_callback: SocketDataCallback):
        self._data_callback = data_callback

    def send(self, data: bytes):
        ...

    def get_send_buffer(self):
        ...

    @abstractmethod
    def close(self) -> None:
        ...

    @abstractmethod
    def may_delete(self) -> bool:
        ...

    @abstractmethod
    def delete(self) -> None:
        ...

    @abstractmethod
    def activity(self) -> None:
        ...

    @abstractmethod
    def fileno(self) -> int:
        ...


class SocketDataCallback(object):
    DataCallback = Callable[[Socket, bytes or None, object], None]

    def __init__(self, callback: DataCallback, user_data: object) -> None:
        super().__init__()
        self._callback: SocketDataCallback.DataCallback = callback
        self._user_data: object = user_data

    def __call__(self, sock: Socket, data: bytes or None) -> None:
        return self._callback(sock, data, self._user_data)


class ListenerSocket(Socket):

    def __init__(self, port: int, data_socket_factory: DataSocketFactory) -> None:
        super().__init__()

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.bind(('', port))
        self._sock.listen(5)
        self._socket_factory: DataSocketFactory = data_socket_factory
        self._delete = False

        LOGGER.debug(f"Created listener socket")

    def close(self) -> None:
        self._sock.shutdown(socket.SHUT_RDWR)
        self._sock.close()
        self._delete = True

    def may_delete(self) -> bool:
        return self._delete

    def delete(self) -> None:
        # TODO: Do cleanup. Maybe add deletion hooks?
        pass

    def activity(self) -> None:
        LOGGER.debug(f"Activity on listener socket")
        new_sock, addr = self._sock.accept()
        self._socket_factory.build(new_sock)

    def fileno(self) -> int:
        return self._sock.fileno()


class Hook(ABC):
    HookFunction = Callable[[Socket, object or None], None]

    def __init__(self, callback: HookFunction, user_data: object or None) -> None:
        super().__init__()
        self._callback: Hook.HookFunction = callback
        self._user_data: object or None = user_data

    def __call__(self, sock: Socket) -> None:
        return self._callback(sock, self._user_data)


class CreationHook(Hook):

    def __call__(self, sock: Socket) -> None:
        LOGGER.debug("Creation hook called")
        super().__call__(sock)


class DeletionHook(Hook):

    def __call__(self, sock: Socket) -> None:
        LOGGER.debug("Deletion hook called")
        super().__call__(sock)


class DataSocketFactory(object):

    def __init__(self, data_callback: SocketDataCallback) -> None:
        super().__init__()

        self._creation_hooks: List[CreationHook] = []
        self._deletion_hooks: List[DeletionHook] = []
        self._data_callback: SocketDataCallback = data_callback

        LOGGER.debug(f"Created new DataSocketFactory")

    def add_creation_hook(self, hook: CreationHook):
        self._creation_hooks.append(hook)

    def add_deletion_hook(self, hook: DeletionHook):
        self._deletion_hooks.append(hook)

    def build(self, sock: socket.socket) -> Socket:
        new_socket = DataSocket(sock, self._deletion_hooks)
        new_socket.set_data_callback(self._data_callback)

        for hook in self._creation_hooks:
            hook(new_socket)
        return new_socket


class DataSocket(Socket):

    def __init__(self, sock: socket.socket, deletion_hooks: List[DeletionHook]) -> None:
        super().__init__()

        self._sock = sock
        self._deletion_hooks = deletion_hooks
        self._delete = False

        LOGGER.debug(f"Created data socket")

    def close(self) -> None:
        self._sock.shutdown(socket.SHUT_RDWR)
        self._delete = True

    def may_delete(self) -> bool:
        return self._delete

    def delete(self) -> None:
        for hook in self._deletion_hooks:
            hook(self)

    def activity(self) -> None:
        LOGGER.debug(f"Activity on data socket")

        data = self._sock.recv(6500)
        if not data:
            self._delete = True
            return None

        return self.data_callback(data)

    def fileno(self) -> int:
        return self._sock.fileno()


class NetworkManager(object):

    def __init__(self) -> None:
        super().__init__()
        self._sockets: List[Socket] = []

        LOGGER.debug(f"Created NetworkManager")

    def register_socket(self, sock: Socket):
        self._sockets.append(sock)

        LOGGER.debug(f"Registered socket")

    def delete_socket(self, sock: Socket):
        self._sockets.remove(sock)
        # In C we deallocate here, since we own the socket
        LOGGER.debug(f"Removed socket")

    def process(self, timeout):
        readable, _, _ = select.select(self._sockets, [], [], timeout)

        for sock in readable:
            sock.activity()
            if sock.may_delete():
                sock.delete()


class Server(object):
    MAX_TIMEOUT = 0.050  # Max timeout in ms between main-loop iterations

    def __init__(self, network_manager: NetworkManager) -> None:
        super().__init__()

        self._network_manager = network_manager

    @staticmethod
    def handle_socket_data(sock: Socket, data, user_data: object):
        self: Server = user_data
        print("Create new channel?? (Depends on message type...)")
        print(f"Data is: <<{data}>>")

    def run_iterate(self, wait_internal=False):
        now = datetime.datetime.now().timestamp()
        latest = now + self.MAX_TIMEOUT

        next_repeated = now + random.randint(1, 100) / 1000  # random repeated callback time for simulation

        if next_repeated > latest:
            next_repeated = latest

        timeout = (next_repeated - now) if wait_internal else 0

        self._network_manager.process(timeout)

        # Multicast processing


def main() -> None:
    network_manager = NetworkManager()
    server = Server(network_manager)

    data_callback = SocketDataCallback(server.handle_socket_data, server)

    network_manager_creation_hook = CreationHook(lambda sock, user_data: user_data.register_socket(sock),
                                                 network_manager)
    network_manager_deletion_hook = DeletionHook(lambda sock, user_data: user_data.delete_socket(sock),
                                                 network_manager)

    data_socket_factory = DataSocketFactory(data_callback)
    data_socket_factory.add_creation_hook(network_manager_creation_hook)
    data_socket_factory.add_deletion_hook(network_manager_deletion_hook)

    listener_socket_4840 = ListenerSocket(4840, data_socket_factory)
    listener_socket_4844 = ListenerSocket(4844, data_socket_factory)

    network_manager.register_socket(listener_socket_4840)
    network_manager.register_socket(listener_socket_4844)

    try:
        while True:
            server.run_iterate(wait_internal=True)
    except KeyboardInterrupt:
        pass


#################################
# TRACING NOT RELEVANT FOR DEMO #
#################################
def make_trace_string(trace_string, frame):
    code = frame.f_code
    if code.co_name == "main":
        trace_string = f"->{code.co_name}()" + trace_string
    elif code.co_name == "<module>":
        trace_string = f"->{code.co_name}" + trace_string
    elif code.co_name == "__init__":
        # print(frame.f_locals)
        classname = str(frame.f_locals['__class__']).split('.')[-1].split("'")[0]
        trace_string = f"->{classname}.{code.co_name}()" + trace_string
    else:
        if "self" in frame.f_locals:
            classname = str(type(frame.f_locals['self'])).split('.')[-1].split("'")[0]
            trace_string = f"->{classname}.{code.co_name}()" + trace_string
        else:
            trace_string = f"->{code.co_name}()" + trace_string

    return trace_string


def trace_calls_and_returns(frame, event, arg):
    co = frame.f_code
    func_name = co.co_name
    if func_name == 'write' or func_name == "fileno" or func_name == "run_iterate" or func_name == "process":
        return
    line_no = frame.f_lineno
    filename = co.co_filename
    if not filename.endswith("server.py"):
        return
    if event == 'call':
        outer_frame = frame.f_back
        trace_string = make_trace_string("", frame)
        while outer_frame:
            trace_string = make_trace_string(trace_string, outer_frame)

            outer_frame = outer_frame.f_back

        print(trace_string)
        return trace_calls_and_returns
    elif event == 'return':
        # print('%s => %s' % (func_name, arg))
        pass
    return


if __name__ == "__main__":
    sys.settrace(trace_calls_and_returns)
    main()
