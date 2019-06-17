/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018-2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/types_generated_handling.h>
#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/plugin/networking/sockets.h>
#include <open62541/plugin/networkmanager.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

typedef struct {
    UA_Socket socket;
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
} UA_Socket_tcpDataSocket;

typedef struct {
    UA_Socket_tcpDataSocket socket;
    UA_String endpointUrl;
    UA_UInt32 timeout;
    struct addrinfo *server;
    UA_DateTime dtTimeout;
    UA_DateTime connStart;
} UA_Socket_tcpClientDataSocket;

static UA_StatusCode
UA_TCP_DataSocket_close(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;
    if(internalSocket->flaggedForDeletion)
        return UA_STATUSCODE_GOOD;

    UA_LOG_DEBUG(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK, "Shutting down socket %i", (int)sock->id);
    if((UA_SOCKET)sock->id != UA_INVALID_SOCKET)
        UA_shutdown((UA_SOCKET)sock->id, 2);
    sock->socketState = UA_SOCKETSTATE_CLOSED;
    internalSocket->flaggedForDeletion = true;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
UA_TCP_DataSocket_mayDelete(UA_Socket *sock) {
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;
    return internalSocket->flaggedForDeletion;
}

static UA_Boolean
UA_TCP_ClientDataSocket_mayDelete(UA_Socket *sock) {
    UA_Socket_tcpClientDataSocket *const internalSocket = (UA_Socket_tcpClientDataSocket *const)sock;
    if((UA_DateTime_nowMonotonic() - internalSocket->connStart) >= internalSocket->dtTimeout &&
       sock->socketState == UA_SOCKETSTATE_NEW) {
        sock->close(sock);
        UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                       "Trying to connect to %.*s timed out",
                       (int)internalSocket->endpointUrl.length,
                       internalSocket->endpointUrl.data);
    }
    return internalSocket->socket.flaggedForDeletion;
}

static UA_StatusCode
UA_TCP_DataSocket_free(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;

    UA_SocketCallback_call(sock->freeCallback, sock);

    UA_LOG_DEBUG(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK, "Freeing socket %i", (int)sock->id);

    if((UA_SOCKET)sock->id != UA_INVALID_SOCKET)
        UA_close((UA_SOCKET)sock->id);
    UA_String_deleteMembers(&sock->discoveryUrl);
    UA_ByteString_deleteMembers(&internalSocket->receiveBuffer);
    UA_ByteString_deleteMembers(&internalSocket->sendBuffer);
    UA_free(sock);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_activity(UA_Socket *sock, UA_Boolean readActivity, UA_Boolean writeActivity) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;

    if(internalSocket->flaggedForDeletion)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    switch(sock->socketState) {
    case UA_SOCKETSTATE_NEW: {
        if(writeActivity) {
#ifdef _WIN32
            /* Windows does not have any getsockopt equivalent and it is not needed there */
            sock->socketState = UA_SOCKETSTATE_OPEN;
#else
            OPTVAL_TYPE so_error;
            socklen_t len = sizeof(so_error);

            int ret = UA_getsockopt((UA_SOCKET)sock->id, SOL_SOCKET, SO_ERROR, &so_error, &len);

            if(ret != 0 || so_error != 0) {
                if(so_error != ECONNREFUSED) {
                    UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                   "Connection on socket %d failed with error: %s",
                                   (UA_SOCKET)sock->id,
                                   strerror(ret == 0 ? so_error : UA_ERRNO));
                    return UA_STATUSCODE_BADCOMMUNICATIONERROR;
                } else {
                    UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK, "Connection refused");

                    return UA_STATUSCODE_BADCONNECTIONREJECTED;
                }
            } else {
                sock->socketState = UA_SOCKETSTATE_OPEN;
            }

            /* We are connected. Reset socket to blocking */
            UA_StatusCode retval = UA_socket_set_blocking((UA_SOCKET)sock->id);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                               "Could not set the client socket to blocking");
                return retval;
            }
#endif
            /* we are no longer interested in write activity since we are now connected */
            sock->waitForWriteActivity = false;
            if(sock->socketState == UA_SOCKETSTATE_OPEN)
                return UA_SocketCallback_call(sock->openCallback, sock);
        } else {
            UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                           "Read activity on not connected socket!");
        }
        break;
    }
    case UA_SOCKETSTATE_OPEN: {
        if(readActivity) {
            // we want to read as many bytes as possible.
            // the code called in the callback is responsible for disassembling the data
            // into e.g. chunks and copying it.
            ssize_t bytesReceived = UA_recv((int)sock->id,
                                            (char *)internalSocket->receiveBuffer.data,
                                            internalSocket->receiveBuffer.length, 0);

            if(bytesReceived < 0) {
                if(UA_ERRNO == UA_WOULDBLOCK || UA_ERRNO == UA_EAGAIN || UA_ERRNO == UA_INTERRUPTED) {
                    return UA_STATUSCODE_GOOD;
                }
                UA_LOG_SOCKET_ERRNO_WRAP(
                    UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                 "Error while receiving data from socket: %s", errno_str));
                return UA_STATUSCODE_BADCOMMUNICATIONERROR;
            }
            if(bytesReceived == 0) {
                UA_LOG_INFO(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                            "Socket %i | Performing orderly shutdown", (int)sock->id);
                internalSocket->flaggedForDeletion = true;
                return UA_STATUSCODE_GOOD;
            }

            internalSocket->receiveBufferOut.length = (size_t)bytesReceived;

            return UA_Socket_DataCallback_call(sock, &internalSocket->receiveBufferOut);
        }
        break;
    }
    case UA_SOCKETSTATE_CLOSED: {
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_send(UA_Socket *sock, UA_ByteString *buffer) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;
    if(buffer->length > internalSocket->sendBuffer.length ||
       buffer->data != internalSocket->sendBuffer.data) {
        UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "sendBuffer length exceeds originally allocated length, "
                     "or the data pointer was modified.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(internalSocket->flaggedForDeletion)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    int flags = MSG_NOSIGNAL;

    size_t totalBytesSent = 0;

    do {
        ssize_t bytesSent = 0;
        do {
            bytesSent = UA_send((int)sock->id,
                                (const char *)buffer->data + totalBytesSent,
                                buffer->length - totalBytesSent, flags);
            if(bytesSent < 0 && UA_ERRNO != UA_EAGAIN && UA_ERRNO != UA_INTERRUPTED) {
                UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                             "Error while sending data over socket");
                return UA_STATUSCODE_BADCOMMUNICATIONERROR;
            }
        } while(bytesSent < 0);
        totalBytesSent += (size_t)bytesSent;
    } while(totalBytesSent < buffer->length);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_open(UA_Socket *sock) {
    /* nothing to do for server side data sockets */
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    sock->socketState = UA_SOCKETSTATE_OPEN;

    return UA_SocketCallback_call(sock->openCallback, sock);
}

static UA_StatusCode
UA_TCP_DataSocket_acquireSendBuffer(UA_Socket *sock, size_t bufferSize, UA_ByteString **p_buffer) {
    if(sock == NULL || p_buffer == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_Socket_tcpDataSocket *const internalSocket = (UA_Socket_tcpDataSocket *const)sock;

    if(bufferSize > internalSocket->sendBuffer.length)
        return UA_STATUSCODE_BADINTERNALERROR;

    internalSocket->sendBufferOut = internalSocket->sendBuffer;
    if(bufferSize > 0)
        internalSocket->sendBufferOut.length = bufferSize;
    *p_buffer = &internalSocket->sendBufferOut;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_releaseSendBuffer(UA_Socket *sock, UA_ByteString *buffer) {
    if(sock == NULL || buffer == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    /* Nothing to do here in this implementation since we reuse the buffer */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_TCP_DataSocket_disableNaglesAlgorithm(UA_Socket *sock) {
    int dummy = 1;
    if(UA_setsockopt((int)sock->id, IPPROTO_TCP, TCP_NODELAY,
                     (const char *)&dummy, sizeof(dummy)) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Cannot set socket option TCP_NODELAY. Error: %s",
                         errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static void
UA_TCP_DataSocket_logPeerName(UA_Socket *sock, struct sockaddr_storage *remote) {
#ifdef UA_getnameinfo
    /* Get the peer name for logging */
    char remote_name[100];
    int res = UA_getnameinfo((struct sockaddr *)remote,
                             sizeof(struct sockaddr_storage),
                             remote_name, sizeof(remote_name),
                             NULL, 0, NI_NUMERICHOST);
    if(res == 0) {
        UA_LOG_INFO(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                    "Socket %i | New connection over TCP from %s",
                    (int)sock->id, remote_name);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                                "Socket %i | New connection over TCP, "
                                                "getnameinfo failed with error: %s",
                                                (int)sock->id, errno_str));
    }
#else
    UA_LOG_INFO(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                "Socket %i | New connection over TCP",
                (int)sock->id);
#endif
}

static UA_StatusCode
UA_TCP_DataSocket_init(UA_UInt64 sockFd,
                       UA_UInt32 sendBufferSize,
                       UA_UInt32 recvBufferSize,
                       UA_NetworkManager *networkManager,
                       UA_Socket_tcpDataSocket *sock,
                       void *application) {
    if(sock == NULL || networkManager == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    sock->socket.application = application;
    sock->socket.isListener = false;
    sock->socket.id = (UA_UInt64)sockFd;
    sock->socket.close = UA_TCP_DataSocket_close;
    sock->socket.mayDelete = UA_TCP_DataSocket_mayDelete;
    sock->socket.free = UA_TCP_DataSocket_free;
    sock->socket.activity = UA_TCP_DataSocket_activity;
    sock->socket.send = UA_TCP_DataSocket_send;
    sock->socket.open = UA_TCP_DataSocket_open;
    sock->socket.acquireSendBuffer = UA_TCP_DataSocket_acquireSendBuffer;
    sock->socket.releaseSendBuffer = UA_TCP_DataSocket_releaseSendBuffer;
    sock->socket.networkManager = networkManager;
    sock->socket.socketState = UA_SOCKETSTATE_NEW;
    sock->socket.waitForReadActivity = true;
    sock->socket.waitForWriteActivity = false;
    sock->flaggedForDeletion = false;

    UA_StatusCode retval = UA_ByteString_allocBuffer(&sock->receiveBuffer, recvBufferSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate receive buffer for socket with error %s",
                     UA_StatusCode_name(retval));
        goto error;
    }
    sock->receiveBufferOut = sock->receiveBuffer;

    retval = UA_ByteString_allocBuffer(&sock->sendBuffer, sendBufferSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate receive buffer for socket with error %s",
                     UA_StatusCode_name(retval));
        goto error;
    }
    sock->sendBufferOut = sock->sendBuffer;

    return retval;
error:
    UA_ByteString_deleteMembers(&sock->receiveBuffer);
    UA_ByteString_deleteMembers(&sock->sendBuffer);
    return retval;
}

UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(const UA_SocketConfig *parameters, UA_SocketCallbackFunction const creationCallback) {
    if(parameters == NULL || parameters->additionalParameters == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Socket *listenerSocket = ((UA_TCP_DataSocket_AcceptFrom_AdditionalParameters *)parameters->additionalParameters)
        ->listenerSocket;
    if(listenerSocket == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_SOCKET newSockFd = UA_accept((UA_SOCKET)listenerSocket->id,
                                    (struct sockaddr *)&remote, &remote_size);
    if(newSockFd == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                           "Accept failed with %s", errno_str));
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Error accepting socket. Got invalid file descriptor");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_LOG_DEBUG(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "New TCP sock (fd: %i) accepted from listener socket (fd: %i)",
                 (int)newSockFd, (int)listenerSocket->id);

    UA_Socket_tcpDataSocket *const sock = (UA_Socket_tcpDataSocket *const)UA_malloc(sizeof(UA_Socket_tcpDataSocket));
    UA_StatusCode retval;
    if(sock == NULL) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate data socket internal data. Out of memory");
        UA_close(newSockFd);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(sock, 0, sizeof(UA_Socket_tcpDataSocket));

    retval = UA_TCP_DataSocket_init((UA_UInt64)newSockFd,
                                    parameters->sendBufferSize,
                                    parameters->recvBufferSize,
                                    parameters->networkManager,
                                    sock,
                                    listenerSocket->application);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate socket resources with error %s",
                     UA_StatusCode_name(retval));
        UA_close(newSockFd);
        UA_free(sock);
        return retval;
    }

    retval = UA_String_copy(&listenerSocket->discoveryUrl, &sock->socket.discoveryUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to copy discovery url while creating data socket");
        goto error;
    }

    retval = UA_socket_set_nonblocking(newSockFd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Encountered error %s while setting socket to non blocking.",
                     UA_StatusCode_name(retval));
        goto error;
    }

    retval = UA_TCP_DataSocket_disableNaglesAlgorithm((UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    UA_TCP_DataSocket_logPeerName((UA_Socket *)sock, &remote);

    retval = UA_SocketCallback_call(parameters->networkManagerCallback, (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Creation callback returned error %s.",
                     UA_StatusCode_name(retval));
        goto error;
    }

    retval = UA_SocketCallback_call(creationCallback, (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Creation callback returned error %s.",
                     UA_StatusCode_name(retval));
        goto error;
    }

    return retval;

error:
    if(sock != NULL) {
        sock->socket.close((UA_Socket *)sock);
        sock->socket.free((UA_Socket *)sock);
    }
    return retval;
}

#define UA_HOSTNAME_MAX_LENGTH 512

static UA_StatusCode
UA_TCP_ClientDataSocket_open(UA_Socket *sock) {
    if(sock == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Socket_tcpClientDataSocket *internalSocket = (UA_Socket_tcpClientDataSocket *)sock;

    internalSocket->dtTimeout = internalSocket->timeout * UA_DATETIME_MSEC;
    internalSocket->connStart = UA_DateTime_nowMonotonic();

    /* Non blocking connect */
    int error = UA_connect((UA_SOCKET)sock->id, internalSocket->server->ai_addr,
                           (socklen_t)internalSocket->server->ai_addrlen);

    if((error == -1) && (UA_ERRNO != UA_ERR_CONNECTION_PROGRESS)) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                           "Connection to %.*s failed with error: %s",
                           (int)internalSocket->endpointUrl.length,
                           internalSocket->endpointUrl.data, errno_str));
        retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
        goto error;
    }

#ifdef SO_NOSIGPIPE
    int val = 1;
    int sso_result = UA_setsockopt((UA_SOCKET)sock->id, SOL_SOCKET,
                                   SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if(sso_result < 0)
        UA_LOG_WARNING(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                       "Couldn't set SO_NOSIGPIPE");
#endif

    /* We want to wait for write activity since this signals
     * that the socket is connected and ready to send */
    sock->waitForWriteActivity = true;

    /* The open callback will be called later if the connection succeeds */
    return retval;

error:
    sock->close(sock);
    return retval;
}

static UA_StatusCode
UA_TCP_ClientDataSocket_free(UA_Socket *sock) {
    UA_Socket_tcpClientDataSocket *internalSocket = (UA_Socket_tcpClientDataSocket *)sock;
    UA_String_deleteMembers(&internalSocket->endpointUrl);
    if(internalSocket->server != NULL)
        UA_freeaddrinfo(internalSocket->server);
    return UA_TCP_DataSocket_free(sock);
}

UA_StatusCode
UA_TCP_ClientDataSocket(const UA_SocketConfig *socketParameters, UA_SocketCallbackFunction const creationCallback) {
    if(socketParameters == NULL || socketParameters->createSocket == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NetworkManager *networkManager = socketParameters->networkManager;
    UA_Socket_tcpClientDataSocket *const sock = (UA_Socket_tcpClientDataSocket *const)UA_malloc(
        sizeof(UA_Socket_tcpClientDataSocket));
    if(sock == NULL) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate data socket internal data. Out of memory");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(sock, 0, sizeof(UA_Socket_tcpClientDataSocket));

    UA_StatusCode retval = UA_TCP_DataSocket_init((UA_UInt64)UA_INVALID_SOCKET,
                                                  socketParameters->sendBufferSize,
                                                  socketParameters->recvBufferSize,
                                                  networkManager,
                                                  (UA_Socket_tcpDataSocket *)sock,
                                                  socketParameters->application);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to allocate socket resources with error %s",
                     UA_StatusCode_name(retval));
        UA_free(sock);
        return retval;
    }

    UA_String_copy(&((const UA_ClientSocketConfig *)socketParameters)->targetEndpointUrl, &sock->endpointUrl);
    sock->timeout = ((const UA_ClientSocketConfig *)socketParameters)->timeout;

    sock->socket.socket.open = UA_TCP_ClientDataSocket_open;
    sock->socket.socket.free = UA_TCP_ClientDataSocket_free;
    sock->socket.socket.mayDelete = UA_TCP_ClientDataSocket_mayDelete;

    UA_String hostnameString = UA_STRING_NULL;
    UA_String pathString = UA_STRING_NULL;
    UA_UInt16 port = 0;
    char hostname[UA_HOSTNAME_MAX_LENGTH];

    retval = UA_parseEndpointUrl(&sock->endpointUrl, &hostnameString,
                                 &port, &pathString);
    if(retval != UA_STATUSCODE_GOOD || hostnameString.length >= UA_HOSTNAME_MAX_LENGTH) {
        UA_LOG_WARNING(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                       "Server url is invalid: %.*s",
                       (int)sock->endpointUrl.length, sock->endpointUrl.data);

        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto error;
    }
    memcpy(hostname, hostnameString.data, hostnameString.length);
    hostname[hostnameString.length] = 0;

    if(port == 0) {
        port = 4840;
        UA_LOG_INFO(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                    "No port defined, using default port %d", port);
    }

    struct addrinfo hints;
    sock->server = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    char portStr[6];
    UA_snprintf(portStr, 6, "%d", port);
    int error = UA_getaddrinfo(hostname, portStr, &hints, &sock->server);
    if(error != 0 || sock->server == NULL) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(UA_LOG_WARNING(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                                    "DNS lookup of %s failed with error %s", hostname, errno_str));
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto error;
    }

    UA_SOCKET client_sockfd = UA_socket(sock->server->ai_family,
                                        sock->server->ai_socktype,
                                        sock->server->ai_protocol);
    if(client_sockfd == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                                "Could not create client socket: %s", errno_str));
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto error;
    }

    /* Non blocking connect to be able to timeout */
    retval = UA_socket_set_nonblocking(client_sockfd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                       "Could not set the client socket to non blocking");
        goto error;
    }

    sock->socket.socket.id = (UA_UInt64)client_sockfd;

    retval = UA_SocketCallback_call(socketParameters->networkManagerCallback, (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(socketParameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Creation callback returned error %s.",
                     UA_StatusCode_name(retval));
        goto after_nm_register_error;
    }

    retval = UA_SocketCallback_call(creationCallback, (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(socketParameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Creation callback returned error %s.",
                     UA_StatusCode_name(retval));
        goto after_nm_register_error;
    }
    return retval;

error:
    sock->socket.socket.close((UA_Socket *)sock);
    sock->socket.socket.free((UA_Socket *)sock);
    return retval;

after_nm_register_error:
    sock->socket.socket.close((UA_Socket *)sock);
    return retval;
}
