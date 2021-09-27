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
#include "ua_util_internal.h"

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

/* run only when the server is stopped */
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

static void
ServerNetworkLayerTCP_stop(UA_ServerNetworkLayer *nl, UA_Server *server) {
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP *)nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the TCP network layer");

    /* Close the server sockets */
    layer->serverSocketsSize = 0;
    UA_deinitialize_architecture_network();
}
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
        UA_free(e);
        if(nl->statistics) {
            nl->statistics->currentConnectionCount--;
        }
    }

    /* Free the layer */
    UA_free(layer);
}
static UA_StatusCode UA_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                            UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    return cm->allocNetworkBuffer(cm, ctx->connectionId, buf, length);
}

static UA_StatusCode UA_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    return cm->sendWithConnection(cm, ctx->connectionId, buf);
}

static
    void UA_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    cm->freeNetworkBuffer(cm, ctx->connectionId, buf);
}

static void UA_Connection_close(UA_Connection *connection) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    cm->closeConnection(cm, ctx->connectionId);
}

static void
shutdownCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                 void *connectionContext) {

    if (connectionContext != NULL) {
        UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext *)connectionContext;
        if(!ctx->isInitial) {
            UA_free(ctx);
        }
    }
}

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER,
                 "connection callback for id: %lu", connectionId);

    UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext *) *connectionContext;

    if (UA_EventLoop_getState(cm->eventSource.eventLoop) == UA_EVENTLOOPSTATE_STOPPING && stat != UA_STATUSCODE_BADCONNECTIONCLOSED) {
        UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER,
                     "stopping eventloop so pending msgs will not be processed");
        return;
    }

    if (stat == UA_STATUSCODE_BADCONNECTIONCLOSED) {
        UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER, "closing connection");

        shutdownCallback(cm, connectionId, *connectionContext);
        return;
    }

    if (ctx->isInitial) {
        UA_ConnectionContext *newCtx = (UA_ConnectionContext*) UA_calloc(1, sizeof(UA_ConnectionContext));
        newCtx->base.isInitial = false;
        newCtx->base.cm = ctx->cm;
        newCtx->base.server = ctx->server;
        newCtx->connectionId = connectionId;
        newCtx->connection.close = UA_Connection_close;
        newCtx->connection.free = NULL;
        newCtx->connection.getSendBuffer = UA_Connection_getSendBuffer;
        newCtx->connection.recv = NULL;
        newCtx->connection.releaseRecvBuffer = UA_Connection_releaseBuffer;
        newCtx->connection.releaseSendBuffer = UA_Connection_releaseBuffer;
        newCtx->connection.send = UA_Connection_send;
        newCtx->connection.state = UA_CONNECTIONSTATE_CLOSED;

        newCtx->connection.handle = newCtx;

        *connectionContext = newCtx;
    }

    UA_ConnectionContext *conCtx = (UA_ConnectionContext *) *connectionContext;

    if (msg.length > 0) {
        UA_Server_processBinaryMessage(ctx->server, &conCtx->connection, &msg);
    }
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerTCP(UA_ConnectionConfig config, UA_UInt16 port,
                         UA_UInt16 maxConnections) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    nl.clear = ServerNetworkLayerTCP_clear;
    nl.localConnectionConfig = config;
    nl.start = ServerNetworkLayerTCP_start;
    nl.listen = NULL;
    nl.stop = ServerNetworkLayerTCP_stop;
    nl.handle = NULL;

    /* TODO: check mem and error (and return statuscode errors */
    UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext*) UA_malloc(sizeof(UA_BasicConnectionContext));
    memset(ctx, 0, sizeof(UA_BasicConnectionContext));

    ctx->isInitial = true;

    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
    nl.connectionManager = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    UA_ConnectionManager *cm = nl.connectionManager;

    ctx->cm = cm;
    cm->connectionCallback = connectionCallback;
    cm->initialConnectionContext = ctx;
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);

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

    UA_BasicClientConnectionContext *ctx = (UA_BasicClientConnectionContext *) connection->handle;
    UA_Client *client = ctx->client;

    UA_CHECK_ERROR(!ctx->isInitial, return, &UA_Client_getConfig(client)->logger, UA_LOGCATEGORY_NETWORK, "should not "
                                                                                            "be initial");
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

    /* Get a socket and connect (only once) if not already done in a previous
     * call. On win32, calling connect multiple times is not recommended on
     * non-blocking sockets
     * (https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect).
     * On posix it is also not necessary to call connect multiple times.
     *
     * Identification of successfull connection is done using select (writeable/errorfd)
     * and getsockopt using SO_ERROR on win32 and posix.
     */
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
        int sso_result = setsockopt(connection->sockfd, SOL_SOCKET, SO_NOSIGPIPE,
                                    (void *)&val, sizeof(val));
        if(sso_result < 0)
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK, "Couldn't set SO_NOSIGPIPE");
#endif
        int error = UA_connect(connection->sockfd, tcpConnection->server->ai_addr,
                               tcpConnection->server->ai_addrlen);

        /* Connection successful */
        if(error == 0) {
            connection->state = UA_CONNECTIONSTATE_ESTABLISHED;
            return UA_STATUSCODE_GOOD;
        }

        /* The connection failed */
        if((UA_ERRNO != UA_ERR_CONNECTION_PROGRESS)) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Connection to %.*s failed with error: %s",
                           (int)tcpConnection->endpointUrl.length,
                           tcpConnection->endpointUrl.data, strerror(UA_ERRNO));
            ClientNetworkLayerTCP_close(connection);
            return UA_STATUSCODE_BADDISCONNECT;
        }
    }

    /* Use select to wait until connected. Return with a half-opened connection
     * after a timeout. */
    UA_UInt32 timeout_usec = timeout * 1000;

#ifdef _OS9000
    /* OS-9 cannot use select for checking write sockets. Therefore, we need to
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

    /* On windows select both writing and error fdset */
    fd_set writing_fdset;
    FD_ZERO(&writing_fdset);
    UA_fd_set(connection->sockfd, &writing_fdset);
    fd_set error_fdset;
    FD_ZERO(&error_fdset);
#ifdef _WIN32
    UA_fd_set(connection->sockfd, &error_fdset);
#endif
    struct timeval tmptv = {(long int)(timeout_usec / 1000000),
                            (int)(timeout_usec % 1000000)};

    int ret = UA_select((UA_Int32)(connection->sockfd + 1), NULL, &writing_fdset,
                        &error_fdset, &tmptv);

    /* When select fails abort connection */
    if(ret == -1) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s failed with error: %s",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data, strerror(UA_ERRNO));
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    int resultsize = UA_fd_isset(connection->sockfd, &writing_fdset);
#endif

    /* Any errors on the socket reported? */
    OPTVAL_TYPE so_error = 0;
    socklen_t len = sizeof(so_error);
    ret = UA_getsockopt(connection->sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if(ret != 0 || so_error != 0) {
        // UA_LOG_SOCKET_ERRNO_GAI_WRAP because of so_error
#ifndef _WIN32
        char *errno_str = strerror(ret == 0 ? so_error : UA_ERRNO);
#else
        char *errno_str = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, ret == 0 ? so_error : WSAGetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errno_str, 0,
                       NULL);
#endif
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s failed with error: %s",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data, errno_str);
#ifdef _WIN32
        LocalFree(errno_str);
#endif
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* The connection is fully opened. Otherwise, select has timed out. But we
     * can retry. */
    if(resultsize == 1)
        connection->state = UA_CONNECTIONSTATE_ESTABLISHED;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ClientConnectionEventloopTCP_poll(UA_Connection *connection, UA_UInt32 timeout,
                            const UA_Logger *logger) {
    return UA_ClientConnectionTCP_poll(connection, timeout, logger);
}


UA_Connection
UA_ClientConnectionEventloopTCP_init(UA_ConnectionConfig config, const UA_String endpointUrl,
                            UA_UInt32 timeout, const UA_Logger *logger) {
    UA_initialize_architecture_network();

    UA_Connection connection;
    memset(&connection, 0, sizeof(UA_Connection));

    connection.state = UA_CONNECTIONSTATE_OPENING;
    connection.sockfd = UA_INVALID_SOCKET;
    connection.send = NULL;
    connection.recv = NULL;
    connection.close = ClientNetworkLayerTCP_close;
    connection.free = ClientNetworkLayerTCP_free;
    connection.getSendBuffer = connection_getsendbuffer;
    connection.releaseSendBuffer = connection_releasesendbuffer;
    connection.releaseRecvBuffer = connection_releaserecvbuffer;

    return connection;
}


UA_Connection
UA_ClientConnectionTCP_init(UA_ConnectionConfig config, const UA_String endpointUrl,
                            UA_UInt32 timeout, const UA_Logger *logger) {
    UA_initialize_architecture_network();

    UA_Connection connection;
    memset(&connection, 0, sizeof(UA_Connection));

    connection.state = UA_CONNECTIONSTATE_OPENING;
    connection.sockfd = UA_INVALID_SOCKET;
    connection.send = NULL;
    connection.recv = NULL;
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

