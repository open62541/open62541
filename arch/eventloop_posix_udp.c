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

    size_t fdsSize;
    LIST_HEAD(, UA_RegisteredFD) fds;

    UA_ByteString rxBuffer; /* Reuse the receiver buffer. The size is configured
                             * via the recv-bufsize parameter.*/
} UDPConnectionManager;

static UA_StatusCode
UDPConnectionManager_register(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, rfd);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LIST_INSERT_HEAD(&ucm->fds, rfd, es_pointers);
    ucm->fdsSize++;
    return UA_STATUSCODE_GOOD;
}

static void
UDPConnectionManager_deregister(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;
    UA_EventLoopPOSIX_deregisterFD(el, rfd);
    LIST_REMOVE(rfd, es_pointers);
    UA_assert(ucm->fdsSize > 0);
    ucm->fdsSize--;
}

static UA_StatusCode
UDP_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                       UA_ByteString *buf, size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
UDP_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
static UA_StatusCode
UDP_setNonBlocking(UA_FD sockfd) {
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
UDP_setNoSigPipe(UA_FD sockfd) {
#ifdef SO_NOSIGPIPE
    int val = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/* Test if the ConnectionManager can be stopped */
static void
UDP_checkStopped(UDPConnectionManager *ucm) {
    if(ucm->fdsSize == 0 && ucm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(ucm->cm.eventSource.eventLoop->logger,
                     UA_LOGCATEGORY_NETWORK,
                     "UDP\t| All sockets closed, the EventLoop has stopped");

        UA_ByteString_clear(&ucm->rxBuffer);
        ucm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

static UA_StatusCode
UDP_close(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Closing connection", (unsigned)rfd->fd);

    /* Deregister from the EventLoop */
    UDPConnectionManager_deregister(ucm, rfd);

    /* Close the socket */
    int ret = UA_close(rfd->fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP %u\t| Socket closed", (unsigned)rfd->fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Could not close the socket (%s)",
                          (unsigned)rfd->fd, errno_str));
    }

    /* Free the rfd */
    UA_free(rfd);

    /* Stop if the ucm is stopping and this was the last open socket */
    UDP_checkStopped(ucm);

    return UA_STATUSCODE_GOOD;
}

/* Gets called when a socket receives data or closes */
static void
UDP_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                             short event) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Activity on the socket", (unsigned)rfd->fd);

    /* Write-Event, a new connection has opened.  */
    if(event == UA_FDEVENT_OUT) {
        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP %u\t| Opening a new connection", (unsigned)rfd->fd);

        /* A new socket has opened. Signal it to the application. */
        cm->connectionCallback(cm, (uintptr_t)rfd->fd, &rfd->context,
                               UA_STATUSCODE_GOOD, 0, NULL, UA_BYTESTRING_NULL);

        /* Now we are interested in read-events. */
        rfd->listenEvents = UA_FDEVENT_IN;
        UA_EventLoopPOSIX_modifyFD(el, rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Allocate receive buffer", (unsigned)rfd->fd);

    /* Use the already allocated receive-buffer */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_ByteString response = ucm->rxBuffer;

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED) {
            return; /* Temporary error on an non-blocking socket */
        }

        /* For UDP sockets UA_WOULDBLOCK for recv means that we actually do
         * not receive anything although a signal for EPOLL was received
         * The signal mus thus be EPOLLERR signaling for the write end of a pipe
         * that the read end has been closed */
        UA_assert(UA_ERRNO == UA_WOULDBLOCK || UA_ERRNO == UA_AGAIN);

        /* Orderly shutdown of the socket. Signal to the application and then
         * close the socket. We end up in this code-path after shutdown was
         * called on the socket. Here, we then are in the next EventLoop
         * iteration and the socket is known to be unused. */

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP %u\t| recv signaled the socket was shutdown",
                     (unsigned)rfd->fd);

        cm->connectionCallback(cm, (uintptr_t)rfd->fd, &rfd->context,
                               UA_STATUSCODE_BADCONNECTIONCLOSED,
                               0, NULL, UA_BYTESTRING_NULL);
        UDP_close(ucm, rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Received message of size %u",
                 (unsigned)rfd->fd, (unsigned)ret);

    /* Callback to the application layer */
    response.length = (size_t)ret; /* Set the length of the received buffer */
    cm->connectionCallback(cm, (uintptr_t)rfd->fd, &rfd->context,
                           UA_STATUSCODE_GOOD, 0, NULL, response);
}

static void
UDP_registerListenSocket(UA_ConnectionManager *cm, struct addrinfo *ai) {
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

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
                UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                               "UDP\t| getnameinfo(...) could not resolve the hostname (%s)",
                               errno_str));
        }
    }
    
    /* Create the server socket */
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %s(%s)",
                          (unsigned)listenSocket, hoststr, portstr, errno_str));
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New server socket for \"%s\" on port %s",
                (unsigned)listenSocket, hoststr, portstr);

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY,
                     (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not set an IPv6 socket to IPv6 only, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }
#endif

    /* Allow rebinding to the IP/port combination. Eg. to restart the server. */
    if(UA_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not make the socket reusable, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Set the socket non-blocking */
    if(UDP_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not set the socket non-blocking, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Supress interrupts from the socket */
    if(UDP_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not disable SIGPIPE, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Bind socket to address */
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error binding the socket to the address (%s), closing",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Allocate the UA_RegisteredFD */
    UA_RegisteredFD *newrfd = (UA_RegisteredFD*)
        (UA_RegisteredFD*)UA_malloc(sizeof(UA_RegisteredFD));
    if(!newrfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error allocating memory for the socket, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    newrfd->fd = listenSocket;
    newrfd->es = &cm->eventSource;
    newrfd->callback = (UA_FDCallback)UDP_connectionSocketCallback;
    newrfd->context = cm->initialConnectionContext;
    newrfd->listenEvents = UA_FDEVENT_IN;

    /* Register in the EventLoop */
    UA_StatusCode res = UDPConnectionManager_register(ucm, newrfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error registering the socket, closing",
                       (unsigned)listenSocket);
        UA_free(newrfd);
        UA_close(listenSocket);
        return;
    }
}

static UA_StatusCode
UDP_registerListenSocketDomainName(UA_ConnectionManager *cm, const char *hostname,
                                   const char *port) {
    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
#if UA_IPV6
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET;   /* IPv4 only */
#endif
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;
#ifdef AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG; /* Only return IPv4/IPv6 if at least one
                                      * such address is configured */
#endif

    int retcode = UA_getaddrinfo(hostname, port, &hints, &res);
    if(retcode != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
           UA_LOG_WARNING(cm->eventSource.eventLoop->logger,
                          UA_LOGCATEGORY_NETWORK,
                          "UDP\t| getaddrinfo lookup for \"%s\" on port %s failed (%s)",
                          hostname, port, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add listen sockets */
    struct addrinfo *ai = res;
    while(ai) {
        UDP_registerListenSocket(cm, ai);
        ai = ai->ai_next;
    }
    UA_freeaddrinfo(res);
    return UA_STATUSCODE_GOOD;
}

#ifdef WIN32
static
UA_RegisteredFD *
findRegisteredFD(UDPConnectionManager *cm, uintptr_t connectionId) {
    UA_RegisteredFD *rfd, *rfd_tmp;
    LIST_FOREACH_SAFE(rfd, &cm->fds, es_pointers, rfd_tmp) {

        if (rfd->fd == connectionId) {
            return rfd;
        }
    }
    return NULL;
}
static
void delayedShutdownCallback(void* application, void* data) {

    UA_ConnectionManager *cm = (UA_ConnectionManager*) application;
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_RegisteredFD* rfd = (UA_RegisteredFD *) data;

    UA_ByteString response = ucm->rxBuffer;
    int ret = UA_recv(rfd->fd, (char *)response.data, response.length, MSG_DONTWAIT);
    if(ret == 0 || ret == -1) {
        cm->connectionCallback(cm, (uintptr_t)rfd->fd, &rfd->context,
                               UA_STATUSCODE_BADCONNECTIONCLOSED,
                               0, NULL, UA_BYTESTRING_NULL);
        UDP_close(ucm, rfd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                         UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Error shutting down the socket , windows implementation for processing pending data is not implemented (%s)",
                         (unsigned)rfd->fd, errno_str));
        return;
    }
}
#endif

static UA_StatusCode
UDP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger,
                 UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Shutdown called", (unsigned)connectionId);

   /* Shutdown, will be picked up by the next iteration of the event loop */
#ifndef _WIN32
    int res = UA_shutdown((UA_FD)connectionId, SHUT_RDWR);
#else
    /* In windows we must let the eventloop call shutdown explicitly as a delayed callback in the next cycle
     * shutdown(...) will not cause an event for select(...) on the given file descriptor
     * */
    int res = UA_shutdown((UA_FD)connectionId, SD_SEND);
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_RegisteredFD* rfd = findRegisteredFD(ucm, (UA_FD) connectionId);
    UA_EventLoop *el = cm->eventSource.eventLoop;

    UA_DelayedCallback *explicitlyDelayedShutdownCallback = UA_malloc(sizeof(UA_DelayedCallback));
    explicitlyDelayedShutdownCallback->callback = delayedShutdownCallback;
    explicitlyDelayedShutdownCallback->application = cm;
    explicitlyDelayedShutdownCallback->data = rfd;
    el->addDelayedCallback(el, explicitlyDelayedShutdownCallback);
#endif
    if(res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(cm->eventSource.eventLoop->logger,
                           UA_LOGCATEGORY_NETWORK,
                           "UDP %u\t| Error shutting down the socket (%s)",
                           (unsigned)connectionId, errno_str));
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
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
            UA_LOG_DEBUG(cm->eventSource.eventLoop->logger,
                         UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Attempting to send", (unsigned)connectionId);
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
                                    "UDP %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    UDP_shutdownConnection(cm, connectionId);
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
                                        "UDP %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        UDP_shutdownConnection(cm, connectionId);
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

static UA_StatusCode
UDP_openConnection(UA_ConnectionManager *cm,
                   size_t paramsSize, const UA_KeyValuePair *params,
                   void *context) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Get the connection parameters */
    char hostname[256];
    char portStr[16];

    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Open UDP Connection: No port defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_snprintf(portStr, 6, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *host = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "hostname"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!host) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Open UDP Connection: No hostname defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(host->length >= 256) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Open UDP Connection: Hostname too long, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    strncpy(hostname, (const char*)host->data, host->length);
    hostname[host->length] = 0;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP\t| Open a connection to \"%s\" on port %s", hostname, portStr);

    /* Create the socket description from the connectString
     * TODO: Make this non-blocking */
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    int error = getaddrinfo(hostname, portStr, &hints, &info);
    if(error != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Lookup of %s failed with error %d - %s",
                       hostname, error, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a socket */
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_FD) {
        freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not create socket to connect to %s (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Set the socket options */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UDP_setNonBlocking(newSock);
    res |= UDP_setNoSigPipe(newSock);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not set socket options: %s", errno_str));
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
                           "UDP\t| Connecting the socket to %s failed (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Allocate the UA_RegisteredFD */
    UA_RegisteredFD *newrfd = (UA_RegisteredFD*)
        (UA_RegisteredFD*)UA_malloc(sizeof(UA_RegisteredFD));
    if(!newrfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error allocating memory for the socket, closing",
                       (unsigned)newSock);
        UA_close(newSock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newrfd->fd = newSock;
    newrfd->es = &cm->eventSource;
    newrfd->callback = (UA_FDCallback)UDP_connectionSocketCallback;
    newrfd->context = context;
    newrfd->listenEvents = UA_FDEVENT_OUT; /* Switched to _IN once the
                                            * connection is open */

    /* Register the fd to trigger when output is possible (the connection is open) */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    res = UDPConnectionManager_register(ucm, newrfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Registering the socket for %s failed", hostname);
        UA_close(newSock);
        UA_free(newrfd);
        return res;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New connection to \"%s\" on port %s",
                (unsigned)newSock, hostname, portStr);

    return UA_STATUSCODE_GOOD;
}

/* Asynchronously register the listenSocket */
static UA_StatusCode
UDP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "To start the UDP ConnectionManager, "
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
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| No port configured, don't accept connections");
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
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP\t| Listening on all interfaces");
        UDP_registerListenSocketDomainName(cm, NULL, portno);
    } else if(hostNames->type != &UA_TYPES[UA_TYPES_STRING]) {
        /* Wrong datatype */
           UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                        "UDP\t| The hostnames have to be strings");
           return UA_STATUSCODE_BADINTERNALERROR;
    } else {
        size_t interfaces = hostNames->arrayLength;
        if(UA_Variant_isScalar(hostNames))
            interfaces = 1;
        if(interfaces == 0) {
            UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                         "UDP\t| Listening on all interfaces");
            UDP_registerListenSocketDomainName(cm, NULL, portno);
        } else {
            /* Iterate over the configured hostnames */
            UA_String *hostStrings = (UA_String*)hostNames->data;
            for(size_t i = 0; i < hostNames->arrayLength; i++) {
                char hostname[512];
                if(hostStrings[i].length >= sizeof(hostname))
                    continue;
                memcpy(hostname, hostStrings[i].data, hostStrings->length);
                hostname[hostStrings->length] = '\0';
                UDP_registerListenSocketDomainName(cm, hostname, portno);
            }
        }
    }

    /* The receive buffersize was configured? */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_UInt16 rxBufSize = 2u << 14; /* The default is 16kb */
    const UA_UInt16 *configRxBufSize = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(cm->eventSource.params,
                                 cm->eventSource.paramsSize,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    UA_StatusCode res = UA_ByteString_allocBuffer(&ucm->rxBuffer, rxBufSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
UDP_eventSourceStop(UA_ConnectionManager *cm) {
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_LOG_INFO(cm->eventSource.eventLoop->logger,
                UA_LOGCATEGORY_NETWORK, "UDP\t| Shutting down the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shut down all registered fd. The cm is set to "stopped" when the last fd
     * is closed and deregistered in the callback from the EventLoop. */
    UA_RegisteredFD *rfd, *rfd_tmp;
    LIST_FOREACH_SAFE(rfd, &ucm->fds, es_pointers, rfd_tmp) {
        UDP_close(ucm, rfd);
        // UDP_shutdownConnection(cm, (uintptr_t)rfd->fd);
    }

    /* All sockets closed? Otherwise iterate some more. */
    UDP_checkStopped(ucm);
}

static UA_StatusCode
UDP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                     UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| The EventSource must be stopped before it can be deleted");
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

static const char *udpName = "udp";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_UDP(const UA_String eventSourceName) {
    UDPConnectionManager *cm = (UDPConnectionManager*)
        UA_calloc(1, sizeof(UDPConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *)) UDP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))UDP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))UDP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)udpName);
    cm->cm.openConnection = UDP_openConnection;
    cm->cm.allocNetworkBuffer = UDP_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = UDP_freeNetworkBuffer;
    cm->cm.sendWithConnection = UDP_sendWithConnection;
    cm->cm.closeConnection = UDP_shutdownConnection;
    return &cm->cm;
}

