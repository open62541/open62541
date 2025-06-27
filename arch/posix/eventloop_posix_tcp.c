/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "open62541/types.h"
#include "eventloop_posix.h"

#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP) || defined(UA_ARCHITECTURE_WIN32)

/* Configuration parameters */
#define TCP_MANAGERPARAMS 2

static UA_KeyValueRestriction tcpManagerParams[TCP_MANAGERPARAMS] = {
    {{0, UA_STRING_STATIC("recv-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-bufsize")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false}
};

#define TCP_PARAMETERSSIZE 5
#define TCP_PARAMINDEX_ADDR 0
#define TCP_PARAMINDEX_PORT 1
#define TCP_PARAMINDEX_LISTEN 2
#define TCP_PARAMINDEX_VALIDATE 3
#define TCP_PARAMINDEX_REUSE 4

static UA_KeyValueRestriction tcpConnectionParams[TCP_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, true},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("reuse")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

typedef struct {
    UA_RegisteredFD rfd;

    UA_ConnectionManager_connectionCallback applicationCB;
    void *application;
    void *context;
} TCP_FD;

static void
TCP_shutdown(UA_ConnectionManager *cm, TCP_FD *conn);

static UA_StatusCode
TCP_registerListenSockets(UA_POSIXConnectionManager *pcm, const char *hostname,
                          UA_UInt16 port, void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback,
                          UA_Boolean validate, UA_Boolean reuseaddr);

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
TCP_checkStopped(UA_POSIXConnectionManager *pcm) {
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex);

    if(pcm->fdsSize == 0 &&
       pcm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(pcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| All sockets closed, the EventLoop has stopped");
        pcm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

static void
TCP_delayedReopen(void *application, void *context) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_ConnectionManager *cm = &pcm->cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_DeregisteredListenFD *listenRfd = (UA_DeregisteredListenFD*)context;
    UA_RegisteredFD *rfd = listenRfd->listenFd;
    TCP_FD *conn = (TCP_FD*)rfd;

    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "TCP %u\t| Delayed reopen of the listen socket",
                 (unsigned)conn->rfd.fd);

    char hostname[UA_MAXHOSTNAME_LENGTH] = {0};
    mp_snprintf(hostname, UA_MAXHOSTNAME_LENGTH, "%.*s",
                (int)rfd->hostname.length, (char*)rfd->hostname.data);

    TCP_registerListenSockets(pcm, hostname, rfd->port, conn->application,
                              conn->context, conn->applicationCB, false, rfd->reuseaddr);

    LIST_REMOVE(listenRfd, pointers);
    UA_String_clear(&listenRfd->listenFd->hostname);
    UA_free(listenRfd->listenFd);
    UA_free(listenRfd);

    if(!pcm->listenFDs.lh_first) {
        el->maxSocketsLimitReached = false;
    }

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
addListenSockets(void *application) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    UA_DeregisteredListenFD *listenFd;
    LIST_FOREACH(listenFd, &pcm->listenFDs, pointers) {
        if(listenFd->listenFd->dc.callback) {
            UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                         "TCP %u\t| Cannot close - already closing",
                         (unsigned)listenFd->listenFd->fd);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
             "TCP %u\t| Reopen listen socket triggered",
             (unsigned)listenFd->listenFd->fd);

        /* Add to the delayed callback list. Will be cleaned up in the next
         * iteration. */
        UA_DelayedCallback *dc = &listenFd->listenFd->dc;
        dc->callback = TCP_delayedReopen;
        dc->application = application;
        dc->context = listenFd;

        /* Adding a delayed callback does not take a lock */
        UA_EventLoopPOSIX_addDelayedCallback(pcm->cm.eventSource.eventLoop, dc);
    }
    return UA_STATUSCODE_GOOD;
}

static void
TCP_delayedClose(void *application, void *context) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_ConnectionManager *cm = &pcm->cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    TCP_FD *conn = (TCP_FD*)context;

    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "TCP %u\t| Delayed closing of the connection",
                 (unsigned)conn->rfd.fd);

    /* Ensure reuse is possible right away. Port-stealing is no longer an issue
     * as the socket gets closed anyway. And we do not want to wait for the
     * timeout to open a new socket for the same address and port. */
    UA_EventLoopPOSIX_setReusable(conn->rfd.fd);

    /* Deregister from the EventLoop */
    UA_EventLoopPOSIX_deregisterFD(el, &conn->rfd);

    /* Deregister internally */
    ZIP_REMOVE(UA_FDTree, &pcm->fds, &conn->rfd);
    UA_assert(pcm->fdsSize > 0);
    pcm->fdsSize--;

    /* Signal closing to the application */
    conn->applicationCB(cm, (uintptr_t)conn->rfd.fd,
                        conn->application, &conn->context,
                        UA_CONNECTIONSTATE_CLOSING,
                        &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);

    /* Close the socket */
    UA_RESET_ERRNO;
    int ret = UA_close(conn->rfd.fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Socket closed", (unsigned)conn->rfd.fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not close the socket (%s)",
                          (unsigned)conn->rfd.fd, errno_str));
    }

    /* Resuming listen sockets in the socket list when socket space becomes available */
    if(el->maxSocketsLimitReached) {
        addListenSockets(application);
    }

    UA_String_clear(&conn->rfd.hostname);
    UA_free(conn);

    /* Check if this was the last connection for a closing ConnectionManager */
    TCP_checkStopped(pcm);

    UA_UNLOCK(&el->elMutex);
}

static int
getSockError(TCP_FD *conn) {
    int error = 0;
#ifndef UA_ARCHITECTURE_WIN32
    socklen_t errlen = sizeof(int);
    int err = UA_getsockopt(conn->rfd.fd, SOL_SOCKET, SO_ERROR, &error, &errlen);
#else
    int errlen = (int)sizeof(int);
    int err = UA_getsockopt((SOCKET)conn->rfd.fd, SOL_SOCKET, SO_ERROR,
                         (char*)&error, &errlen);
#endif
    return (err == 0) ? error : err;
}

/* Gets called when a connection socket opens, receives data or closes */
static void
TCP_connectionSocketCallback(UA_ConnectionManager *cm, TCP_FD *conn,
                             short event) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Activity on the socket",
                 (unsigned)conn->rfd.fd);

    /* Error. The connection has closed. */
    if(event == UA_FDEVENT_ERR) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| The connection closes with error %i",
                    (unsigned)conn->rfd.fd, getSockError(conn));
        TCP_shutdown(cm, conn);
        return;
    }

    /* Write-Event, a new connection has opened. But some errors come as an
     * out-event. For example if the remote side could not be reached to
     * initiate the connection. So we check manually for error conditions on
     * the socket. */
    if(event == UA_FDEVENT_OUT) {
        int error = getSockError(conn);
        if(error != 0) {
            UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "TCP %u\t| The connection closes with error %i",
                        (unsigned)conn->rfd.fd, error);
            TCP_shutdown(cm, conn);
            return;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| Opening a new connection",
                     (unsigned)conn->rfd.fd);

        /* Now we are interested in read-events. */
        conn->rfd.listenEvents = UA_FDEVENT_IN;
        UA_EventLoopPOSIX_modifyFD(el, &conn->rfd);

        /* A new socket has opened. Signal it to the application. */
        conn->applicationCB(cm, (uintptr_t)conn->rfd.fd,
                            conn->application, &conn->context,
                            UA_CONNECTIONSTATE_ESTABLISHED,
                            &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Allocate receive buffer",
                 (unsigned)conn->rfd.fd);

    /* Use the already allocated receive-buffer */
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_ByteString response = pcm->rxBuffer;

    /* Receive */
    UA_RESET_ERRNO;
#ifndef UA_ARCHITECTURE_WIN32
    ssize_t ret = UA_recv(conn->rfd.fd, (char*)response.data,
                          response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(conn->rfd.fd, (char*)response.data,
                      response.length, MSG_DONTWAIT);
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
                        (unsigned)conn->rfd.fd, errno_str));
        TCP_shutdown(cm, conn);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Received message of size %u",
                 (unsigned)conn->rfd.fd, (unsigned)ret);

    /* Callback to the application layer */
    response.length = (size_t)ret; /* Set the length of the received buffer */
    conn->applicationCB(cm, (uintptr_t)conn->rfd.fd,
                        conn->application, &conn->context,
                        UA_CONNECTIONSTATE_ESTABLISHED,
                        &UA_KEYVALUEMAP_NULL, response);
}

static void *
removeListenSockets(void *application, UA_RegisteredFD *rfd) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)application;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    int optval;
    socklen_t optlen = sizeof(optval);

    if(UA_getsockopt(rfd->fd, SOL_SOCKET, SO_ACCEPTCONN, &optval, &optlen) == 0 && optval) {
        TCP_FD *fd = (TCP_FD*)rfd;
        /* Check if it's already listed for reopening */
        UA_Boolean alreadyAdded = UA_FALSE;
        UA_DeregisteredListenFD *listenFd;
        LIST_FOREACH(listenFd, &pcm->listenFDs, pointers) {
            if(UA_String_equal(&listenFd->listenFd->hostname, &rfd->hostname)){
                alreadyAdded = UA_TRUE;
                break;
            }
        }

        if(!alreadyAdded) {
            listenFd = (UA_DeregisteredListenFD*)UA_calloc(1, sizeof(UA_DeregisteredListenFD));
            listenFd->listenFd = rfd;
            LIST_INSERT_HEAD(&pcm->listenFDs, listenFd, pointers);
        }

        /* Ensure reuse is possible right away. Port-stealing is no longer an issue
         * as the socket gets closed anyway. And we do not want to wait for the
         * timeout to open a new socket for the same address and port. */
        UA_EventLoopPOSIX_setReusable(rfd->fd);
        UA_EventLoopPOSIX_deregisterFD(el, rfd);

        /* Deregister internally */
        ZIP_REMOVE(UA_FDTree, &pcm->fds, rfd);
        UA_assert(pcm->fdsSize > 0);
        pcm->fdsSize--;

        /* Signal closing to the application */
        fd->applicationCB(&pcm->cm, (uintptr_t)rfd->fd,
                          fd->application, &fd->context,
                          UA_CONNECTIONSTATE_BLOCKING,
                          &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);

        /* Close the socket */
        UA_RESET_ERRNO;
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

        if(alreadyAdded) {
            UA_String_clear(&rfd->hostname);
            UA_free(rfd);
        }
    }

    return NULL;
}

/* Gets called when a new connection opens or if the listenSocket is closed */
static void
TCP_listenSocketCallback(UA_ConnectionManager *cm, TCP_FD *conn, short event) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Callback on server socket",
                 (unsigned)conn->rfd.fd);

    /* Try to accept a new connection */
    UA_RESET_ERRNO;
    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_FD newsockfd = UA_accept(conn->rfd.fd, (struct sockaddr*)&remote, &remote_size);
    if(newsockfd == UA_INVALID_FD) {
        /* Temporary error -- retry */
        if(UA_IS_TEMPORARY_ACCEPT_ERROR(UA_ERRNO))
            return;

        /* Close the listen socket */
        if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPING) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                               "TCP %u\t| Error %s, closing the server socket",
                               (unsigned)conn->rfd.fd, errno_str));
        }

        TCP_shutdown(cm, conn);
        return;
    }

    /* Log the name of the remote host */
    UA_RESET_ERRNO;
    char hoststr[UA_MAXHOSTNAME_LENGTH];
    int get_res = UA_getnameinfo((struct sockaddr *)&remote, sizeof(remote),
                                 hoststr, sizeof(hoststr),
                                 NULL, 0, NI_NUMERICHOST);
    if(get_res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| getnameinfo(...) could not resolve the "
                          "hostname (%s)", (unsigned)conn->rfd.fd, errno_str));
    }
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| Connection opened from \"%s\" via the server socket %u",
                (unsigned)newsockfd, hoststr, (unsigned)conn->rfd.fd);

    /* Configure the new socket */
    UA_RESET_ERRNO;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* res |= UA_EventLoopPOSIX_setNonBlocking(newsockfd); Inherited from the listen-socket */
    res |= UA_EventLoopPOSIX_setNoSigPipe(newsockfd); /* Supress interrupts from the socket */
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
    TCP_FD *newConn = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newConn) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)newsockfd);
        UA_close(newsockfd);
        return;
    }

    newConn->rfd.fd = newsockfd;
    newConn->rfd.listenEvents = UA_FDEVENT_IN;
    newConn->rfd.es = &cm->eventSource;
    newConn->rfd.eventSourceCB = (UA_FDCallback)TCP_connectionSocketCallback;
    newConn->applicationCB = conn->applicationCB;
    newConn->application = conn->application;
    newConn->context = conn->context;

    /* Register in the EventLoop. Signal to the user if registering failed. */
    res = UA_EventLoopPOSIX_registerFD(el, &newConn->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error registering the socket",
                       (unsigned)newsockfd);
        UA_free(newConn);
        UA_close(newsockfd);
        return;
    }

    /* Register internally in the EventSource */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &newConn->rfd);
    pcm->fdsSize++;

    /* Verify whether the maximum socket limit has been exceeded.
     * If true, remove listen sockets from the socket list to stop
     * accepting additional connection requests */
    const UA_UInt32 *maxSockets = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &cm->eventSource.params, UA_QUALIFIEDNAME(0, "max-connections"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(maxSockets && *maxSockets != 0 && pcm->fdsSize >= (size_t)*maxSockets) {
        ZIP_ITER(UA_FDTree, &pcm->fds, removeListenSockets, cm);
        el->maxSocketsLimitReached = true;
    }

    /* Forward the remote hostname to the application */
    UA_KeyValuePair kvp;
    kvp.key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_String hostName = UA_STRING(hoststr);
    UA_Variant_setScalar(&kvp.value, &hostName, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap kvm;
    kvm.mapSize = 1;
    kvm.map = &kvp;

    /* The socket has opened. Signal it to the application. */
    newConn->applicationCB(cm, (uintptr_t)newsockfd,
                           newConn->application, &newConn->context,
                           UA_CONNECTIONSTATE_ESTABLISHED,
                           &kvm, UA_BYTESTRING_NULL);
}

static UA_StatusCode
TCP_registerListenSocket(UA_POSIXConnectionManager *pcm, struct addrinfo *ai,
                         const char *hostname, UA_UInt16 port,
                         void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback,
                         UA_Boolean validate, UA_Boolean reuseaddr) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    /* Check that the maximum number of sockets has not been exceeded */
    const UA_UInt32 *maxSockets = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &pcm->cm.eventSource.params, UA_QUALIFIEDNAME(0, "max-connections"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(el->maxSocketsLimitReached || (maxSockets && *maxSockets != 0 && pcm->fdsSize >= (size_t)*maxSockets)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Unable to establish connection: no available sockets");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Translate INADDR_ANY to IPv4/IPv6 address */
    UA_RESET_ERRNO;
    char addrstr[UA_MAXHOSTNAME_LENGTH];
    int get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 addrstr, sizeof(addrstr), NULL, 0, 0);
    if(get_res != 0) {
        get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 addrstr, sizeof(addrstr),
                                 NULL, 0, NI_NUMERICHOST);
        if(get_res != 0) {
            addrstr[0] = 0;
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                               "TCP\t| getnameinfo(...) could not resolve the "
                               "hostname (%s)", errno_str));
        }
    }

    /* Create the server socket */
    UA_RESET_ERRNO;
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %u (%s)",
                          (unsigned)listenSocket, addrstr, port, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
#if UA_IPV6
    int optval = 1;
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
    if(reuseaddr &&
       UA_EventLoopPOSIX_setReusable(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not make the socket addr reusable",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the socket non-blocking */
    if(UA_EventLoopPOSIX_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not set the socket non-blocking",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Supress interrupts from the socket */
    if(UA_EventLoopPOSIX_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Could not disable SIGPIPE",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Bind socket to address */
    UA_RESET_ERRNO;
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);

    /* Get the port being used if dynamic porting was used */
    if(port == 0) {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        socklen_t len = sizeof(sin);
        UA_getsockname(listenSocket, (struct sockaddr *)&sin, &len);
        port = ntohs(sin.sin_port);
    }

    /* If the INADDR_ANY is used, use the local hostname */
    char hoststr[UA_MAXHOSTNAME_LENGTH];
    if(hostname) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Creating listen socket for \"%s\" on port %u",
                    (unsigned)listenSocket, hostname, port);
    } else {
        UA_gethostname(hoststr, UA_MAXHOSTNAME_LENGTH);
        hostname = hoststr;
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Creating listen socket for \"%s\" "
                    "(with local hostname \"%s\") on port %u",
                    (unsigned)listenSocket, addrstr, hostname, port);
    }

    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error binding the socket to the address %s (%s)",
                          (unsigned)listenSocket, hostname, errno_str));
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Only validate, don't actually start listening */
    if(validate) {
        UA_EventLoopPOSIX_setReusable(listenSocket); /* Ensure reuse is possible */
        UA_close(listenSocket);
        return UA_STATUSCODE_GOOD;
    }

    /* Start listening */
    UA_RESET_ERRNO;
    if(UA_listen(listenSocket, UA_MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Error listening on the socket (%s)",
                          (unsigned)listenSocket, errno_str));
        UA_EventLoopPOSIX_setReusable(listenSocket); /* Ensure reuse is possible */
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Allocate the connection */
    TCP_FD *newConn = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newConn) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)listenSocket);
        UA_EventLoopPOSIX_setReusable(listenSocket); /* Ensure reuse is possible */
        UA_close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    newConn->rfd.fd = listenSocket;
    newConn->rfd.listenEvents = UA_FDEVENT_IN;
    newConn->rfd.es = &pcm->cm.eventSource;
    newConn->rfd.eventSourceCB = (UA_FDCallback)TCP_listenSocketCallback;
    newConn->applicationCB = connectionCallback;
    newConn->application = application;
    newConn->context = context;

    /* Information to reopen listen socket */
    newConn->rfd.hostname = UA_String_fromChars(hostname);
    newConn->rfd.port = port;
    newConn->rfd.reuseaddr = reuseaddr;

    /* Register in the EventLoop */
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, &newConn->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error registering the socket",
                       (unsigned)listenSocket);
        UA_free(newConn);
        UA_EventLoopPOSIX_setReusable(listenSocket); /* Ensure reuse is possible */
        UA_close(listenSocket);
        return res;
    }

    /* Register internally */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &newConn->rfd);
    pcm->fdsSize++;

    /* Set up the callback parameters */
    UA_String listenAddress = UA_STRING((char*)(uintptr_t)hostname);
    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "listen-address");
    UA_Variant_setScalar(&params[0].value, &listenAddress, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen-port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap paramMap = {2, params};

    if(el->maxSocketsLimitReached) {
        /* Announce the reopening of the listen-socket in the application */
        connectionCallback(&pcm->cm, (uintptr_t)listenSocket,
                           application, &newConn->context,
                           UA_CONNECTIONSTATE_REOPENING,
                           &paramMap, UA_BYTESTRING_NULL);
        return UA_STATUSCODE_GOOD;
    }

    /* Announce the listen-socket in the application */
    connectionCallback(&pcm->cm, (uintptr_t)listenSocket,
                       application, &newConn->context,
                       UA_CONNECTIONSTATE_ESTABLISHED,
                       &paramMap, UA_BYTESTRING_NULL);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_registerListenSockets(UA_POSIXConnectionManager *pcm, const char *hostname,
                          UA_UInt16 port, void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback,
                          UA_Boolean validate, UA_Boolean reuseaddr) {
    UA_LOCK_ASSERT(&((UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop)->elMutex);

    /* Create a string for the port */
    char portstr[6];
    mp_snprintf(portstr, sizeof(portstr), "%d", port);

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

    UA_RESET_ERRNO;
    int retcode = UA_getaddrinfo(hostname, portstr, &hints, &res);
    if(retcode != 0) {
       UA_LOG_SOCKET_ERRNO_GAI_WRAP(
       UA_LOG_WARNING(pcm->cm.eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                      "TCP\t| Lookup for \"%s\" on port %u failed (%s)",
                      hostname, port, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add listen sockets. Aggregate the results to see if at least one
     * listen-socket was established. */
    UA_StatusCode total_result = UA_INT32_MAX;
    struct addrinfo *ai = res;
    while(ai) {
        total_result &= TCP_registerListenSocket(pcm, ai, hostname, port, application, context,
                                                 connectionCallback, validate, reuseaddr);
        ai = ai->ai_next;
    }
    UA_freeaddrinfo(res);

    return total_result;
}

/* Close the connection via a delayed callback */
static void
TCP_shutdown(UA_ConnectionManager *cm, TCP_FD *conn) {
    /* Already closing - nothing to do */
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    if(conn->rfd.dc.callback) {
        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP %u\t| Cannot close - already closing",
                     (unsigned)conn->rfd.fd);
        return;
    }

    /* Shutdown the socket to cancel the current select/epoll */
    UA_shutdown(conn->rfd.fd, UA_SHUT_RDWR);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP %u\t| Shutdown triggered",
                 (unsigned)conn->rfd.fd);

    /* Add to the delayed callback list. Will be cleaned up in the next
     * iteration. */
    UA_DelayedCallback *dc = &conn->rfd.dc;
    dc->callback = TCP_delayedClose;
    dc->application = cm;
    dc->context = conn;

    /* Adding a delayed callback does not take a lock */
    UA_EventLoopPOSIX_addDelayedCallback((UA_EventLoop*)el, dc);
}

static UA_StatusCode
TCP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);

    UA_FD fd = (UA_FD)connectionId;
    TCP_FD *conn = (TCP_FD*)ZIP_FIND(UA_FDTree, &pcm->fds, &fd);
    if(!conn) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Cannot close TCP connection %u - not found",
                       (unsigned)connectionId);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    TCP_shutdown(cm, conn);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       const UA_KeyValueMap *params, UA_ByteString *buf) {
    /* We may not have a lock. But we need not take it. As the connectionId is
     * the fd, no need to do a lookup and access internal data strucures. */

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
            UA_RESET_ERRNO;
            UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                         "TCP %u\t| Attempting to send", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send((UA_FD)connectionId,
                        (const char*)buf->data + nWritten,
                        bytes_to_send, flags);
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN)
                    goto shutdown;

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                do {
                    UA_RESET_ERRNO;
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED)
                        goto shutdown;
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Clean up and return */
    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
    return UA_STATUSCODE_GOOD;

 shutdown:
    /* Error -> shutdown the connection  */
    UA_LOG_SOCKET_ERRNO_WRAP(
       UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP %u\t| Send failed with error %s",
                    (unsigned)connectionId, errno_str));
    TCP_shutdownConnection(cm, connectionId);
    UA_EventLoopPOSIX_freeNetworkBuffer(cm, connectionId, buf);
    return UA_STATUSCODE_BADCONNECTIONCLOSED;
}

/* Create a listen-socket that waits for incoming connections */
static UA_StatusCode
TCP_openPassiveConnection(UA_POSIXConnectionManager *pcm, const UA_KeyValueMap *params,
                          void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback,
                          UA_Boolean validate) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    /* Get the port parameter */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, tcpConnectionParams[TCP_PARAMINDEX_PORT].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port); /* existence is checked before */

    /* Get the address parameter */
    const UA_Variant *addrs =
        UA_KeyValueMap_get(params, tcpConnectionParams[TCP_PARAMINDEX_ADDR].name);
    size_t addrsSize = 0;
    if(addrs) {
        UA_assert(addrs->type == &UA_TYPES[UA_TYPES_STRING]);
        if(UA_Variant_isScalar(addrs))
            addrsSize = 1;
        else
            addrsSize = addrs->arrayLength;
    }

    /* Get the reuseaddr parameter */
    UA_Boolean reuseaddr = false;
    const UA_Boolean *reuseaddrTmp = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, tcpConnectionParams[TCP_PARAMINDEX_REUSE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(reuseaddrTmp)
        reuseaddr = *reuseaddrTmp;

    /* Undefined or empty addresses array -> listen on all interfaces */
    UA_StatusCode retval = UA_STATUSCODE_BADINTERNALERROR;
    if(addrsSize == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "TCP\t| Listening on all interfaces");
        if(TCP_registerListenSockets(pcm, NULL, *port, application,
                                     context, connectionCallback, validate, reuseaddr) == UA_STATUSCODE_GOOD)
            retval = UA_STATUSCODE_GOOD;
        return retval;
    }

    /* Iterate over the configured hostnames */
    UA_String *hostStrings = (UA_String*)addrs->data;
    for(size_t i = 0; i < addrsSize; i++) {
        char hostname[512];
        if(hostStrings[i].length >= sizeof(hostname))
            continue;
        memcpy(hostname, hostStrings[i].data, hostStrings->length);
        hostname[hostStrings->length] = '\0';
        if(TCP_registerListenSockets(pcm, hostname, *port, application,
                                     context, connectionCallback, validate, reuseaddr) == UA_STATUSCODE_GOOD)
            retval = UA_STATUSCODE_GOOD;
    }
    return retval;
}

/* Open a TCP connection to a remote host */
static UA_StatusCode
TCP_openActiveConnection(UA_POSIXConnectionManager *pcm, const UA_KeyValueMap *params,
                         void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback,
                         UA_Boolean validate) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pcm->cm.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    /* Check that the maximum number of sockets has not been exceeded */
    const UA_UInt32 *maxSockets = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &pcm->cm.eventSource.params, UA_QUALIFIEDNAME(0, "max-connections"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(el->maxSocketsLimitReached || (maxSockets && *maxSockets != 0 && pcm->fdsSize >= (size_t)*maxSockets)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Unable to establish connection: no available sockets");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the connection parameters */
    char hostname[UA_MAXHOSTNAME_LENGTH];
    char portStr[UA_MAXPORTSTR_LENGTH];

    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, tcpConnectionParams[TCP_PARAMINDEX_PORT].name,
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_assert(port); /* existence is checked before */
    mp_snprintf(portStr, UA_MAXPORTSTR_LENGTH, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *addr = (const UA_String*)
        UA_KeyValueMap_getScalar(params, tcpConnectionParams[TCP_PARAMINDEX_ADDR].name,
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!addr) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Open TCP Connection: No hostname defined, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(addr->length >= UA_MAXHOSTNAME_LENGTH) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| Open TCP Connection: Hostname too long, aborting");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    strncpy(hostname, (const char*)addr->data, addr->length);
    hostname[addr->length] = 0;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "TCP\t| Open a connection to \"%s\" on port %s", hostname, portStr);

    /* Create the socket description from the connectString
     * TODO: Make this non-blocking */
    UA_RESET_ERRNO;
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = UA_getaddrinfo(hostname, portStr, &hints, &info);
    if(error != 0) {
       UA_LOG_SOCKET_ERRNO_GAI_WRAP(
       UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                      "TCP\t| Lookup of %s failed (%s)",
                      hostname, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a socket */
    UA_RESET_ERRNO;
    UA_FD newSock = UA_socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_FD) {
        UA_freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Could not create socket to connect to %s (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Set the socket options */
    UA_RESET_ERRNO;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_EventLoopPOSIX_setNonBlocking(newSock);
    res |= UA_EventLoopPOSIX_setNoSigPipe(newSock);
    res |= TCP_setNoNagle(newSock);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Could not set socket options: %s", errno_str));
        UA_freeaddrinfo(info);
        UA_close(newSock);
        return res;
    }

    /* Only validate, don't actually open the connection */
    if(validate) {
        UA_freeaddrinfo(info);
        UA_close(newSock);
        return UA_STATUSCODE_GOOD;
    }

    /* Non-blocking connect */
    UA_RESET_ERRNO;
    error = UA_connect(newSock, info->ai_addr, info->ai_addrlen);
    UA_freeaddrinfo(info);
    if(error != 0 &&
       UA_ERRNO != UA_INPROGRESS &&
       UA_ERRNO != UA_WOULDBLOCK) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "TCP\t| Connecting the socket to %s failed (%s)",
                           hostname, errno_str));
        UA_close(newSock);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Allocate the UA_RegisteredFD */
    TCP_FD *newConn = (TCP_FD*)UA_calloc(1, sizeof(TCP_FD));
    if(!newConn) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP %u\t| Error allocating memory for the socket",
                       (unsigned)newSock);
        UA_close(newSock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newConn->rfd.fd = newSock;
    newConn->rfd.es = &pcm->cm.eventSource;
    newConn->rfd.eventSourceCB = (UA_FDCallback)TCP_connectionSocketCallback;
    newConn->rfd.listenEvents = UA_FDEVENT_OUT; /* Switched to _IN once the
                                                 * connection is open */
    newConn->applicationCB = connectionCallback;
    newConn->application = application;
    newConn->context = context;

    /* Register the fd to trigger when output is possible (the connection is open) */
    res = UA_EventLoopPOSIX_registerFD(el, &newConn->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "TCP\t| Registering the socket to connect to %s failed", hostname);
        UA_close(newSock);
        UA_free(newConn);
        return res;
    }

    /* Register internally in the EventSource */
    ZIP_INSERT(UA_FDTree, &pcm->fds, &newConn->rfd);
    pcm->fdsSize++;

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "TCP %u\t| Opening a connection to \"%s\" on port %s",
                (unsigned)newSock, hostname, portStr);

    /* Signal the new connection to the application as asynchonously opening */
    connectionCallback(&pcm->cm, (uintptr_t)newSock,
                       application, &newConn->context,
                       UA_CONNECTIONSTATE_OPENING, &UA_KEYVALUEMAP_NULL,
                       UA_BYTESTRING_NULL);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    UA_LOCK(&el->elMutex);

    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| Cannot open a connection for a "
                     "ConnectionManager that is not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "TCP",
                                        tcpConnectionParams,
                                        TCP_PARAMETERSSIZE, params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return res;
    }

    /* Only validate the parameters? */
    UA_Boolean validate = false;
    const UA_Boolean *validateParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 tcpConnectionParams[TCP_PARAMINDEX_VALIDATE].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validateParam)
        validate = *validateParam;

    /* Listen or active connection? */
    UA_Boolean listen = false;
    const UA_Boolean *listenParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params,
                                 tcpConnectionParams[TCP_PARAMINDEX_LISTEN].name,
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(listenParam)
        listen = *listenParam;

    if(listen) {
        res = TCP_openPassiveConnection(pcm, params, application, context,
                                        connectionCallback, validate);
    } else {
        res = TCP_openActiveConnection(pcm, params, application, context,
                                       connectionCallback, validate);
    }

    UA_UNLOCK(&el->elMutex);
    return res;
}

static UA_StatusCode
TCP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    if(!el)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&el->elMutex);

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "TCP\t| To start the ConnectionManager, it has to be "
                     "registered in an EventLoop and not started yet");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the parameters */
    UA_StatusCode res =
        UA_KeyValueRestriction_validate(el->eventLoop.logger, "TCP",
                                        tcpManagerParams, TCP_MANAGERPARAMS,
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
TCP_shutdownCB(void *application, UA_RegisteredFD *rfd) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)application;
    TCP_shutdown(cm, (TCP_FD*)rfd);
    return NULL;
}

static void
TCP_eventSourceStop(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;
    (void)el;

    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                 "TCP\t| Shutting down the ConnectionManager");

    /* Prevent new connections to open */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shutdown all existing connection */
    ZIP_ITER(UA_FDTree, &pcm->fds, TCP_shutdownCB, cm);

    /* All sockets closed? Otherwise iterate some more. */
    TCP_checkStopped(pcm);

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
TCP_eventSourceDelete(UA_ConnectionManager *cm) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "TCP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ByteString_clear(&pcm->rxBuffer);
    UA_ByteString_clear(&pcm->txBuffer);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);

    return UA_STATUSCODE_GOOD;
}

static const char *tcpName = "tcp";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_TCP(const UA_String eventSourceName) {
    UA_POSIXConnectionManager *cm = (UA_POSIXConnectionManager*)
        UA_calloc(1, sizeof(UA_POSIXConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *))TCP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))TCP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))TCP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)tcpName);
    cm->cm.openConnection = TCP_openConnection;
    cm->cm.allocNetworkBuffer = UA_EventLoopPOSIX_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = UA_EventLoopPOSIX_freeNetworkBuffer;
    cm->cm.sendWithConnection = TCP_sendWithConnection;
    cm->cm.closeConnection = TCP_shutdownConnection;
    return &cm->cm;
}

#endif
