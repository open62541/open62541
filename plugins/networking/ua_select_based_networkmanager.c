/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018-2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#include <open62541/plugin/log.h>
#include <open62541/types_generated_handling.h>
#include <open62541/plugin/networking/networkmanagers.h>
#include <open62541/plugin/networkmanager.h>
#include <open62541/plugin/socket.h>
#include "open62541_queue.h"

typedef struct UA_SocketListEntry {
    UA_Socket *socket;
    LIST_ENTRY(UA_SocketListEntry) pointers;
} UA_SocketListEntry;

typedef enum {
    UA_NETWORKMANAGER_NEW,
    UA_NETWORKMANAGER_RUNNING,
    UA_NETWORKMANAGER_SHUTDOWN,
} UA_NetworkManager_State;

typedef struct {
    UA_NetworkManager baseManager;
    LIST_HEAD(, UA_SocketListEntry) sockets;
    size_t numListenerSockets;
    size_t numSockets;
    UA_NetworkManager_State state;
} UA_NetworkManager_selectBased;

static void *
select_nm_createSocket(UA_NetworkManager *networkManager, size_t socketSize) {
    UA_NetworkManager_selectBased *const internalManager = (UA_NetworkManager_selectBased *const)networkManager;
    if(internalManager->state != UA_NETWORKMANAGER_RUNNING) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Cannot create socket on uninitialized or shutdown network manager");
        return NULL;
    }
    UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Allocating new socket in network manager");

    if(internalManager->numSockets >= FD_SETSIZE) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "The select based network manager cannot handle "
                     "more than %i concurrent connections", FD_SETSIZE);
        return NULL;
    }
    UA_SocketListEntry *socketListEntry = (UA_SocketListEntry *)UA_malloc(sizeof(UA_SocketListEntry) + socketSize);
    if(socketListEntry == NULL)
        return NULL;

    LIST_INSERT_HEAD(&internalManager->sockets, (UA_SocketListEntry *)socketListEntry, pointers);
    ++internalManager->numSockets;

    return socketListEntry + 1;
}

static UA_StatusCode
select_nm_activateSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    UA_NetworkManager_selectBased *const internalManager = (UA_NetworkManager_selectBased *const)networkManager;
    UA_SocketListEntry *desired_entry = ((UA_SocketListEntry *)socket) - 1;

    UA_SocketListEntry *socketListEntry;
    // TODO: We can avoid iteration by simply assuming that the pointer has a list entry prepended.
    // TODO: This removes safety, as other developers might assume that the activate function works for any kind of
    // TODO: socket. I prefer this variant, since externally allocated sockets will simply be ignored without incident.
    LIST_FOREACH(socketListEntry, &internalManager->sockets, pointers) {
        if(socketListEntry == desired_entry) {
            socketListEntry->socket = socket;
            UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Activated socket with id %i",
                         (int)socket->id);
            if(socket->isListener)
                ++internalManager->numListenerSockets;

            return UA_STATUSCODE_GOOD;
        }
    }

    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
select_nm_removeSocket(UA_NetworkManager *networkManager, UA_Socket *socket) {
    UA_NetworkManager_selectBased *const internalManager = (UA_NetworkManager_selectBased *const)networkManager;
    UA_SocketListEntry *socketListEntry, *tmp;

    LIST_FOREACH_SAFE(socketListEntry, &internalManager->sockets, pointers, tmp) {
        if(socketListEntry->socket == socket) {
            if(socket->isListener)
                --internalManager->numListenerSockets;
            --internalManager->numSockets;
            LIST_REMOVE(socketListEntry, pointers);
            UA_free(socketListEntry);
        }
    }
    UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Removed socket with id %i",
                 (int)socket->id);
    return UA_STATUSCODE_GOOD;
}

static UA_Int32
setFDSet(UA_NetworkManager_selectBased *networkManager, fd_set *readfdset, fd_set *writefdset) {
    FD_ZERO(readfdset);
    FD_ZERO(writefdset);
    UA_Int32 highestfd = -1;

    UA_SocketListEntry *socketListEntry;
    LIST_FOREACH(socketListEntry, &networkManager->sockets, pointers) {
        if(socketListEntry->socket->waitForWriteActivity)
            UA_fd_set((UA_SOCKET)socketListEntry->socket->id, writefdset);
        if(socketListEntry->socket->waitForReadActivity)
            UA_fd_set((UA_SOCKET)socketListEntry->socket->id, readfdset);
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
    UA_NetworkManager_selectBased *const internalManager = (UA_NetworkManager_selectBased *const)networkManager;

    fd_set readfdset, writefdset;
    UA_Int32 highestfd = setFDSet(internalManager, &readfdset, &writefdset);
    long int secs = timeout / 1000;
    long int microsecs = (timeout * 1000) % 1000000;
    struct timeval tmptv = {secs, microsecs};
    if(highestfd < 0)
        return UA_STATUSCODE_GOOD;
    if(UA_select(highestfd + 1, &readfdset, &writefdset, NULL, &tmptv) < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                         "Socket select failed with %s", errno_str));
        // we will retry, so do not return bad
        return UA_STATUSCODE_GOOD;
    }

    /* Read from established sockets and check if sockets can be cleaned up */
    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalManager->sockets, pointers, e_tmp) {
        UA_Socket *const socket = socketListEntry->socket;
        if(socket == NULL) {
            UA_LOG_WARNING(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                           "Processing non activated socket. Make sure to activate sockets after allocating them."
                           " Removing it now.");
            LIST_REMOVE(socketListEntry, pointers);
            UA_free(socketListEntry);
            --internalManager->numSockets;
        }
        UA_Boolean readActivity = UA_fd_isset((UA_SOCKET)socket->id, &readfdset);
        UA_Boolean writeActivity = UA_fd_isset((UA_SOCKET)socket->id, &writefdset);
        if(!readActivity && !writeActivity) {
            // Check deletion for all not selected sockets to clean them up if necessary.
            if(socket->mayDelete(socket)) {
                if(socket->isListener)
                    --internalManager->numListenerSockets;
                --internalManager->numSockets;
                socket->clean(socket);
                LIST_REMOVE(socketListEntry, pointers);
                UA_free(socketListEntry);
            }
            continue;
        }

        UA_LOG_TRACE(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Activity on socket with id %i",
                     (int)socket->id);

        retval = socket->activity(socket, readActivity, writeActivity);
        if (retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
            socket->close(socket);
        }

        // We check here for all selected sockets so sockets that are selected
        // but flagged for deletion still get the chance to receive data once more.
        if(socket->mayDelete(socket)) {
            if(socket->isListener)
                --internalManager->numListenerSockets;
            --internalManager->numSockets;
            socket->clean(socket);
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
    fd_set readfdset;
    FD_ZERO(&readfdset);
    fd_set writefdset;
    FD_ZERO(&writefdset);

    if(sock->waitForWriteActivity)
        UA_fd_set((UA_SOCKET)sock->id, &writefdset);
    if(sock->waitForReadActivity)
        UA_fd_set((UA_SOCKET)sock->id, &readfdset);

    long int secs = (long int)timeout / 1000;
    long int microsecs = ((long int)timeout * 1000) % 1000000;
    struct timeval tmptv = {secs, microsecs};

    int resultsize = UA_select((UA_Int32)(sock->id + 1), &readfdset, &writefdset, NULL, &tmptv);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(resultsize == 1) {
        if(sock->mayDelete(sock)) {
            select_nm_removeSocket(networkManager, sock);
            sock->clean(sock);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
        UA_Boolean readActivity = UA_fd_isset((UA_SOCKET)sock->id, &readfdset);
        UA_Boolean writeActivity = UA_fd_isset((UA_SOCKET)sock->id, &writefdset);
        retval = sock->activity(sock, readActivity, writeActivity);
        if (retval != UA_STATUSCODE_GOOD) {
            sock->close(sock);
            return retval;
        }
    }
    if(resultsize == 0) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK, "Socket select timed out");
        return UA_STATUSCODE_BADTIMEOUT;
    }
    if(resultsize == -1) {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                                              "Socket select failed with %s", errno_str));
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
select_nm_getDiscoveryUrls(const UA_NetworkManager *networkManager, UA_String *discoveryUrls[],
                           size_t *discoveryUrlsSize) {
    const UA_NetworkManager_selectBased *const internalManager =
        (const UA_NetworkManager_selectBased *const)networkManager;

    UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                 "Getting discovery urls from network manager");

    UA_String *const urls = (UA_String *const)UA_malloc(sizeof(UA_String) * internalManager->numListenerSockets);
    if(urls == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t position = 0;
    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalManager->sockets, pointers, e_tmp) {
        if(socketListEntry->socket->isListener) {
            if(position >= internalManager->numListenerSockets) {
                UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK,
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
    *discoveryUrlsSize = internalManager->numListenerSockets;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_start(UA_NetworkManager *networkManager) {
    if(networkManager == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_NetworkManager_selectBased *const internalManager =
        (UA_NetworkManager_selectBased *const)networkManager;
    UA_LOG_INFO(networkManager->logger, UA_LOGCATEGORY_NETWORK, "Starting network manager");
    UA_initialize_architecture_network();
    internalManager->state = UA_NETWORKMANAGER_RUNNING;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_shutdown(UA_NetworkManager *networkManager) {
    if(networkManager == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_NetworkManager_selectBased *const internalManager =
        (UA_NetworkManager_selectBased *const)networkManager;

    if(internalManager->state == UA_NETWORKMANAGER_NEW) {
        UA_LOG_ERROR(networkManager->logger, UA_LOGCATEGORY_NETWORK, "Cannot call shutdown before start");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(internalManager->state == UA_NETWORKMANAGER_SHUTDOWN)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(networkManager->logger, UA_LOGCATEGORY_NETWORK, "Shutting down network manager");

    UA_SocketListEntry *socketListEntry;
    LIST_FOREACH(socketListEntry, &internalManager->sockets, pointers) {
        UA_LOG_INFO(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                    "Closing remaining socket with id %i", (int)socketListEntry->socket->id);
        socketListEntry->socket->close(socketListEntry->socket);
    }

    networkManager->process(networkManager, 0);

    internalManager->state = UA_NETWORKMANAGER_SHUTDOWN;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
select_nm_free(UA_NetworkManager *networkManager) {
    if(networkManager == NULL)
        return UA_STATUSCODE_GOOD;

    UA_NetworkManager_selectBased *const internalManager = (UA_NetworkManager_selectBased *const)networkManager;
    // TODO: check return values
    UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK, "Deleting select based network manager");

    if(internalManager->state != UA_NETWORKMANAGER_SHUTDOWN)
        networkManager->shutdown(networkManager);

    UA_SocketListEntry *socketListEntry, *e_tmp;
    LIST_FOREACH_SAFE(socketListEntry, &internalManager->sockets, pointers, e_tmp) {
        UA_LOG_DEBUG(networkManager->logger, UA_LOGCATEGORY_NETWORK,
                     "Removing remaining socket with id %i", (int)socketListEntry->socket->id);
        socketListEntry->socket->clean(socketListEntry->socket);
        LIST_REMOVE(socketListEntry, pointers);
        UA_free(socketListEntry);
    }

    UA_free(networkManager);

    UA_deinitialize_architecture_network();

    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_SelectBasedNetworkManager(const UA_Logger *logger, UA_NetworkManager **p_networkManager) {
    if(p_networkManager == NULL || logger == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_NetworkManager_selectBased *const networkManager =
        (UA_NetworkManager_selectBased *const)UA_malloc(sizeof(UA_NetworkManager_selectBased));
    if(networkManager == NULL) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
            "Could not allocate NetworkManager: Out of memory");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memset(networkManager, 0, sizeof(UA_NetworkManager_selectBased));
    UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK, "Setting up select based network manager");

    networkManager->baseManager.allocateSocket = select_nm_createSocket;
    networkManager->baseManager.activateSocket = select_nm_activateSocket;
    networkManager->baseManager.process = select_nm_process;
    networkManager->baseManager.processSocket = select_nm_processSocket;
    networkManager->baseManager.getDiscoveryUrls = select_nm_getDiscoveryUrls;
    networkManager->baseManager.start = select_nm_start;
    networkManager->baseManager.shutdown = select_nm_shutdown;
    networkManager->baseManager.free = select_nm_free;
    networkManager->baseManager.logger = logger;

    networkManager->numListenerSockets = 0;
    networkManager->numSockets = 0;
    networkManager->state = UA_NETWORKMANAGER_NEW;

    *p_networkManager = (UA_NetworkManager *)networkManager;
    return UA_STATUSCODE_GOOD;
}

