/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Jose Cabral
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#define UA_INTERNAL

#include <open62541/config.h>
#include <open62541/network_tcp.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>

#include "open62541_queue.h"
#include "ua_securechannel.h"

#include <string.h>  // memset

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/****************************/
/* Generic Socket Functions */
/****************************/

static UA_StatusCode
connection_getsendbuffer(UA_Connection *connection,
                         size_t length, UA_ByteString *buf) {
    UA_SecureChannel *channel = connection->channel;
    if(channel && channel->config.sendBufferSize < length)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static void
connection_releasesendbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

static void
connection_releaserecvbuffer(UA_Connection *connection,
                             UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

static UA_StatusCode
connection_write(UA_Connection *connection, UA_ByteString *buf) {
    if(connection->state == UA_CONNECTIONSTATE_CLOSED) {
        UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Prevent OS signals when sending to a closed socket */
    int flags = 0;
    flags |= MSG_NOSIGNAL;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send(connection->sockfd,
                     (const char*)buf->data + nWritten,
                     bytes_to_send, flags);
            if(n < 0 && UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_AGAIN) {
                connection->close(connection);
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
connection_recv(UA_Connection *connection, UA_ByteString *response,
                UA_UInt32 timeout) {
    if(connection->state == UA_CONNECTIONSTATE_CLOSED)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    /* Listen on the socket for the given timeout until a message arrives */
    fd_set fdset;
    FD_ZERO(&fdset);
    UA_fd_set(connection->sockfd, &fdset);
    UA_UInt32 timeout_usec = timeout * 1000;
    struct timeval tmptv = {(long int)(timeout_usec / 1000000),
                            (int)(timeout_usec % 1000000)};
    int resultsize = UA_select(connection->sockfd+1, &fdset, NULL, NULL, &tmptv);

    /* No result */
    if(resultsize == 0)
        return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

    if(resultsize == -1) {
        /* The call to select was interrupted. Act as if it timed out. */
        if(UA_ERRNO == EINTR)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        /* The error cannot be recovered. Close the connection. */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    UA_Boolean internallyAllocated = !response->length;

    /* Allocate the buffer  */
    if(internallyAllocated) {
        size_t bufferSize = 16384; /* Use as default for a new SecureChannel */
        UA_SecureChannel *channel = connection->channel;
        if(channel && channel->config.recvBufferSize > 0)
            bufferSize = channel->config.recvBufferSize;
        UA_StatusCode res = UA_ByteString_allocBuffer(response, bufferSize);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Get the received packet(s) */
    ssize_t ret = UA_recv(connection->sockfd, (char*)response->data, response->length, 0);

    /* The remote side closed the connection */
    if(ret == 0) {
        if(internallyAllocated)
            UA_ByteString_clear(response);
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Error case */
    if(ret < 0) {
        if(internallyAllocated)
            UA_ByteString_clear(response);
        if(UA_ERRNO == UA_INTERRUPTED || (timeout > 0) ?
           false : (UA_ERRNO == UA_EAGAIN || UA_ERRNO == UA_WOULDBLOCK))
            return UA_STATUSCODE_GOOD; /* statuscode_good but no data -> retry */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Set the length of the received buffer */
    response->length = (size_t)ret;
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
    const UA_Logger *logger;
    UA_UInt16 port;
    UA_UInt16 maxConnections;
    UA_SOCKET serverSockets[FD_SETSIZE];
    UA_UInt16 serverSocketsSize;
    LIST_HEAD(, ConnectionEntry) connections;
    UA_UInt16 connectionsSize;
} ServerNetworkLayerTCP;

static void
ServerNetworkLayerTCP_freeConnection(UA_Connection *connection) {
    UA_free(connection);
}

/* This performs only 'shutdown'. 'close' is called when the shutdown
 * socket is returned from select. */
static void
ServerNetworkLayerTCP_close(UA_Connection *connection) {
    if(connection->state == UA_CONNECTIONSTATE_CLOSED)
        return;
    UA_shutdown((UA_SOCKET)connection->sockfd, 2);
    connection->state = UA_CONNECTIONSTATE_CLOSED;
}

static UA_Boolean
purgeFirstConnectionWithoutChannel(ServerNetworkLayerTCP *layer) {
    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers) {
        if(e->connection.channel == NULL) {
            LIST_REMOVE(e, pointers);
            layer->connectionsSize--;
            UA_close(e->connection.sockfd);
            e->connection.free(&e->connection);
            return true;
        }
    }
    return false;
}

static UA_StatusCode
ServerNetworkLayerTCP_add(UA_ServerNetworkLayer *nl, ServerNetworkLayerTCP *layer,
                          UA_Int32 newsockfd, struct sockaddr_storage *remote) {
   if(layer->maxConnections && layer->connectionsSize >= layer->maxConnections &&
      !purgeFirstConnectionWithoutChannel(layer)) {
       return UA_STATUSCODE_BADTCPNOTENOUGHRESOURCES;
   }

    /* Set nonblocking */
    UA_socket_set_nonblocking(newsockfd);//TODO: check return value

    /* Do not merge packets on the socket (disable Nagle's algorithm) */
    int dummy = 1;
    if(UA_setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
               (const char *)&dummy, sizeof(dummy)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK,
                             "Cannot set socket option TCP_NODELAY. Error: %s",
                             errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

#if defined(UA_getnameinfo)
    /* Get the peer name for logging */
    char remote_name[100];
    int res = UA_getnameinfo((struct sockaddr*)remote,
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
#else
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Connection %i | New connection over TCP",
                (int)newsockfd);
#endif
    /* Allocate and initialize the connection */
    ConnectionEntry *e = (ConnectionEntry*)UA_malloc(sizeof(ConnectionEntry));
    if(!e) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Connection *c = &e->connection;
    memset(c, 0, sizeof(UA_Connection));
    c->sockfd = newsockfd;
    c->handle = layer;
    c->send = connection_write;
    c->close = ServerNetworkLayerTCP_close;
    c->free = ServerNetworkLayerTCP_freeConnection;
    c->getSendBuffer = connection_getsendbuffer;
    c->releaseSendBuffer = connection_releasesendbuffer;
    c->releaseRecvBuffer = connection_releaserecvbuffer;
    c->state = UA_CONNECTIONSTATE_OPENING;
    c->openingDate = UA_DateTime_nowMonotonic();

    layer->connectionsSize++;

    /* Add to the linked list */
    LIST_INSERT_HEAD(&layer->connections, e, pointers);
    if(nl->statistics) {
        nl->statistics->currentConnectionCount++;
        nl->statistics->cumulatedConnectionCount++;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addServerSocket(ServerNetworkLayerTCP *layer, struct addrinfo *ai) {
    /* Create the server socket */
    UA_SOCKET newsock = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(newsock == UA_INVALID_SOCKET)
    {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                           "Error opening the server socket: %s", errno_str));
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */

    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(newsock, IPPROTO_IPV6, IPV6_V6ONLY,
                  (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set an IPv6 socket to IPv6 only");
        UA_close(newsock);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;

    }
#endif
    if(UA_setsockopt(newsock, SOL_SOCKET, SO_REUSEADDR,
                  (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not make the socket reusable");
        UA_close(newsock);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }


    if(UA_socket_set_nonblocking(newsock) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the server socket to nonblocking");
        UA_close(newsock);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* Bind socket to address */
    int ret = UA_bind(newsock, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        /* If bind to specific address failed, try to bind *-socket */
        if(ai->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
            if(sin->sin_addr.s_addr != htonl(INADDR_ANY)) {
                sin->sin_addr.s_addr = 0;
                ret = 0;
            }
        }
        if(ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
            if(!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
                memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
                sin6->sin6_scope_id = 0;
                ret = 0;
            }
        }
        if(ret == 0) {
            ret = UA_bind(newsock, ai->ai_addr, (socklen_t)ai->ai_addrlen);
            if(ret == 0) {
                /* The second bind fixed the issue, inform the user. */
                UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Server socket bound to unspecified address");
            }
        }
    }
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                           "Error binding a server socket: %s", errno_str));
        UA_close(newsock);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* Start listening */
    if(UA_listen(newsock, MAXBACKLOG) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                       "Error listening on server socket: %s", errno_str));
        UA_close(newsock);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    if(layer->port == 0) {
        /* Port was automatically chosen. Read it from the OS */
        struct sockaddr_in returned_addr;
        memset(&returned_addr, 0, sizeof(returned_addr));
        socklen_t len = sizeof(returned_addr);
        UA_getsockname(newsock, (struct sockaddr *)&returned_addr, &len);
        layer->port = ntohs(returned_addr.sin_port);
    }

    layer->serverSockets[layer->serverSocketsSize] = newsock;
    layer->serverSocketsSize++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ServerNetworkLayerTCP_start(UA_ServerNetworkLayer *nl, const UA_Logger *logger,
                            const UA_String *customHostname) {
    UA_initialize_architecture_network();

    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;
    layer->logger = logger;

    /* Get addrinfo of the server and create server sockets */
    char hostname[512];
    if(customHostname->length) {
        if(customHostname->length >= sizeof(hostname))
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(hostname, customHostname->data, customHostname->length);
        hostname[customHostname->length] = '\0';
    }
    char portno[6];
    UA_snprintf(portno, 6, "%d", layer->port);
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    int retcode = UA_getaddrinfo(customHostname->length ? hostname : NULL,
                                 portno, &hints, &res);
    if(retcode != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK,
                                                    "getaddrinfo lookup of %s failed with error %d - %s", hostname, retcode, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* There might be serveral addrinfos (for different network cards,
     * IPv4/IPv6). Add a server socket for all of them. */
    struct addrinfo *ai = res;
    for(layer->serverSocketsSize = 0;
        layer->serverSocketsSize < FD_SETSIZE && ai != NULL;
        ai = ai->ai_next) {
        UA_StatusCode statusCode = addServerSocket(layer, ai);
        if(statusCode != UA_STATUSCODE_GOOD)
        {
            UA_freeaddrinfo(res);
            return statusCode;
        }
    }
    UA_freeaddrinfo(res);

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    if(customHostname->length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%.*s:%d/",
                                        (int)customHostname->length, customHostname->data,
                                        layer->port);
        du.data = (UA_Byte*)discoveryUrlBuffer;
    } else {
        char hostnameBuffer[256];
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "opc.tcp://%s:%d/",
                                            hostnameBuffer, layer->port);
            du.data = (UA_Byte*)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK, "Could not get the hostname");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    UA_String_copy(&du, &nl->discoveryUrl);

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
        if((UA_Int32)layer->serverSockets[i] > highestfd)
            highestfd = (UA_Int32)layer->serverSockets[i];
    }

    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers) {
        UA_fd_set(e->connection.sockfd, fdset);
        if((UA_Int32)e->connection.sockfd > highestfd)
            highestfd = (UA_Int32)e->connection.sockfd;
    }

    return highestfd;
}

static UA_StatusCode
ServerNetworkLayerTCP_listen(UA_ServerNetworkLayer *nl, UA_Server *server,
                             UA_UInt16 timeout) {
    /* Every open socket can generate two jobs */
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;

    if(layer->serverSocketsSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Listen on open sockets (including the server) */
    fd_set fdset, errset;
    UA_Int32 highestfd = setFDSet(layer, &fdset);
    setFDSet(layer, &errset);
    struct timeval tmptv = {0, timeout * 1000};
    if(UA_select(highestfd+1, &fdset, NULL, &errset, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_DEBUG(layer->logger, UA_LOGCATEGORY_NETWORK,
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
        UA_SOCKET newsockfd = UA_accept(layer->serverSockets[i],
                                  (struct sockaddr*)&remote, &remote_size);
        if(newsockfd == UA_INVALID_SOCKET)
            continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | New TCP connection on server socket %i",
                    (int)newsockfd, (int)(layer->serverSockets[i]));

        if(ServerNetworkLayerTCP_add(nl, layer, (UA_Int32)newsockfd, &remote) != UA_STATUSCODE_GOOD) {
            UA_close(newsockfd);
        }
    }

    /* Read from established sockets */
    ConnectionEntry *e, *e_tmp;
    UA_DateTime now = UA_DateTime_nowMonotonic();
    LIST_FOREACH_SAFE(e, &layer->connections, pointers, e_tmp) {
        if((e->connection.state == UA_CONNECTIONSTATE_OPENING) &&
            (now > (e->connection.openingDate + (NOHELLOTIMEOUT * UA_DATETIME_MSEC)))) {
            UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                        "Connection %i | Closed by the server (no Hello Message)",
                         (int)(e->connection.sockfd));
            LIST_REMOVE(e, pointers);
            layer->connectionsSize--;
            UA_close(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
            if(nl->statistics) {
                nl->statistics->connectionTimeoutCount--;
                nl->statistics->currentConnectionCount--;
            }
            continue;
        }

        if(!UA_fd_isset(e->connection.sockfd, &errset) &&
           !UA_fd_isset(e->connection.sockfd, &fdset))
          continue;

        UA_LOG_TRACE(layer->logger, UA_LOGCATEGORY_NETWORK,
                    "Connection %i | Activity on the socket",
                    (int)(e->connection.sockfd));

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
                        (int)(e->connection.sockfd));
            LIST_REMOVE(e, pointers);
            layer->connectionsSize--;
            UA_close(e->connection.sockfd);
            UA_Server_removeConnection(server, &e->connection);
            if(nl->statistics) {
                nl->statistics->currentConnectionCount--;
            }
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
        UA_shutdown(layer->serverSockets[i], 2);
        UA_close(layer->serverSockets[i]);
    }
    layer->serverSocketsSize = 0;

    /* Close open connections */
    ConnectionEntry *e;
    LIST_FOREACH(e, &layer->connections, pointers)
        ServerNetworkLayerTCP_close(&e->connection);

    /* Run recv on client sockets. This picks up the closed sockets and frees
     * the connection. */
    ServerNetworkLayerTCP_listen(nl, server, 0);

    UA_deinitialize_architecture_network();
}

/* run only when the server is stopped */
static void
ServerNetworkLayerTCP_clear(UA_ServerNetworkLayer *nl) {
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;
    UA_String_clear(&nl->discoveryUrl);

    /* Hard-close and remove remaining connections. The server is no longer
     * running. So this is safe. */
    ConnectionEntry *e, *e_tmp;
    LIST_FOREACH_SAFE(e, &layer->connections, pointers, e_tmp) {
        LIST_REMOVE(e, pointers);
        layer->connectionsSize--;
        UA_close(e->connection.sockfd);
        UA_free(e);
        if(nl->statistics) {
            nl->statistics->currentConnectionCount--;
        }
    }

    /* Free the layer */
    UA_free(layer);
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerTCP(UA_ConnectionConfig config, UA_UInt16 port,
                         UA_UInt16 maxConnections) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    nl.clear = ServerNetworkLayerTCP_clear;
    nl.localConnectionConfig = config;
    nl.start = ServerNetworkLayerTCP_start;
    nl.listen = ServerNetworkLayerTCP_listen;
    nl.stop = ServerNetworkLayerTCP_stop;
    nl.handle = NULL;

    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP*)
        UA_calloc(1,sizeof(ServerNetworkLayerTCP));
    if(!layer)
        return nl;
    nl.handle = layer;

    layer->port = port;
    layer->maxConnections = maxConnections;

    return nl;
}

typedef struct TCPClientConnection {
    struct addrinfo hints, *server;
    UA_DateTime connStart;
    UA_String endpointUrl;
    UA_UInt32 timeout;
} TCPClientConnection;

/***************************/
/* Client NetworkLayer TCP */
/***************************/

static void
ClientNetworkLayerTCP_close(UA_Connection *connection) {
    if(connection->state == UA_CONNECTIONSTATE_CLOSED)
        return;

    if(connection->sockfd != UA_INVALID_SOCKET) {
        UA_shutdown(connection->sockfd, 2);
        UA_close(connection->sockfd);
    }
    connection->state = UA_CONNECTIONSTATE_CLOSED;
}

static void
ClientNetworkLayerTCP_free(UA_Connection *connection) {
    if(!connection->handle)
        return;

    TCPClientConnection *tcpConnection = (TCPClientConnection *)connection->handle;
    if(tcpConnection->server)
        UA_freeaddrinfo(tcpConnection->server);
    UA_String_clear(&tcpConnection->endpointUrl);
    UA_free(tcpConnection);
    connection->handle = NULL;
}

UA_StatusCode
UA_ClientConnectionTCP_poll(UA_Connection *connection, UA_UInt32 timeout,
                            const UA_Logger *logger) {
    if(connection->state == UA_CONNECTIONSTATE_CLOSED)
        return UA_STATUSCODE_BADDISCONNECT;
    if(connection->state == UA_CONNECTIONSTATE_ESTABLISHED)
        return UA_STATUSCODE_GOOD;

    /* Connection timeout? */
    TCPClientConnection *tcpConnection = (TCPClientConnection*) connection->handle;
    if((UA_Double) (UA_DateTime_nowMonotonic() - tcpConnection->connStart)
       > (UA_Double) tcpConnection->timeout * UA_DATETIME_MSEC ) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK, "Timed out");
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Get a socket */
    if(connection->sockfd == UA_INVALID_SOCKET) {
        connection->sockfd = UA_socket(tcpConnection->server->ai_family,
                                       tcpConnection->server->ai_socktype,
                                       tcpConnection->server->ai_protocol);
        if(connection->sockfd == UA_INVALID_SOCKET) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                    "Could not create client socket: %s", strerror(UA_ERRNO));
            ClientNetworkLayerTCP_close(connection);
            return UA_STATUSCODE_BADDISCONNECT;
        }

        /* Non blocking connect to be able to timeout */
        if(UA_socket_set_nonblocking(connection->sockfd) != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Could not set the client socket to nonblocking");
            ClientNetworkLayerTCP_close(connection);
            return UA_STATUSCODE_BADDISCONNECT;
        }

        /* Don't have the socket create interrupt signals */
#ifdef SO_NOSIGPIPE
        int val = 1;
        int sso_result = setsockopt(connection->sockfd, SOL_SOCKET,
                                    SO_NOSIGPIPE, (void*)&val, sizeof(val));
        if(sso_result < 0)
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Couldn't set SO_NOSIGPIPE");
#endif
    }

    /* Non-blocking connect */
    int error = UA_connect(connection->sockfd, tcpConnection->server->ai_addr,
                           tcpConnection->server->ai_addrlen);

    /* Connection successful */
    if(error == 0) {
        connection->state = UA_CONNECTIONSTATE_ESTABLISHED;
        return UA_STATUSCODE_GOOD;
    }

    /* The connection failed */
    if(UA_ERRNO != UA_ERR_CONNECTION_PROGRESS) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s failed with error: %s",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data, strerror(UA_ERRNO));
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Use select to wait until connected. Return with a half-opened connection
     * after a timeout. */
    UA_UInt32 timeout_usec = timeout * 1000;

#ifdef _OS9000
    /* OS-9 can't use select for checking write sockets. Therefore, we need to
     * use connect until success or failed */
    int resultsize = 0;
    do {
        u_int32 time = 0x80000001;
        signal_code sig;

        timeout_usec -= 1000000/256;    // Sleep 1/256 second
        if(timeout_usec < 0)
            break;

        _os_sleep(&time, &sig);
        error = connect(connection->sockfd, tcpConnection->server->ai_addr,
                        tcpConnection->server->ai_addrlen);
        if((error == -1 && UA_ERRNO == EISCONN) || (error == 0))
            resultsize = 1;
        if(error == -1 && UA_ERRNO != EALREADY && UA_ERRNO != EINPROGRESS)
            break;
    } while(resultsize == 0);
#else
    /* Wait in a select-call until the connection fully opens or the timeout
     * happens */
    fd_set fdset;
    FD_ZERO(&fdset);
    UA_fd_set(connection->sockfd, &fdset);
    struct timeval tmptv = { (long int) (timeout_usec / 1000000),
                             (int) (timeout_usec % 1000000) };
    int resultsize = UA_select((UA_Int32) (connection->sockfd + 1), NULL,
                               &fdset, NULL, &tmptv);
#endif

#ifndef _WIN32
    /* Any errors on the socket reported? */
    OPTVAL_TYPE so_error = 0;
    socklen_t len = sizeof(so_error);
    int ret = UA_getsockopt(connection->sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if(ret != 0 || so_error != 0) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s failed with error: %s",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data,
                       strerror(ret == 0 ? so_error : UA_ERRNO));
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }
#endif

    /* The connection is fully opened. Otherwise, select has timed out. But we
     * can retry. */
    if(resultsize == 1)
        connection->state = UA_CONNECTIONSTATE_ESTABLISHED;

    return UA_STATUSCODE_GOOD;
}

UA_Connection
UA_ClientConnectionTCP_init(UA_ConnectionConfig config, const UA_String endpointUrl,
                            UA_UInt32 timeout, const UA_Logger *logger) {
    UA_initialize_architecture_network();

    UA_Connection connection;
    memset(&connection, 0, sizeof(UA_Connection));

    connection.state = UA_CONNECTIONSTATE_OPENING;
    connection.sockfd = UA_INVALID_SOCKET;
    connection.send = connection_write;
    connection.recv = connection_recv;
    connection.close = ClientNetworkLayerTCP_close;
    connection.free = ClientNetworkLayerTCP_free;
    connection.getSendBuffer = connection_getsendbuffer;
    connection.releaseSendBuffer = connection_releasesendbuffer;
    connection.releaseRecvBuffer = connection_releaserecvbuffer;

    TCPClientConnection *tcpClientConnection = (TCPClientConnection*)
        UA_malloc(sizeof(TCPClientConnection));
    if(!tcpClientConnection) {
        connection.state = UA_CONNECTIONSTATE_CLOSED;
        return connection;
    }
    memset(tcpClientConnection, 0, sizeof(TCPClientConnection));
    connection.handle = (void*) tcpClientConnection;
    tcpClientConnection->timeout = timeout;
    UA_String hostnameString = UA_STRING_NULL;
    UA_String pathString = UA_STRING_NULL;
    UA_UInt16 port = 0;
    char hostname[512];
    tcpClientConnection->connStart = UA_DateTime_nowMonotonic();
    UA_String_copy(&endpointUrl, &tcpClientConnection->endpointUrl);

    UA_StatusCode parse_retval =
        UA_parseEndpointUrl(&endpointUrl, &hostnameString, &port, &pathString);
    if(parse_retval != UA_STATUSCODE_GOOD || hostnameString.length > 511) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Server url is invalid: %.*s",
                       (int)endpointUrl.length, endpointUrl.data);
        connection.state = UA_CONNECTIONSTATE_CLOSED;
        return connection;
    }
    memcpy(hostname, hostnameString.data, hostnameString.length);
    hostname[hostnameString.length] = 0;

    if(port == 0) {
        port = 4840;
        UA_LOG_INFO(logger, UA_LOGCATEGORY_NETWORK,
                    "No port defined, using default port %" PRIu16, port);
    }

    memset(&tcpClientConnection->hints, 0, sizeof(tcpClientConnection->hints));
    tcpClientConnection->hints.ai_family = AF_UNSPEC;
    tcpClientConnection->hints.ai_socktype = SOCK_STREAM;
    char portStr[6];
    UA_snprintf(portStr, 6, "%d", port);
    int error = UA_getaddrinfo(hostname, portStr, &tcpClientConnection->hints,
                               &tcpClientConnection->server);
    if(error != 0 || !tcpClientConnection->server) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                                    "DNS lookup of %s failed with error %d - %s",
                                                    hostname, error, errno_str));
        connection.state = UA_CONNECTIONSTATE_CLOSED;
        return connection;
    }

    /* Return connection with state UA_CONNECTIONSTATE_OPENING */
    return connection;
}
