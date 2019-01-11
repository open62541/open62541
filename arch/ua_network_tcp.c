/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Jose Cabral
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#define UA_INTERNAL
#include "ua_network_tcp.h"
#include "ua_log_stdout.h"
#include "open62541_queue.h"
#include "ua_util.h"

#include <string.h> // memset

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

/****************************/
/* Generic Socket Functions */
/****************************/

static UA_StatusCode
connection_getsendbuffer(UA_Connection_old *connection,
                         size_t length, UA_ByteString *buf) {
    if(length > connection->config.sendBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static void
connection_releasesendbuffer(UA_Connection_old *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static void
connection_releaserecvbuffer(UA_Connection_old *connection,
                             UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static UA_StatusCode
connection_write(UA_Connection_old *connection, UA_ByteString *buf) {
    if(connection->state == UA_CONNECTION_CLOSED) {
        UA_ByteString_deleteMembers(buf);
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
connection_recv(UA_Connection_old *connection, UA_ByteString *response,
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
                                (int)(timeout_usec % 1000000)};
        int resultsize = UA_select(connection->sockfd+1, &fdset, NULL,
                                NULL, &tmptv);

        /* No result */
        if(resultsize == 0)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        if(resultsize == -1) {
            /* The call to select was interrupted manually. Act as if it timed
             * out */
            if(UA_ERRNO == EINTR)
                return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

            /* The error cannot be recovered. Close the connection. */
            connection->close(connection);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    response->data = (UA_Byte*)UA_malloc(connection->config.recvBufferSize);
    if(!response->data) {
        response->length = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY; /* not enough memory retry */
    }

    size_t offset = connection->incompleteChunk.length;
    size_t remaining = connection->config.recvBufferSize - offset;

    /* Get the received packet(s) */
    ssize_t ret = UA_recv(connection->sockfd, (char*)&response->data[offset],
                          remaining, 0);

    /* The remote side closed the connection */
    if(ret == 0) {
        UA_ByteString_deleteMembers(response);
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Error case */
    if(ret < 0) {
        UA_ByteString_deleteMembers(response);
        if(UA_ERRNO == UA_INTERRUPTED || (timeout > 0) ?
           false : (UA_ERRNO == UA_EAGAIN || UA_ERRNO == UA_WOULDBLOCK))
            return UA_STATUSCODE_GOOD; /* statuscode_good but no data -> retry */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Preprend the last incompleteChunk into the buffer */
    memcpy(response->data, connection->incompleteChunk.data,
           connection->incompleteChunk.length);
    UA_ByteString_deleteMembers(&connection->incompleteChunk);

    /* Set the length of the received buffer */
    response->length = offset + (size_t)ret;
    return UA_STATUSCODE_GOOD;
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
ClientNetworkLayerTCP_close(UA_Connection_old *connection) {
    if (connection->state == UA_CONNECTION_CLOSED)
        return;

    if(connection->sockfd != UA_INVALID_SOCKET) {
        UA_shutdown(connection->sockfd, 2);
        UA_close(connection->sockfd);
    }
    connection->state = UA_CONNECTION_CLOSED;
}

static void
ClientNetworkLayerTCP_free(UA_Connection_old *connection) {
    if (connection->handle){
        TCPClientConnection *tcpConnection = (TCPClientConnection *)connection->handle;
        if(tcpConnection->server)
          UA_freeaddrinfo(tcpConnection->server);
        UA_free(tcpConnection);
    }
}

UA_StatusCode UA_ClientConnectionTCP_poll(UA_Client *client, void *data) {
    UA_Connection_old *connection = (UA_Connection_old*) data;

    if (connection->state == UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADDISCONNECT;

    TCPClientConnection *tcpConnection =
                    (TCPClientConnection*) connection->handle;

    UA_DateTime connStart = UA_DateTime_nowMonotonic();
    UA_SOCKET clientsockfd = connection->sockfd;

    UA_ClientConfig *config = UA_Client_getConfig(client);

    if (connection->state == UA_CONNECTION_ESTABLISHED) {
            UA_Client_removeRepeatedCallback(client, connection->connectCallbackID);
            connection->connectCallbackID = 0;
            return UA_STATUSCODE_GOOD;
    }
    if ((UA_Double) (UA_DateTime_nowMonotonic() - tcpConnection->connStart)
                    > tcpConnection->timeout* UA_DATETIME_MSEC ) {
            // connection timeout
            ClientNetworkLayerTCP_close(connection);
            UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                            "Timed out");
            return UA_STATUSCODE_BADDISCONNECT;

    }
    /* On linux connect may immediately return with ECONNREFUSED but we still want to try to connect */
    /* Thus use a loop and retry until timeout is reached */

    /* Get a socket */
    if(clientsockfd <= 0) {
        clientsockfd = UA_socket(tcpConnection->server->ai_family,
                                 tcpConnection->server->ai_socktype,
                                 tcpConnection->server->ai_protocol);
        connection->sockfd = (UA_Int32)clientsockfd; /* cast for win32 */
    }

    if(clientsockfd == UA_INVALID_SOCKET) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not create client socket: %s", strerror(UA_ERRNO));
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Non blocking connect to be able to timeout */
    if(UA_socket_set_nonblocking(clientsockfd) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the client socket to nonblocking");
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Non blocking connect */
    int error = UA_connect(clientsockfd, tcpConnection->server->ai_addr,
                    tcpConnection->server->ai_addrlen);

    if ((error == -1) && (UA_ERRNO != UA_ERR_CONNECTION_PROGRESS)) {
            ClientNetworkLayerTCP_close(connection);
            UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                           "Connection to  failed with error: %s", strerror(UA_ERRNO));
            return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Use select to wait and check if connected */
    if (error == -1 && (UA_ERRNO == UA_ERR_CONNECTION_PROGRESS)) {
        /* connection in progress. Wait until connected using select */

        UA_UInt32 timeSinceStart =
                        (UA_UInt32) ((UA_Double) (UA_DateTime_nowMonotonic() - connStart)
                                        * UA_DATETIME_MSEC);
#ifdef _OS9000
        /* OS-9 can't use select for checking write sockets.
         * Therefore, we need to use connect until success or failed
         */
        UA_UInt32 timeout_usec = (tcpConnection->timeout - timeSinceStart)
                        * 1000;
        int resultsize = 0;
        do {
            u_int32 time = 0x80000001;
            signal_code sig;

            timeout_usec -= 1000000/256;    // Sleep 1/256 second
            if (timeout_usec < 0)
                break;

            _os_sleep(&time,&sig);
            error = connect(clientsockfd, tcpConnection->server->ai_addr,
                        tcpConnection->server->ai_addrlen);
            if ((error == -1 && UA_ERRNO == EISCONN) || (error == 0))
                resultsize = 1;
            if (error == -1 && UA_ERRNO != EALREADY && UA_ERRNO != EINPROGRESS)
                break;
        }
        while(resultsize == 0);
#else
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(clientsockfd, &fdset);
        UA_UInt32 timeout_usec = (tcpConnection->timeout - timeSinceStart)
                        * 1000;
        struct timeval tmptv = { (long int) (timeout_usec / 1000000),
                        (int) (timeout_usec % 1000000) };

        int resultsize = UA_select((UA_Int32) (clientsockfd + 1), NULL, &fdset,
        NULL, &tmptv);
#endif
        if (resultsize == 1) {
            /* Windows does not have any getsockopt equivalent and it is not needed there */
#ifdef _WIN32
            connection->sockfd = clientsockfd;
            connection->state = UA_CONNECTION_ESTABLISHED;
            return UA_STATUSCODE_GOOD;
#else
            OPTVAL_TYPE so_error;
            socklen_t len = sizeof so_error;

            int ret = UA_getsockopt(clientsockfd, SOL_SOCKET, SO_ERROR, &so_error,
                            &len);

            if (ret != 0 || so_error != 0) {
                /* on connection refused we should still try to connect */
                /* connection refused happens on localhost or local ip without timeout */
                if (so_error != ECONNREFUSED) {
                        // general error
                        ClientNetworkLayerTCP_close(connection);
                        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                                        "Connection to failed with error: %s",
                                        strerror(ret == 0 ? so_error : UA_ERRNO));
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
    UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_NETWORK,
                    "Couldn't set SO_NOSIGPIPE");
#endif

    return UA_STATUSCODE_GOOD;

}

UA_Connection_old
UA_ClientConnectionTCP_init(UA_ConnectionConfig config, const char *endpointUrl,
                            const UA_UInt32 timeout, const UA_Logger *logger) {
    UA_Connection_old connection;
    memset(&connection, 0, sizeof(UA_Connection_old));

    connection.state = UA_CONNECTION_OPENING;
    connection.config = config;
    connection.send = connection_write;
    connection.recv = connection_recv;
    connection.close = ClientNetworkLayerTCP_close;
    connection.free = ClientNetworkLayerTCP_free;
    connection.getSendBuffer = connection_getsendbuffer;
    connection.releaseSendBuffer = connection_releasesendbuffer;
    connection.releaseRecvBuffer = connection_releaserecvbuffer;

    TCPClientConnection *tcpClientConnection = (TCPClientConnection*) UA_malloc(
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
            connection.state = UA_CONNECTION_CLOSED;
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
    UA_snprintf(portStr, 6, "%d", port);
    int error = UA_getaddrinfo(hostname, portStr, &tcpClientConnection->hints,
                    &tcpClientConnection->server);
    if (error != 0 || !tcpClientConnection->server) {
      UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                                 "DNS lookup of %s failed with error %s", hostname, errno_str));
      connection.state = UA_CONNECTION_CLOSED;
      return connection;
    }
    return connection;
}

UA_Connection_old
UA_ClientConnectionTCP(UA_ConnectionConfig config, const char *endpointUrl,
                       const UA_UInt32 timeout, const UA_Logger *logger) {
    UA_initialize_architecture_network();

    UA_Connection_old connection;
    memset(&connection, 0, sizeof(UA_Connection_old));
    connection.state = UA_CONNECTION_CLOSED;
    connection.config = config;
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
    hints.ai_protocol = IPPROTO_TCP;
    char portStr[6];
    UA_snprintf(portStr, 6, "%d", port);
    int error = UA_getaddrinfo(hostname, portStr, &hints, &server);
    if(error != 0 || !server) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                              "DNS lookup of %s failed with error %s", hostname, errno_str));
        return connection;
    }

    UA_Boolean connected = false;
    UA_DateTime dtTimeout = timeout * UA_DATETIME_MSEC;
    UA_DateTime connStart = UA_DateTime_nowMonotonic();
    UA_SOCKET clientsockfd;

    /* On linux connect may immediately return with ECONNREFUSED but we still
     * want to try to connect. So use a loop and retry until timeout is
     * reached. */
    do {
        /* Get a socket */
        clientsockfd = UA_socket(server->ai_family,
                              server->ai_socktype,
                              server->ai_protocol);
        if(clientsockfd == UA_INVALID_SOCKET) {
            UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                                    "Could not create client socket: %s", errno_str));
            UA_freeaddrinfo(server);
            return connection;
        }

        connection.state = UA_CONNECTION_OPENING;

        /* Connect to the server */
        connection.sockfd = clientsockfd;

        /* Non blocking connect to be able to timeout */
        if (UA_socket_set_nonblocking(clientsockfd) != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Could not set the client socket to nonblocking");
            ClientNetworkLayerTCP_close(&connection);
            UA_freeaddrinfo(server);
            return connection;
        }

        /* Non blocking connect */
        error = UA_connect(clientsockfd, server->ai_addr, (socklen_t)server->ai_addrlen);

        if ((error == -1) && (UA_ERRNO != UA_ERR_CONNECTION_PROGRESS)) {
            ClientNetworkLayerTCP_close(&connection);
            UA_LOG_SOCKET_ERRNO_WRAP(
                    UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                   "Connection to %s failed with error: %s",
                                   endpointUrl, errno_str));
            UA_freeaddrinfo(server);
            return connection;
        }

        /* Use select to wait and check if connected */
        if (error == -1 && (UA_ERRNO == UA_ERR_CONNECTION_PROGRESS)) {
            /* connection in progress. Wait until connected using select */
            UA_DateTime timeSinceStart = UA_DateTime_nowMonotonic() - connStart;
            if(timeSinceStart > dtTimeout)
                break;

#ifdef _OS9000
            /* OS-9 can't use select for checking write sockets.
             * Therefore, we need to use connect until success or failed
             */
            UA_DateTime timeout_usec = (dtTimeout - timeSinceStart) / UA_DATETIME_USEC;
            int resultsize = 0;
            do {
                u_int32 time = 0x80000001;
                signal_code sig;

                timeout_usec -= 1000000/256;    // Sleep 1/256 second
                if (timeout_usec < 0)
                    break;

                _os_sleep(&time,&sig);
                error = connect(clientsockfd, server->ai_addr, server->ai_addrlen);
                if ((error == -1 && UA_ERRNO == EISCONN) || (error == 0))
                    resultsize = 1;
                if (error == -1 && UA_ERRNO != EALREADY && UA_ERRNO != EINPROGRESS)
                    break;
            }
            while(resultsize == 0);
#else
            fd_set fdset;
            FD_ZERO(&fdset);
            UA_fd_set(clientsockfd, &fdset);
            UA_DateTime timeout_usec = (dtTimeout - timeSinceStart) / UA_DATETIME_USEC;
            struct timeval tmptv = {(long int) (timeout_usec / 1000000),
                                    (int) (timeout_usec % 1000000)};

            int resultsize = UA_select((UA_Int32)(clientsockfd + 1), NULL, &fdset, NULL, &tmptv);
#endif

            if(resultsize == 1) {
#ifdef _WIN32
                /* Windows does not have any getsockopt equivalent and it is not
                 * needed there */
                connected = true;
                break;
#else
                OPTVAL_TYPE so_error;
                socklen_t len = sizeof so_error;

                int ret = UA_getsockopt(clientsockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                if (ret != 0 || so_error != 0) {
                    /* on connection refused we should still try to connect */
                    /* connection refused happens on localhost or local ip without timeout */
                    if (so_error != ECONNREFUSED) {
                        ClientNetworkLayerTCP_close(&connection);
                        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                                       "Connection to %s failed with error: %s",
                                       endpointUrl, strerror(ret == 0 ? so_error : UA_ERRNO));
                        UA_freeaddrinfo(server);
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

    UA_freeaddrinfo(server);

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
    if(UA_socket_set_blocking(clientsockfd) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the client socket to blocking");
        ClientNetworkLayerTCP_close(&connection);
        return connection;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    int sso_result = UA_setsockopt(connection.sockfd, SOL_SOCKET,
                                SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if(sso_result < 0)
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Couldn't set SO_NOSIGPIPE");
#endif

    return connection;
}
