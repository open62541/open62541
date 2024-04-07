/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"

#define IPV4_PREFIX_MASK 0xF0
#define IPV4_MULTICAST_PREFIX 0xE0
#if UA_IPV6
#   define IPV6_PREFIX_MASK 0xFF
#   define IPV6_MULTICAST_PREFIX 0xFF
#endif

/* Configuration parameters */

#define UDP_MANAGERPARAMS 2

static UA_KeyValueRestriction udpManagerParams[UDP_MANAGERPARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

#define UDP_PARAMETERSSIZE 9
#define UDP_PARAMINDEX_LISTEN 0
#define UDP_PARAMINDEX_ADDR 1
#define UDP_PARAMINDEX_PORT 2
#define UDP_PARAMINDEX_INTERFACE 3
#define UDP_PARAMINDEX_TTL 4
#define UDP_PARAMINDEX_LOOPBACK 5
#define UDP_PARAMINDEX_REUSE 6
#define UDP_PARAMINDEX_SOCKPRIO 7
#define UDP_PARAMINDEX_VALIDATE 8

static UA_KeyValueRestriction udpConnectionParams[UDP_PARAMETERSSIZE] = {
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

/* A registered file descriptor with an additional method pointer */
typedef struct {
    UA_RegisteredFD rfd;

    UA_ConnectionManager_connectionCallback applicationCB;
    void *application;
    void *context;

    struct sockaddr_storage sendAddr;
#ifdef _WIN32
    size_t sendAddrLength;
#else
    socklen_t sendAddrLength;
#endif
} UDP_FD;

typedef enum {
    MULTICASTTYPE_NONE = 0,
    MULTICASTTYPE_IPV4,
    MULTICASTTYPE_IPV6
} MultiCastType;

static UA_Boolean
isMulticastAddress(const UA_Byte *address, UA_Byte mask, UA_Byte prefix) {
    return (address[0] & mask) == prefix;
}

static MultiCastType
multiCastType(struct addrinfo *info) {
    const UA_Byte *address;
    if(info->ai_family == AF_INET) {
        address = (UA_Byte *)&((struct sockaddr_in *)info->ai_addr)->sin_addr;
        if(isMulticastAddress(address, IPV4_PREFIX_MASK, IPV4_MULTICAST_PREFIX))
            return MULTICASTTYPE_IPV4;
#if UA_IPV6
    } else if(info->ai_family == AF_INET6) {
        address = (UA_Byte *)&((struct sockaddr_in6 *)info->ai_addr)->sin6_addr;
        if(isMulticastAddress(address, IPV6_PREFIX_MASK, IPV6_MULTICAST_PREFIX))
            return MULTICASTTYPE_IPV6;
#endif
    }
    return MULTICASTTYPE_NONE;
}

typedef union {
    struct ip_mreq ipv4;
#if UA_IPV6
    struct ipv6_mreq ipv6;
#endif
} IpMulticastRequest;

static UA_StatusCode
setupMulticastRequest(UA_FD socket, IpMulticastRequest *req, const UA_KeyValueMap *params,
                      struct addrinfo *info, const UA_Logger *logger) {
    /* Was an interface defined? */
    const UA_String *netif = (const UA_String*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_INTERFACE].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!netif) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| No network interface defined for multicast. "
                       "That means the first suitable network interface is used.",
                       (unsigned)socket);
    }

    if(info->ai_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)info->ai_addr;
        req->ipv4.imr_multiaddr = sin->sin_addr;
        req->ipv4.imr_interface.s_addr = htonl(INADDR_ANY); /* default ANY */
        if(!netif)
            return UA_STATUSCODE_GOOD;
        UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * netif->length + 1);
        memcpy(interfaceAsChar, netif->data, netif->length);
        interfaceAsChar[netif->length] = 0;
        if(UA_inet_pton(AF_INET, interfaceAsChar, &req->ipv4.imr_interface) <= 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                         "UDP\t| Interface configuration preparation failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        return UA_STATUSCODE_GOOD;
#if UA_IPV6
    } else if(info->ai_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)info->ai_addr;
        req->ipv6.ipv6mr_multiaddr = sin6->sin6_addr;
        req->ipv6.ipv6mr_interface = 0; /* default ANY interface */
        if(!netif)
            return UA_STATUSCODE_GOOD;
        UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * netif->length + 1);
        memcpy(interfaceAsChar, netif->data, netif->length);
        interfaceAsChar[netif->length] = 0;
        req->ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
        if(req->ipv6.ipv6mr_interface == 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                         "UDP\t| Interface configuration preparation failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        return UA_STATUSCODE_GOOD;
#endif
    }
    return UA_STATUSCODE_BADINTERNALERROR;
}

/* Retrieves hostname and port from given key value parameters.
 *
 * @param[in] params the parameter map to retrieve from
 * @param[out] hostname the retrieved hostname when present, NULL otherwise
 * @param[out] portStr the retrieved port when present, NULL otherwise
 * @param[in] logger the logger to log information
 * @return -1 upon error, 0 if there was no host or port parameter, 1 if
 *         host and port are present */
static int
getHostAndPortFromParams(const UA_KeyValueMap *params,
                         char *hostname, char *portStr, const UA_Logger *logger) {
    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_PORT].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port); /* checked before */
    mp_snprintf(portStr, UA_MAXPORTSTR_LENGTH, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *host = (const UA_String*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!host) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| No address configured");
        return -1;
    }
    if(host->length >= UA_MAXHOSTNAME_LENGTH) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Open UDP Connection: Hostname too long, aborting");
        return -1;
    }
    strncpy(hostname, (const char*)host->data, host->length);
    hostname[host->length] = 0;
    return 1;
}

static int
getConnectionInfoFromParams(const UA_KeyValueMap *params,
                            char *hostname, char *portStr,
                            struct addrinfo **info, const UA_Logger *logger) {
    int foundParams = getHostAndPortFromParams(params, hostname, portStr, logger);
    if(foundParams < 0)
        return -1;

    /* Create the socket description from the connectString
     * TODO: Make this non-blocking */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    int error = getaddrinfo(hostname, portStr, &hints, info);
    if(error != 0) {
#ifdef _WIN32
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Lookup of %s failed with error %d - %s",
                           hostname, error, errno_str));
#else
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Lookup of %s failed with error %s",
                       hostname, gai_strerror(error));
#endif
        return -1;
    }
    return 1;
}

/* Set loop back data to your host */
static UA_StatusCode
setLoopBackData(UA_SOCKET sockfd, UA_Boolean enableLoopback,
                int ai_family, const UA_Logger *logger) {
    /* The loopback option has a different integer size between IPv4 and IPv6.
     * Some operating systems (e.g. OpenBSD) handle this very strict. Hence the
     * different "enable" variables below are required. */
    int retcode;
#if UA_IPV6
    if(ai_family == AF_INET6) {
        unsigned int enable6 = enableLoopback;
        retcode = UA_setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                                &enable6, sizeof(enable6));
    } else
#endif
    {
        unsigned char enable = enableLoopback;
        retcode = UA_setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
                                &enable, sizeof (enable));
    }
    if(retcode < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Loopback setup failed: "
                         "Cannot set socket option IP_MULTICAST_LOOP. Error: %s",
                         (unsigned)sockfd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setTimeToLive(UA_SOCKET sockfd, UA_UInt32 messageTTL,
              int ai_family, const UA_Logger *logger) {
    /* Set Time to live (TTL). Value of 1 prevent forward beyond the local network. */
#if UA_IPV6
    if(UA_setsockopt(sockfd,
                     ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
                     (const char *)&messageTTL,
                     sizeof(messageTTL)) < 0)
#else
    if(UA_setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                     (const char *)&messageTTL,
                     sizeof(messageTTL)) < 0)
#endif
    {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP %u\t| Time to live setup failed: "
                           "Cannot set socket option IP_MULTICAST_TTL. Error: %s",
                           (unsigned)sockfd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setReuseAddress(UA_SOCKET sockfd, UA_Boolean enableReuse, const UA_Logger *logger) {
    /* Set reuse address -> enables sharing of the same listening address on
     * different sockets */
    int enableReuseVal = (enableReuse) ? 1 : 0;
    if(UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                     (const char*)&enableReuseVal, sizeof(enableReuseVal)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP %u\t| Reuse address setup failed: "
                           "Cannot set socket option SO_REUSEADDR. Error: %s",
                           (unsigned)sockfd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef __linux__
static UA_StatusCode
setSocketPriority(UA_SOCKET sockfd, UA_UInt32 socketPriority,
                  const UA_Logger *logger) {
    int prio = (int)socketPriority;
    if(UA_setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(int)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Socket priority setup failed: "
                         "Cannot set socket option SO_PRIORITY. Error: %s",
                         (unsigned)sockfd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
setConnectionConfig(UA_FD socket, const UA_KeyValueMap *params,
                    int ai_family, const UA_Logger *logger) {
    /* Set socket config that is always set */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_EventLoopPOSIX_setNonBlocking(socket);
    res |= UA_EventLoopPOSIX_setNoSigPipe(socket);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
    int optval = 1;
#if UA_IPV6
    if(ai_family == AF_INET6 &&
       UA_setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY,
                     (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not set an IPv6 socket to IPv6 only, closing",
                       (unsigned)socket);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
#endif

    /* Set socket settings from the parameters */
    const UA_UInt32 *messageTTL = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_TTL].name,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(messageTTL)
        res |= setTimeToLive(socket, *messageTTL, ai_family, logger);

    const UA_Boolean *enableLoopback = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_LOOPBACK].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(enableLoopback)
        res |= setLoopBackData(socket, *enableLoopback, ai_family, logger);

    const UA_Boolean *enableReuse = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_REUSE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(enableReuse)
        res |= setReuseAddress(socket, *enableReuse, logger);

#ifdef __linux__
    const UA_UInt32 *socketPriority = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_SOCKPRIO].name,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(socketPriority)
        res |= setSocketPriority(socket, *socketPriority, logger);
#endif

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not set socket options: %s", errno_str));
    }
    return res;
}

static UA_StatusCode
setupListenMultiCast(UA_FD fd, struct addrinfo *info, const UA_KeyValueMap *params,
                     MultiCastType multiCastType, const UA_Logger *logger) {
    IpMulticastRequest req;
    UA_StatusCode res = setupMulticastRequest(fd, &req, params, info, logger);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    int result = -1;
    if(info->ai_family == AF_INET && multiCastType == MULTICASTTYPE_IPV4) {
        result = UA_setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                               &req.ipv4, sizeof(req.ipv4));
#if UA_IPV6
    } else if(info->ai_family == AF_INET6 && multiCastType == MULTICASTTYPE_IPV6) {
        result = UA_setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                               &req.ipv6, sizeof(req.ipv6));
#endif
    }

    if(result < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Cannot set socket for multicast receiving. Error: %s",
                         (unsigned)fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setupSendMultiCast(UA_FD fd, struct addrinfo *info, const UA_KeyValueMap *params,
                   MultiCastType multiCastType, const UA_Logger *logger) {
    IpMulticastRequest req;
    UA_StatusCode res = setupMulticastRequest(fd, &req, params, info, logger);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    int result = -1;
    if(info->ai_family == AF_INET && multiCastType == MULTICASTTYPE_IPV4) {
        result = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
#ifdef _WIN32
                            (const char *)
#endif
                            &req.ipv4.imr_interface, sizeof(struct in_addr));
#if UA_IPV6
    } else if(info->ai_family == AF_INET6 && multiCastType == MULTICASTTYPE_IPV6) {
        result = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
#ifdef _WIN32
                            (const char *)
#endif
                            &req.ipv6.ipv6mr_interface, sizeof(req.ipv6.ipv6mr_interface));
#endif
    }

    if(result < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Cannot set socket for multicast sending. Error: %s",
                         (unsigned)fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* Test if the ConnectionManager can be stopped */
static void
UDP_checkStopped(UA_POSIXConnectionManager *pcm) {
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex, 1);

    if(pcm->fdsSize == 0 &&
       pcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(pcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| All sockets closed, the EventLoop has stopped");
        pcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

/* This method must not be called from the application directly, but from within
 * the EventLoop. Otherwise we cannot be sure whether the file descriptor is
 * still used after calling close. */
static void
UDP_close(UA_POSIXConnectionManager *pcm, UDP_FD *conn) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Closing connection",
                 (unsigned)conn->rfd.fd);

    /* Deregister from the EventLoop */
    UA_EventLoopPOSIX_deregisterFD(el, &conn->rfd);

    /* Deregister internally */
    ZIP_REMOVE(UA_FDTree, &pcm->fds, &conn->rfd);
    UA_assert(pcm->fdsSize > 0);
    pcm->fdsSize--;

    /* Signal closing to the application */
    UA_UNLOCK(&el->elMutex);
    conn->applicationCB(&pcm->cm, (uintptr_t)conn->rfd.fd,
                        conn->application, &conn->context,
                        UA_CONNECTIONSTATE_CLOSING,
                        &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    UA_LOCK(&el->elMutex);

    /* Close the socket */
    int ret = UA_close(conn->rfd.fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP %u\t| Socket closed", (unsigned)conn->rfd.fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Could not close the socket (%s)",
                          (unsigned)conn->rfd.fd, errno_str));
    }

    UA_free(conn);

    /* Stop if the ucm is stopping and this was the last open socket */
    UDP_checkStopped(pcm);
}

static void
UDP_delayedClose(void *application, void *context) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UDP_FD *conn = (UDP_FD*)context;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "UDP %u\t| Delayed closing of the connection",
                 (unsigned)conn->rfd.fd);
    UA_LOCK(&el->elMutex);
    UDP_close(pcm, conn);
    UA_UNLOCK(&el->elMutex);
}

/* Gets called when a socket receives data or closes */
static void
UDP_connectionSocketCallback(UA_POSIXConnectionManager *pcm, UDP_FD *conn,
                             short event) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Activity on the socket",
                 (unsigned)conn->rfd.fd);

    if(event == UA_FDEVENT_ERR) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "UDP %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)conn->rfd.fd, errno_str));
        UDP_close(pcm, conn);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Allocate receive buffer", (unsigned)conn->rfd.fd);

    /* Use the already allocated receive-buffer */
    UA_ByteString response = pcm->rxBuffer;

    /* Receive */
    struct sockaddr_storage source;
#ifndef _WIN32
    socklen_t sourceSize = (socklen_t)sizeof(struct sockaddr_storage);
    ssize_t ret = recvfrom(conn->rfd.fd, (char*)response.data, response.length,
                           MSG_DONTWAIT, (struct sockaddr*)&source, &sourceSize);
#else
    int sourceSize = (int)sizeof(struct sockaddr_storage);
    int ret = recvfrom(conn->rfd.fd, (char*)response.data, (int)response.length,
                       MSG_DONTWAIT, (struct sockaddr*)&source, &sourceSize);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED)
            return;

        /* Orderly shutdown of the socket. We can immediately close as no method
         * "below" in the call stack will use the socket in this iteration of
         * the EventLoop. */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "UDP %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)conn->rfd.fd, errno_str));
        UDP_close(pcm, conn);
        return;
    }

    response.length = (size_t)ret; /* Set the length of the received buffer */

    /* Extract message source and port */
    char sourceAddr[64];
    UA_UInt16 sourcePort;
    switch(source.ss_family) {
        case AF_INET:
            inet_ntop(AF_INET, &((struct sockaddr_in *)&source)->sin_addr,
                    sourceAddr, 64);
            sourcePort = htons(((struct sockaddr_in *)&source)->sin_port);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)&source)->sin6_addr),
                    sourceAddr, 64);
            sourcePort = htons(((struct sockaddr_in6 *)&source)->sin6_port);
            break;
        default:
            sourceAddr[0] = 0;
            sourcePort = 0;
    }

    UA_String sourceAddrStr = UA_STRING(sourceAddr);
    UA_KeyValuePair kvp[2];
    kvp[0].key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_Variant_setScalar(&kvp[0].value, &sourceAddrStr, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "remote-port");
    UA_Variant_setScalar(&kvp[1].value, &sourcePort, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap kvm = {2, kvp};

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Received message of size %u from %s on port %u",
                 (unsigned)conn->rfd.fd, (unsigned)ret,
                 sourceAddr, sourcePort);

    /* Callback to the application layer */
    UA_UNLOCK(&el->elMutex);
    conn->applicationCB(&pcm->cm, (uintptr_t)conn->rfd.fd,
                        conn->application, &conn->context,
                        UA_CONNECTIONSTATE_ESTABLISHED,
                        &kvm, response);
    UA_LOCK(&el->elMutex);
}

static UA_StatusCode
UDP_registerListenSocket(UA_POSIXConnectionManager *pcm, UA_UInt16 port,
                         struct addrinfo *info, const UA_KeyValueMap *params,
                         void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback,
                         UA_Boolean validate) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Get logging information */
    char hoststr[UA_MAXHOSTNAME_LENGTH];
    int get_res = UA_getnameinfo(info->ai_addr, info->ai_addrlen,
                                 hoststr, sizeof(hoststr),
                                 NULL, 0, NI_NUMERICHOST);
    if(get_res != 0) {
        hoststr[0] = 0;
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP\t| getnameinfo(...) could not resolve the hostname (%s)",
                          errno_str));
        if(validate) {
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
        }
    }

    /* Create the listen socket */
    UA_FD listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %u (%s)",
                          (unsigned)listenSocket, hoststr, port, errno_str));
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New listen socket for \"%s\" on port %u",
                (unsigned)listenSocket, hoststr, port);

    /* Set the socket configuration per the parameters */
    UA_StatusCode res =
        setConnectionConfig(listenSocket, params,
                            info->ai_family, el->eventLoop.logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_close(listenSocket);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    /* Are we going to prepare a socket for multicast? */
    MultiCastType mc = multiCastType(info);

    /* Bind socket to the address */
#ifdef _WIN32
    /* On windows we need to bind the socket to INADDR_ANY before registering
     * for the multicast group */
    int ret = -1;
    if(mc != MULTICASTTYPE_NONE) {
        if(info->ai_family == AF_INET) {
            struct sockaddr_in *orig = (struct sockaddr_in *)info->ai_addr;
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = htonl(INADDR_ANY);
            sin.sin_port = orig->sin_port;
            ret = bind(listenSocket, (struct sockaddr*)&sin, sizeof(sin));
        } else if(info->ai_family == AF_INET6) {
            struct sockaddr_in6 *orig = (struct sockaddr_in6 *)info->ai_addr;
            struct sockaddr_in6 sin6;
            memset(&sin6, 0, sizeof(sin6));
            sin6.sin6_family = AF_INET6;
            sin6.sin6_addr = in6addr_any;
            sin6.sin6_port = orig->sin6_port;
            ret = bind(listenSocket, (struct sockaddr*)&sin6, sizeof(sin6));
        }
    } else {
        ret = bind(listenSocket, info->ai_addr, (socklen_t)info->ai_addrlen);
    }
#else
    int ret = bind(listenSocket, info->ai_addr, (socklen_t)info->ai_addrlen);
#endif
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error binding the socket to the address (%s), closing",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    /* Enable multicast if this is a multicast address */
    if(mc != MULTICASTTYPE_NONE) {
        res = setupListenMultiCast(listenSocket, info, params,
                                   mc, el->eventLoop.logger);
        if(res != UA_STATUSCODE_GOOD) {
            UA_close(listenSocket);
            return res;
        }
    }

    /* Validation is complete - close and return */
    if(validate) {
        UA_close(listenSocket);
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate the UA_RegisteredFD */
    UDP_FD *newudpfd = (UDP_FD*)UA_calloc(1, sizeof(UDP_FD));
    if(!newudpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error allocating memory for the socket, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    newudpfd->rfd.fd = listenSocket;
    newudpfd->rfd.es = &pcm->cm.eventSource;
    newudpfd->rfd.listenEvents = UA_FDEVENT_IN;
    newudpfd->rfd.eventSourceCB = (UA_FDCallback)UDP_connectionSocketCallback;
    newudpfd->applicationCB = connectionCallback;
    newudpfd->application = application;
    newudpfd->context = context;

    /* Register in the EventLoop */
    res = UA_EventLoopPOSIX_registerFD(el, &newudpfd->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error registering the socket, closing",
                       (unsigned)listenSocket);
        UA_free(newudpfd);
        UA_close(listenSocket);
        return res;
    }

    /* Register internally in the EventSource */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &newudpfd->rfd);
    pcm->fdsSize++;

    /* Register the listen socket in the application */
    UA_UNLOCK(&el->elMutex);
    connectionCallback(&pcm->cm, (uintptr_t)newudpfd->rfd.fd,
                       application, &newudpfd->context,
                       UA_CONNECTIONSTATE_ESTABLISHED,
                       &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    UA_LOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_registerListenSockets(UA_POSIXConnectionManager *pcm, const char *hostname,
                          UA_UInt16 port, const UA_KeyValueMap *params,
                          void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback,
                          UA_Boolean validate) {
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex, 1);

    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
#if UA_IPV6
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET;   /* IPv4 only */
#endif
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    /* Set up the port string */
    char portstr[6];
    mp_snprintf(portstr, 6, "%d", port);

    int retcode = getaddrinfo(hostname, portstr, &hints, &res);
    if(retcode != 0) {
#ifdef _WIN32
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
           UA_LOG_WARNING(pcm->cm.eventSource.eventLoop->logger,
                          UA_LOGCATEGORY_NETWORK,
                          "UDP\t| getaddrinfo lookup for \"%s\" on port %u failed (%s)",
                          hostname, port, errno_str));
#else
        UA_LOG_WARNING(pcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| getaddrinfo lookup for \"%s\" on port %u failed (%s)",
                       hostname, port, gai_strerror(retcode));
#endif
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    /* Add listen sockets */
    struct addrinfo *ai = res;
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    while(ai) {
        rv = UDP_registerListenSocket(pcm, port, ai, params, application,
                                      context, connectionCallback, validate);
        if(rv != UA_STATUSCODE_GOOD)
            break;
        ai = ai->ai_next;
    }
    freeaddrinfo(res);
    return rv;
}

/* Close the connection via a delayed callback */
static void
UDP_shutdown(UA_ConnectionManager *cm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)cm->eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    if(rfd->dc.callback) {
        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP %u\t| Cannot close - already closing",
                     (unsigned)rfd->fd);
        return;
    }

    /* Shutdown the socket to cancel the current select/epoll */
    shutdown(rfd->fd, UA_SHUT_RDWR);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Shutdown called", (unsigned)rfd->fd);

    UA_DelayedCallback *dc = &rfd->dc;
    dc->callback = UDP_delayedClose;
    dc->application = cm;
    dc->context = rfd;

    /* Don't use the "public" el->addDelayedCallback. It takes a lock. */
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
}

static UA_StatusCode
UDP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)cm->eventSource.eventLoop;
    UA_FD fd = (UA_FD)connectionId;

    UA_LOCK(&el->elMutex);
    UA_RegisteredFD *rfd = ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!rfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Cannot close UDP connection %u - not found",
                       (unsigned)connectionId);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UDP_shutdown(cm, rfd);
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params,
                       UA_ByteString *buf) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    UA_LOCK(&el->elMutex);

    /* Look up the registered UDP socket */
    UA_FD fd = (UA_FD)connectionId;
    UDP_FD *conn = (UDP_FD*)ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!conn) {
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Attempting to send", (unsigned)connectionId);

            /* Prevent OS signals when sending to a closed socket */
            int flags = MSG_NOSIGNAL;
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_sendto((UA_FD)connectionId, (const char*)buf->data + nWritten,
                          bytes_to_send, flags, (struct sockaddr*)&conn->sendAddr,
                          conn->sendAddrLength);
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED &&
                   UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN) {
                    UA_LOG_SOCKET_ERRNO_WRAP(
                       UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                                    "UDP %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    UA_UNLOCK(&el->elMutex);
                    UDP_shutdownConnection(cm, connectionId);
                    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                struct pollfd tmp_poll_fd;
                tmp_poll_fd.fd = (UA_FD)connectionId;
                tmp_poll_fd.events = UA_POLLOUT;
                do {
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED) {
                        UA_LOG_SOCKET_ERRNO_WRAP(
                           UA_LOG_ERROR(el->eventLoop.logger,
                                        UA_LOGCATEGORY_NETWORK,
                                        "UDP %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
                        UDP_shutdown(cm, &conn->rfd);
                        UA_UNLOCK(&el->elMutex);
                        return UA_STATUSCODE_BADCONNECTIONCLOSED;
                    }
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_UNLOCK(&el->elMutex);
    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
registerSocketAndDestinationForSend(const UA_KeyValueMap *params,
                                    const char *hostname, struct addrinfo *info,
                                    int error, UDP_FD *ufd, UA_FD *sock,
                                    const UA_Logger *logger) {
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    *sock = newSock;
    if(newSock == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not create socket to connect to %s (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }
    UA_StatusCode res = setConnectionConfig(newSock, params, info->ai_family, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_close(newSock);
        return res;
    }

    /* Prepare socket for multicast */
    MultiCastType mc = multiCastType(info);
    if(mc != MULTICASTTYPE_NONE) {
        res = setupSendMultiCast(newSock, info, params, mc, logger);
        if(res != UA_STATUSCODE_GOOD) {
            UA_close(newSock);
            return res;
        }
    }

    memcpy(&ufd->sendAddr, info->ai_addr, info->ai_addrlen);
    ufd->sendAddrLength = info->ai_addrlen;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_openSendConnection(UA_POSIXConnectionManager *pcm, const UA_KeyValueMap *params,
                       void *application, void *context,
                       UA_ConnectionManager_connectionCallback connectionCallback,
                       UA_Boolean validate) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Get the connection parameters */
    char hostname[UA_MAXHOSTNAME_LENGTH];
    char portStr[UA_MAXPORTSTR_LENGTH];
    struct addrinfo *info = NULL;

    int error = getConnectionInfoFromParams(params, hostname,
                                            portStr, &info, el->eventLoop.logger);
    if(error < 0 || info == NULL) {
        if(info != NULL) {
            freeaddrinfo(info);
        }
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Opening a connection failed");
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP\t| Open a connection to \"%s\" on port %s", hostname, portStr);

    /* Allocate the UA_RegisteredFD */
    UDP_FD *conn = (UDP_FD*)UA_calloc(1, sizeof(UDP_FD));
    if(!conn) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Error allocating memory for the socket, closing");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Create a socket and register the destination address from the provided parameters */
    UA_FD newSock = UA_INVALID_FD;
    UA_StatusCode res =
        registerSocketAndDestinationForSend(params, hostname, info,
                                            error, conn, &newSock,
                                            el->eventLoop.logger);
    freeaddrinfo(info);
    if(validate && res == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP %u\t| Connection validated to \"%s\" on port %s",
                    (unsigned)newSock, hostname, portStr);
        UA_close(newSock);
        UA_free(conn);
        return UA_STATUSCODE_GOOD;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(conn);
        return res;
    }

    conn->rfd.fd = newSock;
    conn->rfd.listenEvents = 0;
    conn->rfd.es = &pcm->cm.eventSource;
    conn->rfd.eventSourceCB = (UA_FDCallback)UDP_connectionSocketCallback;
    conn->applicationCB = connectionCallback;
    conn->application = application;
    conn->context = context;

    /* Register the fd to trigger when output is possible (the connection is open) */
    res = UA_EventLoopPOSIX_registerFD(el, &conn->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Registering the socket for %s failed", hostname);
        UA_close(newSock);
        UA_free(conn);
        return res;
    }

    /* Register internally in the EventSource */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &conn->rfd);
    pcm->fdsSize++;

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New connection to \"%s\" on port %s",
                (unsigned)newSock, hostname, portStr);

    /* Signal the connection as opening. The connection fully opens in the next
     * iteration of the EventLoop */
    UA_UNLOCK(&el->elMutex);
    connectionCallback(&pcm->cm, (uintptr_t)newSock, application,
                       &conn->context, UA_CONNECTIONSTATE_ESTABLISHED,
                       &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    UA_LOCK(&el->elMutex);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_openReceiveConnection(UA_POSIXConnectionManager *pcm, const UA_KeyValueMap *params,
                          void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback,
                          UA_Boolean validate) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Get the port */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_PORT].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port); /* checked before */

    /* Get the hostname configuration */
    const UA_Variant *addrs =
        UA_KeyValueMap_get(params, udpConnectionParams[UDP_PARAMINDEX_ADDR].name);
    size_t addrsSize = 0;
    if(addrs) {
        UA_assert(addrs->type == &UA_TYPES[UA_TYPES_STRING]);
        if(UA_Variant_isScalar(addrs))
            addrsSize = 1;
        else
            addrsSize = addrs->arrayLength;
    }

    /* No hostname configured -> listen on all interfaces */
    if(addrsSize == 0) {
        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Listening on all interfaces");
        return UDP_registerListenSockets(pcm, NULL, *port, params, application,
                                         context, connectionCallback, validate);
    }

    /* Iterate over the configured hostnames */
    UA_String *hostStrings = (UA_String*)addrs->data;
    for(size_t i = 0; i < addrsSize; i++) {
        char hn[UA_MAXHOSTNAME_LENGTH];
        if(hostStrings[i].length >= sizeof(hn))
            continue;
        memcpy(hn, hostStrings[i].data, hostStrings->length);
        hn[hostStrings->length] = '\0';
        UA_StatusCode rv =
            UDP_registerListenSockets(pcm, hn, *port, params, application,
                                      context, connectionCallback, validate);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);

    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Cannot open a connection for a "
                     "ConnectionManager that is not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "UDP",
                                        udpConnectionParams,
                                        UDP_PARAMETERSSIZE, params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return res;
    }

    UA_Boolean validate = false;
    const UA_Boolean *validationValue = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_VALIDATE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validationValue)
        validate = *validationValue;

    UA_Boolean listen = false;
    const UA_Boolean *listenValue = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, udpConnectionParams[UDP_PARAMINDEX_LISTEN].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(listenValue)
        listen = *listenValue;

    if(listen) {
        res = UDP_openReceiveConnection(pcm, params, application, context,
                                        connectionCallback, validate);
    } else {
        res = UDP_openSendConnection(pcm, params, application, context,
                                     connectionCallback, validate);
    }
    UA_UNLOCK(&el->elMutex);
    return res;
}

static UA_StatusCode
UDP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| To start the ConnectionManager, "
                     "it has to be registered in an EventLoop and not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "UDP",
                                        udpManagerParams, UDP_MANAGERPARAMS,
                                        &cm->eventSource.params);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    /* Allocate the rx buffer */
    res = UA_EventLoopPOSIX_allocateStaticBuffers(pcm);
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;

 finish:
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void *
UDP_shutdownCB(void *application, UA_RegisteredFD *rfd) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)application;
    UDP_shutdown(cm, rfd);
    return NULL;
}

static void
UDP_eventSourceStop(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    (void)el;
    UA_LOCK(&el->elMutex);

    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "UDP\t| Shutting down the ConnectionManager");

    /* Prevent new connections to open */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shutdown all existing connection */
    ZIP_ITER(UA_FDTree, &pcm->fds, UDP_shutdownCB, cm);

    /* Check if stopped once more (also checking inside UDP_close, but there we
     * don't check if there is no rfd at all) */
    UDP_checkStopped(pcm);

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
UDP_eventSourceDelete(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString_clear(&pcm->rxBuffer);
    UA_ByteString_clear(&pcm->txBuffer);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);

    return UA_STATUSCODE_GOOD;
}

static const char *udpName = "udp";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_UDP(const UA_String eventSourceName) {
    UA_POSIXConnectionManager *cm = (UA_POSIXConnectionManager*)
        UA_calloc(1, sizeof(UA_POSIXConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *))UDP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))UDP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))UDP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)udpName);
    cm->cm.openConnection = UDP_openConnection;
    cm->cm.allocNetworkBuffer = UA_EventLoopPOSIX_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = UA_EventLoopPOSIX_freeNetworkBuffer;
    cm->cm.sendWithConnection = UDP_sendWithConnection;
    cm->cm.closeConnection = UDP_shutdownConnection;
    return &cm->cm;
}
