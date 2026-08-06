/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#include <open62541/config.h>

#ifdef UA_ARCHITECTURE_WIN32

#include "eventloop_iocp.h"

#include "../../deps/mp_printf.h"
#include <iphlpapi.h>
#include <stdlib.h>

#define UDP_MANAGER_PARAMS 8
#define UDP_CONNECTION_PARAMS 9

static UA_KeyValueRestriction udpManagerParams[UDP_MANAGER_PARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-low-watermark")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-high-watermark")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-hard-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-message-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-queue-global-limit")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("udp-receive-depth")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

#define UDP_PARAM_LISTEN 0
#define UDP_PARAM_ADDRESS 1
#define UDP_PARAM_PORT 2
#define UDP_PARAM_INTERFACE 3
#define UDP_PARAM_TTL 4
#define UDP_PARAM_LOOPBACK 5
#define UDP_PARAM_REUSE 6
#define UDP_PARAM_SOCKPRIO 7
#define UDP_PARAM_VALIDATE 8

static UA_KeyValueRestriction udpConnectionParams[UDP_CONNECTION_PARAMS] = {
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, true},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("interface")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("ttl")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("loopback")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("reuse")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("sockpriority")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

typedef enum {
    MULTICAST_NONE,
    MULTICAST_IPV4,
    MULTICAST_IPV6
} MulticastType;

typedef struct {
    UA_IOCPOperation operation;
    WSABUF wsabuf;
    UA_ByteString buffer;
    SOCKADDR_STORAGE sourceAddress;
    int sourceAddressLength;
    DWORD flags;
} UA_IOCPUDPReceiveOperation;

typedef struct {
    UA_IOCPOperation operation;
    WSABUF wsabuf;
    UA_IOCPSendBuffer *buffer;
    SOCKADDR_STORAGE destinationAddress;
    int destinationAddressLength;
} UA_IOCPUDPSendOperation;

typedef struct {
    UA_RegisteredSocketIOCP registeredSocket;
    SOCKADDR_STORAGE peerAddress;
    int peerAddressLength;
    UA_Boolean hasPeerAddress;
    UA_IOCPUDPReceiveOperation *receiveOperations;
    UA_UInt32 receiveDepth;
    size_t outstandingSendBytes;
    size_t outstandingSendMessages;
    size_t sendQueueHardLimit;
    size_t sendQueueMessageLimit;
} UA_UDPConnectionIOCP;

typedef struct {
    UA_ConnectionManagerIOCP base;
    UA_UInt32 receiveDepth;
} UA_UDPConnectionManagerIOCP;

static UA_StatusCode submitReceive(UA_UDPConnectionIOCP *connection,
                                   UA_IOCPUDPReceiveOperation *receive);
static void beginClose(UA_UDPConnectionIOCP *connection, UA_StatusCode reason);

static MulticastType
multicastType(const struct sockaddr *address) {
    if(address->sa_family == AF_INET) {
        UA_UInt32 value = ntohl(((const SOCKADDR_IN*)address)->sin_addr.s_addr);
        return ((value & 0xf0000000u) == 0xe0000000u) ?
            MULTICAST_IPV4 : MULTICAST_NONE;
    }
    if(address->sa_family == AF_INET6) {
        const SOCKADDR_IN6 *address6 = (const SOCKADDR_IN6*)address;
        return (address6->sin6_addr.u.Byte[0] == 0xff) ?
            MULTICAST_IPV6 : MULTICAST_NONE;
    }
    return MULTICAST_NONE;
}

static SOCKET
UDP_createOverlappedSocket(int family) {
    return WSASocketW(family, SOCK_DGRAM, IPPROTO_UDP,
                      NULL, 0, WSA_FLAG_OVERLAPPED);
}

static UA_StatusCode
UDP_setIPv6Only(SOCKET socket, int family) {
    if(family != AF_INET6)
        return UA_STATUSCODE_GOOD;
    int value = 1;
    return (setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY,
                       (const char*)&value, sizeof(value)) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADCONNECTIONREJECTED;
}

static UA_StatusCode
setSocketOptions(SOCKET socket, int family,
                 const UA_KeyValueMap *params) {
    UA_StatusCode result = UDP_setIPv6Only(socket, family);

    if(UA_KeyValueMap_getScalar(
           params, udpConnectionParams[UDP_PARAM_SOCKPRIO].name,
           &UA_TYPES[UA_TYPES_UINT32]))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    const UA_Boolean *reuse = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_REUSE].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(reuse) {
        int value = *reuse ? 1 : 0;
        if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                      (const char*)&value, sizeof(value)) != 0)
            result |= UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    const UA_UInt32 *ttl = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_TTL].name,
        &UA_TYPES[UA_TYPES_UINT32]);
    if(ttl) {
        int option = (family == AF_INET6) ?
            IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL;
        int level = (family == AF_INET6) ? IPPROTO_IPV6 : IPPROTO_IP;
        if(setsockopt(socket, level, option,
                      (const char*)ttl, sizeof(*ttl)) != 0)
            result |= UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    const UA_Boolean *loopback = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_LOOPBACK].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(loopback) {
        int level = (family == AF_INET6) ? IPPROTO_IPV6 : IPPROTO_IP;
        int option = (family == AF_INET6) ?
            IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP;
        if(family == AF_INET6) {
            DWORD value = *loopback ? 1u : 0u;
            if(setsockopt(socket, level, option,
                          (const char*)&value, sizeof(value)) != 0)
                result |= UA_STATUSCODE_BADCONNECTIONREJECTED;
        } else {
            UCHAR value = *loopback ? 1u : 0u;
            if(setsockopt(socket, level, option,
                          (const char*)&value, sizeof(value)) != 0)
                result |= UA_STATUSCODE_BADCONNECTIONREJECTED;
        }
    }
    return result;
}

static UA_StatusCode
copyInterfaceName(const UA_KeyValueMap *params,
                  char *buffer, size_t bufferSize) {
    const UA_String *interfaceName = (const UA_String*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_INTERFACE].name,
        &UA_TYPES[UA_TYPES_STRING]);
    if(!interfaceName) {
        buffer[0] = 0;
        return UA_STATUSCODE_GOOD;
    }
    if(interfaceName->length >= bufferSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    memcpy(buffer, interfaceName->data, interfaceName->length);
    buffer[interfaceName->length] = 0;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
resolveInterfaceIndex(int family, const char *interfaceName,
                      ULONG *interfaceIndex) {
    unsigned int directIndex = if_nametoindex(interfaceName);
    if(directIndex != 0) {
        *interfaceIndex = (ULONG)directIndex;
        return UA_STATUSCODE_GOOD;
    }

    ULONG bufferLength = 0;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                  GAA_FLAG_SKIP_DNS_SERVER;
    DWORD result = GetAdaptersAddresses(
        (ULONG)family, flags, NULL, NULL, &bufferLength);
    if(result != ERROR_BUFFER_OVERFLOW || bufferLength == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    IP_ADAPTER_ADDRESSES *adapters =
        (IP_ADAPTER_ADDRESSES*)UA_malloc(bufferLength);
    if(!adapters)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    result = GetAdaptersAddresses(
        (ULONG)family, flags, NULL, adapters, &bufferLength);
    if(result != NO_ERROR) {
        UA_free(adapters);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode status = UA_STATUSCODE_BADINVALIDARGUMENT;
    for(IP_ADAPTER_ADDRESSES *adapter = adapters;
        adapter; adapter = adapter->Next) {
        ULONG index = (family == AF_INET) ?
            adapter->IfIndex : adapter->Ipv6IfIndex;
        if(index == 0)
            continue;
        if(adapter->AdapterName &&
           strcmp(adapter->AdapterName, interfaceName) == 0) {
            *interfaceIndex = index;
            status = UA_STATUSCODE_GOOD;
            break;
        }

        for(IP_ADAPTER_UNICAST_ADDRESS *address =
                adapter->FirstUnicastAddress;
            address; address = address->Next) {
            char numericAddress[NI_MAXHOST] = {0};
            if(getnameinfo(address->Address.lpSockaddr,
                           address->Address.iSockaddrLength,
                           numericAddress, sizeof(numericAddress),
                           NULL, 0, NI_NUMERICHOST) == 0 &&
               strcmp(numericAddress, interfaceName) == 0) {
                *interfaceIndex = index;
                status = UA_STATUSCODE_GOOD;
                break;
            }
        }
        if(status == UA_STATUSCODE_GOOD)
            break;
    }

    UA_free(adapters);
    return status;
}

static UA_StatusCode
configureMulticast(SOCKET socket, const struct sockaddr *group,
                   const UA_KeyValueMap *params, UA_Boolean receive) {
    MulticastType type = multicastType(group);
    if(type == MULTICAST_NONE)
        return UA_STATUSCODE_GOOD;

    char interfaceName[UA_MAXHOSTNAME_LENGTH];
    UA_StatusCode result =
        copyInterfaceName(params, interfaceName, sizeof(interfaceName));
    if(result != UA_STATUSCODE_GOOD)
        return result;

    if(type == MULTICAST_IPV4) {
        struct ip_mreq request;
        memset(&request, 0, sizeof(request));
        request.imr_multiaddr = ((const SOCKADDR_IN*)group)->sin_addr;
        request.imr_interface.s_addr = htonl(INADDR_ANY);
        if(interfaceName[0]) {
            ULONG interfaceIndex = 0;
            result = resolveInterfaceIndex(
                AF_INET, interfaceName, &interfaceIndex);
            if(result != UA_STATUSCODE_GOOD)
                return result;
            request.imr_interface.s_addr = htonl(interfaceIndex);
        }

        int option = receive ? IP_ADD_MEMBERSHIP : IP_MULTICAST_IF;
        const char *value = receive ? (const char*)&request :
                                      (const char*)&request.imr_interface;
        int length = receive ? sizeof(request) :
                               sizeof(request.imr_interface);
        if(setsockopt(socket, IPPROTO_IP, option, value, length) != 0)
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
        return UA_STATUSCODE_GOOD;
    }

    struct ipv6_mreq request6;
    memset(&request6, 0, sizeof(request6));
    request6.ipv6mr_multiaddr =
        ((const SOCKADDR_IN6*)group)->sin6_addr;
    if(interfaceName[0]) {
        ULONG interfaceIndex = 0;
        result = resolveInterfaceIndex(
            AF_INET6, interfaceName, &interfaceIndex);
        if(result != UA_STATUSCODE_GOOD)
            return result;
        request6.ipv6mr_interface = interfaceIndex;
    }

    int option = receive ? IPV6_JOIN_GROUP : IPV6_MULTICAST_IF;
    const char *value = receive ? (const char*)&request6 :
                                  (const char*)&request6.ipv6mr_interface;
    int length = receive ? sizeof(request6) :
                           sizeof(request6.ipv6mr_interface);
    if(setsockopt(socket, IPPROTO_IPV6, option, value, length) != 0)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    return UA_STATUSCODE_GOOD;
}

static void
UDP_checkStopped(UA_UDPConnectionManagerIOCP *manager) {
    if(manager->base.cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       manager->base.socketCount == 0)
        manager->base.cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static void
UDP_notifyClosing(UA_UDPConnectionIOCP *connection) {
    UA_RegisteredSocketIOCP *rs = &connection->registeredSocket;
    if(!rs->callbackOpened || rs->closingCallbackSent || !rs->callback)
        return;
    rs->closingCallbackSent = true;
    rs->callback(&rs->manager->cm, (uintptr_t)rs->socket,
                 rs->application, &rs->context,
                 UA_CONNECTIONSTATE_CLOSING,
                 &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
}

static void
delayedClose(void *application, void *context) {
    (void)context;
    UA_UDPConnectionIOCP *connection = (UA_UDPConnectionIOCP*)application;
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)connection->registeredSocket.manager;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);
    UDP_notifyClosing(connection);
    for(UA_UInt32 i = 0; i < connection->receiveDepth; i++)
        UA_ByteString_clear(&connection->receiveOperations[i].buffer);
    UA_free(connection->receiveOperations);
    if(connection->registeredSocket.socket != INVALID_SOCKET)
        closesocket(connection->registeredSocket.socket);
    UA_IOCP_removeSocket(&manager->base, &connection->registeredSocket);
    UA_free(connection);
    UDP_checkStopped(manager);
    UA_UNLOCK(&el->elMutex);
}

static void
beginClose(UA_UDPConnectionIOCP *connection, UA_StatusCode reason) {
    UA_RegisteredSocketIOCP *rs = &connection->registeredSocket;
    if(rs->state == UA_IOCP_SOCKET_CLOSING)
        return;
    rs->state = UA_IOCP_SOCKET_CLOSING;
    (void)reason;
    UA_IOCP_cancelSocket(rs);
    UA_IOCP_maybeQueueClose(rs);
}

static void
UDP_receiveCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    UA_IOCPUDPReceiveOperation *receive =
        (UA_IOCPUDPReceiveOperation*)operation;
    UA_UDPConnectionIOCP *connection =
        (UA_UDPConnectionIOCP*)operation->owner;

    if(operation->owner->state == UA_IOCP_SOCKET_CLOSING) {
        UA_IOCP_maybeQueueClose(operation->owner);
        return;
    }

    if(error == 0 || error == WSAEMSGSIZE) {
        char host[NI_MAXHOST] = {0};
        char service[NI_MAXSERV] = {0};
        getnameinfo((struct sockaddr*)&receive->sourceAddress,
                    receive->sourceAddressLength,
                    host, sizeof(host), service, sizeof(service),
                    NI_NUMERICHOST | NI_NUMERICSERV);
        unsigned long portValue = strtoul(service, NULL, 10);
        UA_UInt16 port = (portValue <= 65535u) ?
            (UA_UInt16)portValue : 0;
        UA_String remote = UA_STRING(host);
        UA_KeyValuePair pairs[2];
        pairs[0].key = UA_QUALIFIEDNAME(0, "remote-address");
        UA_Variant_setScalar(&pairs[0].value, &remote,
                             &UA_TYPES[UA_TYPES_STRING]);
        pairs[1].key = UA_QUALIFIEDNAME(0, "remote-port");
        UA_Variant_setScalar(&pairs[1].value, &port,
                             &UA_TYPES[UA_TYPES_UINT16]);
        UA_KeyValueMap params = {2, pairs};
        UA_ByteString message = {bytes, receive->buffer.data};
        operation->owner->callback(&operation->owner->manager->cm,
                                   (uintptr_t)operation->owner->socket,
                                   operation->owner->application,
                                   &operation->owner->context,
                                   UA_CONNECTIONSTATE_ESTABLISHED,
                                   &params, message);
    } else {
        beginClose(connection, UA_IOCP_statusFromSocketError(error));
    }

    if(operation->owner->state != UA_IOCP_SOCKET_CLOSING) {
        UA_StatusCode result = submitReceive(connection, receive);
        if(result != UA_STATUSCODE_GOOD)
            beginClose(connection, result);
    } else {
        UA_IOCP_maybeQueueClose(operation->owner);
    }
}

static UA_StatusCode
submitReceive(UA_UDPConnectionIOCP *connection,
              UA_IOCPUDPReceiveOperation *receive) {
    receive->operation.owner = &connection->registeredSocket;
    receive->operation.handler = UDP_receiveCompletion;
    receive->wsabuf.buf = (CHAR*)receive->buffer.data;
    receive->wsabuf.len = (ULONG)((receive->buffer.length > ULONG_MAX) ?
                                  ULONG_MAX : receive->buffer.length);
    receive->sourceAddressLength = sizeof(receive->sourceAddress);
    receive->flags = 0;
    memset(&receive->sourceAddress, 0, sizeof(receive->sourceAddress));

    UA_IOCP_prepareOperation(&receive->operation);
    int result = WSARecvFrom(connection->registeredSocket.socket,
                             &receive->wsabuf, 1, NULL, &receive->flags,
                             (struct sockaddr*)&receive->sourceAddress,
                             &receive->sourceAddressLength,
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
UDP_sendCompletion(UA_IOCPOperation *operation, DWORD bytes, DWORD error) {
    (void)bytes;
    (void)error;
    UA_IOCPUDPSendOperation *send = (UA_IOCPUDPSendOperation*)operation;
    UA_UDPConnectionIOCP *connection =
        (UA_UDPConnectionIOCP*)operation->owner;
    UA_ConnectionManagerIOCP *manager =
        connection->registeredSocket.manager;
    UA_IOCPSendBuffer *buffer = send->buffer;
    if(buffer) {
        UA_assert(connection->outstandingSendMessages > 0);
        UA_assert(connection->outstandingSendBytes >= buffer->length);
        UA_assert(manager->globalQueuedSendBytes >= buffer->length);
        connection->outstandingSendMessages--;
        connection->outstandingSendBytes -= buffer->length;
        manager->globalQueuedSendBytes -= buffer->length;
        UA_IOCP_releaseSendBuffer(manager, buffer);
    }
    UA_free(send);
    if(error != 0 &&
       connection->registeredSocket.state != UA_IOCP_SOCKET_CLOSING)
        beginClose(connection, UA_IOCP_statusFromSocketError(error));
    if(connection->registeredSocket.state == UA_IOCP_SOCKET_CLOSING)
        UA_IOCP_maybeQueueClose(&connection->registeredSocket);
}

static UA_StatusCode
submitSend(UA_UDPConnectionIOCP *connection,
           UA_IOCPUDPSendOperation *send) {
    send->operation.owner = &connection->registeredSocket;
    send->operation.handler = UDP_sendCompletion;
    send->wsabuf.buf = (CHAR*)send->buffer->data;
    send->wsabuf.len = (ULONG)send->buffer->length;

    UA_IOCP_prepareOperation(&send->operation);
    int result = WSASendTo(connection->registeredSocket.socket,
                           &send->wsabuf, 1, NULL, 0,
                           (struct sockaddr*)&send->destinationAddress,
                           send->destinationAddressLength,
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

static UA_StatusCode
initializeReceiveOperations(UA_UDPConnectionManagerIOCP *manager,
                            UA_UDPConnectionIOCP *connection) {
    connection->receiveDepth = manager->receiveDepth;
    if(connection->receiveDepth == 0)
        return UA_STATUSCODE_GOOD;

    connection->receiveOperations = (UA_IOCPUDPReceiveOperation*)
        UA_calloc(connection->receiveDepth,
                  sizeof(UA_IOCPUDPReceiveOperation));
    if(!connection->receiveOperations)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for(UA_UInt32 i = 0; i < connection->receiveDepth; i++) {
        UA_StatusCode result = UA_ByteString_allocBuffer(
            &connection->receiveOperations[i].buffer,
            manager->base.receiveBufferSize);
        if(result != UA_STATUSCODE_GOOD)
            return result;
    }

    for(UA_UInt32 i = 0; i < connection->receiveDepth; i++) {
        UA_StatusCode result =
            submitReceive(connection, &connection->receiveOperations[i]);
        if(result != UA_STATUSCODE_GOOD)
            return result;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createConnection(UA_UDPConnectionManagerIOCP *manager,
                 SOCKET socket, UA_Boolean listening,
                 const struct sockaddr *peer, int peerLength,
                 void *application, void *context,
                 UA_ConnectionManager_connectionCallback callback,
                 UA_UDPConnectionIOCP **resultConnection) {
    UA_UDPConnectionIOCP *connection = (UA_UDPConnectionIOCP*)
        UA_calloc(1, sizeof(UA_UDPConnectionIOCP));
    if(!connection)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    connection->sendQueueHardLimit = manager->base.sendQueueHardLimit;
    connection->sendQueueMessageLimit = manager->base.sendQueueMessageLimit;
    if(peer && peerLength > 0) {
        memcpy(&connection->peerAddress, peer, (size_t)peerLength);
        connection->peerAddressLength = peerLength;
        connection->hasPeerAddress = true;
    }

    UA_StatusCode result = UA_IOCP_insertSocket(
        &manager->base, &connection->registeredSocket,
        UA_IOCP_SOCKET_UDP_READY, socket,
        callback, application, context);
    if(result != UA_STATUSCODE_GOOD) {
        UA_free(connection);
        return result;
    }
    connection->registeredSocket.delayedClose.callback = delayedClose;
    connection->registeredSocket.delayedClose.application = connection;
    *resultConnection = connection;

    if(listening) {
        result = initializeReceiveOperations(manager, connection);
        if(result != UA_STATUSCODE_GOOD) {
            beginClose(connection, result);
            return result;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
bindReceiveSocket(SOCKET socket, const struct addrinfo *ai,
                  MulticastType multicast) {
    if(multicast == MULTICAST_IPV4) {
        SOCKADDR_IN any;
        memset(&any, 0, sizeof(any));
        any.sin_family = AF_INET;
        any.sin_addr.s_addr = htonl(INADDR_ANY);
        any.sin_port = ((SOCKADDR_IN*)ai->ai_addr)->sin_port;
        return (bind(socket, (struct sockaddr*)&any, sizeof(any)) == 0) ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    if(multicast == MULTICAST_IPV6) {
        SOCKADDR_IN6 any6;
        memset(&any6, 0, sizeof(any6));
        any6.sin6_family = AF_INET6;
        any6.sin6_addr = in6addr_any;
        any6.sin6_port = ((SOCKADDR_IN6*)ai->ai_addr)->sin6_port;
        return (bind(socket, (struct sockaddr*)&any6, sizeof(any6)) == 0) ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    return (bind(socket, ai->ai_addr, (int)ai->ai_addrlen) == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADCONNECTIONREJECTED;
}

static UA_StatusCode
registerReceiveSocket(UA_UDPConnectionManagerIOCP *manager,
                      const struct addrinfo *ai,
                      const UA_KeyValueMap *params,
                      void *application, void *context,
                      UA_ConnectionManager_connectionCallback callback,
                      UA_Boolean validate) {
    SOCKET socket = UDP_createOverlappedSocket(ai->ai_family);
    if(socket == INVALID_SOCKET)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    UA_StatusCode result = setSocketOptions(socket, ai->ai_family, params);
    MulticastType multicast = multicastType(ai->ai_addr);
    if(result == UA_STATUSCODE_GOOD)
        result = bindReceiveSocket(socket, ai, multicast);
    if(result == UA_STATUSCODE_GOOD && multicast != MULTICAST_NONE)
        result = configureMulticast(socket, ai->ai_addr, params, true);
    if(result != UA_STATUSCODE_GOOD) {
        closesocket(socket);
        return result;
    }
    if(validate) {
        closesocket(socket);
        return UA_STATUSCODE_GOOD;
    }

    UA_UDPConnectionIOCP *connection = NULL;
    const struct sockaddr *peer =
        (multicast != MULTICAST_NONE) ? ai->ai_addr : NULL;
    int peerLength =
        (multicast != MULTICAST_NONE) ? (int)ai->ai_addrlen : 0;
    result = createConnection(manager, socket, true,
                              peer, peerLength, application,
                              context, callback, &connection);
    if(result != UA_STATUSCODE_GOOD) {
        if(!connection)
            closesocket(socket);
        return result;
    }

    connection->registeredSocket.callbackOpened = true;
    callback(&manager->base.cm, (uintptr_t)socket,
             application, &connection->registeredSocket.context,
             UA_CONNECTIONSTATE_ESTABLISHED,
             &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
openReceive(UA_UDPConnectionManagerIOCP *manager,
            const UA_KeyValueMap *params,
            void *application, void *context,
            UA_ConnectionManager_connectionCallback callback,
            UA_Boolean validate) {
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_PORT].name,
        &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port);

    const UA_Variant *addresses = UA_KeyValueMap_get(
        params, udpConnectionParams[UDP_PARAM_ADDRESS].name);
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
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *resolved = NULL;
        if(getaddrinfo(host, portString, &hints, &resolved) != 0)
            continue;
        for(struct addrinfo *ai = resolved; ai; ai = ai->ai_next) {
            UA_StatusCode result = registerReceiveSocket(
                manager, ai, params, application, context,
                callback, validate);
            if(result == UA_STATUSCODE_GOOD)
                aggregate = UA_STATUSCODE_GOOD;
        }
        freeaddrinfo(resolved);
    }
    return aggregate;
}

static UA_StatusCode
openSend(UA_UDPConnectionManagerIOCP *manager,
         const UA_KeyValueMap *params,
         void *application, void *context,
         UA_ConnectionManager_connectionCallback callback,
         UA_Boolean validate) {
    const UA_Variant *addressValue = UA_KeyValueMap_get(
        params, udpConnectionParams[UDP_PARAM_ADDRESS].name);
    const UA_String *address = NULL;
    if(addressValue &&
       (UA_Variant_isScalar(addressValue) ||
        addressValue->arrayLength == 1))
        address = (const UA_String*)addressValue->data;
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_PORT].name,
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
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    struct addrinfo *resolved = NULL;
    if(getaddrinfo(hostname, portString, &hints, &resolved) != 0)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    UA_StatusCode result = UA_STATUSCODE_BADCONNECTIONREJECTED;
    for(struct addrinfo *ai = resolved; ai; ai = ai->ai_next) {
        SOCKET socket = UDP_createOverlappedSocket(ai->ai_family);
        if(socket == INVALID_SOCKET)
            continue;
        result = setSocketOptions(socket, ai->ai_family, params);
        if(result == UA_STATUSCODE_GOOD &&
           multicastType(ai->ai_addr) != MULTICAST_NONE)
            result = configureMulticast(socket, ai->ai_addr, params, false);
        if(result != UA_STATUSCODE_GOOD) {
            closesocket(socket);
            continue;
        }
        if(validate) {
            closesocket(socket);
            result = UA_STATUSCODE_GOOD;
            break;
        }

        UA_UDPConnectionIOCP *connection = NULL;
        result = createConnection(manager, socket, false,
                                  ai->ai_addr, (int)ai->ai_addrlen,
                                  application, context, callback,
                                  &connection);
        if(result != UA_STATUSCODE_GOOD) {
            if(!connection)
                closesocket(socket);
            continue;
        }
        connection->registeredSocket.callbackOpened = true;
        callback(&manager->base.cm, (uintptr_t)socket,
                 application, &connection->registeredSocket.context,
                 UA_CONNECTIONSTATE_ESTABLISHED,
                 &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        break;
    }

    freeaddrinfo(resolved);
    return result;
}

static UA_StatusCode
openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
               void *application, void *context,
               UA_ConnectionManager_connectionCallback callback) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode result = UA_KeyValueRestriction_validate(
        el->eventLoop.logger, "UDP", udpConnectionParams,
        UDP_CONNECTION_PARAMS, params);
    if(result != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return result;
    }

    UA_Boolean validate = false;
    const UA_Boolean *validateValue = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_VALIDATE].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validateValue)
        validate = *validateValue;
    UA_Boolean listen = false;
    const UA_Boolean *listenValue = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, udpConnectionParams[UDP_PARAM_LISTEN].name,
        &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(listenValue)
        listen = *listenValue;

    result = listen ?
        openReceive(manager, params, application, context, callback, validate) :
        openSend(manager, params, application, context, callback, validate);
    UA_UNLOCK(&el->elMutex);
    return result;
}

static UA_StatusCode
sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                   const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)cm;
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
    if(!rs || rs->state != UA_IOCP_SOCKET_UDP_READY) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    UA_UDPConnectionIOCP *connection = (UA_UDPConnectionIOCP*)rs;
    if(!connection->hasPeerAddress || buffer->length > ULONG_MAX) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_Boolean overLimit =
        connection->outstandingSendMessages >= connection->sendQueueMessageLimit ||
        connection->outstandingSendBytes > connection->sendQueueHardLimit ||
        buffer->length > connection->sendQueueHardLimit -
                         connection->outstandingSendBytes ||
        manager->base.globalQueuedSendBytes >
            manager->base.globalSendQueueHardLimit ||
        buffer->length > manager->base.globalSendQueueHardLimit -
                         manager->base.globalQueuedSendBytes;
    if(overLimit) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADWOULDBLOCK;
    }

    UA_IOCPUDPSendOperation *send = (UA_IOCPUDPSendOperation*)
        UA_calloc(1, sizeof(UA_IOCPUDPSendOperation));
    if(!send) {
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    send->buffer = buffer;
    memcpy(&send->destinationAddress, &connection->peerAddress,
           (size_t)connection->peerAddressLength);
    send->destinationAddressLength = connection->peerAddressLength;
    connection->outstandingSendBytes += buffer->length;
    connection->outstandingSendMessages++;
    manager->base.globalQueuedSendBytes += buffer->length;

    UA_StatusCode result = submitSend(connection, send);
    if(result != UA_STATUSCODE_GOOD) {
        connection->outstandingSendBytes -= buffer->length;
        connection->outstandingSendMessages--;
        manager->base.globalQueuedSendBytes -= buffer->length;
        UA_IOCP_releaseSendBuffer(&manager->base, buffer);
        UA_free(send);
        beginClose(connection, result);
    }

    UA_UNLOCK(&el->elMutex);
    return result;
}

static UA_StatusCode
closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)cm;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);
    UA_RegisteredSocketIOCP *rs =
        UA_IOCP_findSocket(&manager->base, connectionId);
    if(!rs) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    beginClose((UA_UDPConnectionIOCP*)rs,
               UA_STATUSCODE_BADCONNECTIONCLOSED);
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_getManagerUInt32(UA_KeyValueMap *params, const char *name,
                 UA_UInt32 defaultValue, UA_UInt32 *result) {
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    const UA_UInt32 *value = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, key, &UA_TYPES[UA_TYPES_UINT32]);
    if(value) {
        *result = *value;
        return UA_STATUSCODE_GOOD;
    }
    *result = defaultValue;
    return UA_KeyValueMap_setScalar(params, key, result,
                                    &UA_TYPES[UA_TYPES_UINT32]);
}

static UA_StatusCode
UDP_eventSourceStart(UA_EventSource *eventSource) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el =
        (UA_EventLoopWIN32*)eventSource->eventLoop;
    UA_LOCK(&el->elMutex);
    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode result = UA_KeyValueRestriction_validate(
        el->eventLoop.logger, "UDP", udpManagerParams,
        UDP_MANAGER_PARAMS, &eventSource->params);
    if(result == UA_STATUSCODE_GOOD)
        result = UA_IOCP_configureConnectionManager(&manager->base);
    if(result == UA_STATUSCODE_GOOD)
        result = UDP_getManagerUInt32(
            &eventSource->params, "udp-receive-depth",
            UA_IOCP_DEFAULT_UDP_RECEIVE_DEPTH,
            &manager->receiveDepth);
    if(result == UA_STATUSCODE_GOOD &&
       (manager->receiveDepth == 0 || manager->receiveDepth > 64))
        result = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(result == UA_STATUSCODE_GOOD) {
        manager->base.eventLoop = el;
        eventSource->state = UA_EVENTSOURCESTATE_STARTED;
    }

    UA_UNLOCK(&el->elMutex);
    return result;
}

static void *
UDP_shutdownSocketCallback(void *context, UA_RegisteredSocketIOCP *rs) {
    (void)context;
    beginClose((UA_UDPConnectionIOCP*)rs,
               UA_STATUSCODE_BADCONNECTIONCLOSED);
    return NULL;
}

static void
UDP_eventSourceStop(UA_EventSource *eventSource) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)eventSource;
    UA_EventLoopWIN32 *el = manager->base.eventLoop;
    UA_LOCK(&el->elMutex);
    eventSource->state = UA_EVENTSOURCESTATE_STOPPING;
    ZIP_ITER(UA_IOCPSocketTree, &manager->base.sockets,
             UDP_shutdownSocketCallback, manager);
    UDP_checkStopped(manager);
    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
UDP_eventSourceFree(UA_EventSource *eventSource) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)eventSource;
    if(eventSource->state != UA_EVENTSOURCESTATE_STOPPED &&
       eventSource->state != UA_EVENTSOURCESTATE_FRESH)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_IOCP_clearConnectionManager(&manager->base);
    UA_free(manager);
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_new_WIN32_UDP(const UA_String eventSourceName) {
    UA_UDPConnectionManagerIOCP *manager =
        (UA_UDPConnectionManagerIOCP*)UA_calloc(
            1, sizeof(UA_UDPConnectionManagerIOCP));
    if(!manager)
        return NULL;
    static char udpProtocol[] = "udp";
    UA_IOCP_initConnectionManager(&manager->base, eventSourceName,
                                  UA_STRING(udpProtocol));
    manager->base.cm.eventSource.start = UDP_eventSourceStart;
    manager->base.cm.eventSource.stop = UDP_eventSourceStop;
    manager->base.cm.eventSource.free = UDP_eventSourceFree;
    manager->base.cm.openConnection = openConnection;
    manager->base.cm.sendWithConnection = sendWithConnection;
    manager->base.cm.closeConnection = closeConnection;
    return &manager->base.cm;
}

#endif /* UA_ARCHITECTURE_WIN32 */
