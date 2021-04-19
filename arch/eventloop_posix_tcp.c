/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

typedef struct {
    UA_ConnectionManager cm;
    size_t fdCount; /* Number of fd registered in the EventLoop */
    size_t recvBufferSize;
} TCPConnectionManager;

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

/* Set the socket non-blocking */
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

static UA_StatusCode
TCP_close(UA_ConnectionManager *cm, UA_FD fd) {
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Closing TCP connection for fd: %u", (unsigned)fd);

    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;

    /* Close the fd and deregister */
    int ret = UA_close(fd);
    if(ret != 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode sc = UA_EventLoop_deregisterFD(tcm->cm.eventSource.eventLoop, fd);
    if(sc != UA_STATUSCODE_GOOD)
        return sc;

    /* Reduce the count */
    UA_assert(tcm->fdCount > 0);
    tcm->fdCount--;

    /* Closed? */
    if(tcm->fdCount == 0 && cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "All fds closed, eventloop will be stopped");
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
    return UA_STATUSCODE_GOOD;
}

/* Gets called when a connection socket opens, receives data or closes */
static void
TCP_connectionSocketCallback(UA_ConnectionManager *cm, UA_FD fd,
                             void **fdcontext, short event) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Connection socket callback, fd: %u", (unsigned)fd);

    /* Write-Event, a new connection has opened.  */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(event & UA_POSIX_EVENT_WRITE) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "opening a new socket for fd: %u", (unsigned)fd);

        /* The socket has opened. Signal it to the application. */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_GOOD, UA_BYTESTRING_NULL);

        /* Now we are interested in read-events. */
        UA_EventLoop_modifyFD(cm->eventSource.eventLoop, fd, UA_POSIX_EVENT_READ,
                              (UA_FDCallback) TCP_connectionSocketCallback, *fdcontext);
        return;
    }

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Allocate receive buffer for fd: %u", (unsigned)fd);

    /* Allocate the receive-buffer */
    UA_ByteString response;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    res = UA_ByteString_allocBuffer(&response, tcm->recvBufferSize);
    if(res != UA_STATUSCODE_GOOD)
        return; /* Retry in the next iteration */

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Try recv on fd: %u", (unsigned)fd);

    /* Receive */
#ifndef _WIN32
    ssize_t ret = recv(fd, (char*)response.data, response.length, MSG_DONTWAIT);
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "recv(...) returned: %zd for fd: %u", ret, (unsigned)fd);
#else
    int ret = recv(fd, (char*)response.data, response.length, MSG_DONTWAIT);
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "recv(...) returned: %d for fd: %u", ret, (unsigned)fd);
#endif

    if(ret > 0) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "Process received response for fd: %u", (unsigned)fd);

        /* Callback to the application layer */
        response.length = (size_t)ret; /* Set the length of the received buffer */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_GOOD, response);
    } else if(UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_EAGAIN) {
        /* Orderly shutdown of the connection. Signal to the application and
         * then close the connection. We end up in this path after shutdown was
         * called on the socket. Here, we then are in the next EventLoop
         * iteration and the socket is known to be unused. */
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "recv singaled close of connection for fd: %u", (unsigned)fd);

        /* Close the connection if not a temporary error on a nonblocking socket */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_BADCONNECTIONCLOSED,
                               UA_BYTESTRING_NULL);
        TCP_close(cm, fd);
    }

    UA_ByteString_clear(&response);
}

/* Gets called when a new connection opens or if the listenSocket is closed */
static void
TCP_listenSocketCallback(UA_ConnectionManager *cm, UA_FD fd,
                         void **fdcontext, short event) {
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Attempting to listen on socket, fd: %u", (unsigned)fd);

    /* Try to accept a new connection */
    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_FD newsockfd = UA_accept(fd, (struct sockaddr*)&remote, &remote_size);
    if(newsockfd == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Error during accept: %s", errno_str));
        /* Close the listen socket */
        if(UA_ERRNO != UA_INTERRUPTED)
            TCP_close(cm, fd);
        return;
    }

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Socket accept succesfull: %u", (unsigned)fd);

    /* Configure the new socket */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= TCP_setNonBlocking(newsockfd); /* Set the socket non-blocking */
    res |= TCP_setNoSigPipe(newsockfd);   /* Supress interrupts from the socket */
    res |= TCP_setNoNagle(newsockfd);     /* Disable Nagle's algorithm */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Error tcp options: %s, closing socket", errno_str));
        /* Close the listen socket */
        UA_close(newsockfd);
        return;
    }

    void *ctx = cm->initialConnectionContext;
    /* The socket has opened. Signal it to the application. The callback can
     * switch out the context. So put it into a temp variable.  */
    cm->connectionCallback(cm, (uintptr_t)newsockfd, &ctx, UA_STATUSCODE_GOOD,
                           UA_BYTESTRING_NULL);

    /* Register in the EventLoop. Signal to the user if registering failed. */
    res = UA_EventLoop_registerFD(cm->eventSource.eventLoop, newsockfd,
                                  UA_POSIX_EVENT_READ,
                                  (UA_FDCallback) TCP_connectionSocketCallback,
                                  &cm->eventSource, ctx);
    if(res != UA_STATUSCODE_GOOD) {
        cm->connectionCallback(cm, (uintptr_t)newsockfd, &ctx,
                               UA_STATUSCODE_BADINTERNALERROR, UA_BYTESTRING_NULL);
        UA_close(newsockfd);
    }

    /* Increase the count of registered fd */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    tcm->fdCount++;
}

static void
TCP_registerListenSocket(UA_ConnectionManager *cm, struct addrinfo *ai) {
    /* Create the server socket */
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                           "Error opening the listen socket: %s", errno_str));
        return;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY,
                     (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "Could not set an IPv6 socket to IPv6 only");
        UA_close(listenSocket);
        return;
    }
#endif

    /* Allow rebinding to the IP/port combination. Eg. to restart the server. */
    if(UA_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "Could not make the socket reusable");
        UA_close(listenSocket);
        return;
    }

    /* Set the socket non-blocking */
    if(TCP_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "Could not set the listen socket to nonblocking");
        UA_close(listenSocket);
        return;
    }

    /* Supress interrupts from the socket */
    if(TCP_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_close(listenSocket);
        return;
    }

    /* Bind socket to address */
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "Error binding a listen socket: %s", errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Start listening */
    if(UA_listen(listenSocket, UA_MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "Error listening on listen socket: %s", errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Register the socket */
    UA_StatusCode res =
        UA_EventLoop_registerFD(cm->eventSource.eventLoop, listenSocket,
                                UA_POSIX_EVENT_READ,
                                (UA_FDCallback) TCP_listenSocketCallback,
                                &cm->eventSource, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "Error registering the listen socket in the EventLoop");
        UA_close(listenSocket);
        return;
    }

    /* Increase the registered fd count */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    tcm->fdCount++;
}

static UA_StatusCode
TCP_registerListenSocketDomainName(UA_ConnectionManager *cm, const char *hostname,
                                   const char *port) {
    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
#if UA_IPV6
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET;   /* IPv4 only */
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
#ifdef AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG; /* Only return IPv4/IPv6 if at least one
                                      * such address is configured */
#endif

#if UA_LOGLEVEL <= 300
    const char *printHost = hostname;
    if(!printHost)
        printHost = "*";
    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "Start listening for TCP connections "
                "on port %s (hostname %s)", port, printHost);
#endif

    int retcode = UA_getaddrinfo(hostname, port, &hints, &res);
    if(retcode != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "getaddrinfo lookup failed with error %s", errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add listen sockets */
    struct addrinfo *ai = res;
    while(ai) {
        TCP_registerListenSocket(cm, ai);
        ai = ai->ai_next;
    }
    UA_freeaddrinfo(res);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "Attempting to shut down fd: %u", (unsigned)connectionId);
    /* Shutdown, will be picked up by the next iteration of the event loop */
#ifndef _WIN32
    int res = UA_shutdown((UA_FD)connectionId, SHUT_RDWR);
#else
    int res = UA_shutdown((UA_FD)connectionId, SD_BOTH);
#endif
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(res != 0)
        retval = TCP_close(cm, (UA_FD)connectionId);
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK, "shutdown called for fd: %u",
                 (unsigned)connectionId);
    return retval;
}

static UA_StatusCode
TCP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       UA_ByteString *buf) {
    /* Prevent OS signals when sending to a closed socket */
    int flags = MSG_NOSIGNAL;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {

            UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                         UA_LOGCATEGORY_NETWORK,
                         "Attempting to send with fd: %u", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send((UA_FD)connectionId,
                        (const char*)buf->data + nWritten,
                        bytes_to_send, flags);
            if(n < 0 && UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_AGAIN) {
                UA_LOG_SOCKET_ERRNO_GAI_WRAP(
                    UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                                 UA_LOGCATEGORY_NETWORK,
                                 "Send failed with error %s on fd %u",
                                 errno_str, (unsigned)connectionId));
                TCP_shutdownConnection(cm, connectionId);
                UA_ByteString_clear(buf);
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm, const UA_String connectString,
                   void *context) {
    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "Open a TCP connection to %.*s",
                (int)connectString.length, connectString.data);

    /* Disect the connectString */
    char hostname[256];
    char portStr[16];
    const char *colon = (const char*)
        memchr(connectString.data, ':', connectString.length);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(colon) {
        size_t hostnameLen = (uintptr_t)(colon - (const char*)connectString.data);
        size_t portStrLen = connectString.length - hostnameLen - 1;
        if(hostnameLen < 256 && portStrLen < 16) {
            strncpy(hostname, (const char*)connectString.data, hostnameLen);
            hostname[hostnameLen] = 0;
            strncpy(portStr, colon+1, portStrLen);
            portStr[portStrLen] = 0;
        } else {
            res = UA_STATUSCODE_BADINTERNALERROR;
        }
    } else {
        res = UA_STATUSCODE_BADINTERNALERROR;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK, "Invalid connection string");
        return res;
    }

    /* Create the socket description from the connectString
     * TODO: Make this non-blocking */
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = getaddrinfo(hostname, portStr, &hints, &info);
    if(error != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "Lookup of %s failed with error %d - %s",
                       hostname, error, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a socket */
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_SOCKET) {
        freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Could not create client socket: %s", errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Set the socket options */
    res |= TCP_setNonBlocking(newSock);
    res |= TCP_setNoSigPipe(newSock);
    res |= TCP_setNoNagle(newSock);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Could not set socket options: %s", errno_str));
        freeaddrinfo(info);
        close(newSock);
        return res;
    }

    /* Non-blocking connect */
    error = UA_connect(newSock, info->ai_addr, info->ai_addrlen);
    freeaddrinfo(info);
    if(error != 0 && UA_ERRNO != UA_ERR_CONNECTION_PROGRESS) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Connecting the socket failed: %s", errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Register the fd to trigger when output is possible (the connection is open) */
    res = UA_EventLoop_registerFD(cm->eventSource.eventLoop, newSock, UA_POSIX_EVENT_WRITE,
                                  (UA_FDCallback) TCP_connectionSocketCallback,
                                  &cm->eventSource, context);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                         UA_LOGCATEGORY_NETWORK,
                         "registering the socket failed");
        UA_close(newSock);
        return res;
    }

    /* Increase the count */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    tcm->fdCount++;

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "successfully connected socket: %u", (unsigned)newSock);

    return UA_STATUSCODE_GOOD;
}

/* Asynchronously register the listenSocket */
static UA_StatusCode
TCP_eventSourceStart(UA_ConnectionManager *cm) {
    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP, "To start the TCP ConnectionManager, "
                     "it has to be registered in an EventLoop and not started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Initialize networking */
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Listening on a socket? */
    const UA_Variant *portConfig =
        UA_ConfigParameter_getScalarParameter(cm->eventSource.parameters,
                           "listen-port", &UA_TYPES[UA_TYPES_UINT16]);
    if(!portConfig)
        return UA_STATUSCODE_GOOD;

    /* Prepare the port parameter as a string */
    UA_UInt16 port = *(UA_UInt16*)portConfig->data;
    char portno[6];
    UA_snprintf(portno, 6, "%d", port);

    /* Get the hostnames configuration */
    const UA_Variant *hostNames =
        UA_ConfigParameter_getArrayParameter(cm->eventSource.parameters,
                           "listen-hostnames", &UA_TYPES[UA_TYPES_STRING]);
    if(!hostNames) {
        /* No hostnames configured */
        TCP_registerListenSocketDomainName(cm, NULL, portno);
    } else {
        /* Iterate over the configured hostnames */
        UA_String *hostStrings = (UA_String*)hostNames->data;
        for(size_t i = 0; i < hostNames->arrayLength; i++) {
            char hostname[512];
            if(hostStrings[i].length >= sizeof(hostname))
                continue;
            memcpy(hostname, hostStrings[i].data, hostStrings->length);
            hostname[hostStrings->length] = '\0';
            TCP_registerListenSocketDomainName(cm, hostname, portno);
        }
    }

    /* Not a single listen socket configured? Then the ConnectionManager is in
     * stopped state. */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    if(tcm->fdCount == 0) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK, "Could not register a socket "
                     "to listen for TCP connections");
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
TCP_shutdownCallback(UA_EventSource *es, UA_FD fd, void *fdcontext, short event) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
    UA_StatusCode rv = TCP_shutdownConnection(cm, (uintptr_t)fd);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "Error shutting down the socket: %u (%s)", (unsigned)fd, errno_str));
    } else {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "shutting down socket successful: %u", (unsigned)fd);
    }
}

static void
TCP_eventSourceStop(UA_ConnectionManager *cm) {
    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "Shutting down the TCP EventSource");

    /* Shut down all registered fd. The cm is set to "stopped" when the last fd
     * is closed and deregistered in the callback from the EventLoop. */
    UA_EventLoop_iterateFD(cm->eventSource.eventLoop, &cm->eventSource,
                           TCP_shutdownCallback);
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;

    /* Closed? */
    if(tcm->fdCount == 0 && cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING)
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "TCP EventSource successfully stopped");
}

static UA_StatusCode
TCP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "The EventLoop must be stopped before can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_deinitialize_architecture_network();

    while(cm->eventSource.parameters)
        UA_ConfigParameter_delete(&cm->eventSource.parameters);

    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_TCP_new(const UA_String eventSourceName) {
    TCPConnectionManager *cm = (TCPConnectionManager*)
        UA_calloc(1, sizeof(TCPConnectionManager));
    if(!cm)
        return NULL;

    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *)) TCP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))TCP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))TCP_eventSourceDelete;
    cm->cm.openConnection = TCP_openConnection;
    cm->cm.allocNetworkBuffer = TCP_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = TCP_freeNetworkBuffer;
    cm->cm.sendWithConnection = TCP_sendWithConnection;
    cm->cm.closeConnection = TCP_shutdownConnection;
    cm->recvBufferSize = 1 << 14; /* TODO: Read from the config */
    return &cm->cm;
}
