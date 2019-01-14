/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_types_generated_handling.h"
#include "ua_sockets.h"
#include "ua_types.h"

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

    HookListEntry *hookListEntry;
    LIST_FOREACH(hookListEntry, &sock->deletionHooks.list, pointers) {
        UA_SocketHook_call(hookListEntry->hook, sock);
    }

    UA_LOG_DEBUG(sock->logger, UA_LOGCATEGORY_NETWORK, "Freeing socket %i", (int)sock->id);

    UA_Socket_cleanupHooks(sock);
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

    sockData->receiveBufferOut.length = (size_t)bytesReceived;
    UA_LOG_DEBUG(sock->logger, UA_LOGCATEGORY_NETWORK,
                 "Received %i bytes", (int)bytesReceived);

    if(sock->dataCallback.callback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Socket_dataCallback(sock, &sockData->receiveBufferOut);

    return UA_STATUSCODE_GOOD;
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

UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(UA_Socket *listenerSocket, UA_Logger *logger, UA_UInt32 sendBufferSize,
                             UA_UInt32 recvBufferSize, HookList creationHooks, HookList deletionHooks,
                             UA_Socket_DataCallback dataCallback) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Socket *sock = (UA_Socket *)UA_malloc(sizeof(UA_Socket));
    if(sock == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Ran out of memory while creating data sock.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(sock, 0, sizeof(UA_Socket));

    TCPDataSocketData *internalData = (TCPDataSocketData *)UA_malloc(sizeof(TCPDataSocketData));
    if(internalData == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Ran out of memory while creating data socket internal data");
        UA_free(sock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sock->internalData = internalData;
    sock->dataCallback = dataCallback;

    retval = UA_Socket_addDeletionHooks(sock, deletionHooks);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;


    sock->logger = logger;
    internalData->flaggedForDeletion = false;

    retval = UA_ByteString_allocBuffer(&internalData->receiveBuffer, recvBufferSize);
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

    retval = UA_String_copy(&listenerSocket->discoveryUrl, &sock->discoveryUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to copy discovery url while creating data socket");
        goto error;
    }
    sock->isListener = false;

    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_SOCKET newsockfd = UA_accept((int)listenerSocket->id,
                                    (struct sockaddr *)&remote, &remote_size);
    if(newsockfd == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "Accept failed with %s", errno_str));
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Error accepting socket. Got invalid file descriptor");
        goto error;
    }
    UA_LOG_TRACE(logger, UA_LOGCATEGORY_NETWORK,
                 "New TCP sock (fd: %i) accepted from listener socket (fd: %i)",
                 newsockfd, (int)listenerSocket->id);

    retval = UA_socket_set_nonblocking(newsockfd);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Encountered error %s while setting socket to nonblocking.",
                     UA_StatusCode_name(retval));
        goto error;
    }

    sock->id = (UA_UInt64)newsockfd;

    retval = UA_TCP_DataSocket_disableNaglesAlgorithm(sock);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    UA_TCP_DataSocket_logPeerName(sock, &remote);

    sock->close = UA_TCP_DataSocket_close;
    sock->mayDelete = UA_TCP_DataSocket_mayDelete;
    sock->free = UA_TCP_DataSocket_free;
    sock->activity = UA_TCP_DataSocket_activity;
    sock->send = UA_TCP_DataSocket_send;
    sock->open = UA_TCP_DataSocket_open;
    sock->getSendBuffer = UA_TCP_DataSocket_getSendBuffer;

    HookListEntry *hookListEntry;
    LIST_FOREACH(hookListEntry, &creationHooks.list, pointers) {
        retval = UA_SocketHook_call(hookListEntry->hook, sock);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "Creation hook returned error %s. "
                         "Continuing to call other hooks before returning.",
                         UA_StatusCode_name(retval));
        }
    }

    return retval;

error:
    UA_Socket_cleanupHooks(sock);
    UA_String_deleteMembers(&sock->discoveryUrl);
    UA_ByteString_deleteMembers(&internalData->receiveBuffer);
    UA_ByteString_deleteMembers(&internalData->sendBuffer);
    UA_free(internalData);
    UA_free(sock);
    return retval;
}
