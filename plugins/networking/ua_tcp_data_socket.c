/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_util.h"
#include "ua_types_generated_handling.h"
#include "ua_sockets.h"
#include "ua_types.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

typedef struct {
    UA_Boolean flaggedForDeletion;
    UA_ByteString receiveBuffer;
    /**
     * This is the same buffer as the receiveBuffer, but the length is the actual received length.
     */
    UA_ByteString receiveBufferOut;

    UA_ByteString sendBuffer;
    /**
     * This is the same buffer as the sendBuffer, but the length is set by external code to
     * the amount of data that is to be sent.
     */
    UA_ByteString sendBufferOut;
} TCPDataSocketData;

typedef struct {
    TCPDataSocketData dataSocketData;
    UA_String endpointUrl;
    UA_UInt32 timeout;
    UA_SocketHook openHook;
} TCPClientDataSocketData;

static UA_StatusCode
UA_TCP_DataSocket_close(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;
    if(sockData->flaggedForDeletion)
        return UA_STATUSCODE_GOOD;

    UA_LOG_DEBUG(sock->logger, UA_LOGCATEGORY_NETWORK, "Shutting down socket %i", (int)sock->id);
    UA_shutdown((UA_SOCKET)sock->id, 2);
    sockData->flaggedForDeletion = true;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
UA_TCP_DataSocket_mayDelete(UA_Socket *sock) {
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;
    return sockData->flaggedForDeletion;
}

static UA_StatusCode
UA_TCP_DataSocket_free(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;

    UA_SocketHook_call(sock->deletionHook, sock);

    UA_LOG_DEBUG(sock->logger, UA_LOGCATEGORY_NETWORK, "Freeing socket %i", (int)sock->id);

    UA_close((int)sock->id);
    UA_String_deleteMembers(&sock->discoveryUrl);
    UA_ByteString_deleteMembers(&sockData->receiveBuffer);
    UA_ByteString_deleteMembers(&sockData->sendBuffer);
    UA_free(sockData);
    UA_free(sock);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_activity(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;
    if(sockData->flaggedForDeletion)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    // we want to read as many bytes as possible.
    // the code called in the callback is responsible for disassembling the data
    // into e.g. chunks and copying it.
    ssize_t bytesReceived = UA_recv((int)sock->id,
                                    (char *)sockData->receiveBuffer.data,
                                    sockData->receiveBuffer.length, 0);

    if(bytesReceived < 0) {
        if(UA_ERRNO == UA_WOULDBLOCK || UA_ERRNO == UA_EAGAIN || UA_ERRNO == UA_INTERRUPTED) {
            return UA_STATUSCODE_GOOD;
        }
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(sock->logger, UA_LOGCATEGORY_NETWORK,
                         "Error while receiving data from socket: %s", errno_str));
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    if(bytesReceived == 0) {
        UA_LOG_INFO(sock->logger, UA_LOGCATEGORY_NETWORK,
                    "Socket %i | Performing orderly shutdown", (int)sock->id);
        sockData->flaggedForDeletion = true;
        return UA_STATUSCODE_GOOD;
    }

    sockData->receiveBufferOut.length = (size_t)bytesReceived;

    if(sock->dataCallback.callback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_Socket_dataCallback(sock, &sockData->receiveBufferOut);
}

static UA_StatusCode
UA_TCP_DataSocket_send(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;
    if(sockData->sendBufferOut.length > sockData->sendBuffer.length ||
       sockData->sendBufferOut.data != sockData->sendBuffer.data) {
        UA_LOG_ERROR(sock->logger, UA_LOGCATEGORY_NETWORK,
                     "sendBuffer length exceeds originally allocated length, "
                     "or the data pointer was modified.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(sockData->flaggedForDeletion)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    int flags = MSG_NOSIGNAL;

    size_t totalBytesSent = 0;

    do {
        ssize_t bytesSent = 0;
        do {
            bytesSent = UA_send((int)sock->id,
                                (const char *)sockData->sendBufferOut.data + totalBytesSent,
                                sockData->sendBufferOut.length - totalBytesSent, flags);
            if(bytesSent < 0 && UA_ERRNO != UA_EAGAIN && UA_ERRNO != UA_INTERRUPTED) {
                UA_LOG_ERROR(sock->logger, UA_LOGCATEGORY_NETWORK,
                             "Error while sending data over socket");
                return UA_STATUSCODE_BADCOMMUNICATIONERROR;
            }
        } while(bytesSent < 0);
        totalBytesSent += (size_t)bytesSent;
    } while(totalBytesSent < sockData->sendBufferOut.length);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_open(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;
    (void)sockData;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_getSendBuffer(UA_Socket *sock, size_t bufferSize, UA_ByteString **p_buffer) {
    if(sock == NULL || p_buffer == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPDataSocketData *const sockData = (TCPDataSocketData *const)sock->internalData;

    if(bufferSize > sockData->sendBuffer.length)
        return UA_STATUSCODE_BADINTERNALERROR;

    sockData->sendBufferOut = sockData->sendBuffer;
    if(bufferSize > 0)
        sockData->sendBufferOut.length = bufferSize;
    *p_buffer = &sockData->sendBufferOut;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_disableNaglesAlgorithm(UA_Socket *sock) {
    int dummy = 1;
    if(UA_setsockopt((int)sock->id, IPPROTO_TCP, TCP_NODELAY,
                     (const char *)&dummy, sizeof(dummy)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(sock->logger, UA_LOGCATEGORY_NETWORK,
                         "Cannot set socket option TCP_NODELAY. Error: %s",
                         errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static void
UA_TCP_DataSocket_logPeerName(UA_Socket *sock, struct sockaddr_storage *remote) {
#if defined(UA_getnameinfo)
    /* Get the peer name for logging */
    char remote_name[100];
    int res = UA_getnameinfo((struct sockaddr *)remote,
                             sizeof(struct sockaddr_storage),
                             remote_name, sizeof(remote_name),
                             NULL, 0, NI_NUMERICHOST);
    if(res == 0) {
        UA_LOG_INFO(sock->logger, UA_LOGCATEGORY_NETWORK,
                    "Socket %i | New connection over TCP from %s",
                    (int)sock->id, remote_name);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                                                "Socket %i | New connection over TCP, "
                                                "getnameinfo failed with error: %s",
                                                (int)sock->id, errno_str));
    }
#else
    UA_LOG_INFO(sock->logger, UA_LOGCATEGORY_NETWORK,
                "Socket %i | New connection over TCP",
                (int)sock->id);
#endif
}

static UA_StatusCode
UA_TCP_DataSocket_allocate(UA_UInt64 sockFd, UA_UInt32 sendBufferSize,
                           UA_UInt32 recvBufferSize, UA_SocketHook *deletionHook,
                           UA_Socket_DataCallback *dataCallback, UA_Logger *logger,
                           void *allocatedInternalData, UA_Socket **p_sock) {
    if(allocatedInternalData == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Socket *sock = (UA_Socket *)UA_malloc(sizeof(UA_Socket));
    if(sock == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Ran out of memory while creating data sock.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(sock, 0, sizeof(UA_Socket));

    TCPDataSocketData *internalData = (TCPDataSocketData *)allocatedInternalData;
    sock->internalData = internalData;
    if(dataCallback != NULL)
        sock->dataCallback = *dataCallback;
    if(deletionHook != NULL)
        sock->deletionHook = *deletionHook;
    sock->isListener = false;
    sock->id = (UA_UInt64)sockFd;
    sock->close = UA_TCP_DataSocket_close;
    sock->mayDelete = UA_TCP_DataSocket_mayDelete;
    sock->free = UA_TCP_DataSocket_free;
    sock->activity = UA_TCP_DataSocket_activity;
    sock->send = UA_TCP_DataSocket_send;
    sock->open = UA_TCP_DataSocket_open;
    sock->getSendBuffer = UA_TCP_DataSocket_getSendBuffer;
    sock->logger = logger;
    internalData->flaggedForDeletion = false;

    UA_StatusCode retval = UA_ByteString_allocBuffer(&internalData->receiveBuffer, recvBufferSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate receive buffer for socket with error %s",
                     UA_StatusCode_name(retval));
        goto error;
    }
    internalData->receiveBufferOut = internalData->receiveBuffer;

    retval = UA_ByteString_allocBuffer(&internalData->sendBuffer, sendBufferSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate receive buffer for socket with error %s",
                     UA_StatusCode_name(retval));
        goto error;
    }
    internalData->sendBufferOut = internalData->sendBuffer;

    *p_sock = sock;
    return retval;
error:
    UA_ByteString_deleteMembers(&internalData->receiveBuffer);
    UA_ByteString_deleteMembers(&internalData->sendBuffer);
    UA_free(sock);
    return retval;
}

UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(UA_Socket *listenerSocket, UA_Logger *logger, UA_UInt32 sendBufferSize,
                             UA_UInt32 recvBufferSize, UA_SocketHook creationHook, UA_SocketHook deletionHook,
                             UA_Socket_DataCallback dataCallback) {
    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_SOCKET newSockFd = UA_accept((int)listenerSocket->id,
                                    (struct sockaddr *)&remote, &remote_size);
    if(newSockFd == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Accept failed with %s", errno_str));
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Error accepting socket. Got invalid file descriptor");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_LOG_TRACE(logger, UA_LOGCATEGORY_NETWORK,
                 "New TCP sock (fd: %i) accepted from listener socket (fd: %i)",
                 newSockFd, (int)listenerSocket->id);

    UA_Socket *sock = NULL;
    void *internalData = UA_malloc(sizeof(TCPDataSocketData));
    UA_StatusCode retval;
    if(internalData == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate data socket internal data. Out of memory");
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }
    retval = UA_TCP_DataSocket_allocate((UA_UInt64)newSockFd, sendBufferSize, recvBufferSize,
                                        &deletionHook, &dataCallback, logger, internalData, &sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate socket resources with error %s",
                     UA_StatusCode_name(retval));
        goto error;
    }

    retval = UA_String_copy(&listenerSocket->discoveryUrl, &sock->discoveryUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to copy discovery url while creating data socket");
        goto error;
    }

    retval = UA_socket_set_nonblocking(newSockFd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Encountered error %s while setting socket to non blocking.",
                     UA_StatusCode_name(retval));
        goto error;
    }

    retval = UA_TCP_DataSocket_disableNaglesAlgorithm(sock);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    UA_TCP_DataSocket_logPeerName(sock, &remote);

    retval = UA_SocketHook_call(creationHook, sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Creation hook returned error %s.",
                     UA_StatusCode_name(retval));
        return retval;
    }

    return retval;

error:
    if(internalData != NULL)
        UA_free(internalData);
    if(sock != NULL)
        UA_String_deleteMembers(&sock->discoveryUrl);
    UA_close(newSockFd);
    return retval;
}

#define UA_HOSTNAME_MAX_LENGTH 512

static UA_StatusCode
UA_TCP_ClientDataSocket_open(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    TCPClientDataSocketData *internalData = (TCPClientDataSocketData *)sock->internalData;

    UA_String hostnameString = UA_STRING_NULL;
    UA_String pathString = UA_STRING_NULL;
    UA_UInt16 port = 0;
    char hostname[UA_HOSTNAME_MAX_LENGTH];

    UA_StatusCode retval =
        UA_parseEndpointUrl(&internalData->endpointUrl, &hostnameString,
                            &port, &pathString);
    if(retval != UA_STATUSCODE_GOOD || hostnameString.length >= UA_HOSTNAME_MAX_LENGTH) {
        UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                       "Server url is invalid: %*.s",
                       (int)internalData->endpointUrl.length, internalData->endpointUrl.data);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    memcpy(hostname, hostnameString.data, hostnameString.length);
    hostname[hostnameString.length] = 0;

    if(port == 0) {
        port = 4840;
        UA_LOG_INFO(sock->logger, UA_LOGCATEGORY_NETWORK,
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
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                                                    "DNS lookup of %s failed with error %s", hostname, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Boolean connected = false;
    UA_DateTime dtTimeout = internalData->timeout * UA_DATETIME_MSEC;
    UA_DateTime connStart = UA_DateTime_nowMonotonic();
    UA_SOCKET client_sockfd;

    /* On linux connect may immediately return with ECONNREFUSED but we still
     * want to try to connect. So use a loop and retry until timeout is
     * reached. */
    do {
        /* Get a socket */
        client_sockfd = UA_socket(server->ai_family,
                                 server->ai_socktype,
                                 server->ai_protocol);
        if(client_sockfd == UA_INVALID_SOCKET) {
            UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                                                    "Could not create client socket: %s", errno_str));
            UA_freeaddrinfo(server);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Non blocking connect to be able to timeout */
        retval = UA_socket_set_nonblocking(client_sockfd);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                           "Could not set the client socket to non blocking");
            goto error;
        }

        /* Non blocking connect */
        error = UA_connect(client_sockfd, server->ai_addr, (socklen_t)server->ai_addrlen);

        if((error == -1) && (UA_ERRNO != UA_ERR_CONNECTION_PROGRESS)) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                               "Connection to %*.s failed with error: %s",
                               (int)internalData->endpointUrl.length,
                               internalData->endpointUrl.data, errno_str));
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto error;
        }

        /* Use select to wait and check if connected */
        if(error == -1 && (UA_ERRNO == UA_ERR_CONNECTION_PROGRESS)) {
            /* connection in progress. Wait until connected using select */
            UA_DateTime timeSinceStart = UA_DateTime_nowMonotonic() - connStart;
            if(timeSinceStart > dtTimeout)
                break;

#ifdef _OS9000
            /* OS-9 can't use select for checking write sockets.
             * Therefore, we need to use connect until success or failed
             */
            UA_DateTime timeoutMicroseconds = (dtTimeout - timeSinceStart) / UA_DATETIME_USEC;
            int resultSize = 0;
            do {
                u_int32 time = 0x80000001;
                signal_code sig;

                timeoutMicroseconds -= 1000000/256;    // Sleep 1/256 second
                if (timeoutMicroseconds < 0)
                    break;

                _os_sleep(&time,&sig);
                error = connect(client_sockfd, server->ai_addr, server->ai_addrlen);
                if ((error == -1 && UA_ERRNO == EISCONN) || (error == 0))
                    resultSize = 1;
                if (error == -1 && UA_ERRNO != EALREADY && UA_ERRNO != EINPROGRESS)
                    break;
            }
            while(resultSize == 0);
#else
            fd_set fdSet;
            FD_ZERO(&fdSet);
            UA_fd_set(client_sockfd, &fdSet);
            UA_DateTime timeoutMicroseconds = (dtTimeout - timeSinceStart) / UA_DATETIME_USEC;
            struct timeval tmp_tv = {(long int)(timeoutMicroseconds / 1000000),
                                    (int)(timeoutMicroseconds % 1000000)};

            int resultSize = UA_select((UA_Int32)(client_sockfd + 1), NULL, &fdSet, NULL, &tmp_tv);
#endif

            if(resultSize == 1) {
#ifdef _WIN32
                /* Windows does not have any getsockopt equivalent and it is not
                 * needed there */
                connected = true;
                break;
#else
                OPTVAL_TYPE so_error;
                socklen_t len = sizeof so_error;

                int ret = UA_getsockopt(client_sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

                if(ret != 0 || so_error != 0) {
                    /* on connection refused we should still try to connect */
                    /* connection refused happens on localhost or local ip without timeout */
                    if(so_error != ECONNREFUSED) {
                        UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                                       "Connection to %*.s failed with error: %s",
                                       (int)internalData->endpointUrl.length,
                                       internalData->endpointUrl.data,
                                       strerror(ret == 0 ? so_error : UA_ERRNO));
                        retval = UA_STATUSCODE_BADINTERNALERROR;
                        goto error;
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
        UA_shutdown(client_sockfd, 2);
        UA_close(client_sockfd);
    } while((UA_DateTime_nowMonotonic() - connStart) < dtTimeout);

    UA_freeaddrinfo(server);

    if(!connected) {
        /* connection timeout */
        UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                       "Trying to connect to %*.s timed out",
                       (int)internalData->endpointUrl.length,
                       internalData->endpointUrl.data);
        retval = UA_STATUSCODE_BADTIMEOUT;
        goto error;
    }


    /* We are connected. Reset socket to blocking */
    retval = UA_socket_set_blocking(client_sockfd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the client socket to blocking");
        goto error;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    int sso_result = UA_setsockopt(client_sockfd, SOL_SOCKET,
                                   SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if(sso_result < 0)
        UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                       "Couldn't set SO_NOSIGPIPE");
#endif

    sock->id = (UA_UInt64)client_sockfd;

    UA_SocketHook_call(internalData->openHook, sock);

    return retval;
error:
    if(client_sockfd != UA_INVALID_SOCKET) {
        UA_shutdown(client_sockfd, 2);
        UA_close(client_sockfd);
    }
    UA_freeaddrinfo(server);
    return retval;
}

static UA_StatusCode
UA_TCP_ClientDataSocket_free(UA_Socket *sock) {
    TCPClientDataSocketData *internalData = (TCPClientDataSocketData *)sock->internalData;
    UA_String_deleteMembers(&internalData->endpointUrl);
    return UA_TCP_DataSocket_free(sock);
}

UA_StatusCode
UA_TCP_ClientDataSocket(const char *endpointUrl,
                        UA_UInt32 timeout, UA_Logger *logger,
                        UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize,
                        UA_SocketHook creationHook, UA_SocketHook openHook) {
    TCPClientDataSocketData *internalData =
        (TCPClientDataSocketData *)UA_malloc(sizeof(TCPClientDataSocketData));
    if(internalData == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate data socket internal data. Out of memory");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Socket *sock = NULL;
    UA_StatusCode retval = UA_TCP_DataSocket_allocate(0, sendBufferSize, recvBufferSize,
                                                      NULL, NULL, logger, internalData, &sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate socket resources with error %s",
                     UA_StatusCode_name(retval));
        UA_free(internalData);
        return retval;
    }

    UA_String endpointUrlString = UA_STRING((char *)(uintptr_t)endpointUrl);
    UA_String_copy(&endpointUrlString, &internalData->endpointUrl);
    internalData->openHook = openHook;
    internalData->timeout = timeout;

    sock->open = UA_TCP_ClientDataSocket_open;
    sock->free = UA_TCP_ClientDataSocket_free;

    retval = UA_SocketHook_call(creationHook, sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(internalData);
        UA_free(sock);
        return retval;
    }

    return retval;
}
