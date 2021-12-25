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

static UA_StatusCode
TCP_close(UA_ConnectionManager *cm, UA_FD fd) {
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Closing connection", (unsigned)fd);

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

    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "TCP %u\t| Socket closed", (unsigned)fd);

    /* Stopped? */
    if(tcm->fdCount == 0 && cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "TCP\t| All sockets closed, the EventLoop has stopped");
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
                 "TCP %u\t| Activity on the socket", (unsigned)fd);

    /* Write-Event, a new connection has opened.  */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(event == UA_POLLOUT) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| Opening a new connection", (unsigned)fd);

        /* The socket has opened. Signal it to the application. */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_GOOD, 0, NULL, UA_BYTESTRING_NULL);

        /* Now we are interested in read-events. */
        UA_EventLoop_modifyFD(cm->eventSource.eventLoop, fd, UA_POLLIN,
                              (UA_FDCallback)TCP_connectionSocketCallback,
                              *fdcontext);
        return;
    }

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Allocate receive buffer", (unsigned)fd);

    /* Allocate the receive-buffer */
    UA_ByteString response;
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    res = UA_ByteString_allocBuffer(&response, tcm->recvBufferSize);
    if(res != UA_STATUSCODE_GOOD)
        return; /* Retry in the next iteration */

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(fd, (char*)response.data, response.length, MSG_DONTWAIT);
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| recv(...) returned %zd", (unsigned)fd, ret);
#else
    int ret = UA_recv(fd, (char*)response.data, response.length, MSG_DONTWAIT);
    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| recv(...) returned %d", (unsigned)fd, ret);
#endif

    if(ret > 0) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| Received message of size %u",
                     (unsigned)fd, (unsigned)ret);

        /* Callback to the application layer */
        response.length = (size_t)ret; /* Set the length of the received buffer */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_GOOD, 0, NULL, response);
    } else if(UA_ERRNO != UA_INTERRUPTED &&
              UA_ERRNO != UA_WOULDBLOCK &&
              UA_ERRNO != UA_AGAIN) {
        /* Orderly shutdown of the connection. Signal to the application and
         * then close the connection. We end up in this path after shutdown was
         * called on the socket. Here, we then are in the next EventLoop
         * iteration and the socket is known to be unused. */
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| recv signaled closed connection", (unsigned)fd);

        /* Close the connection if not a temporary error on a nonblocking socket */
        cm->connectionCallback(cm, (uintptr_t)fd, fdcontext,
                               UA_STATUSCODE_BADCONNECTIONCLOSED,
                               0, NULL, UA_BYTESTRING_NULL);
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
                 "TCP %u\t| Callback on server socket", (unsigned)fd);

    /* Try to accept a new connection */
    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_FD newsockfd = UA_accept(fd, (struct sockaddr*)&remote, &remote_size);
    if(newsockfd == UA_INVALID_FD) {
        /* Temporary error -- retry */
        if(UA_ERRNO == UA_INTERRUPTED)
            return;

        /* Close the listen socket */
        if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPING) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                               UA_LOGCATEGORY_NETWORK,
                               "TCP %u\t| Error %s, closing the server socket",
                               (unsigned)fd, errno_str));
        }
        TCP_close(cm, fd);
        return;
    }

    /* Log the name of the remote host */
#if UA_LOGLEVEL <= 300
    char hoststr[256];
    int get_res = UA_getnameinfo((struct sockaddr *)&remote, sizeof(remote),
                                 hoststr, sizeof(hoststr), NULL, 0, 0);
    if(get_res != 0) {
        get_res = UA_getnameinfo((struct sockaddr *)&remote, sizeof(remote),
                                 hoststr, sizeof(hoststr), NULL, 0, NI_NUMERICHOST);
        if(get_res != 0) {
            hoststr[0] = 0;
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                               UA_LOGCATEGORY_NETWORK,
                               "TCP %u\t| getnameinfo(...) could not resolve the "
                               "hostname (%s)", (unsigned)fd, errno_str));
        }
    }
    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| Connection opened from \"%s\" via the server socket %u",
                (unsigned)newsockfd, hoststr, (unsigned)fd);
#endif

    /* Configure the new socket */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* res |= TCP_setNonBlocking(newsockfd); Inherited from the listen-socket */
    res |= TCP_setNoSigPipe(newsockfd);   /* Supress interrupts from the socket */
    res |= TCP_setNoNagle(newsockfd);     /* Disable Nagle's algorithm */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "TCP %u\t| Error seeting the TCP options (%s), closing",
                           (unsigned)newsockfd, errno_str));
        /* Close the new socket */
        UA_close(newsockfd);
        return;
    }

    void *ctx = cm->initialConnectionContext;
    /* The socket has opened. Signal it to the application. The callback can
     * switch out the context. So put it into a temp variable.  */
    cm->connectionCallback(cm, (uintptr_t)newsockfd, &ctx, UA_STATUSCODE_GOOD,
                           0, NULL, UA_BYTESTRING_NULL);

    /* Register in the EventLoop. Signal to the user if registering failed. */
    res = UA_EventLoop_registerFD(cm->eventSource.eventLoop, newsockfd, UA_POLLIN,
                                  (UA_FDCallback)TCP_connectionSocketCallback,
                                  &cm->eventSource, ctx);
    if(res != UA_STATUSCODE_GOOD) {
        cm->connectionCallback(cm, (uintptr_t)newsockfd, &ctx,
                               UA_STATUSCODE_BADINTERNALERROR,
                               0, NULL, UA_BYTESTRING_NULL);
        UA_close(newsockfd);
    }

    /* Increase the count of registered fd */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    tcm->fdCount++;
}

static void
TCP_registerListenSocket(UA_ConnectionManager *cm, struct addrinfo *ai) {
    /* Get logging information */
    char hoststr[256];
    char portstr[16];
    int get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 hoststr, sizeof(hoststr),
                                 portstr, sizeof(portstr), NI_NUMERICSERV);
    if(get_res != 0) {
        get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 hoststr, sizeof(hoststr),
                                 portstr, sizeof(portstr),
                                 NI_NUMERICHOST | NI_NUMERICSERV);
        if(get_res != 0) {
            hoststr[0] = 0;
            portstr[0] = 0;
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                               UA_LOGCATEGORY_NETWORK,
                               "TCP\t| getnameinfo(...) could not resolve the hostname (%s)",
                               errno_str));
        }
    }
    
    /* Create the server socket */
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %s(%s)",
                          (unsigned)listenSocket, hoststr, portstr, errno_str));
        return;
    }

    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| New server socket for \"%s\" on port %s",
                (unsigned)listenSocket, hoststr, portstr);

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
                       "TCP %u\t| Could not set an IPv6 socket to IPv6 only, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }
#endif

    /* Allow rebinding to the IP/port combination. Eg. to restart the server. */
    if(UA_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not make the socket reusable, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Set the socket non-blocking */
    if(TCP_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not set the socket non-blocking, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Supress interrupts from the socket */
    if(TCP_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not disable SIGPIPE, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Bind socket to address */
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error binding the socket to the address (%s), closing",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Start listening */
    if(UA_listen(listenSocket, UA_MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error listening on the socket (%s), closing",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Register the socket */
    UA_StatusCode res =
        UA_EventLoop_registerFD(cm->eventSource.eventLoop, listenSocket, UA_POLLIN,
                                (UA_FDCallback)TCP_listenSocketCallback,
                                &cm->eventSource, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                       UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error registering the socket in the "
                       "EventLoop, closing", (unsigned)listenSocket);
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

    int retcode = UA_getaddrinfo(hostname, port, &hints, &res);
    if(retcode != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                          UA_LOGCATEGORY_NETWORK,
                          "TCP\t| getaddrinfo lookup for \"%s\" on port %s failed (%s)",
                          hostname, port, errno_str));
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
                 "TCP %u\t| Shutdown called", (unsigned)connectionId);

    /* Shutdown, will be picked up by the next iteration of the event loop */
#ifndef _WIN32
    int res = UA_shutdown((UA_FD)connectionId, SHUT_RDWR);
#else
    int res = UA_shutdown((UA_FD)connectionId, SD_BOTH);
#endif
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
                           "TCP %u\t| Error shutting down the socket (%s), closing",
                           (unsigned)connectionId, errno_str));
        retval = TCP_close(cm, (UA_FD)connectionId);
    }
    return retval;
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
            UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                         UA_LOGCATEGORY_NETWORK,
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
                       UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
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
                } while(poll_ret == 0 || (poll_ret < 0 && UA_ERRNO == UA_INTERRUPTED));
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm,
                   size_t paramsSize, const UA_KeyValuePair *params,
                   void *context) {
    /* Get the connection parameters */
    char hostname[256];
    char portStr[16];

    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "target-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Open TCP Connection: No target port defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_snprintf(portStr, 6, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *host = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "target-hostname"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!host) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Open TCP Connection: No target hostname defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(host->length >= 256) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Open TCP Connection: No target hostname too long, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    strncpy(hostname, (const char*)host->data, host->length);
    hostname[host->length] = 0;

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                 UA_LOGCATEGORY_NETWORK, "TCP\t| Open a connection to \"%s\" on port %s",
                 hostname, portStr);

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
                       "TCP\t| Lookup of %s failed with error %d - %s",
                       hostname, error, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a socket */
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_FD) {
        freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
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
            UA_LOG_WARNING(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                           UA_LOGCATEGORY_NETWORK,
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
            UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                         UA_LOGCATEGORY_NETWORK,
                         "TCP\t| Connecting the socket to %s failed (%s)",
                         hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Register the fd to trigger when output is possible (the connection is open) */
    res = UA_EventLoop_registerFD(cm->eventSource.eventLoop, newSock, UA_POLLOUT,
                                  (UA_FDCallback)TCP_connectionSocketCallback,
                                  &cm->eventSource, context);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Registering the socket to connect to %s failed", hostname);
        UA_close(newSock);
        return res;
    }

    /* Increase the count */
    TCPConnectionManager *tcm = (TCPConnectionManager*)cm;
    tcm->fdCount++;

    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| New connection to \"%s\" on port %s",
                (unsigned)newSock, hostname, portStr);

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
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(cm->eventSource.params,
                                 cm->eventSource.paramsSize,
                                 UA_QUALIFIEDNAME(0, "listen-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| No port configured, don't accept connections");
        return UA_STATUSCODE_GOOD;
    }

    /* Prepare the port parameter as a string */
    char portno[6];
    UA_snprintf(portno, 6, "%d", *port);

    /* Get the hostnames configuration */
    const UA_Variant *hostNames =
        UA_KeyValueMap_get(cm->eventSource.params,
                           cm->eventSource.paramsSize,
                           UA_QUALIFIEDNAME(0, "listen-hostnames"));
    if(!hostNames) {
        /* No hostnames configured */
        UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                    UA_LOGCATEGORY_NETWORK, "TCP\t| Listening on all interfaces");
        TCP_registerListenSocketDomainName(cm, NULL, portno);
    } else if(hostNames->type != &UA_TYPES[UA_TYPES_STRING]) {
        /* Wrong datatype */
           UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                        UA_LOGCATEGORY_EVENTLOOP,
                        "TCP\t| The hostnames have to be strings");
           return UA_STATUSCODE_BADINTERNALERROR;
    } else {
        size_t interfaces = hostNames->arrayLength;
        if(UA_Variant_isScalar(hostNames))
            interfaces = 1;
        if(interfaces == 0) {
            UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                         UA_LOGCATEGORY_EVENTLOOP, "TCP\t| Listening on all interfaces");
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
    }

    /* The receive buffersize was configured? */
    const UA_UInt16 *bufSize = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(cm->eventSource.params,
                                 cm->eventSource.paramsSize,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(bufSize)
        ((TCPConnectionManager*)cm)->recvBufferSize = *bufSize;
    
    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
TCP_shutdownCallback(UA_EventSource *es, UA_FD fd, void *fdcontext, short event) {
    TCP_shutdownConnection((UA_ConnectionManager*)es, (uintptr_t)fd);
}

static void
TCP_eventSourceStop(UA_ConnectionManager *cm) {
    UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                UA_LOGCATEGORY_NETWORK, "TCP\t| Shutting down the ConnectionManager");

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
                UA_LOGCATEGORY_NETWORK, "TCP\t| EventSource successfully stopped");
}

static UA_StatusCode
TCP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(cm->eventSource.eventLoop),
                     UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_deinitialize_architecture_network();

    /* Delete the parameters */
    UA_Array_delete(cm->eventSource.params,
                    cm->eventSource.paramsSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    cm->eventSource.params = NULL;
    cm->eventSource.paramsSize = 0;

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

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
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
