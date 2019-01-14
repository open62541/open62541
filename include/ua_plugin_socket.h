/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */


#ifndef OPEN62541_UA_PLUGIN_SOCKET_H
#define OPEN62541_UA_PLUGIN_SOCKET_H

#include "open62541_queue.h"
#include "ua_types.h"
#include "ua_plugin_log.h"

typedef struct UA_Socket UA_Socket;
typedef struct UA_SocketHook UA_SocketHook;
typedef struct UA_SocketConfig UA_SocketConfig;
typedef struct UA_SocketFactory UA_SocketFactory;

/**
     * The signature of the dataCallback that needs to be implemented.
     *
     * @param callbackContext The context set by the callback owner.
     * @param data the data buffer the socket received the data to.
     *             Data in this buffer will be lost after the call returns.
     * @param socket the socket that the data was received on. TODO: do we need this param?
     */
typedef UA_StatusCode (*UA_Socket_dataCallbackFunction)(void *callbackContext,
                                                        UA_ByteString *data,
                                                        UA_Socket *socket);

typedef struct {

    /**
     * The data callback will be called by the socket if data is available.
     * The data buffer is passed to the function and will be cleaned up after the callback returns.
     */
    UA_Socket_dataCallbackFunction callback;

    /**
     * This context is set by the callback owner. It can contain any kind of data that is
     * needed in the callback when it is called.
     */
    void *callbackContext;
} UA_Socket_DataCallback;

typedef struct UA_SocketListEntry {
    UA_Socket *socket;
    LIST_ENTRY(UA_SocketListEntry) pointers;
} UA_SocketListEntry;

typedef struct UA_SocketList {
    LIST_HEAD(, UA_SocketListEntry) list;
} UA_SocketList;

typedef UA_StatusCode (*UA_SocketHookFunction)(void *, UA_Socket *);

struct UA_SocketHook {
    UA_SocketHookFunction hook;

    void *hookContext;
};

typedef struct HookListEntry {
    UA_SocketHook hook;
    LIST_ENTRY(HookListEntry) pointers;
} HookListEntry;

/**
 * Convenience alias for passing the list as parameter.
 */
typedef struct HookList {
    LIST_HEAD(, HookListEntry) list;
} HookList;

struct UA_Socket {
    /**
     * The socket id. Used by the NetworkManager to map to an internal representation (e.g. file descriptor)
     */
    UA_UInt64 id;

    /**
     * These hooks are called when the socket is deleted.
     */
    HookList deletionHooks;

    UA_Logger *logger;

    /**
     * The discovery url that can be used to connect to the server on this socket.
     * Data sockets have discovery urls as well, because it needs to be checked,
     * if the discovery url in a hello message is the same as the one used to connect
     * to the listener socket. That means the discovery url is inherited
     * from the listener socket.
     */
    UA_String discoveryUrl;

    /**
     * This flag indicates if the socket is a listener socket that accepts new connections.
     */
    UA_Boolean isListener;

    /**
     * If the socket is able to create new sockets (e.g. by accepting),
     * the socketFactory is used to create the child sockets.
     * The factory may be NULL, in which case no new sockets will be created,
     * even if it were possible.
     */
    UA_SocketFactory *socketFactory;

    /**
     * Starts/Opens the socket for operation. This step is separate from the initialization
     * so the sockets can be configured without starting to listen already.
     *
     * @param socket the socket to perform the operation on.
     * @return
     */
    UA_StatusCode (*open)(UA_Socket *socket);

    /**
     * Closes the socket. This typically also signals the mayDelete function to return true,
     * indicating that the socket can be safely deleted on the next NetworkManager iteration.
     * @param socket the socket to perform the operation on.
     * @return
     */
    UA_StatusCode (*close)(UA_Socket *socket);

    /**
     * Checks if the socket can be deleted because it has been closed by the local application,
     * or if it was closed remotely.
     * @param socket the socket to perform the operation on.
     * @return true, if the socket can be deleted, false otherwise.
     */
    UA_Boolean (*mayDelete)(UA_Socket *socket);

    /**
     * This function deletes the socket and frees all resources allocated by it.
     * After calling this function the behavior for all following calls is undefined.
     *
     * Because a socket might be kept in several places, the deleteMembers function
     * will call all registered deletionHooks so that the references to this
     * socket can be properly cleaned up.
     *
     * @param socket
     * @return
     */
    UA_StatusCode (*free)(UA_Socket *socket);

    /**
     * This function can be called to process data pending on the socket.
     * Normally it should only be called once the socket is available
     * for writing or reading (e.g. after it was selected by a select call).
     *
     * Internally depending on the implementation a callback may be called
     * if there was data that needs to be further processed by the application.
     *
     * Listener sockets will typically create a new socket and call the
     * appropriate creation hooks.
     *
     * @param socket The socket to perform the operation on.
     * @return
     */
    UA_StatusCode (*activity)(UA_Socket *socket);

    /**
     * The dataCallback is called by the socket once it has sufficient data
     * that it can pass on to be processed.
     * The data ByteString will be reused or deallocated once the callback returns.
     * If the data is needed beyond the call, it needs to be copied, otherwise
     * it will be lost.
     */
    UA_Socket_DataCallback dataCallback;

    /**
     * Sends the data contained in the send buffer. The data in the buffer is lost
     * after calling send.
     * Always call getSendBuffer to get a buffer and write the data to that buffer.
     * The length needs to be set to the amount of bytes to send.
     * The length may not exceed the originally allocated length.
     * @param socket the socket to perform the operation on.
     * @return
     */
    UA_StatusCode (*send)(UA_Socket *socket);

    /**
     * This function can be used to get a send buffer from the socket implementation.
     * To send data, directly write to this buffer. Calling send, will send the
     * contained data. The length of the buffer determines the bytes sent.
     *
     * @param socket the socket to perform the operation on.
     * @param buffer the pointer the the allocated buffer
     * @return
     */
    UA_StatusCode (*getSendBuffer)(UA_Socket *socket, size_t bufferSize, UA_ByteString **p_buffer);

    /**
     * This opaque pointer points to implementation specific internal data.
     * It is initialized and allocated when a new socket is created and deallocated
     * when the socket is deleted.
     */
    void *internalData;
};

static inline UA_StatusCode
UA_Socket_addDeletionHook(UA_Socket *sock, UA_SocketHook hook) {
    HookListEntry *hookListEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
    if(hookListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    hookListEntry->hook = hook;

    LIST_INSERT_HEAD(&sock->deletionHooks.list, hookListEntry, pointers);
    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_Socket_cleanupHooks(UA_Socket *sock) {
    HookListEntry *hookListEntry;
    HookListEntry *tmp;
    LIST_FOREACH_SAFE(hookListEntry, &sock->deletionHooks.list, pointers, tmp) {
        LIST_REMOVE(hookListEntry, pointers);
        UA_free(hookListEntry);
    }

    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_Socket_addDeletionHooks(UA_Socket *sock, HookList deletionHooks) {
    HookListEntry *hookListEntry = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_StatusCode lastError = UA_STATUSCODE_GOOD;
    LIST_FOREACH(hookListEntry, &deletionHooks.list, pointers) {
        lastError = UA_Socket_addDeletionHook(sock, hookListEntry->hook);
        if(lastError != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(sock->logger, UA_LOGCATEGORY_NETWORK,
                           "Failed to add deletion hook to socket: %s",
                           UA_StatusCode_name(lastError));
            retval = lastError;
        }
    }

    return retval;
}


/**
 * Configuration parameters for sockets created at startup.
 */
struct UA_SocketConfig {
    UA_UInt32 recvBufferSize;
    UA_UInt32 sendBufferSize;
    UA_UInt16 port;
    UA_Logger *logger;
    UA_ByteString customHostname;

    /**
     * This function is called by the server to create all configured listener sockets.
     * The delayed configuration makes sure, that initialization is done during
     * server startup. Also, only the server will have ownership of the Sockets.
     * @param config
     * @param socketHook The socketHook is called once for each socket that is created.
     */
    UA_StatusCode (*createSocket)(UA_SocketConfig *config, UA_SocketHook socketHook);
};

/**
 * Convenience wrapper for calling socket hooks.
 * Does a sanity check before calling the hook.
 *
 * @param hook the hook to call
 * @param sock the socket parameter of the hook.
 */
static inline UA_StatusCode
UA_SocketHook_call(UA_SocketHook hook, UA_Socket *sock) {
    if(hook.hook == NULL || sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return hook.hook(hook.hookContext, sock);
}

struct UA_SocketFactory {
    HookList creationHooks;
    HookList deletionHooks;

    UA_Logger *logger;

    /**
     * This function can be used to build a socket.
     * After the socket is built, the creation hooks are called and the socket is also passed
     * the deletion hooks, which it will then later call when it is deleted, in order to perform
     * proper cleanup.
     *
     * @param factory the factory to perform the operation on.
     * @param listenerSocket the socket the DataSocket is created from (accepted from)
     * @param additionalData Any data that needs to be passed from listener to data sockets.
     * @return
     */
    UA_StatusCode (*buildSocket)(UA_SocketFactory *factory, UA_Socket *listenerSocket,
                                 void *additionalData);

    UA_Socket_DataCallback socketDataCallback;
};

static inline UA_StatusCode
UA_SocketFactory_addCreationHook(UA_SocketFactory *factory, UA_SocketHook hook) {
    HookListEntry *hookListEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
    if(hookListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    hookListEntry->hook = hook;

    LIST_INSERT_HEAD(&factory->creationHooks.list, hookListEntry, pointers);
    UA_LOG_TRACE(factory->logger, UA_LOGCATEGORY_NETWORK, "Added deletion hook.");
    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_SocketFactory_addDeletionHook(UA_SocketFactory *factory, UA_SocketHook hook) {
    HookListEntry *hookListEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
    if(hookListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    hookListEntry->hook = hook;

    LIST_INSERT_HEAD(&factory->deletionHooks.list, hookListEntry, pointers);
    UA_LOG_TRACE(factory->logger, UA_LOGCATEGORY_NETWORK, "Added deletion hook");
    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_SocketFactory_init(UA_SocketFactory *factory, UA_Logger *logger) {
    memset(factory, 0, sizeof(UA_SocketFactory));
    factory->logger = logger;

    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_SocketFactory_deleteMembers(UA_SocketFactory *factory) {
    HookListEntry *hookListEntry;
    HookListEntry *tmp;
    LIST_FOREACH_SAFE(hookListEntry, &factory->creationHooks.list, pointers, tmp) {
        LIST_REMOVE(hookListEntry, pointers);
        UA_free(hookListEntry);
    }
    LIST_FOREACH_SAFE(hookListEntry, &factory->deletionHooks.list, pointers, tmp) {
        LIST_REMOVE(hookListEntry, pointers);
        UA_free(hookListEntry);
    }

    return UA_STATUSCODE_GOOD;
}


/**
 * Convenience Wrapper for calling the dataCallback of a socket.
 */
static inline UA_StatusCode
UA_Socket_dataCallback(UA_Socket *socket, UA_ByteString *data) {
    if (socket == NULL || socket->dataCallback.callback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return socket->dataCallback.callback(socket->dataCallback.callbackContext, data, socket);
}

#endif //OPEN62541_UA_PLUGIN_SOCKET_H
