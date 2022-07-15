/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"

#define UA_MAXBACKLOG 100

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

/* A registered file descriptor with an additional method pointer */
typedef struct {
    UA_RegisteredFD fd;
    UA_ConnectionManager_connectionCallback connectionCallback;
} TCP_FD;

typedef struct {
    UA_ConnectionManager cm;

    size_t fdsSize;
    LIST_HEAD(, UA_RegisteredFD) fds;

    UA_ByteString rxBuffer; /* Reuse the receiver buffer. The size is configured
                             * via the recv-bufsize parameter.*/
} TCPConnectionManager;

static void
TCP_shutdown(UA_ConnectionManager *cm, UA_RegisteredFD *rfd);

static UA_StatusCode
TCPConnectionManager_register(TCPConnectionManager *tcm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)tcm->cm.eventSource.eventLoop;
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, rfd);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LIST_INSERT_HEAD(&tcm->fds, rfd, es_pointers);
    tcm->fdsSize++;
    return UA_STATUSCODE_GOOD;
}

static void
TCPConnectionManager_deregister(TCPConnectionManager *tcm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)tcm->cm.eventSource.eventLoop;
    UA_EventLoopPOSIX_deregisterFD(el, rfd);
    LIST_REMOVE(rfd, es_pointers);
    UA_assert(tcm->fdsSize > 0);
    tcm->fdsSize--;
}

static UA_StatusCode
TCP_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                       UA_ByteString *buf, size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
TCP_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
static UA_StatusCode
TCP_setNonBlocking(UA_FD sockfd) {
#ifndef _WIN32
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/* Don't have the socket create interrupt signals */
static UA_StatusCode
TCP_setNoSigPipe(UA_FD sockfd) {
#ifdef SO_NOSIGPIPE
    int val = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/* Do not merge packets on the socket (disable Nagle's algorithm) */
static UA_StatusCode
TCP_setNoNagle(UA_FD sockfd) {
    int val = 1;
    int res = UA_setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* Test if the ConnectionManager can be stopped */
static void
TCP_checkStopped(TCPConnectionManager *tcm) {
    if(tcm->fdsSize == 0 && tcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(tcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| All sockets closed, the EventLoop has stopped");

        UA_ByteString_clear(&tcm->rxBuffer);
        tcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

static UA_StatusCode
TCP_close(TCPConnectionManager *tcm, UA_RegisteredFD *rfd) {
    TCP_FD *tcpfd = (TCP_FD*)rfd;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)tcm->cm.eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Closing connection", (unsigned)rfd->fd);

    /* Deregister from the EventLoop */
    TCPConnectionManager_deregister(tcm, rfd);

    /* Signal closing to the application */
    tcpfd->connectionCallback(&tcm->cm, (uintptr_t)rfd->fd,
                              rfd->application, &rfd->context,
                              UA_CONNECTIONSTATE_CLOSING,
                              0, NULL, UA_BYTESTRING_NULL);

    /* Close the socket */
    int ret = UA_close(rfd->fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Socket closed", (unsigned)rfd->fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not close the socket (%s)",
                          (unsigned)rfd->fd, errno_str));
    }

    /* Don't call UA_free(rfd). This might be done automatically via the delayed
     * callback that calls TCP_close. */

    /* Stop if the tcm is stopping and this was the last open socket */
    TCP_checkStopped(tcm);

    return UA_STATUSCODE_GOOD;
}

static void
TCP_delayedClose(void *application, void *context) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)application;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_RegisteredFD* rfd = (UA_RegisteredFD *)context;
    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "TCP %u\t| Delayed closing of the connection", (unsigned)rfd->fd);
    TCP_close(tcm, rfd);
    /* Don't call free here. This is done automatically via the delayed callback
     * mechanism. */
}

static int
getSockError(UA_RegisteredFD *rfd) {
    int error = 0;
#ifndef _WIN32
    socklen_t errlen = sizeof(int);
    int err = getsockopt(rfd->fd, SOL_SOCKET, SO_ERROR, &error, &errlen);
#else
    int errlen = (int)sizeof(int);
    int err = getsockopt((SOCKET)rfd->fd, SOL_SOCKET, SO_ERROR,
                         (char*)&error, &errlen);
#endif
    return (err == 0) ? error : err;
}

/* Gets called when a connection socket opens, receives data or closes */
static void
TCP_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                             short event) {
    TCP_FD *tcpfd = (TCP_FD*)rfd;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Activity on the socket", (unsigned)rfd->fd);

    /* Error. The connection has closed. */
    if(event == UA_FDEVENT_ERR) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| The connection closes with error %i",
                    (unsigned)rfd->fd, getSockError(rfd));
        TCP_shutdown(cm, rfd);
        return;
    }

    /* Write-Event, a new connection has opened. But some errors come as an
     * out-event. For example if the remote side could not be reached to
     * initiate the connection. So we check manually for error conditions on
     * the socket. */
    if(event == UA_FDEVENT_OUT) {
        int error = getSockError(rfd);
        if(error != 0) {
            UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "TCP %u\t| The connection closes with error %i",
                        (unsigned)rfd->fd, error);
            TCP_shutdown(cm, rfd);
            return;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| Opening a new connection", (unsigned)rfd->fd);

        /* A new socket has opened. Signal it to the application. */
        tcpfd->connectionCallback(cm, (uintptr_t)rfd->fd,
                                  rfd->application, &rfd->context,
                                  UA_CONNECTIONSTATE_ESTABLISHED, 0, NULL,
                                  UA_BYTESTRING_NULL);

        /* Now we are interested in read-events. */
        rfd->listenEvents = UA_FDEVENT_IN;
        UA_EventLoopPOSIX_modifyFD(el, rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Allocate receive buffer", (unsigned)rfd->fd);

    /* Use the already allocated receive-buffer */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_ByteString response = tcm->rxBuffer;

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED ||
           UA_ERRNO == UA_WOULDBLOCK ||
           UA_ERRNO == UA_AGAIN)
            return; /* Temporary error on an non-blocking socket */

        /* Orderly shutdown of the socket */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "TCP %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));
        TCP_shutdown(cm, rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Received message of size %u",
                 (unsigned)rfd->fd, (unsigned)ret);

    /* Callback to the application layer */
    response.length = (size_t)ret; /* Set the length of the received buffer */
    tcpfd->connectionCallback(cm, (uintptr_t)rfd->fd,
                              rfd->application, &rfd->context,
                              UA_CONNECTIONSTATE_ESTABLISHED,
                              0, NULL, response);
}

/* Gets called when a new connection opens or if the listenSocket is closed */
static void
TCP_listenSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd, short event) {
    TCP_FD *tcpfd = (TCP_FD*)rfd;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Callback on server socket", (unsigned)rfd->fd);

    /* Try to accept a new connection */
    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_FD newsockfd = UA_accept(rfd->fd, (struct sockaddr*)&remote, &remote_size);
    if(newsockfd == UA_INVALID_FD) {
        /* Temporary error -- retry */
        if(UA_ERRNO == UA_INTERRUPTED)
            return;

        /* Close the listen socket */
        if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPING) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                               "TCP %u\t| Error %s, closing the server socket",
                               (unsigned)rfd->fd, errno_str));
        }

        TCP_shutdown(cm, rfd);
        return;
    }

    /* Log the name of the remote host */
    char hoststr[256];
    int get_res = UA_getnameinfo((struct sockaddr *)&remote, sizeof(remote),
                                 hoststr, sizeof(hoststr),
                                 NULL, 0, NI_NUMERICHOST);
    if(get_res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| getnameinfo(...) could not resolve the "
                          "hostname (%s)", (unsigned)rfd->fd, errno_str));
    }
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| Connection opened from \"%s\" via the server socket %u",
                (unsigned)newsockfd, hoststr, (unsigned)rfd->fd);

    /* Configure the new socket */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* res |= TCP_setNonBlocking(newsockfd); Inherited from the listen-socket */
    res |= TCP_setNoSigPipe(newsockfd);   /* Supress interrupts from the socket */
    res |= TCP_setNoNagle(newsockfd);     /* Disable Nagle's algorithm */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                           "TCP %u\t| Error seeting the TCP options (%s)",
                           (unsigned)newsockfd, errno_str));
        /* Close the new socket */
        UA_close(newsockfd);
        return;
    }

    /* Allocate the UA_RegisteredFD */
    TCP_FD *newtcpfd = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newtcpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)newsockfd);
        UA_close(newsockfd);
        return;
    }
    newtcpfd->fd.fd = newsockfd;
    newtcpfd->fd.es = &cm->eventSource;
    newtcpfd->fd.callback = (UA_FDCallback)TCP_connectionSocketCallback;
    newtcpfd->fd.application = rfd->application;
    newtcpfd->fd.context = rfd->context;
    newtcpfd->fd.listenEvents = UA_FDEVENT_IN;
    newtcpfd->connectionCallback = tcpfd->connectionCallback;

    /* Register in the EventLoop. Signal to the user if registering failed. */
    res = TCPConnectionManager_register(tcm, &newtcpfd->fd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error registering the socket",
                       (unsigned)newsockfd);
        UA_free(newtcpfd);
        UA_close(newsockfd);
        return;
    }

    /* Forward the remote hostname to the application */
    UA_KeyValuePair kvp;
    kvp.key = UA_QUALIFIEDNAME(0, "remote-hostname");
    UA_String hostName = UA_STRING(hoststr);
    UA_Variant_setScalar(&kvp.value, &hostName, &UA_TYPES[UA_TYPES_STRING]);

    /* The socket has opened. Signal it to the application. */
    tcpfd->connectionCallback(cm, (uintptr_t)newsockfd,
                              newtcpfd->fd.application, &newtcpfd->fd.context,
                              UA_CONNECTIONSTATE_ESTABLISHED,
                              1, &kvp, UA_BYTESTRING_NULL);
}

static UA_StatusCode
TCP_registerListenSocket(UA_ConnectionManager *cm, struct addrinfo *ai,
                         UA_UInt16 port, void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback) {
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Get the hostname information */
    char hoststr[256];
    int get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 hoststr, sizeof(hoststr),
                                 NULL, 0, NI_NUMERICHOST);
    if(get_res != 0) {
        hoststr[0] = 0;
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP\t| getnameinfo(...) could not resolve the "
                          "hostname (%s)", errno_str));
    }

    /* Create the server socket */
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %u (%s)",
                          (unsigned)listenSocket, hoststr, port, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| Creating server socket for \"%s\" on port %u",
                (unsigned)listenSocket, hoststr, port);

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY,
                     (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not set an IPv6 socket to IPv6 only",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#endif

    /* Allow rebinding to the IP/port combination. Eg. to restart the server. */
    if(UA_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not make the socket reusable",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the socket non-blocking */
    if(TCP_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not set the socket non-blocking",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Supress interrupts from the socket */
    if(TCP_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not disable SIGPIPE",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Bind socket to address */
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error binding the socket to the address %s (%s)",
                          (unsigned)listenSocket, hoststr, errno_str));
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start listening */
    if(UA_listen(listenSocket, UA_MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error listening on the socket (%s)",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Allocate the UA_RegisteredFD */
    TCP_FD *newtcpfd = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newtcpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    newtcpfd->fd.fd = listenSocket;
    newtcpfd->fd.es = &cm->eventSource;
    newtcpfd->fd.callback = (UA_FDCallback)TCP_listenSocketCallback;
    newtcpfd->fd.application = application;
    newtcpfd->fd.context = context;
    newtcpfd->fd.listenEvents = UA_FDEVENT_IN;
    newtcpfd->connectionCallback = connectionCallback;

    /* Register in the EventLoop */
    UA_StatusCode res = TCPConnectionManager_register(tcm, &newtcpfd->fd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error registering the socket",
                       (unsigned)listenSocket);
        UA_free(newtcpfd);
        UA_close(listenSocket);
        return res;
    }

    /* Set up the callback parameters */
    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "listen-hostname");
    params[1].key = UA_QUALIFIEDNAME(0, "listen-port");
    UA_String hostname;
    size_t paramsSize = 0;
    if(hoststr[0] != 0) {
        paramsSize = 2;
        hostname = UA_STRING(hoststr);
        UA_Variant_setScalar(&params[0].value, &hostname, &UA_TYPES[UA_TYPES_STRING]);
        UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    }

    /* Announce the listen-socket in the application */
    connectionCallback(cm, (uintptr_t)listenSocket, application,
                       &newtcpfd->fd.context,
                       UA_CONNECTIONSTATE_ESTABLISHED,
                       paramsSize, params, UA_BYTESTRING_NULL);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_registerListenSockets(UA_ConnectionManager *cm, const char *hostname,
                          UA_UInt16 port, void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback) {
    /* Create a string for the port */
    char portstr[6];
    UA_snprintf(portstr, sizeof(portstr), "%d", port);

    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
#if UA_IPV6
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET;   /* IPv4 only */
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
#ifdef AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG; /* Only return IPv4/IPv6 if at least one
                                      * such address is configured */
#endif

    int retcode = UA_getaddrinfo(hostname, portstr, &hints, &res);
    if(retcode != 0) {
#ifdef _WIN32
        UA_LOG_SOCKET_ERRNO_WRAP(
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "tcp\t| Lookup for \"%s\" on port %u failed (%s)",
                       hostname, port, errno_str));
#else
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Lookup for \"%s\" on port %u failed (%s)",
                       hostname, port, gai_strerror(retcode));
#endif
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add listen sockets. Aggregate the results to see if at least one
     * listen-socket was established. */
    UA_StatusCode total_result = UA_INT32_MAX;
    struct addrinfo *ai = res;
    while(ai) {
        total_result &= TCP_registerListenSocket(cm, ai, port, application, context,
                                                 connectionCallback);
        ai = ai->ai_next;
    }
    UA_freeaddrinfo(res);

    return total_result;
}

static
UA_RegisteredFD *
TCP_findRegisteredFD(TCPConnectionManager *tcm, uintptr_t connectionId) {
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &tcm->fds, es_pointers) {
        if(rfd->fd == (UA_FD)connectionId)
            return rfd;
    }
    return NULL;
}

/* Close the connection via a delayed callback */
static void
TCP_shutdown(UA_ConnectionManager *cm, UA_RegisteredFD *rfd) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    if(rfd->dc.callback) {
        UA_LOG_INFO(el->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Cannot close - already closing",
                    (unsigned)rfd->fd);
        return;
    }

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Shutdown called", (unsigned)rfd->fd);

    UA_DelayedCallback *dc = &rfd->dc;
    dc->callback = TCP_delayedClose;
    dc->application = cm;
    dc->context = rfd;
    el->addDelayedCallback(el, dc);
}

static UA_StatusCode
TCP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_RegisteredFD *rfd = TCP_findRegisteredFD(tcm, connectionId);
    if(!rfd) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Cannot close TCP connection %u - not found",
                       (unsigned)connectionId);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    TCP_shutdown(cm, rfd);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       size_t paramsSize, const UA_KeyValuePair *params,
                       UA_ByteString *buf) {
    /* Prevent OS signals when sending to a closed socket */
    int flags = MSG_NOSIGNAL;

    struct pollfd tmp_poll_fd;
    tmp_poll_fd.fd = (UA_FD)connectionId;
    tmp_poll_fd.events = UA_POLLOUT;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                         "TCP %u\t| Attempting to send", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send((UA_FD)connectionId,
                        (const char*)buf->data + nWritten,
                        bytes_to_send, flags);
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED &&
                   UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN) {
                    UA_LOG_SOCKET_ERRNO_GAI_WRAP(
                       UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                                    UA_LOGCATEGORY_NETWORK,
                                    "TCP %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    TCP_shutdownConnection(cm, connectionId);
                    UA_ByteString_clear(buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                do {
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED) {
                        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
                           UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                                        UA_LOGCATEGORY_NETWORK,
                                        "TCP %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        TCP_shutdownConnection(cm, connectionId);
                        UA_ByteString_clear(buf);
                        return UA_STATUSCODE_BADCONNECTIONCLOSED;
                    }
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

/* Create a listen-socket that waits for incoming connections */
static UA_StatusCode
TCP_openPassiveConnection(UA_ConnectionManager *cm,
                          size_t paramsSize, const UA_KeyValuePair *params,
                          void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Get the port parameter */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "listen-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| The listen-port was not correctly configured");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the hostnames parameter */
    const UA_Variant *hostNames =
        UA_KeyValueMap_get(params, paramsSize, UA_QUALIFIEDNAME(0, "listen-hostnames"));
    if(!hostNames) {
        /* No listen-hostnames parameter -> listen on all interfaces */
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP\t| Listening on all interfaces");
        return TCP_registerListenSockets(cm, NULL, *port, application,
                                         context, connectionCallback);
    }

    /* Wrong datatype for the hostnames */
    if(hostNames->type != &UA_TYPES[UA_TYPES_STRING] ||
       UA_Variant_isScalar(hostNames)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| The listen-hostnames have to be an array of strings");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Empty array -> listen on all interfaces */
    size_t interfaces = hostNames->arrayLength;
    if(interfaces == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Listening on all interfaces");
        return TCP_registerListenSockets(cm, NULL, *port, application,
                                         context, connectionCallback);
    }

    /* Iterate over the configured hostnames */
    UA_String *hostStrings = (UA_String*)hostNames->data;
    for(size_t i = 0; i < hostNames->arrayLength; i++) {
        char hostname[512];
        if(hostStrings[i].length >= sizeof(hostname))
            continue;
        memcpy(hostname, hostStrings[i].data, hostStrings->length);
        hostname[hostStrings->length] = '\0';
        TCP_registerListenSockets(cm, hostname, *port, application,
                                  context, connectionCallback);
    }

    return UA_STATUSCODE_GOOD;
}

/* Open a TCP connection to a remote host */
static UA_StatusCode
TCP_openActiveConnection(UA_ConnectionManager *cm,
                         size_t paramsSize, const UA_KeyValuePair *params,
                         void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback) {
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    if(paramsSize != 2) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Port and hostname need to be defined for the connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the connection parameters */
    char hostname[256];
    char portStr[16];

    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize, UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Open TCP Connection: No port defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_snprintf(portStr, 6, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *host = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize, UA_QUALIFIEDNAME(0, "hostname"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!host) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Open TCP Connection: No hostname defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(host->length >= 256) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Open TCP Connection: Hostname too long, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    strncpy(hostname, (const char*)host->data, host->length);
    hostname[host->length] = 0;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP\t| Open a connection to \"%s\" on port %s", hostname, portStr);

    /* Create the socket description from the connectString
     * TODO: Make this non-blocking */
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = getaddrinfo(hostname, portStr, &hints, &info);
    if(error != 0) {
#ifdef _WIN32
        UA_LOG_SOCKET_ERRNO_WRAP(
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Lookup of %s failed (%s)",
                       hostname, errno_str));
#else
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Lookup of %s failed (%s)",
                       hostname, gai_strerror(error));
#endif
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a socket */
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_FD) {
        freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Could not create socket to connect to %s (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Set the socket options */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= TCP_setNonBlocking(newSock);
    res |= TCP_setNoSigPipe(newSock);
    res |= TCP_setNoNagle(newSock);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Could not set socket options: %s", errno_str));
        freeaddrinfo(info);
        UA_close(newSock);
        return res;
    }

    /* Non-blocking connect */
    error = UA_connect(newSock, info->ai_addr, info->ai_addrlen);
    freeaddrinfo(info);
    if(error != 0 &&
       UA_ERRNO != UA_INPROGRESS &&
       UA_ERRNO != UA_WOULDBLOCK) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Connecting the socket to %s failed (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Allocate the UA_RegisteredFD */
    TCP_FD *newtcpfd = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newtcpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)newSock);
        UA_close(newSock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newtcpfd->fd.fd = newSock;
    newtcpfd->fd.es = &cm->eventSource;
    newtcpfd->fd.callback = (UA_FDCallback)TCP_connectionSocketCallback;
    newtcpfd->fd.application = application;
    newtcpfd->fd.context = context;
    newtcpfd->fd.listenEvents = UA_FDEVENT_OUT; /* Switched to _IN once the
                                                 * connection is open */
    newtcpfd->connectionCallback = connectionCallback;

    /* Register the fd to trigger when output is possible (the connection is open) */
    res = TCPConnectionManager_register(tcm, &newtcpfd->fd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Registering the socket to connect to %s failed", hostname);
        UA_close(newSock);
        UA_free(newtcpfd);
        return res;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| New connection to \"%s\" on port %s",
                (unsigned)newSock, hostname, portStr);

    /* Signal the new connection to the application as asynchonously opening */
    connectionCallback(cm, (uintptr_t)newSock,
                       application, &newtcpfd->fd.context,
                       UA_CONNECTIONSTATE_OPENING, 0, NULL,
                       UA_BYTESTRING_NULL);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm,
                   size_t paramsSize, const UA_KeyValuePair *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Cannot open a connection for a "
                     "ConnectionManager that is not started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* If the "port"-parameter is defined, then try to open an active
     * connection. Otherwise try to open a socket that listens for incoming TCP
     * connections. */
    const UA_Variant *val =
        UA_KeyValueMap_get(params, paramsSize, UA_QUALIFIEDNAME(0, "port"));
    if(val) {
        return TCP_openActiveConnection(cm, paramsSize, params,
                                        application, context, connectionCallback);
    } else {
        return TCP_openPassiveConnection(cm, paramsSize, params,
                                         application, context, connectionCallback);
    }
}

static UA_StatusCode
TCP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "To start the TCP ConnectionManager, it has to be "
                     "registered in an EventLoop and not started yet");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Configure the receive buffer */
    UA_UInt32 rxBufSize = 2u << 16; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(cm->eventSource.params, cm->eventSource.paramsSize,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    UA_StatusCode res = UA_ByteString_allocBuffer(&tcm->rxBuffer, rxBufSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;

    return UA_STATUSCODE_GOOD;
}

static void
TCP_eventSourceStop(UA_ConnectionManager *cm) {
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "TCP\t| Shutting down the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shut down all registered fd. The cm is set to "stopped" when the last fd
     * is closed and deregistered in the callback from the EventLoop. */
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &tcm->fds, es_pointers) {
        TCP_shutdown(cm, rfd);
    }

    /* All sockets closed? Otherwise iterate some more. */
    TCP_checkStopped(tcm);
}

static UA_StatusCode
TCP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Delete the parameters */
    UA_Array_delete(cm->eventSource.params, cm->eventSource.paramsSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    cm->eventSource.params = NULL;
    cm->eventSource.paramsSize = 0;

    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *tcpName = "tcp";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_TCP(const UA_String eventSourceName) {
    TCPConnectionManager *cm = (TCPConnectionManager*)
        UA_calloc(1, sizeof(TCPConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *)) TCP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))TCP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))TCP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)tcpName);
    cm->cm.openConnection = TCP_openConnection;
    cm->cm.allocNetworkBuffer = TCP_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = TCP_freeNetworkBuffer;
    cm->cm.sendWithConnection = TCP_sendWithConnection;
    cm->cm.closeConnection = TCP_shutdownConnection;
    return &cm->cm;
}
