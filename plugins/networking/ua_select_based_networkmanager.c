/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "open62541_queue.h"
#include "ua_plugin_log.h"
#include "ua_types_generated_handling.h"
#include "ua_networkmanagers.h"

typedef struct {
    const UA_Logger *logger;
    UA_SocketList sockets;
    size_t numListenerSockets;
} SelectNMData;


static UA_StatusCode
select_nm_registerSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    UA_SocketListEntry *socketListEntry = (UA_SocketListEntry *)UA_malloc(sizeof(UA_SocketListEntry));
    if(socketListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    socketListEntry->socket = socket;

    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;

    LIST_INSERT_HEAD(&internalData->sockets.list, socketListEntry, pointers);
    UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK, "Registered socket with id %lu", socket->id);
    if(socket->isListener)
        ++internalData->numListenerSockets;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_unregisterSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;
    UA_SocketListEntry *socketListEntry, *tmp;

    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets.list, pointers, tmp) {
        if(socketListEntry->socket == socket) {
            if(socket->isListener)
                --internalData->numListenerSockets;
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

    UA_SocketListEntry *socketListEntry;
    LIST_FOREACH(socketListEntry, &nmData->sockets.list, pointers) {
        UA_fd_set(socketListEntry->socket->id, fdset);
        if((UA_Int32)socketListEntry->socket->id > highestfd)
            highestfd = (UA_Int32)socketListEntry->socket->id;
    }

    return highestfd;
}

static UA_StatusCode
select_nm_process(UA_NetworkManager *networkManager, UA_UInt16 timeout) {
    if(networkManager == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;

    UA_LOG_TRACE(internalData->logger, UA_LOGCATEGORY_NETWORK,
                 "Processing sockets in select based network manager");

    fd_set fdset;
    // fd_set errset;
    UA_Int32 highestfd = setFDSet(internalData, &fdset);
    // setFDSet(internalData, &errset);
    struct timeval tmptv = {0, timeout * 1000};
    if(UA_select(highestfd + 1, &fdset, NULL, NULL, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK,
                         "Socket select failed with %s", errno_str));
        // we will retry, so do not return bad
        return UA_STATUSCODE_GOOD;
    }

    /* Read from established sockets */
    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets.list, pointers, e_tmp) {
        UA_Socket *const socket = socketListEntry->socket;
        if(!UA_fd_isset(socket->id, &fdset)) {
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
        if (retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
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
select_nm_processSocket(UA_NetworkManager *networkManager, UA_UInt32 timeout,
                        UA_Socket *sock) {
    if(networkManager == NULL || sock == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    fd_set fdset;
    FD_ZERO(&fdset);
    UA_fd_set(sock->id, &fdset);
    struct timeval tmptv = {0, timeout * 1000};

    int resultsize = UA_select((UA_Int32)(sock->id + 1), NULL, &fdset, NULL, &tmptv);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(resultsize == 1) {
        if(sock->mayDelete(sock)) {
            networkManager->unregisterSocket(networkManager, sock);
            sock->free(sock);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
        retval = sock->activity(sock);
        if (retval != UA_STATUSCODE_GOOD) {
            sock->close(sock);
            return retval;
        }
    }
    return retval;
}

static UA_StatusCode
select_nm_getDiscoveryUrls(const UA_NetworkManager *networkManager, UA_String *discoveryUrls[],
                           size_t *discoveryUrlsSize) {
    SelectNMData *const internalData = (SelectNMData *const)networkManager->internalData;

    UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK,
                 "Getting discovery urls from network manager");

    UA_String *const urls = (UA_String *const)UA_malloc(sizeof(UA_String) * internalData->numListenerSockets);
    if(urls == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t position = 0;
    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets.list, pointers, e_tmp) {
        if(socketListEntry->socket->isListener) {
            if(position >= internalData->numListenerSockets) {
                UA_LOG_ERROR(internalData->logger, UA_LOGCATEGORY_NETWORK,
                             "Mismatch between found listener sockets and maximum array size.");
                UA_free(urls);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            urls[position] = socketListEntry->socket->discoveryUrl;
            //UA_ByteString_copy(&socketListEntry->socket->discoveryUrl, &urls[position]);
            ++position;
        }
    }

    *discoveryUrls = urls;
    *discoveryUrlsSize = internalData->numListenerSockets;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_shutdown(UA_NetworkManager *networkManager) {
    if(networkManager == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    SelectNMData *const internalData = (SelectNMData *const)networkManager->internalData;
    UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK, "Shutting down network manager");

    UA_SocketListEntry *socketListEntry;
    LIST_FOREACH(socketListEntry, &internalData->sockets.list, pointers) {
        UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK,
                     "Closing remaining socket with id %lu", socketListEntry->socket->id);
        socketListEntry->socket->close(socketListEntry->socket);
    }

    networkManager->process(networkManager, 0);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_deleteMembers(UA_NetworkManager *networkManager) {
    SelectNMData *const internalData = (SelectNMData *const)networkManager->internalData;
    // TODO: check return values
    UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK, "Deleting select based network manager");

    networkManager->shutdown(networkManager);

    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalData->sockets.list, pointers, e_tmp) {
        UA_LOG_DEBUG(internalData->logger, UA_LOGCATEGORY_NETWORK,
                     "Removing remaining socket with id %lu", socketListEntry->socket->id);
        socketListEntry->socket->free(socketListEntry->socket);
        LIST_REMOVE(socketListEntry, pointers);
        UA_free(socketListEntry);
    }

    if(networkManager->internalData != NULL)
        UA_free(networkManager->internalData);

    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_SelectBasedNetworkManager(const UA_Logger *logger, UA_NetworkManager *networkManager) {
    if(networkManager == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    memset(networkManager, 0, sizeof(UA_NetworkManager));
    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK, "Setting up select based network manager");

    networkManager->registerSocket = select_nm_registerSocket;
    networkManager->unregisterSocket = select_nm_unregisterSocket;
    networkManager->process = select_nm_process;
    networkManager->processSocket = select_nm_processSocket;
    networkManager->getDiscoveryUrls = select_nm_getDiscoveryUrls;
    networkManager->shutdown = select_nm_shutdown;
    networkManager->deleteMembers = select_nm_deleteMembers;

    networkManager->internalData = UA_malloc(sizeof(SelectNMData));
    SelectNMData *internalData = (SelectNMData *)networkManager->internalData;
    if(internalData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(internalData, 0, sizeof(SelectNMData));

    internalData->logger = logger;

    return UA_STATUSCODE_GOOD;
}

