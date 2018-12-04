/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <ua_plugin_log.h>
#include <queue.h>
#include "ua_networkmanagers.h"

typedef struct SocketListEntry {
    UA_Socket *socket;
    LIST_ENTRY(SocketListEntry) pointers;
} SocketListEntry;

typedef struct {
    UA_Logger logger;
    LIST_HEAD(, SocketListEntry) sockets;
} SelectNMData;


static UA_StatusCode
select_nm_registerSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    SocketListEntry *socketListEntry = (SocketListEntry *)UA_malloc(sizeof(SocketListEntry));
    if(socketListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    socketListEntry->socket = socket;

    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;

    LIST_INSERT_HEAD(&internalData->sockets, socketListEntry, pointers);
    UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK, "Registered socket with id %lu", socket->id);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_unregisterSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;
    SocketListEntry *socketListEntry, *tmp;

    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets, pointers, tmp) {
        if(socketListEntry->socket == socket) {
            LIST_REMOVE(socketListEntry, pointers);
            UA_free(socketListEntry);
        }
    }
    UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK, "Unregistered socket with id %lu", socket->id);
    return UA_STATUSCODE_GOOD;
}

static UA_Int32
setFDSet(SelectNMData *nmData, fd_set *fdset) {
    FD_ZERO(fdset);
    UA_Int32 highestfd = 0;

    SocketListEntry *socketListEntry;
    LIST_FOREACH(socketListEntry, &nmData->sockets, pointers) {
        UA_fd_set(socketListEntry->socket->id, fdset);
        if((UA_Int32)socketListEntry->socket->id > highestfd)
            highestfd = (UA_Int32)socketListEntry->socket->id;
    }

    return highestfd;
}

static UA_StatusCode
select_nm_process(UA_NetworkManager *networkManager, UA_UInt16 timeout) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;

    /* Listen on open sockets (including the server) */
    fd_set fdset, errset;
    UA_Int32 highestfd = setFDSet(internalData, &fdset);
    setFDSet(internalData, &errset);
    struct timeval tmptv = {0, timeout * 1000};
    if(UA_select(highestfd + 1, &fdset, NULL, &errset, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(internalData->logger, UA_LOGCATEGORY_NETWORK,
                           "Socket select failed with %s", errno_str));
        // we will retry, so do not return bad
        return UA_STATUSCODE_GOOD;
    }

    /* Read from established sockets */
    SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets, pointers, e_tmp) {
        UA_Socket *const socket = socketListEntry->socket;
        if(!UA_fd_isset(socket->id, &errset) &&
           !UA_fd_isset(socket->id, &fdset)) {
            // Check deletion for all not selected sockets to clean them up if necessary.
            if(socket->mayDelete(socket)) {
                socket->free(socket);
                LIST_REMOVE(socketListEntry, pointers);
                UA_free(socketListEntry);
            }
            continue;
        }

        UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK,
                     "Activity on socket with id %lu",
                     socket->id);

        retval = socket->activity(socket);
        if (retval != UA_STATUSCODE_GOOD) {
            socket->close(socket);
        }

        // We check here for all selected sockets so sockets that are selected
        // but flagged for deletion still get the chance to receive data once more.
        if(socket->mayDelete(socket)) {
            socket->free(socket);
            LIST_REMOVE(socketListEntry, pointers);
            UA_free(socketListEntry);
        }
    }
    return retval;
}

static UA_StatusCode
select_nm_deleteMembers(UA_NetworkManager *networkManager) {
    SelectNMData *internalData = networkManager->internalData;
    // TODO: check return values
    UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK, "Deleting select based network manager");

    SocketListEntry *socketListEntry, *e_tmp;

    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets, pointers, e_tmp) {
        UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK,
                     "Removing remaining socket with id %lu", socketListEntry->socket->id);
        socketListEntry->socket->close(socketListEntry->socket);
        socketListEntry->socket->free(socketListEntry->socket);
        LIST_REMOVE(socketListEntry, pointers);
        UA_free(socketListEntry);
    }

    if(networkManager->internalData != NULL)
        UA_free(networkManager->internalData);

    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_SelectBasedNetworkManager(UA_NetworkManager **p_networkManager) {

    UA_NetworkManager *networkManager = UA_malloc(sizeof(UA_NetworkManager));
    if(networkManager == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(networkManager, 0, sizeof(UA_NetworkManager));

    networkManager->registerSocket = select_nm_registerSocket;
    networkManager->unregisterSocket = select_nm_unregisterSocket;
    networkManager->process = select_nm_process;
    networkManager->deleteMembers = select_nm_deleteMembers;

    networkManager->internalData = UA_malloc(sizeof(SelectNMData));
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;
    if(internalData == NULL) {
        UA_free(networkManager);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    *p_networkManager = networkManager;

    return UA_STATUSCODE_GOOD;
}

