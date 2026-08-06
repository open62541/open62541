/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#include <open62541/config.h>

#ifdef UA_ARCHITECTURE_WIN32

#include "eventloop_iocp.h"

#include "../../deps/mp_printf.h"
#include <mswsock.h>

#define TCP_MANAGER_PARAMS 9
#define TCP_CONNECTION_PARAMS 5

static UA_KeyValueRestriction tcpManagerParams[TCP_MANAGER_PARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("max-connections")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-low-watermark")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-high-watermark")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-hard-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-message-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-global-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-stall-timeout")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

#define TCP_PARAM_ADDRESS 0
#define TCP_PARAM_PORT 1
#define TCP_PARAM_LISTEN 2
#define TCP_PARAM_VALIDATE 3
#define TCP_PARAM_REUSE 4

static UA_KeyValueRestriction tcpConnectionParams[TCP_CONNECTION_PARAMS] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, true},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("reuse")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

typedef struct {
    UA_IOCPOperation operation;
    SOCKET acceptSocket;
    DWORD receivedBytes;
    UA_Byte addressBuffer[UA_IOCP_ACCEPT_ADDRESS_BYTES];
} UA_IOCPAcceptOperation;

typedef struct {
    UA_IOCPOperation operation;
    SOCKADDR_STORAGE remoteAddress;
    int remoteAddressLength;
    DWORD bytesSent;
} UA_IOCPConnectOperation;

typedef struct {
    UA_IOCPOperation operation;
    WSABUF wsabuf;
    UA_ByteString buffer;
    DWORD flags;
} UA_IOCPTCPReceiveOperation;

typedef struct {
    UA_IOCPOperation operation;
    WSABUF wsabuf;
    UA_IOCPSendBuffer *buffer;
    size_t offset;
} UA_IOCPTCPSendOperation;

typedef struct UA_TCPListenerIOCP {
    UA_RegisteredSocketIOCP registeredSocket;
    UA_UInt16 port;
    int family;
    UA_Boolean acceptsPaused;
    LPFN_ACCEPTEX acceptEx;
    UA_IOCPAcceptOperation accepts[UA_IOCP_ACCEPT_DEPTH];
} UA_TCPListenerIOCP;

typedef union {
    UA_IOCPConnectOperation connect;
    UA_IOCPTCPReceiveOperation receive;
} UA_IOCPTCPInputOperation;

typedef struct UA_TCPConnectionIOCP {
    UA_RegisteredSocketIOCP registeredSocket;
    LIST_ENTRY(UA_TCPConnectionIOCP) stalledEntry;
    UA_IOCPTCPInputOperation input;
    UA_IOCPTCPSendOperation activeSend;
    LPFN_CONNECTEX connectEx;
    UA_IOCPSendBuffer *sendHead;
    UA_IOCPSendBuffer *sendTail;
    size_t queuedSendBytes;
    size_t queuedSendMessages;
    size_t sendQueueLowWatermark;
    size_t sendQueueHighWatermark;
    size_t sendQueueHardLimit;
    size_t sendQueueMessageLimit;
    UA_DateTime lastSendProgress;
    UA_Boolean receivePaused;
    UA_Boolean onStalledList;
    UA_Boolean receiveInitialized;
} UA_TCPConnectionIOCP;

typedef struct {
    UA_ConnectionManagerIOCP base;
    LIST_HEAD(, UA_TCPConnectionIOCP) stalledConnections;
    UA_UInt32 sendStallTimeoutMs;
    UA_UInt64 sendStallTimerId;
} UA_TCPConnectionManagerIOCP;

static void TCP_beginClose(UA_RegisteredSocketIOCP *rs, UA_StatusCode reason);
static UA_StatusCode TCP_submitReceive(UA_TCPConnectionIOCP *connection);
static UA_StatusCode TCP_submitSend(UA_TCPConnectionIOCP *connection);
static void TCP_rearmListeners(UA_TCPConnectionManagerIOCP *manager);
static void acceptCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error);

static SOCKET
createOverlappedSocket(int family, int type, int protocol) {
    return WSASocketW(family, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
}

static UA_StatusCode
setNoNagle(SOCKET socket) {
    int value = 1;
    return (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY,
                       (const char*)&value, sizeof(value)) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
setIPv6Only(SOCKET socket, int family) {
    if(family != AF_INET6)
        return UA_STATUSCODE_GOOD;
    int value = 1;
    return (setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY,
                       (const char*)&value, sizeof(value)) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
setReusable(SOCKET socket) {
    int value = 1;
    return (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                       (const char*)&value, sizeof(value)) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
loadExtension(SOCKET socket, GUID guid, void **function) {
    DWORD bytes = 0;
    return (WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                     &guid, sizeof(guid), function, sizeof(*function),
                     &bytes, NULL, NULL) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}


static UA_Boolean
managerAtCapacity(UA_TCPConnectionManagerIOCP *manager) {
    return (manager->base.maxConnections != 0 &&
            manager->base.connectionCount >= manager->base.maxConnections);
}

static void
checkStopped(UA_TCPConnectionManagerIOCP *manager) {
    if(manager->base.cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       manager->base.socketCount == 0)
        manager->base.cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static void
notifyClosing(UA_RegisteredSocketIOCP *rs) {
    if(!rs->callbackOpened || rs->closingCallbackSent || !rs->callback)
        return;
    rs->closingCallbackSent = true;
    rs->callback(&rs->manager->cm, (uintptr_t)rs->socket,
                 rs->application, &rs->context,
                 UA_CONNECTIONSTATE_CLOSING,
                 &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
}

static void
removeFromStalledList(UA_TCPConnectionIOCP *connection) {
    if(!connection->onStalledList)
        return;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)connection->registeredSocket.manager;
    LIST_REMOVE(connection, stalledEntry);
    connection->onStalledList = false;
    connection->lastSendProgress = 0;
    if(LIST_EMPTY(&manager->stalledConnections) &&
       manager->sendStallTimerId != 0) {
        manager->base.eventLoop->eventLoop.removeTimer(
            &manager->base.eventLoop->eventLoop, manager->sendStallTimerId);
        manager->sendStallTimerId = 0;
    }
}

static void
stallTimerCallback(void *application, void *data) {
    (void)data;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)application;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_DateTime now = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    UA_DateTime limit = (UA_DateTime)manager->sendStallTimeoutMs * UA_DATETIME_MSEC;
    UA_TCPConnectionIOCP *connection = LIST_FIRST(&manager->stalledConnections);
    while(connection) {
        UA_TCPConnectionIOCP *next = LIST_NEXT(connection, stalledEntry);
        if(connection->lastSendProgress != 0 &&
           now - connection->lastSendProgress >= limit)
            TCP_beginClose(&connection->registeredSocket, UA_STATUSCODE_BADTIMEOUT);
        connection = next;
    }
}

static UA_StatusCode
addToStalledList(UA_TCPConnectionIOCP *connection) {
    if(connection->onStalledList)
        return UA_STATUSCODE_GOOD;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)connection->registeredSocket.manager;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    connection->lastSendProgress =
        el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    connection->onStalledList = true;
    LIST_INSERT_HEAD(&manager->stalledConnections, connection, stalledEntry);
    if(manager->sendStallTimeoutMs == 0 || manager->sendStallTimerId != 0)
        return UA_STATUSCODE_GOOD;
    UA_Double interval = (UA_Double)manager->sendStallTimeoutMs / 4.0;
    if(interval < 100.0)
        interval = 100.0;
    UA_StatusCode result = el->eventLoop.addTimer(
        &el->eventLoop, stallTimerCallback, manager, NULL, interval, NULL,
        UA_TIMERPOLICY_CURRENTTIME, &manager->sendStallTimerId);
    if(result != UA_STATUSCODE_GOOD) {
        LIST_REMOVE(connection, stalledEntry);
        connection->onStalledList = false;
        connection->lastSendProgress = 0;
    }
    return result;
}

static void
freeQueuedSend(UA_TCPConnectionIOCP *connection, UA_IOCPSendBuffer *buffer) {
    UA_ConnectionManagerIOCP *manager = connection->registeredSocket.manager;
    UA_assert(connection->queuedSendMessages > 0);
    UA_assert(connection->queuedSendBytes >= buffer->length);
    UA_assert(manager->globalQueuedSendBytes >= buffer->length);
    connection->queuedSendMessages--;
    connection->queuedSendBytes -= buffer->length;
    manager->globalQueuedSendBytes -= buffer->length;
    UA_IOCP_releaseSendBuffer(manager, buffer);
}

static void
clearWaitingSends(UA_TCPConnectionIOCP *connection) {
    UA_IOCPSendBuffer *buffer = connection->sendHead;
    connection->sendHead = NULL;
    connection->sendTail = NULL;
    while(buffer) {
        UA_IOCPSendBuffer *next = buffer->next;
        freeQueuedSend(connection, buffer);
        buffer = next;
    }
}

static void
connectionDelayedClose(void *application, void *context) {
    (void)context;
    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)application;
    UA_RegisteredSocketIOCP *rs = &connection->registeredSocket;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)rs->manager;
    UA_LOCK(&manager->base.eventLoop->elMutex);
    notifyClosing(rs);
    removeFromStalledList(connection);
    if(connection->activeSend.buffer) {
        freeQueuedSend(connection, connection->activeSend.buffer);
        connection->activeSend.buffer = NULL;
    }
    clearWaitingSends(connection);
    if(connection->receiveInitialized)
        UA_ByteString_clear(&connection->input.receive.buffer);
    UA_IOCP_removeSocket(&manager->base, rs);
    if(rs->socket != INVALID_SOCKET) {
        closesocket(rs->socket);
        rs->socket = INVALID_SOCKET;
    }
    UA_free(connection);
    TCP_rearmListeners(manager);
    checkStopped(manager);
    UA_UNLOCK(&manager->base.eventLoop->elMutex);
}

static void
listenerDelayedClose(void *application, void *context) {
    (void)context;
    UA_TCPListenerIOCP *listener = (UA_TCPListenerIOCP*)application;
    UA_RegisteredSocketIOCP *rs = &listener->registeredSocket;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)rs->manager;
    UA_LOCK(&manager->base.eventLoop->elMutex);
    notifyClosing(rs);
    for(size_t i = 0; i < UA_IOCP_ACCEPT_DEPTH; i++) {
        if(listener->accepts[i].acceptSocket != INVALID_SOCKET)
            closesocket(listener->accepts[i].acceptSocket);
    }
    if(rs->socket != INVALID_SOCKET)
        closesocket(rs->socket);
    UA_IOCP_removeSocket(&manager->base, rs);
    UA_free(listener);
    checkStopped(manager);
    UA_UNLOCK(&manager->base.eventLoop->elMutex);
}

static void
TCP_beginClose(UA_RegisteredSocketIOCP *rs, UA_StatusCode reason) {
    if(rs->state == UA_IOCP_SOCKET_CLOSING)
        return;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)rs->manager;
    UA_IOCPSocketState oldState = rs->state;
    rs->state = UA_IOCP_SOCKET_CLOSING;
    (void)reason;
    if(oldState == UA_IOCP_SOCKET_TCP_CONNECTING ||
       oldState == UA_IOCP_SOCKET_TCP_ESTABLISHED) {
        UA_assert(manager->base.connectionCount > 0);
        manager->base.connectionCount--;
    }
    if(oldState == UA_IOCP_SOCKET_TCP_ESTABLISHED) {
        UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)rs;
        connection->receivePaused = true;
        removeFromStalledList(connection);
        clearWaitingSends(connection);
        if(connection->activeSend.buffer &&
           !connection->activeSend.operation.submitted) {
            freeQueuedSend(connection, connection->activeSend.buffer);
            connection->activeSend.buffer = NULL;
        }
        shutdown(rs->socket, SD_BOTH);
    }
    UA_IOCP_cancelSocket(rs);
    UA_IOCP_maybeQueueClose(rs);
}

static void
reportRemoteAddress(UA_TCPConnectionIOCP *connection,
                    UA_ConnectionState state) {
    UA_RegisteredSocketIOCP *rs = &connection->registeredSocket;
    char host[NI_MAXHOST] = {0};
    SOCKADDR_STORAGE address;
    int addressLength = sizeof(address);
    if(getpeername(rs->socket, (struct sockaddr*)&address, &addressLength) == 0)
        getnameinfo((struct sockaddr*)&address, addressLength,
                    host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    UA_String remote = UA_STRING(host);
    UA_KeyValuePair pair;
    pair.key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_Variant_setScalar(&pair.value, &remote, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {1, &pair};
    rs->callbackOpened = true;
    rs->callback(&rs->manager->cm, (uintptr_t)rs->socket,
                 rs->application, &rs->context, state,
                 &params, UA_BYTESTRING_NULL);
}

static UA_StatusCode
initializeReceive(UA_TCPConnectionIOCP *connection) {
    if(connection->receiveInitialized)
        return UA_STATUSCODE_GOOD;
    UA_ConnectionManagerIOCP *manager = connection->registeredSocket.manager;
    memset(&connection->input.receive, 0, sizeof(connection->input.receive));
    connection->input.receive.operation.owner = &connection->registeredSocket;
    connection->receiveInitialized = true;
    return UA_ByteString_allocBuffer(&connection->input.receive.buffer,
                                     manager->receiveBufferSize);
}

static void
receiveCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)operation->owner;
    if(operation->owner->state == UA_IOCP_SOCKET_CLOSING) {
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }
    if(error != 0 || bytes == 0) {
        UA_StatusCode reason = error ?
            UA_IOCP_statusFromSocketError(error) :
            UA_STATUSCODE_BADCONNECTIONCLOSED;
        TCP_beginClose(operation->owner, reason);
        return;
    }
    UA_ByteString message = {bytes, connection->input.receive.buffer.data};
    operation->owner->callback(&operation->owner->manager->cm,
                               (uintptr_t)operation->owner->socket,
                               operation->owner->application,
                               &operation->owner->context,
                               UA_CONNECTIONSTATE_ESTABLISHED,
                               &UA_KEYVALUEMAP_NULL, message);
    if(operation->owner->state != UA_IOCP_SOCKET_CLOSING &&
       !connection->receivePaused) {
        UA_StatusCode result = TCP_submitReceive(connection);
        if(result != UA_STATUSCODE_GOOD)
            TCP_beginClose(operation->owner, result);
    } else {
        UA_IOCP_maybeQueueClose(operation->owner);
    }
}

static UA_StatusCode
TCP_submitReceive(UA_TCPConnectionIOCP *connection) {
    UA_IOCPTCPReceiveOperation *receive = &connection->input.receive;
    receive->operation.owner = &connection->registeredSocket;
    receive->operation.handler = receiveCompletion;
    receive->wsabuf.buf = (CHAR*)receive->buffer.data;
    receive->wsabuf.len = (ULONG)((receive->buffer.length > ULONG_MAX) ?
                                  ULONG_MAX : receive->buffer.length);
    receive->flags = 0;
    UA_IOCP_prepareOperation(&receive->operation);
    int result = WSARecv(connection->registeredSocket.socket,
                         &receive->wsabuf, 1, NULL, &receive->flags,
                         &receive->operation.overlapped, NULL);
    if(result == SOCKET_ERROR) {
        DWORD error = (DWORD)WSAGetLastError();
        if(error != WSA_IO_PENDING) {
            UA_IOCP_abortOperation(&receive->operation);
            return UA_IOCP_statusFromSocketError(error);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
resumeReceiveIfNeeded(UA_TCPConnectionIOCP *connection) {
    if(!connection->receivePaused ||
       connection->registeredSocket.state != UA_IOCP_SOCKET_TCP_ESTABLISHED ||
       connection->queuedSendBytes > connection->sendQueueLowWatermark)
        return;
    connection->receivePaused = false;
    removeFromStalledList(connection);
    if(!connection->input.receive.operation.submitted) {
        UA_StatusCode result = TCP_submitReceive(connection);
        if(result != UA_STATUSCODE_GOOD)
            TCP_beginClose(&connection->registeredSocket, result);
    }
}

static void
sendCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)operation->owner;
    UA_IOCPTCPSendOperation *send = &connection->activeSend;
    UA_IOCPSendBuffer *buffer = send->buffer;
    if(!buffer) {
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }
    if(error != 0 || bytes == 0) {
        freeQueuedSend(connection, buffer);
        send->buffer = NULL;
        UA_StatusCode reason = error ?
            UA_IOCP_statusFromSocketError(error) :
            UA_STATUSCODE_BADCONNECTIONCLOSED;
        TCP_beginClose(operation->owner, reason);
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }
    send->offset += bytes;
    connection->lastSendProgress =
        operation->owner->manager->eventLoop->eventLoop.dateTime_nowMonotonic(
            &operation->owner->manager->eventLoop->eventLoop);
    if(send->offset < buffer->length) {
        UA_StatusCode result = TCP_submitSend(connection);
        if(result != UA_STATUSCODE_GOOD) {
            freeQueuedSend(connection, buffer);
            send->buffer = NULL;
            TCP_beginClose(operation->owner, result);
        }
        return;
    }
    freeQueuedSend(connection, buffer);
    send->buffer = NULL;
    send->offset = 0;
    if(operation->owner->state == UA_IOCP_SOCKET_CLOSING) {
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }
    if(connection->sendHead) {
        send->buffer = connection->sendHead;
        connection->sendHead = connection->sendHead->next;
        if(!connection->sendHead)
            connection->sendTail = NULL;
        send->buffer->next = NULL;
        UA_StatusCode result = TCP_submitSend(connection);
        if(result != UA_STATUSCODE_GOOD) {
            freeQueuedSend(connection, send->buffer);
            send->buffer = NULL;
            TCP_beginClose(operation->owner, result);
            return;
        }
    }
    resumeReceiveIfNeeded(connection);
}

static UA_StatusCode
TCP_submitSend(UA_TCPConnectionIOCP *connection) {
    UA_IOCPTCPSendOperation *send = &connection->activeSend;
    UA_assert(send->buffer && !send->operation.submitted);
    size_t remaining = send->buffer->length - send->offset;
    send->operation.owner = &connection->registeredSocket;
    send->operation.handler = sendCompletion;
    send->wsabuf.buf = (CHAR*)send->buffer->data + send->offset;
    send->wsabuf.len = (ULONG)((remaining > ULONG_MAX) ? ULONG_MAX : remaining);
    UA_IOCP_prepareOperation(&send->operation);
    int result = WSASend(connection->registeredSocket.socket,
                         &send->wsabuf, 1, NULL, 0,
                         &send->operation.overlapped, NULL);
    if(result == SOCKET_ERROR) {
        DWORD error = (DWORD)WSAGetLastError();
        if(error != WSA_IO_PENDING) {
            UA_IOCP_abortOperation(&send->operation);
            return UA_IOCP_statusFromSocketError(error);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
connectCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    (void)bytes;
    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)operation->owner;
    UA_RegisteredSocketIOCP *rs = &connection->registeredSocket;
    if(rs->state == UA_IOCP_SOCKET_CLOSING) {
        UA_IOCP_maybeQueueClose(rs);
        return;
    }
    if(error != 0 ||
       setsockopt(rs->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
                  NULL, 0) != 0) {
        TCP_beginClose(rs, UA_IOCP_statusFromSocketError(
            error ? error : (DWORD)WSAGetLastError()));
        return;
    }
    rs->state = UA_IOCP_SOCKET_TCP_ESTABLISHED;
    UA_StatusCode result = initializeReceive(connection);
    if(result == UA_STATUSCODE_GOOD)
        result = TCP_submitReceive(connection);
    if(result != UA_STATUSCODE_GOOD) {
        TCP_beginClose(rs, result);
        return;
    }
    reportRemoteAddress(connection, UA_CONNECTIONSTATE_ESTABLISHED);
}

static UA_StatusCode
submitConnect(UA_TCPConnectionIOCP *connection,
              const struct sockaddr *remoteAddress,
              int remoteAddressLength) {
    UA_IOCPConnectOperation *connect = &connection->input.connect;
    memset(connect, 0, sizeof(*connect));
    connect->operation.owner = &connection->registeredSocket;
    connect->operation.handler = connectCompletion;
    memcpy(&connect->remoteAddress, remoteAddress, (size_t)remoteAddressLength);
    connect->remoteAddressLength = remoteAddressLength;

    SOCKADDR_STORAGE local;
    memset(&local, 0, sizeof(local));
    int localLength;
    if(remoteAddress->sa_family == AF_INET6) {
        ((SOCKADDR_IN6*)&local)->sin6_family = AF_INET6;
        localLength = sizeof(SOCKADDR_IN6);
    } else {
        ((SOCKADDR_IN*)&local)->sin_family = AF_INET;
        localLength = sizeof(SOCKADDR_IN);
    }
    if(bind(connection->registeredSocket.socket,
            (struct sockaddr*)&local, localLength) != 0)
        return UA_IOCP_statusFromSocketError((DWORD)WSAGetLastError());

    UA_IOCP_prepareOperation(&connect->operation);
    BOOL result = connection->connectEx(
        connection->registeredSocket.socket,
        (struct sockaddr*)&connect->remoteAddress,
        connect->remoteAddressLength, NULL, 0,
        &connect->bytesSent, &connect->operation.overlapped);
    if(!result) {
        DWORD error = (DWORD)WSAGetLastError();
        if(error != WSA_IO_PENDING) {
            UA_IOCP_abortOperation(&connect->operation);
            return UA_IOCP_statusFromSocketError(error);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
initializeAcceptedConnection(UA_TCPListenerIOCP *listener,
                             SOCKET acceptedSocket) {
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)listener->registeredSocket.manager;
    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)
        UA_calloc(1, sizeof(UA_TCPConnectionIOCP));
    if(!connection) {
        closesocket(acceptedSocket);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    connection->sendQueueLowWatermark = manager->base.sendQueueLowWatermark;
    connection->sendQueueHighWatermark = manager->base.sendQueueHighWatermark;
    connection->sendQueueHardLimit = manager->base.sendQueueHardLimit;
    connection->sendQueueMessageLimit = manager->base.sendQueueMessageLimit;

    UA_StatusCode result = UA_IOCP_insertSocket(
        &manager->base, &connection->registeredSocket,
        UA_IOCP_SOCKET_TCP_ESTABLISHED, acceptedSocket,
        listener->registeredSocket.callback,
        listener->registeredSocket.application,
        listener->registeredSocket.context);
    if(result != UA_STATUSCODE_GOOD) {
        UA_free(connection);
        closesocket(acceptedSocket);
        return result;
    }
    connection->registeredSocket.delayedClose.callback = connectionDelayedClose;
    connection->registeredSocket.delayedClose.application = connection;
    manager->base.connectionCount++;

    result = initializeReceive(connection);
    if(result == UA_STATUSCODE_GOOD)
        result = TCP_submitReceive(connection);
    if(result != UA_STATUSCODE_GOOD) {
        TCP_beginClose(&connection->registeredSocket, result);
        return result;
    }
    reportRemoteAddress(connection, UA_CONNECTIONSTATE_ESTABLISHED);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
submitAccept(UA_TCPListenerIOCP *listener, UA_IOCPAcceptOperation *accept) {
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)listener->registeredSocket.manager;
    if(listener->registeredSocket.state == UA_IOCP_SOCKET_CLOSING ||
       manager->base.cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    if(managerAtCapacity(manager)) {
        listener->acceptsPaused = true;
        return UA_STATUSCODE_GOOD;
    }

    accept->acceptSocket =
        createOverlappedSocket(listener->family, SOCK_STREAM, IPPROTO_TCP);
    if(accept->acceptSocket == INVALID_SOCKET)
        return UA_IOCP_statusFromSocketError((DWORD)WSAGetLastError());
    memset(accept->addressBuffer, 0, sizeof(accept->addressBuffer));
    accept->receivedBytes = 0;
    accept->operation.owner = &listener->registeredSocket;
    accept->operation.handler = acceptCompletion;

    UA_IOCP_prepareOperation(&accept->operation);
    BOOL result = listener->acceptEx(
        listener->registeredSocket.socket, accept->acceptSocket,
        accept->addressBuffer, 0,
        sizeof(SOCKADDR_STORAGE) + 16,
        sizeof(SOCKADDR_STORAGE) + 16,
        &accept->receivedBytes, &accept->operation.overlapped);
    if(!result) {
        DWORD error = (DWORD)WSAGetLastError();
        if(error != WSA_IO_PENDING) {
            UA_IOCP_abortOperation(&accept->operation);
            closesocket(accept->acceptSocket);
            accept->acceptSocket = INVALID_SOCKET;
            return UA_IOCP_statusFromSocketError(error);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
acceptCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    (void)bytes;
    UA_IOCPAcceptOperation *accept = (UA_IOCPAcceptOperation*)operation;
    UA_TCPListenerIOCP *listener = (UA_TCPListenerIOCP*)operation->owner;
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)operation->owner->manager;
    SOCKET acceptedSocket = accept->acceptSocket;
    accept->acceptSocket = INVALID_SOCKET;

    if(operation->owner->state == UA_IOCP_SOCKET_CLOSING) {
        if(acceptedSocket != INVALID_SOCKET)
            closesocket(acceptedSocket);
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }

    if(error != 0 || managerAtCapacity(manager)) {
        if(acceptedSocket != INVALID_SOCKET)
            closesocket(acceptedSocket);
        if(error != 0 && error != ERROR_OPERATION_ABORTED)
            TCP_beginClose(operation->owner, UA_IOCP_statusFromSocketError(error));
    } else {
        SOCKET listenSocket = operation->owner->socket;
        if(setsockopt(acceptedSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                      (const char*)&listenSocket, sizeof(listenSocket)) != 0 ||
           setNoNagle(acceptedSocket) != UA_STATUSCODE_GOOD) {
            closesocket(acceptedSocket);
        } else {
            initializeAcceptedConnection(listener, acceptedSocket);
        }
    }

    if(operation->owner->state != UA_IOCP_SOCKET_CLOSING) {
        UA_StatusCode result = submitAccept(listener, accept);
        if(result != UA_STATUSCODE_GOOD &&
           result != UA_STATUSCODE_BADCONNECTIONCLOSED)
            TCP_beginClose(operation->owner, result);
    }
    UA_IOCP_maybeQueueClose(operation->owner);
}

static void *
rearmListenerCallback(void *context, UA_RegisteredSocketIOCP *rs) {
    (void)context;
    if(rs->state != UA_IOCP_SOCKET_TCP_LISTENER)
        return NULL;
    UA_TCPListenerIOCP *listener = (UA_TCPListenerIOCP*)rs;
    if(!listener->acceptsPaused)
        return NULL;
    listener->acceptsPaused = false;
    for(size_t i = 0; i < UA_IOCP_ACCEPT_DEPTH; i++) {
        if(!listener->accepts[i].operation.submitted &&
           listener->accepts[i].acceptSocket == INVALID_SOCKET) {
            UA_StatusCode result = submitAccept(listener, &listener->accepts[i]);
            if(result != UA_STATUSCODE_GOOD) {
                TCP_beginClose(rs, result);
                break;
            }
        }
    }
    return NULL;
}

static void
TCP_rearmListeners(UA_TCPConnectionManagerIOCP *manager) {
    if(managerAtCapacity(manager) ||
       manager->base.cm.eventSource.state != UA_EVENTSOURCESTATE_STARTED)
        return;
    ZIP_ITER(UA_IOCPSocketTree, &manager->base.sockets,
             rearmListenerCallback, manager);
}

static UA_StatusCode
registerListener(UA_TCPConnectionManagerIOCP *manager,
                 const struct addrinfo *ai, const char *hostname,
                 UA_UInt16 port, void *application, void *context,
                 UA_ConnectionManager_connectionCallback callback,
                 UA_Boolean validate, UA_Boolean reuseAddress) {
    if(manager->base.maxConnections != 0 &&
       manager->base.socketCount >= manager->base.maxConnections)
        return UA_STATUSCODE_BADINTERNALERROR;

    SOCKET socket = createOverlappedSocket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(socket == INVALID_SOCKET)
        return UA_IOCP_statusFromSocketError((DWORD)WSAGetLastError());

    UA_StatusCode result = setIPv6Only(socket, ai->ai_family);
    if(reuseAddress)
        result |= setReusable(socket);
    if(result != UA_STATUSCODE_GOOD ||
       bind(socket, ai->ai_addr, (int)ai->ai_addrlen) != 0) {
        closesocket(socket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(validate) {
        closesocket(socket);
        return UA_STATUSCODE_GOOD;
    }
    if(listen(socket, UA_MAXBACKLOG) != 0) {
        closesocket(socket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    LPFN_ACCEPTEX acceptEx = NULL;
    GUID acceptGuid = WSAID_ACCEPTEX;
    result = loadExtension(socket, acceptGuid, (void**)&acceptEx);
    if(result != UA_STATUSCODE_GOOD) {
        closesocket(socket);
        return result;
    }

    UA_TCPListenerIOCP *listener = (UA_TCPListenerIOCP*)
        UA_calloc(1, sizeof(UA_TCPListenerIOCP));
    if(!listener) {
        closesocket(socket);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    listener->family = ai->ai_family;
    listener->port = port;
    listener->acceptEx = acceptEx;
    for(size_t i = 0; i < UA_IOCP_ACCEPT_DEPTH; i++)
        listener->accepts[i].acceptSocket = INVALID_SOCKET;

    result = UA_IOCP_insertSocket(&manager->base,
                                  &listener->registeredSocket,
                                  UA_IOCP_SOCKET_TCP_LISTENER,
                                  socket, callback, application, context);
    if(result != UA_STATUSCODE_GOOD) {
        UA_free(listener);
        closesocket(socket);
        return result;
    }
    listener->registeredSocket.delayedClose.callback = listenerDelayedClose;
    listener->registeredSocket.delayedClose.application = listener;

    if(port == 0) {
        SOCKADDR_STORAGE local;
        int localLength = sizeof(local);
        if(getsockname(socket, (struct sockaddr*)&local, &localLength) == 0) {
            if(local.ss_family == AF_INET)
                listener->port = ntohs(((SOCKADDR_IN*)&local)->sin_port);
            else
                listener->port = ntohs(((SOCKADDR_IN6*)&local)->sin6_port);
        }
    }


    for(size_t i = 0; i < UA_IOCP_ACCEPT_DEPTH; i++) {
        result = submitAccept(listener, &listener->accepts[i]);
        if(result != UA_STATUSCODE_GOOD) {
            TCP_beginClose(&listener->registeredSocket, result);
            return result;
        }
    }

    UA_String listenAddress = hostname ?
        UA_STRING((char*)(uintptr_t)hostname) : UA_STRING_NULL;
    UA_KeyValuePair pairs[2];
    pairs[0].key = UA_QUALIFIEDNAME(0, "listen-address");
    UA_Variant_setScalar(&pairs[0].value, &listenAddress,
                         &UA_TYPES[UA_TYPES_STRING]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "listen-port");
    UA_Variant_setScalar(&pairs[1].value, &listener->port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap callbackParams = {2, pairs};
    listener->registeredSocket.callbackOpened = true;
    callback(&manager->base.cm, (uintptr_t)socket, application,
             &listener->registeredSocket.context,
             UA_CONNECTIONSTATE_ESTABLISHED,
             &callbackParams, UA_BYTESTRING_NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
openPassive(UA_TCPConnectionManagerIOCP *manager,
            const UA_KeyValueMap *params, void *application, void *context,
            UA_ConnectionManager_connectionCallback callback,
            UA_Boolean validate) {
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_PORT].name,
        &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port);

    UA_Boolean reuseAddress = false;
    const UA_Boolean *reuse = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_REUSE].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(reuse)
        reuseAddress = *reuse;

    const UA_Variant *addresses = UA_KeyValueMap_get(
        params, tcpConnectionParams[TCP_PARAM_ADDRESS].name);
    size_t addressCount = addresses ?
        (UA_Variant_isScalar(addresses) ? 1 : addresses->arrayLength) : 0;

    UA_StatusCode aggregate = UA_STATUSCODE_BADCONNECTIONREJECTED;
    size_t iterations = addressCount ? addressCount : 1;
    for(size_t i = 0; i < iterations; i++) {
        char hostname[UA_MAXHOSTNAME_LENGTH];
        const char *host = NULL;
        if(addressCount) {
            UA_String *strings = (UA_String*)addresses->data;
            if(strings[i].length >= sizeof(hostname))
                continue;
            memcpy(hostname, strings[i].data, strings[i].length);
            hostname[strings[i].length] = 0;
            host = hostname;
        }

        char portString[UA_MAXPORTSTR_LENGTH];
        mp_snprintf(portString, sizeof(portString), "%u", (unsigned)*port);
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *resolved = NULL;
        if(getaddrinfo(host, portString, &hints, &resolved) != 0)
            continue;
        for(struct addrinfo *ai = resolved; ai; ai = ai->ai_next) {
            UA_StatusCode result = registerListener(
                manager, ai, host, *port, application, context,
                callback, validate, reuseAddress);
            if(result == UA_STATUSCODE_GOOD)
                aggregate = UA_STATUSCODE_GOOD;
        }
        freeaddrinfo(resolved);
    }
    return aggregate;
}

static UA_StatusCode
openActive(UA_TCPConnectionManagerIOCP *manager,
           const UA_KeyValueMap *params, void *application, void *context,
           UA_ConnectionManager_connectionCallback callback,
           UA_Boolean validate) {
    if(managerAtCapacity(manager))
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    const UA_String *address = (const UA_String*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_ADDRESS].name,
        &UA_TYPES[UA_TYPES_STRING]);
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_PORT].name,
        &UA_TYPES[UA_TYPES_UINT16]);
    if(!address || address->length >= UA_MAXHOSTNAME_LENGTH)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    char hostname[UA_MAXHOSTNAME_LENGTH];
    memcpy(hostname, address->data, address->length);
    hostname[address->length] = 0;
    char portString[UA_MAXPORTSTR_LENGTH];
    mp_snprintf(portString, sizeof(portString), "%u", (unsigned)*port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    struct addrinfo *resolved = NULL;
    if(getaddrinfo(hostname, portString, &hints, &resolved) != 0)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    UA_StatusCode result = UA_STATUSCODE_BADCONNECTIONREJECTED;
    for(struct addrinfo *ai = resolved; ai; ai = ai->ai_next) {
        SOCKET socket = createOverlappedSocket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if(socket == INVALID_SOCKET)
            continue;
        if(setNoNagle(socket) != UA_STATUSCODE_GOOD) {
            closesocket(socket);
            continue;
        }
        LPFN_CONNECTEX connectEx = NULL;
        GUID connectGuid = WSAID_CONNECTEX;
        if(loadExtension(socket, connectGuid,
                         (void**)&connectEx) != UA_STATUSCODE_GOOD) {
            closesocket(socket);
            continue;
        }
        if(validate) {
            closesocket(socket);
            result = UA_STATUSCODE_GOOD;
            break;
        }

        UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)
            UA_calloc(1, sizeof(UA_TCPConnectionIOCP));
        if(!connection) {
            closesocket(socket);
            result = UA_STATUSCODE_BADOUTOFMEMORY;
            break;
        }
        connection->sendQueueLowWatermark = manager->base.sendQueueLowWatermark;
        connection->sendQueueHighWatermark = manager->base.sendQueueHighWatermark;
        connection->sendQueueHardLimit = manager->base.sendQueueHardLimit;
        connection->sendQueueMessageLimit = manager->base.sendQueueMessageLimit;
        connection->connectEx = connectEx;

        result = UA_IOCP_insertSocket(&manager->base,
                                      &connection->registeredSocket,
                                      UA_IOCP_SOCKET_TCP_CONNECTING,
                                      socket, callback, application, context);
        if(result != UA_STATUSCODE_GOOD) {
            UA_free(connection);
            closesocket(socket);
            continue;
        }
        connection->registeredSocket.delayedClose.callback = connectionDelayedClose;
        connection->registeredSocket.delayedClose.application = connection;
        manager->base.connectionCount++;

        connection->registeredSocket.callbackOpened = true;
        callback(&manager->base.cm, (uintptr_t)socket, application,
                 &connection->registeredSocket.context,
                 UA_CONNECTIONSTATE_OPENING,
                 &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        if(connection->registeredSocket.state == UA_IOCP_SOCKET_CLOSING) {
            result = UA_STATUSCODE_BADCONNECTIONCLOSED;
            break;
        }
        result = submitConnect(connection,
                               ai->ai_addr, (int)ai->ai_addrlen);
        if(result != UA_STATUSCODE_GOOD)
            TCP_beginClose(&connection->registeredSocket, result);
        break;
    }
    freeaddrinfo(resolved);
    return result;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback callback) {
    UA_TCPConnectionManagerIOCP *manager = (UA_TCPConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode result = UA_KeyValueRestriction_validate(
        el->eventLoop.logger, "TCP", tcpConnectionParams,
        TCP_CONNECTION_PARAMS, params);
    if(result != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return result;
    }

    UA_Boolean validate = false;
    const UA_Boolean *validateValue = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_VALIDATE].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validateValue)
        validate = *validateValue;
    UA_Boolean listen = false;
    const UA_Boolean *listenValue = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, tcpConnectionParams[TCP_PARAM_LISTEN].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(listenValue)
        listen = *listenValue;

    result = listen ?
        openPassive(manager, params, application, context, callback, validate) :
        openActive(manager, params, application, context, callback, validate);
    UA_UNLOCK(&el->elMutex);
    return result;
}

static UA_StatusCode
TCP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    UA_TCPConnectionManagerIOCP *manager = (UA_TCPConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);

    UA_IOCPSendBuffer *buffer = UA_IOCP_takeNetworkBuffer(&manager->base, buf);
    if(!buffer) {
        UA_IOCP_freeNetworkBuffer(cm, connectionId, buf);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }


    UA_RegisteredSocketIOCP *rs =
        UA_IOCP_findSocket(&manager->base, connectionId);
    if(!rs || rs->state != UA_IOCP_SOCKET_TCP_ESTABLISHED) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    if(buffer->length == 0) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_GOOD;
    }

    UA_TCPConnectionIOCP *connection = (UA_TCPConnectionIOCP*)rs;
    UA_Boolean overLimit =
        connection->queuedSendMessages >= connection->sendQueueMessageLimit ||
        connection->queuedSendBytes > connection->sendQueueHardLimit ||
        buffer->length > connection->sendQueueHardLimit -
                         connection->queuedSendBytes ||
        manager->base.globalQueuedSendBytes >
            manager->base.globalSendQueueHardLimit ||
        buffer->length > manager->base.globalSendQueueHardLimit -
                         manager->base.globalQueuedSendBytes;
    if(overLimit) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        TCP_beginClose(rs, UA_STATUSCODE_BADCONNECTIONCLOSED);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    connection->queuedSendBytes += buffer->length;
    connection->queuedSendMessages++;
    manager->base.globalQueuedSendBytes += buffer->length;

    UA_StatusCode result = UA_STATUSCODE_GOOD;
    if(!connection->activeSend.buffer) {
        connection->activeSend.buffer = buffer;
        result = TCP_submitSend(connection);
        if(result != UA_STATUSCODE_GOOD) {
            freeQueuedSend(connection, buffer);
            connection->activeSend.buffer = NULL;
            TCP_beginClose(rs, result);
        }
    } else {
        buffer->next = NULL;
        if(connection->sendTail)
            connection->sendTail->next = buffer;
        else
            connection->sendHead = buffer;
        connection->sendTail = buffer;
    }

    if(connection->queuedSendBytes > connection->sendQueueHighWatermark) {
        connection->receivePaused = true;
        UA_StatusCode stallResult = addToStalledList(connection);
        if(stallResult != UA_STATUSCODE_GOOD) {
            TCP_beginClose(rs, stallResult);
            if(result == UA_STATUSCODE_GOOD)
                result = stallResult;
        }
    }

    UA_UNLOCK(&el->elMutex);
    return result;
}

static UA_StatusCode
TCP_closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_TCPConnectionManagerIOCP *manager = (UA_TCPConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);
    UA_RegisteredSocketIOCP *rs =
        UA_IOCP_findSocket(&manager->base, connectionId);
    if(!rs) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    TCP_beginClose(rs, UA_STATUSCODE_BADCONNECTIONCLOSED);
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getManagerUInt32(UA_KeyValueMap *params, const char *name,
                 UA_UInt32 defaultValue, UA_UInt32 *result) {
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    const UA_UInt32 *value = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, key, &UA_TYPES[UA_TYPES_UINT32]);
    if(value) {
        *result = *value;
        return UA_STATUSCODE_GOOD;
    }
    *result = defaultValue;
    return UA_KeyValueMap_setScalar(
        params, key, result, &UA_TYPES[UA_TYPES_UINT32]);
}

static UA_StatusCode
eventSourceStart(UA_EventSource *eventSource) {
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)eventSource->eventLoop;
    UA_LOCK(&el->elMutex);
    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode result = UA_KeyValueRestriction_validate(
        el->eventLoop.logger, "TCP", tcpManagerParams,
        TCP_MANAGER_PARAMS, &eventSource->params);
    if(result == UA_STATUSCODE_GOOD)
        result = UA_IOCP_configureConnectionManager(&manager->base);

    UA_UInt32 maxConnections = 0;
    if(result == UA_STATUSCODE_GOOD)
        result = getManagerUInt32(&eventSource->params,
                                  "max-connections", 0, &maxConnections);
    manager->base.maxConnections = maxConnections;
    if(result == UA_STATUSCODE_GOOD)
        result = getManagerUInt32(&eventSource->params,
                                  "send-stall-timeout",
                                  UA_IOCP_DEFAULT_SEND_STALL_TIMEOUT_MS,
                                  &manager->sendStallTimeoutMs);
    if(result == UA_STATUSCODE_GOOD) {
        manager->base.eventLoop = el;
        eventSource->state = UA_EVENTSOURCESTATE_STARTED;
    }
    UA_UNLOCK(&el->elMutex);
    return result;
}

static void *
shutdownSocketCallback(void *context, UA_RegisteredSocketIOCP *rs) {
    (void)context;
    TCP_beginClose(rs, UA_STATUSCODE_BADCONNECTIONCLOSED);
    return NULL;
}

static void
eventSourceStop(UA_EventSource *eventSource) {
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);
    eventSource->state = UA_EVENTSOURCESTATE_STOPPING;
    if(manager->sendStallTimerId != 0) {
        el->eventLoop.removeTimer(&el->eventLoop, manager->sendStallTimerId);
        manager->sendStallTimerId = 0;
    }
    ZIP_ITER(UA_IOCPSocketTree, &manager->base.sockets,
             shutdownSocketCallback, manager);
    checkStopped(manager);
    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
eventSourceFree(UA_EventSource *eventSource) {
    UA_TCPConnectionManagerIOCP *manager =
        (UA_TCPConnectionManagerIOCP*)eventSource;
    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED &&
       eventSource->state != UA_EVENTSOURCESTATE_FRESH)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_IOCP_clearConnectionManager(&manager->base);
    UA_free(manager);
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_new_WIN32_TCP(const UA_String eventSourceName) {
    UA_TCPConnectionManagerIOCP *manager = (UA_TCPConnectionManagerIOCP*)
        UA_calloc(1, sizeof(UA_TCPConnectionManagerIOCP));
    if(!manager)
        return NULL;
    static char tcpProtocol[] = "tcp";
    UA_IOCP_initConnectionManager(&manager->base, eventSourceName,
                                  UA_STRING(tcpProtocol));
    LIST_INIT(&manager->stalledConnections);
    manager->base.cm.eventSource.start = eventSourceStart;
    manager->base.cm.eventSource.stop = eventSourceStop;
    manager->base.cm.eventSource.free = eventSourceFree;
    manager->base.cm.openConnection = TCP_openConnection;
    manager->base.cm.sendWithConnection = TCP_sendWithConnection;
    manager->base.cm.closeConnection = TCP_closeConnection;
    return &manager->base.cm;
}

#endif /* UA_ARCHITECTURE_WIN32 */
