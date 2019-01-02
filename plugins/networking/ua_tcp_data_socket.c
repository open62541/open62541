/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_types_generated_handling.h"
#include "ua_sockets.h"
#include "ua_types.h"

typedef struct {
    HookList deletionHooks;
    UA_Logger *logger;
} TCPDataSocketData;


UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(UA_Socket *listenerSocket, UA_Socket **p_socket,
                             HookList creationHooks, HookList deletionHooks,
                             UA_Logger *logger) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_Socket *socket = (UA_Socket *)UA_malloc(sizeof(UA_Socket));
    if(socket == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Ran out of memory while creating data socket.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(socket, 0, sizeof(UA_Socket));

    TCPDataSocketData *internalData = (TCPDataSocketData *)UA_malloc(sizeof(TCPDataSocketData));
    if(internalData == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Ran out of memory while creating data socket internal data");
        UA_free(socket);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    socket->internalData = internalData;

    // TODO: do we need to copy this here? or can we just copy the head pointer?
    internalData->deletionHooks = deletionHooks;
    /*
    HookListEntry *hookListEntry = NULL;
    LIST_FOREACH(hookListEntry, &creationHooks.list, pointers) {
        HookListEntry *newEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
        newEntry->hook = hookListEntry->hook;
        LIST_INSERT_HEAD(&internalData->deletionHooks.list, newEntry, pointers);
    }
    */

    internalData->logger = logger;

    retval = UA_String_copy(&listenerSocket->discoveryUrl, &socket->discoveryUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Failed to copy discovery url while creating data socket");
        goto error;
    }
    socket->isListener = false;

    struct sockaddr_storage remote;
    socklen_t remote_size = sizeof(remote);
    UA_SOCKET newsockfd = UA_accept((int)listenerSocket->id,
                                    (struct sockaddr *)&remote, &remote_size);
    if(newsockfd == UA_INVALID_SOCKET) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "Error accepting socket. Got invalid file descriptor");
        goto error;
    }
    UA_LOG_TRACE(logger, UA_LOGCATEGORY_NETWORK,
                 "New TCP socket (fd: %i) accepted from listener socket (fd: %i)",
                 newsockfd, (int)listenerSocket->id);

    socket->id = (UA_UInt64)newsockfd;

    *p_socket = socket;

    return UA_STATUSCODE_GOOD;

error:
    UA_free(internalData);
    UA_free(socket);
    return retval;
}
