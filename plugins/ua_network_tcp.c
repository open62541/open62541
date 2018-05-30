/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Jose Cabral
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

/* Disable some security warnings on MSVC */
#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

/* Assume that Windows versions are newer than Windows XP */
#if defined(__MINGW32__) && (!defined(WINVER) || WINVER < 0x501)
# undef WINVER
# undef _WIN32_WINDOWS
# undef _WIN32_WINNT
# define WINVER 0x0501
# define _WIN32_WINDOWS 0x0501
# define _WIN32_WINNT 0x0501
#endif

#include "ua_network_tcp.h"
#include "ua_log_stdout.h"
#include "../deps/queue.h"

#include <stdio.h> // snprintf
#include <string.h> // memset

#if !defined(UA_FREERTOS)
# include <errno.h>
#else
# define AI_PASSIVE 0x01
# define TRUE 1
# define FALSE 0
# define ioctl ioctlsocket
#endif

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket((SOCKET)S)
# define ssize_t int
# define WIN32_INT (int)
# define OPTVAL_TYPE char
# define ERR_CONNECTION_PROGRESS WSAEWOULDBLOCK
#else /* _WIN32 */
# if defined(UA_FREERTOS)
#  define UA_FREERTOS_HOSTNAME "10.200.4.114"
static inline int gethostname_freertos(char* name, size_t len){
  if(strlen(UA_FREERTOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_FREERTOS_HOSTNAME);
  return 0;
}
#define gethostname gethostname_freertos
#  include <lwip/tcpip.h>
#  include <lwip/netdb.h>
#  define CLOSESOCKET(S) lwip_close(S)
#  define sockaddr_storage sockaddr
#  ifdef BYTE_ORDER
#   undef BYTE_ORDER
#  endif
# else /* Not freeRTOS */
#  define CLOSESOCKET(S) close(S)
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <sys/ioctl.h>
#  if defined(_WRS_KERNEL)
#   include <hostLib.h>
#   include <selectLib.h>
#  else /* defined(_WRS_KERNEL) */
#   include <sys/select.h>
#  endif /* defined(_WRS_KERNEL) */
# endif /* Not freeRTOS */

# define SOCKET int
# define WIN32_INT
# define OPTVAL_TYPE int
# define ERR_CONNECTION_PROGRESS EINPROGRESS


# include <fcntl.h>
# include <unistd.h> // read, write, close

# ifdef __QNX__
#  include <sys/socket.h>
# endif
# if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#  include <sys/param.h>
#  if defined(BSD)
#   include<sys/socket.h>
#  endif
# endif
# if !defined(__CYGWIN__) && !defined(UA_FREERTOS)
#  include <netinet/tcp.h>
# endif
#endif /* _WIN32 */

#ifndef UA_sleep_ms
# ifdef _WIN32
#  define UA_sleep_ms(X) Sleep(X)
# else /* _WIN32 */
#  if defined(UA_FREERTOS)
#   define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))
#  else /* Not freeRTOS */
#   if defined(_WRS_KERNEL)
#    include <hostLib.h>
#    include <selectLib.h>
#    define UA_sleep_ms(X)                            \
     {                                                \
     struct timespec timeToSleep;                     \
       timeToSleep.tv_sec = X / 1000;                 \
       timeToSleep.tv_nsec = 1000000 * (X % 1000);    \
       nanosleep(&timeToSleep, NULL);                 \
     }
#   else /* defined(_WRS_KERNEL) */
#    define UA_sleep_ms(X) usleep(X * 1000)
#   endif /* defined(_WRS_KERNEL) */
#  endif /* Not freeRTOS */
# endif /* _WIN32 */
#else /* UA_sleep_ms */
/* With this one can define its own UA_sleep_ms using a preprocessor define.
E.g. see unit tests. */
void UA_sleep_ms(size_t);
#endif

/* unsigned int for windows and workaround to a glibc bug */
/* Additionally if GNU_LIBRARY is not defined, it may be using
 * musl libc (e.g. Docker Alpine) */
#if defined(_WIN32) || defined(__OpenBSD__) || \
    (defined(__GNU_LIBRARY__) && (__GNU_LIBRARY__ <= 6) && \
     (__GLIBC__ <= 2) && (__GLIBC_MINOR__ < 16) || \
    !defined(__GNU_LIBRARY__))
# define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)
#else
# define UA_fd_set(fd, fds) FD_SET(fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET(fd, fds)
#endif

#ifdef UNDER_CE
# define errno WSAGetLastError()
#endif

#ifdef _WIN32
# define errno__ WSAGetLastError()
# define INTERRUPTED WSAEINTR
# define WOULDBLOCK WSAEWOULDBLOCK
# define AGAIN WSAEWOULDBLOCK
#else
# define errno__ errno
# define INTERRUPTED EINTR
# define WOULDBLOCK EWOULDBLOCK
# define AGAIN EAGAIN
#endif

#include "ua_log_socket_error.h"

/****************************/
/* Generic Socket Functions */
/****************************/

static UA_StatusCode
connection_getsendbuffer(UA_Connection *connection,
                         size_t length, UA_ByteString *buf) {
    if(length > connection->remoteConf.recvBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static void
connection_releasesendbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static void
connection_releaserecvbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static UA_StatusCode
connection_write(UA_Connection *connection, UA_ByteString *buf) {
    if(connection->state == UA_CONNECTION_CLOSED) {
        UA_ByteString_deleteMembers(buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Prevent OS signals when sending to a closed socket */
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            size_t bytes_to_send = buf->length - nWritten;
            n = send((SOCKET)connection->sockfd,
                     (const char*)buf->data + nWritten,
                     WIN32_INT bytes_to_send, flags);
            if(n < 0 && errno__ != INTERRUPTED && errno__ != AGAIN) {
                connection->close(connection);
                UA_ByteString_deleteMembers(buf);
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
connection_recv(UA_Connection *connection, UA_ByteString *response,
                UA_UInt32 timeout) {
    if(connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Listen on the socket for the given timeout until a message arrives */
    if(timeout > 0) {
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(connection->sockfd, &fdset);
        UA_UInt32 timeout_usec = timeout * 1000;
        struct timeval tmptv = {(long int)(timeout_usec / 1000000),
                                (long int)(timeout_usec % 1000000)};
        int resultsize = select(connection->sockfd+1, &fdset, NULL,
                                NULL, &tmptv);

        /* No result */
        if(resultsize == 0)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        if(resultsize == -1) {
            /* The call to select was interrupted manually. Act as if it timed
             * out */
            if(errno == EINTR)
                return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

            /* The error cannot be recovered. Close the connection. */
            connection->close(connection);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    response->data = (UA_Byte*)
        UA_malloc(connection->localConf.recvBufferSize);
    if(!response->data) {
        response->length = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY; /* not enough memory retry */
    }

    /* Get the received packet(s) */
    ssize_t ret = recv(connection->sockfd, (char*)response->data,
                       connection->localConf.recvBufferSize, 0);

    /* The remote side closed the connection */
    if(ret == 0) {
        UA_ByteString_deleteMembers(response);
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Error case */
    if(ret < 0) {
        UA_ByteString_deleteMembers(response);
        if(errno__ == INTERRUPTED || (timeout > 0) ?
           false : (errno__ == EAGAIN || errno__ == WOULDBLOCK))
            return UA_STATUSCODE_GOOD; /* statuscode_good but no data -> retry */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Set the length of the received buffer */
    response->length = (size_t)ret;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
socket_set_nonblocking(SOCKET sockfd) {
#ifdef _WIN32
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#elif defined(_WRS_KERNEL) || defined(UA_FREERTOS)
    int on = TRUE;
    if(ioctl(sockfd, FIONBIO, &on) < 0)
      return UA_STATUSCODE_BADINTERNALERROR;
#else
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts|O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
socket_set_blocking(SOCKET sockfd) {
#ifdef _WIN32
    u_long iMode = 0;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#elif defined(_WRS_KERNEL) || defined(UA_FREERTOS)
    int on = FALSE;
    if(ioctl(sockfd, FIONBIO, &on) < 0)
      return UA_STATUSCODE_BADINTERNALERROR;
#else
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts & (~O_NONBLOCK)) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/***************************/
/* Server NetworkLayer TCP */
/***************************/

#define MAXBACKLOG     100
#define NOHELLOTIMEOUT 120000 /* timeout in ms before close the connection
                               * if server does not receive Hello Message */

typedef struct ConnectionEntry {
    UA_Connection connection;
    LIST_ENTRY(ConnectionEntry) pointers;
} ConnectionEntry;

typedef struct {
    UA_Logger logger;
    UA_ConnectionConfig conf;
    UA_UInt16 port;
    UA_Int32 serverSockets[FD_SETSIZE];
    UA_UInt16 serverSocketsSize;
    LIST_HEAD(, ConnectionEntry) connections;
} ServerNetworkLayerTCP;

static void
ServerNetworkLayerTCP_freeConnection(UA_Connection *connection) {
    UA_Connection_deleteMembers(connection);
    UA_free(connection);
}

/* This performs only 'shutdown'. 'close' is called when the shutdown
 * socket is returned from select. */
static void
ServerNetworkLayerTCP_close(UA_Connection *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;
    shutdown((SOCKET)connection->sockfd, 2);
    connection->state = UA_CONNECTION_CLOSED;
}

static UA_StatusCode
ServerNetworkLayerTCP_add(ServerNetworkLayerTCP *layer, UA_Int32 newsockfd,
                          struct sockaddr_storage *remote) {
    /* Set nonblocking */
    socket_set_nonblocking(newsockfd);

    /* Do not merge packets on the socket (disable Nagle's algorithm) */
    int dummy = 1;
    if(setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
               (const char *)&dummy, sizeof(dummy)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK,
                             "Cannot set socket option TCP_NODELAY. Error: %s",
                             errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

#if !defined(UA_FREERTOS)
    /* Get the peer name for logging */
    char remote_name[100];
    int res = getnameinfo((struct sockaddr*)remote,
                          sizeof(struct sockaddr_storage),
                          remote_name, sizeof(remote_name),
                          NULL, 0, NI_NUMERICHOST);
    if(res == 0) {
        UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | New connection over TCP from %s",
                    (int)newsockfd, remote_name);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                                                "Connection %i | New connection over TCP, "
                                                        "getnameinfo failed with error: %s",
                                                (int)newsockfd, errno_str));
    }
#endif
    /* Allocate and initialize the connection */
    ConnectionEntry *e = (ConnectionEntry*)UA_malloc(sizeof(ConnectionEntry));
    if(!e){
        CLOSESOCKET(newsockfd);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Connection *c = &e->connection;
    memset(c, 0, sizeof(UA_Connection));
    c->sockfd = newsockfd;
    c->handle = layer;
    c->localConf = layer->conf;
    c->remoteConf = layer->conf;
    c->send = connection_write;
    c->close = ServerNetworkLayerTCP_close;
    c->free = ServerNetworkLayerTCP_freeConnection;
    c->getSendBuffer = connection_getsendbuffer;
    c->releaseSendBuffer = connection_releasesendbuffer;
    c->releaseRecvBuffer = connection_releaserecvbuffer;
    c->state = UA_CONNECTION_OPENING;
    c->openingDate = UA_DateTime_nowMonotonic();

    /* Add to the linked list */
    LIST_INSERT_HEAD(&layer->connections, e, pointers);
    return UA_STATUSCODE_GOOD;
}

static void
addServerSocket(ServerNetworkLayerTCP *layer, struct addrinfo *ai) {
    /* Create the server socket */
    SOCKET newsock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
#ifdef _WIN32
    if(newsock == INVALID_SOCKET)
#else
    if(newsock < 0)
#endif
    {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Error opening the server socket");
        return;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */

    int optval = 1;
#if !defined(UA_FREERTOS)
    if(ai->ai_family == AF_INET6 &&
       setsockopt(newsock, IPPROTO_IPV6, IPV6_V6ONLY,
                  (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set an IPv6 socket to IPv6 only");
        CLOSESOCKET(newsock);
        return;
    }
#endif
    if(setsockopt(newsock, SOL_SOCKET, SO_REUSEADDR,
                  (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not make the socket reusable");
        CLOSESOCKET(newsock);
        return;
    }


    if(socket_set_nonblocking(newsock) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the server socket to nonblocking");
        CLOSESOCKET(newsock);
        return;
    }

    /* Bind socket to address */
    if(bind(newsock, ai->ai_addr, WIN32_INT ai->ai_addrlen) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                           "Error binding a server socket: %s", errno_str));
        CLOSESOCKET(newsock);
        return;
    }

    /* Start listening */
    if(listen(newsock, MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Error listening on server socket: %s", errno_str));
        CLOSESOCKET(newsock);
        return;
    }

    layer->serverSockets[layer->serverSocketsSize] = (UA_Int32)newsock;
    layer->serverSocketsSize++;
}

static UA_StatusCode
ServerNetworkLayerTCP_start(UA_ServerNetworkLayer *nl, const UA_String *customHostname) {
#ifdef _WIN32
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(wVersionRequested, &wsaData);
#endif

    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    if (customHostname->length) {
        char discoveryUrl[256];
#ifndef _MSC_VER
        du.length = (size_t)snprintf(discoveryUrl, 255, "opc.tcp://%.*s:%d/",
                                     (int)customHostname->length,
                                     customHostname->data,
                                     layer->port);
#else
        du.length = (size_t)_snprintf_s(discoveryUrl, 255, _TRUNCATE,
                                        "opc.tcp://%.*s:%d/",
                                        (int)customHostname->length,
                                        customHostname->data,
                                        layer->port);
#endif
        du.data = (UA_Byte*)discoveryUrl;
    }else{
        char hostname[256];
        if(gethostname(hostname, 255) == 0) {
            char discoveryUrl[256];
#ifndef _MSC_VER
            du.length = (size_t)snprintf(discoveryUrl, 255, "opc.tcp://%s:%d/",
                                         hostname, layer->port);
#else
            du.length = (size_t)_snprintf_s(discoveryUrl, 255, _TRUNCATE,
                                            "opc.tcp://%s:%d/", hostname,
                                            layer->port);
#endif
            du.data = (UA_Byte*)discoveryUrl;
        }
    }
    UA_String_copy(&du, &nl->discoveryUrl);

    /* Get addrinfo of the server and create server sockets */
    char portno[6];
#ifndef _MSC_VER
    snprintf(portno, 6, "%d", layer->port);
#else
    _snprintf_s(portno, 6, _TRUNCATE, "%d", layer->port);
#endif
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
#if defined(UA_FREERTOS)
    hints.ai_protocol = IPPROTO_TCP;
    char hostname[] = UA_FREERTOS_HOSTNAME;
    if(getaddrinfo(hostname, portno, &hints, &res) != 0)
#else
    if(getaddrinfo(NULL, portno, &hints, &res) != 0)
#endif
        return UA_STATUSCODE_BADINTERNALERROR;

    /* There might be serveral addrinfos (for different network cards,
     * IPv4/IPv6). Add a server socket for all of them. */
    struct addrinfo *ai = res;
    for(layer->serverSocketsSize = 0;
        layer->serverSocketsSize < FD_SETSIZE && ai != NULL;
        ai = ai->ai_next)
        addServerSocket(layer, ai);
    freeaddrinfo(res);

    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "TCP network layer listening on %.*s",
                (int)nl->discoveryUrl.length, nl->discoveryUrl.data);
    return UA_STATUSCODE_GOOD;
}

/* After every select, reset the sockets to listen on */
static UA_Int32
setFDSet(ServerNetworkLayerTCP *layer, fd_set *fdset) {
    FD_ZERO(fdset);
    UA_Int32 highestfd = 0;
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        UA_fd_set(layer->serverSockets[i], fdset);
        if(layer->serverSockets[i] > highestfd)
            highestfd = layer->serverSockets[i];
    }

    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers) {
        UA_fd_set(e->connection.sockfd, fdset);
        if(e->connection.sockfd > highestfd)
            highestfd = e->connection.sockfd;
    }

    return highestfd;
}

static UA_StatusCode
ServerNetworkLayerTCP_listen(UA_ServerNetworkLayer *nl, UA_Server *server,
                             UA_UInt16 timeout) {
    /* Every open socket can generate two jobs */
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;

    if (layer->serverSocketsSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Listen on open sockets (including the server) */
    fd_set fdset, errset;
    UA_Int32 highestfd = setFDSet(layer, &fdset);
    setFDSet(layer, &errset);
    struct timeval tmptv = {0, timeout * 1000};
    if (select(highestfd+1, &fdset, NULL, &errset, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                                  "Socket select failed with %s", errno_str));
        // we will retry, so do not return bad
        return UA_STATUSCODE_GOOD;
    }

    /* Accept new connections via the server sockets */
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        if(!UA_fd_isset(layer->serverSockets[i], &fdset))
            continue;

        struct sockaddr_storage remote;
        socklen_t remote_size = sizeof(remote);
        SOCKET newsockfd = accept((SOCKET)layer->serverSockets[i],
                                  (struct sockaddr*)&remote, &remote_size);
#ifdef _WIN32
        if(newsockfd == INVALID_SOCKET)
#else
        if(newsockfd < 0)
#endif
            continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | New TCP connection on server socket %i",
                    (int)newsockfd, layer->serverSockets[i]);

        ServerNetworkLayerTCP_add(layer, (UA_Int32)newsockfd, &remote);
    }

    /* Read from established sockets */
    ConnectionEntry *e, *e_tmp;
    UA_DateTime now = UA_DateTime_nowMonotonic();
    LIST_FOREACH_SAFE(e, &layer->connections, pointers, e_tmp) {
        if ((e->connection.state == UA_CONNECTION_OPENING) &&
            (now > (e->connection.openingDate + (NOHELLOTIMEOUT * UA_DATETIME_MSEC)))){
            UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Closed by the server (no Hello Message)",
                         e->connection.sockfd);
            LIST_REMOVE(e, pointers);
            CLOSESOCKET(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
            continue;
        }

        if(!UA_fd_isset(e->connection.sockfd, &errset) &&
           !UA_fd_isset(e->connection.sockfd, &fdset))
          continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Activity on the socket",
                    e->connection.sockfd);

        UA_ByteString buf = UA_BYTESTRING_NULL;
        UA_StatusCode retval = connection_recv(&e->connection, &buf, 0);

        if(retval == UA_STATUSCODE_GOOD) {
            /* Process packets */
            UA_Server_processBinaryMessage(server, &e->connection, &buf);
            connection_releaserecvbuffer(&e->connection, &buf);
        } else if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            /* The socket is shutdown but not closed */
            UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Closed",
                        e->connection.sockfd);
            LIST_REMOVE(e, pointers);
            CLOSESOCKET(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerTCP_stop(UA_ServerNetworkLayer *nl, UA_Server *server) {
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the TCP network layer");

    /* Close the server sockets */
    for(UA_UInt16 i = 0; i < layer->serverSocketsSize; i++) {
        shutdown((SOCKET)layer->serverSockets[i], 2);
        CLOSESOCKET(layer->serverSockets[i]);
    }
    layer->serverSocketsSize = 0;

    /* Close open connections */
    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers)
        ServerNetworkLayerTCP_close(&e->connection);

    /* Run recv on client sockets. This picks up the closed sockets and frees
     * the connection. */
    ServerNetworkLayerTCP_listen(nl, server, 0);

#ifdef _WIN32
    WSACleanup();
#endif
}

/* run only when the server is stopped */
static void
ServerNetworkLayerTCP_deleteMembers(UA_ServerNetworkLayer *nl) {
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;
    UA_String_deleteMembers(&nl->discoveryUrl);

    /* Hard-close and remove remaining connections. The server is no longer
     * running. So this is safe. */
    ConnectionEntry *e, *e_tmp;
    LIST_FOREACH_SAFE(e, &layer->connections, pointers, e_tmp) {
        LIST_REMOVE(e, pointers);
        CLOSESOCKET(e->connection.sockfd);
        UA_free(e);
    }

    /* Free the layer */
    UA_free(layer);
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerTCP(UA_ConnectionConfig conf, UA_UInt16 port, UA_Logger logger) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP*)
        UA_calloc(1,sizeof(ServerNetworkLayerTCP));
    if(!layer)
        return nl;

    layer->logger = (logger != NULL ? logger : UA_Log_Stdout);
    layer->conf = conf;
    layer->port = port;

    nl.handle = layer;
    nl.start = ServerNetworkLayerTCP_start;
    nl.listen = ServerNetworkLayerTCP_listen;
    nl.stop = ServerNetworkLayerTCP_stop;
    nl.deleteMembers = ServerNetworkLayerTCP_deleteMembers;
    return nl;
}

typedef struct TCPClientConnection {
	struct addrinfo hints, *server;
	UA_DateTime connStart;
	char* endpointURL;
	UA_UInt32 timeout;
} TCPClientConnection;

/***************************/
/* Client NetworkLayer TCP */
/***************************/

static void
ClientNetworkLayerTCP_close(UA_Connection *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;
    shutdown((SOCKET)connection->sockfd, 2);
    CLOSESOCKET(connection->sockfd);
    connection->state = UA_CONNECTION_CLOSED;
}

static void
ClientNetworkLayerTCP_free(UA_Connection *connection) {
    if (connection->handle){
        TCPClientConnection *tcpConnection = (TCPClientConnection *)connection->handle;
        if(tcpConnection->server)
            freeaddrinfo(tcpConnection->server);
        free(tcpConnection);
    }
}

UA_StatusCode UA_ClientConnectionTCP_poll(UA_Client *client, void *data) {
    UA_Connection *connection = (UA_Connection*) data;

    if (connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADDISCONNECT;

    TCPClientConnection *tcpConnection =
                    (TCPClientConnection*) connection->handle;

    UA_DateTime connStart = UA_DateTime_nowMonotonic();
    SOCKET clientsockfd;

    if (connection->state == UA_CONNECTION_ESTABLISHED) {
            UA_Client_removeRepeatedCallback(client, connection->connectCallbackID);
            connection->connectCallbackID = 0;
            return UA_STATUSCODE_GOOD;
    }
    if ((UA_Double) (UA_DateTime_nowMonotonic() - tcpConnection->connStart)
                    > tcpConnection->timeout* UA_DATETIME_MSEC ) {
            // connection timeout
            ClientNetworkLayerTCP_close(connection);
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                            "Timed out");
            return UA_STATUSCODE_BADDISCONNECT;

    }
    /* On linux connect may immediately return with ECONNREFUSED but we still want to try to connect */
    /* Thus use a loop and retry until timeout is reached */

    /* Get a socket */
    clientsockfd = socket(tcpConnection->server->ai_family,
                    tcpConnection->server->ai_socktype,
                    tcpConnection->server->ai_protocol);
    connection->sockfd = (UA_Int32) clientsockfd; /* cast for win32 */

#ifdef _WIN32
    if(clientsockfd == INVALID_SOCKET) {
#else
    if (clientsockfd < 0) {
#endif
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                            "Could not create client socket: %s", strerror(errno__));
            ClientNetworkLayerTCP_close(connection);
            return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Non blocking connect to be able to timeout */
    if (socket_set_nonblocking(clientsockfd) != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                            "Could not set the client socket to nonblocking");
            ClientNetworkLayerTCP_close(connection);
            return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Non blocking connect */
    int error = connect(clientsockfd, tcpConnection->server->ai_addr,
                    WIN32_INT tcpConnection->server->ai_addrlen);

    if ((error == -1) && (errno__ != ERR_CONNECTION_PROGRESS)) {
            ClientNetworkLayerTCP_close(connection);
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                            "Connection to  failed with error: %s", strerror(errno__));
            return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Use select to wait and check if connected */
    if (error == -1 && (errno__ == ERR_CONNECTION_PROGRESS)) {
        /* connection in progress. Wait until connected using select */

        UA_UInt32 timeSinceStart =
                        (UA_UInt32) ((UA_Double) (UA_DateTime_nowMonotonic() - connStart)
                                        * UA_DATETIME_MSEC);

        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(clientsockfd, &fdset);
        UA_UInt32 timeout_usec = (tcpConnection->timeout - timeSinceStart)
                        * 1000;
        struct timeval tmptv = { (long int) (timeout_usec / 1000000),
                        (long int) (timeout_usec % 1000000) };

        int resultsize = select((UA_Int32) (clientsockfd + 1), NULL, &fdset,
        NULL, &tmptv);

        if (resultsize == 1) {
            /* Windows does not have any getsockopt equivalent and it is not needed there */
#ifdef _WIN32
            connection->sockfd = clientsockfd;
            connection->state = UA_CONNECTION_ESTABLISHED;
            return UA_STATUSCODE_GOOD;
#else
            OPTVAL_TYPE so_error;
            socklen_t len = sizeof so_error;

            int ret = getsockopt(clientsockfd, SOL_SOCKET, SO_ERROR, &so_error,
                            &len);

            if (ret != 0 || so_error != 0) {
                /* on connection refused we should still try to connect */
                /* connection refused happens on localhost or local ip without timeout */
                if (so_error != ECONNREFUSED) {
                        // general error
                        ClientNetworkLayerTCP_close(connection);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                                        "Connection to failed with error: %s",
                                        strerror(ret == 0 ? so_error : errno__));
                        return UA_STATUSCODE_BADDISCONNECT;
                }
                /* wait until we try a again. Do not make this too small, otherwise the
                 * timeout is somehow wrong */

        } else {
                connection->state = UA_CONNECTION_ESTABLISHED;
                return UA_STATUSCODE_GOOD;
            }
#endif
        }
    } else {
        connection->state = UA_CONNECTION_ESTABLISHED;
        return UA_STATUSCODE_GOOD;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    int sso_result = setsockopt(connection->sockfd, SOL_SOCKET,
                    SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if(sso_result < 0)
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                    "Couldn't set SO_NOSIGPIPE");
#endif

    return UA_STATUSCODE_GOOD;

}

UA_Connection UA_ClientConnectionTCP_init(UA_ConnectionConfig conf,
		const char *endpointUrl, const UA_UInt32 timeout,
                UA_Logger logger) {
    UA_Connection connection;
    memset(&connection, 0, sizeof(UA_Connection));

    connection.state = UA_CONNECTION_OPENING;
    connection.localConf = conf;
    connection.remoteConf = conf;
    connection.send = connection_write;
    connection.recv = connection_recv;
    connection.close = ClientNetworkLayerTCP_close;
    connection.free = ClientNetworkLayerTCP_free;
    connection.getSendBuffer = connection_getsendbuffer;
    connection.releaseSendBuffer = connection_releasesendbuffer;
    connection.releaseRecvBuffer = connection_releaserecvbuffer;

    TCPClientConnection *tcpClientConnection = (TCPClientConnection*) malloc(
                    sizeof(TCPClientConnection));
    connection.handle = (void*) tcpClientConnection;
    tcpClientConnection->timeout = timeout;
    UA_String endpointUrlString = UA_STRING((char*) (uintptr_t) endpointUrl);
    UA_String hostnameString = UA_STRING_NULL;
    UA_String pathString = UA_STRING_NULL;
    UA_UInt16 port = 0;
    char hostname[512];
    tcpClientConnection->connStart = UA_DateTime_nowMonotonic();

    UA_StatusCode parse_retval = UA_parseEndpointUrl(&endpointUrlString,
                    &hostnameString, &port, &pathString);
    if (parse_retval != UA_STATUSCODE_GOOD || hostnameString.length > 511) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                            "Server url is invalid: %s", endpointUrl);
            return connection;
    }
    memcpy(hostname, hostnameString.data, hostnameString.length);
    hostname[hostnameString.length] = 0;

    if (port == 0) {
            port = 4840;
            UA_LOG_INFO(logger, UA_LOGCATEGORY_NETWORK,
                            "No port defined, using default port %d", port);
    }

    memset(&tcpClientConnection->hints, 0, sizeof(tcpClientConnection->hints));
    tcpClientConnection->hints.ai_family = AF_UNSPEC;
    tcpClientConnection->hints.ai_socktype = SOCK_STREAM;
    char portStr[6];
#ifndef _MSC_VER
    snprintf(portStr, 6, "%d", port);
#else
    _snprintf_s(portStr, 6, _TRUNCATE, "%d", port);
#endif
    int error = getaddrinfo(hostname, portStr, &tcpClientConnection->hints,
                    &tcpClientConnection->server);
    if (error != 0 || !tcpClientConnection->server) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                            "DNS lookup of %s failed with error %s", hostname,
                            gai_strerror(error));
            return connection;
    }
    return connection;
}

UA_Connection
UA_ClientConnectionTCP(UA_ConnectionConfig conf,
                       const char *endpointUrl, const UA_UInt32 timeout,
                       UA_Logger logger) {
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
#endif

    if(logger == NULL) {
        logger = UA_Log_Stdout;
    }

    UA_Connection connection;
    memset(&connection, 0, sizeof(UA_Connection));
    connection.state = UA_CONNECTION_CLOSED;
    connection.localConf = conf;
    connection.remoteConf = conf;
    connection.send = connection_write;
    connection.recv = connection_recv;
    connection.close = ClientNetworkLayerTCP_close;
    connection.free = ClientNetworkLayerTCP_free;
    connection.getSendBuffer = connection_getsendbuffer;
    connection.releaseSendBuffer = connection_releasesendbuffer;
    connection.releaseRecvBuffer = connection_releaserecvbuffer;
    connection.handle = NULL;

    UA_String endpointUrlString = UA_STRING((char*)(uintptr_t)endpointUrl);
    UA_String hostnameString = UA_STRING_NULL;
    UA_String pathString = UA_STRING_NULL;
    UA_UInt16 port = 0;
    char hostname[512];

    UA_StatusCode parse_retval =
        UA_parseEndpointUrl(&endpointUrlString, &hostnameString,
                            &port, &pathString);
    if(parse_retval != UA_STATUSCODE_GOOD || hostnameString.length > 511) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Server url is invalid: %s", endpointUrl);
        return connection;
    }
    memcpy(hostname, hostnameString.data, hostnameString.length);
    hostname[hostnameString.length] = 0;

    if(port == 0) {
        port = 4840;
        UA_LOG_INFO(logger, UA_LOGCATEGORY_NETWORK,
                    "No port defined, using default port %d", port);
    }

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#if defined(UA_FREERTOS)
    hints.ai_protocol = IPPROTO_TCP;
#endif
    char portStr[6];
#ifndef _MSC_VER
    snprintf(portStr, 6, "%d", port);
#else
    _snprintf_s(portStr, 6, _TRUNCATE, "%d", port);
#endif
    int error = getaddrinfo(hostname, portStr, &hints, &server);
    if(error != 0 || !server) {
#if !defined(UA_FREERTOS)
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,

                       "DNS lookup of %s failed with error %s",
                       hostname, gai_strerror(error));
#else
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                        "DNS lookup of %s failed with error",
                        hostname);
#endif
        return connection;
    }

    UA_Boolean connected = UA_FALSE;
    UA_DateTime dtTimeout = timeout * UA_DATETIME_MSEC;
    UA_DateTime connStart = UA_DateTime_nowMonotonic();
    SOCKET clientsockfd;

    /* On linux connect may immediately return with ECONNREFUSED but we still
     * want to try to connect. So use a loop and retry until timeout is
     * reached. */
    do {
        /* Get a socket */
        clientsockfd = socket(server->ai_family,
                              server->ai_socktype,
                              server->ai_protocol);
    #ifdef _WIN32
        if(clientsockfd == INVALID_SOCKET) {
    #else
        if(clientsockfd < 0) {
    #endif
            UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                                    "Could not create client socket: %s", errno_str));
            freeaddrinfo(server);
            return connection;
        }

        connection.state = UA_CONNECTION_OPENING;

        /* Connect to the server */
        connection.sockfd = (UA_Int32) clientsockfd; /* cast for win32 */

        /* Non blocking connect to be able to timeout */
        if (socket_set_nonblocking(clientsockfd) != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Could not set the client socket to nonblocking");
            ClientNetworkLayerTCP_close(&connection);
            freeaddrinfo(server);
            return connection;
        }

        /* Non blocking connect */
        error = connect(clientsockfd, server->ai_addr, WIN32_INT server->ai_addrlen);

        if ((error == -1) && (errno__ != ERR_CONNECTION_PROGRESS)) {
            ClientNetworkLayerTCP_close(&connection);
            UA_LOG_SOCKET_ERRNO_WRAP(
                    UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                   "Connection to %s failed with error: %s",
                                   endpointUrl, errno_str));
            freeaddrinfo(server);
            return connection;
        }

        /* Use select to wait and check if connected */
        if (error == -1 && (errno__ == ERR_CONNECTION_PROGRESS)) {
            /* connection in progress. Wait until connected using select */
            UA_DateTime timeSinceStart = UA_DateTime_nowMonotonic() - connStart;
            if(timeSinceStart > dtTimeout)
                break;

            fd_set fdset;
            FD_ZERO(&fdset);
            UA_fd_set(clientsockfd, &fdset);
            UA_DateTime timeout_usec = (dtTimeout - timeSinceStart) / UA_DATETIME_USEC;
            struct timeval tmptv = {(long int) (timeout_usec / 1000000),
                                    (long int) (timeout_usec % 1000000)};

            int resultsize = select((UA_Int32)(clientsockfd + 1), NULL, &fdset, NULL, &tmptv);

            if(resultsize == 1) {
#ifdef _WIN32
                /* Windows does not have any getsockopt equivalent and it is not
                 * needed there */
                connected = true;
                break;
#else
                OPTVAL_TYPE so_error;
                socklen_t len = sizeof so_error;

                int ret = getsockopt(clientsockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                if (ret != 0 || so_error != 0) {
                    /* on connection refused we should still try to connect */
                    /* connection refused happens on localhost or local ip without timeout */
                    if (so_error != ECONNREFUSED) {
                        ClientNetworkLayerTCP_close(&connection);
                        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                       "Connection to %s failed with error: %s",
                                       endpointUrl, strerror(ret == 0 ? so_error : errno__));
                        freeaddrinfo(server);
                        return connection;
                    }
                    /* wait until we try a again. Do not make this too small, otherwise the
                     * timeout is somehow wrong */
                    UA_sleep_ms(100);
                } else {
                    connected = true;
                    break;
                }
#endif
            }
        } else {
            connected = true;
            break;
        }
        ClientNetworkLayerTCP_close(&connection);

    } while ((UA_DateTime_nowMonotonic() - connStart) < dtTimeout);

    freeaddrinfo(server);

    if(!connected) {
        /* connection timeout */
        if (connection.state != UA_CONNECTION_CLOSED)
            ClientNetworkLayerTCP_close(&connection);
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Trying to connect to %s timed out",
                       endpointUrl);
        return connection;
    }


    /* We are connected. Reset socket to blocking */
    if(socket_set_blocking(clientsockfd) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the client socket to blocking");
        ClientNetworkLayerTCP_close(&connection);
        return connection;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    int sso_result = setsockopt(connection.sockfd, SOL_SOCKET,
                                SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if(sso_result < 0)
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Couldn't set SO_NOSIGPIPE");
#endif

    return connection;
}
