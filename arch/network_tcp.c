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

typedef struct TCPClientConnection {
    struct addrinfo hints, *server;
    UA_DateTime connStart;
    UA_String endpointUrl;
    UA_UInt32 timeout;
} TCPClientConnection;

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

    struct pollfd poll_fd[1];
    poll_fd[0].fd = connection->sockfd;
    poll_fd[0].events = UA_POLLOUT;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;
    do {
        ssize_t n = 0;
        do {
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send(connection->sockfd,
                        (const char*)buf->data + nWritten,
                        bytes_to_send, flags);
            if(n<0) {
                if(UA_ERRNO != UA_INTERRUPTED && UA_ERRNO != UA_AGAIN) {
                    connection->close(connection);
                    UA_ByteString_clear(buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }
                int poll_ret;
                do {
                    poll_ret = UA_poll (poll_fd, 1, 1000);
                } while (poll_ret == 0 || (poll_ret < 0 && UA_ERRNO == UA_INTERRUPTED));
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
        if(UA_ERRNO == UA_INTERRUPTED)
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
           false : (UA_ERRNO == UA_AGAIN || UA_ERRNO == UA_WOULDBLOCK))
            return UA_STATUSCODE_GOOD; /* statuscode_good but no data -> retry */
        connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    /* Set the length of the received buffer */
    response->length = (size_t)ret;
    return UA_STATUSCODE_GOOD;
}


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
        if(UA_ERRNO != UA_WOULDBLOCK && UA_ERRNO != UA_INPROGRESS) {
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
        if(error == -1 && UA_ERRNO != UA_ALREADY && UA_ERRNO != UA_INPROGRESS)
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

    // When select fails abort connection
    if(ret == -1) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s failed with error: %s",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data, strerror(UA_ERRNO));
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADDISCONNECT;
    } else if (ret == 0) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                       "Connection to %.*s timed out",
                       (int)tcpConnection->endpointUrl.length,
                       tcpConnection->endpointUrl.data);
        ClientNetworkLayerTCP_close(connection);
        return UA_STATUSCODE_BADTIMEOUT;
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
    if(resultsize > 0)
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
