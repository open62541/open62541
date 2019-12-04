/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/plugin/socket.h>
#include <open62541/plugin/networking/sockets.h>
#include <libwebsockets.h>
#include <open62541/plugin/networkmanager.h>
#include <open62541/types_generated_handling.h>
#include "wss_private.h"
#include "open62541_queue.h"

typedef struct WSS_SendBufferEntry {
    UA_ByteString *buffer;
    SIMPLEQ_ENTRY(WSS_SendBufferEntry) next;
} WSS_SendBufferEntry;

typedef struct {
    UA_WSS_Socket socket;
    struct lws_context *lwsContext;
    struct lws *wsi;
    SIMPLEQ_HEAD(, WSS_SendBufferEntry) pendingBuffers;
} UA_Socket_wssDSock;

static UA_StatusCode
wss_dsock_open(UA_Socket *sock) {
//    UA_Socket_wssDSock *const internalSock = (UA_Socket_wssDSock *const)sock;

    if(sock->socketState != UA_SOCKETSTATE_NEW) {
        UA_LOG_ERROR(sock->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Calling open on already open socket not supported");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    sock->socketState = UA_SOCKETSTATE_OPEN;

    sock->waitForWriteActivity = false;
    sock->waitForReadActivity = true;

    return UA_SocketCallback_call(sock->openCallback, sock);
}

static UA_StatusCode
wss_dsock_close(UA_Socket *socket) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Closing wss data socket with id %lu", socket->id);
    UA_WSS_Socket *const internalSock = (UA_WSS_Socket *const)socket;
    internalSock->closeOnNextCallback = true;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
wss_dsock_mayDelete(UA_Socket *socket) {
    return socket->socketState == UA_SOCKETSTATE_CLOSED;
}

static UA_StatusCode
wss_dsock_free(UA_Socket *socket) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Deleting wss data socket with id %lu", socket->id);

    UA_StatusCode retval = UA_SocketCallback_call(socket->freeCallback, socket);

    UA_free(socket);
    return retval;
}

static UA_StatusCode
wss_dsock_activity(UA_Socket *sock, UA_Boolean readActivity, UA_Boolean writeActivity) {
    if(sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Socket_wssDSock *const internalSock = (UA_Socket_wssDSock *const)sock;
    struct lws_pollfd pollfd;
    pollfd.fd = (int)sock->id;
    pollfd.events = (short)((sock->waitForReadActivity ? LWS_POLLIN : 0) |
                            (sock->waitForWriteActivity ? LWS_POLLOUT : 0));
    pollfd.revents = (short)((readActivity ? LWS_POLLIN : 0) | (writeActivity ? LWS_POLLOUT : 0));

    lws_service_fd(internalSock->lwsContext, &pollfd);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
wss_dsock_send(UA_Socket *socket, UA_ByteString *buffer) {
    UA_Socket_wssDSock *const internalSock = (UA_Socket_wssDSock *const)socket;
    WSS_SendBufferEntry *const entry = (WSS_SendBufferEntry *const)UA_malloc(sizeof(WSS_SendBufferEntry));
    if(entry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    entry->buffer = buffer;

    SIMPLEQ_INSERT_TAIL(&internalSock->pendingBuffers, entry, next);

    lws_callback_on_writable(internalSock->wsi);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
wss_dsock_acquireSendBuffer(UA_Socket *socket, size_t bufferSize, UA_ByteString **p_buffer) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Acquiring wss data socket send buffer");
    *p_buffer = (UA_ByteString *)UA_malloc(sizeof(UA_ByteString));
    if(*p_buffer == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode retval = UA_ByteString_allocBuffer(*p_buffer, bufferSize + LWS_PRE);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Shrink buffer to exclude LWS padding */
    (*p_buffer)->length -= LWS_PRE;
    (*p_buffer)->data += LWS_PRE;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
wss_dsock_releaseSendBuffer(UA_Socket *socket, UA_ByteString *buffer) {
    UA_LOG_DEBUG(socket->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Releasing wss data socket send buffer");
    buffer->data -= LWS_PRE;
    UA_ByteString_deleteMembers(buffer);
    UA_free(buffer);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WSS_DSock_handleWritable(UA_WSS_Socket *socket) {
    UA_Socket_wssDSock *const internalSock = (UA_Socket_wssDSock *const)socket;
    do {
        WSS_SendBufferEntry *entry = SIMPLEQ_FIRST(&internalSock->pendingBuffers);
        if(!entry)
            break;

        int m = lws_write(internalSock->wsi, entry->buffer->data, entry->buffer->length,
                          LWS_WRITE_BINARY);
        if(m < (int)entry->buffer->length) {
            lwsl_err("ERROR %d writing to ws\n", m);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        wss_dsock_releaseSendBuffer((UA_Socket *)socket, entry->buffer);
        SIMPLEQ_REMOVE_HEAD(&internalSock->pendingBuffers, next);
        UA_free(entry);
    } while(!lws_send_pipe_choked(internalSock->wsi));

    // process remaining messages
    if(SIMPLEQ_FIRST(&internalSock->pendingBuffers)) {
        lws_callback_on_writable(internalSock->wsi);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WSS_DataSocket_AcceptFrom(UA_Socket *listenerSocket, const UA_SocketConfig *parameters,
                             void *additionalParameters,
                             const UA_SocketCallbackFunction creationCallback) {
    if(parameters == NULL || additionalParameters == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Socket_wssDSock *sock = NULL;
    if(parameters->networkManager != NULL) {
        sock = (UA_Socket_wssDSock *const)parameters->networkManager->allocateSocket(parameters->networkManager,
                                                                                       sizeof(UA_Socket_wssDSock));
    } else {
        sock = (UA_Socket_wssDSock *const)UA_malloc(sizeof(UA_Socket_wssDSock));
    }
    if(sock == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(sock, 0, sizeof(UA_Socket_wssDSock));

    UA_WSS_DataSocket_AcceptFrom_AdditionalParameters *extraParams =
        ((UA_WSS_DataSocket_AcceptFrom_AdditionalParameters *)additionalParameters);

    sock->socket.socket.id = extraParams->fd;
    sock->socket.socket.open = wss_dsock_open;
    sock->socket.socket.close = wss_dsock_close;
    sock->socket.socket.mayDelete = wss_dsock_mayDelete;
    sock->socket.socket.clean = wss_dsock_free;
    sock->socket.socket.activity = wss_dsock_activity;
    sock->socket.socket.send = wss_dsock_send;
    sock->socket.socket.acquireSendBuffer = wss_dsock_acquireSendBuffer;
    sock->socket.socket.releaseSendBuffer = wss_dsock_releaseSendBuffer;

    sock->socket.socket.networkManager = parameters->networkManager;
    sock->socket.socket.waitForWriteActivity = false;
    sock->socket.socket.waitForReadActivity = false;
    sock->socket.socket.isListener = false;
    sock->socket.socket.application = listenerSocket->application;
    sock->socket.closeOnNextCallback = false;
    sock->lwsContext = (struct lws_context *)extraParams->lwsContext;
    sock->wsi = (struct lws *)extraParams->wsi;
    SIMPLEQ_INIT(&sock->pendingBuffers);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(parameters->networkManager != NULL) {
        retval = parameters->networkManager->activateSocket(parameters->networkManager, (UA_Socket *)sock);
        if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(parameters->networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Error activating socket in network manager: %s",
                         UA_StatusCode_name(retval));
    }

    retval = extraParams->createCallback(extraParams->listenerSocket,
                                                  (UA_Socket *)sock);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return UA_SocketCallback_call(creationCallback, (UA_Socket *)sock);
}
