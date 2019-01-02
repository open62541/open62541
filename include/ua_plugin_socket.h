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

typedef struct {
    /**
     * The signature of the dataCallback that needs to be implemented.
     *
     * @param socket the socket that the data was received on.
     * @param data the data buffer the socket received the data to.
     *             Data in this buffer will be lost after the call returns.
     * @param callbackContext The context set by the callback owner.
     */
    UA_StatusCode (*callback)(UA_Socket *socket, UA_ByteString *data, void *callbackContext);

    /**
     * This context is set by the callback owner. It can contain any kind of data that is
     * needed in the callback when it is called.
     */
    void *callbackContext;
} UA_Socket_DataCallback;

struct UA_Socket {
    /**
     * The socket id. Used by the NetworkManager to map to an internal representation (e.g. file descriptor)
     */
    UA_UInt64 id;

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
     * Sends the data contained in the data buffer. The buffer will be deallocated
     * by the socket.
     * @param socket the socket to perform the operation on.
     * @param data the pointer to the data buffer that contains the data to send.
     *             The length of the buffer is the amount of bytes that will be sent.
     * @return
     */
    UA_StatusCode (*send)(UA_Socket *socket, UA_ByteString *data);

    /**
     * This function can be used to get a send buffer from the socket implementation.
     * Directly writing to this buffer and passing the pointer to the socket might
     * be faster on some implementations than allocating an own buffer and passing
     * it to the socket.
     * It is advised to always allocate the send buffer with this function and pass
     * that buffer to the send function.
     *
     * @param socket the socket to perform the operation on.
     * @param buffer the pointer the the allocated buffer
     * @return
     */
    UA_StatusCode (*getSendBuffer)(UA_Socket *socket, UA_ByteString **p_buffer);

    /**
     * This opaque pointer points to implementation specific internal data.
     * It is initialized and allocated when a new socket is created and deallocated
     * when the socket is deleted.
     */
    void *internalData;
};

typedef struct {
    UA_StatusCode (*hook)(UA_Socket *socket, void *hookContext);

    void *hookContext;
} UA_SocketHook;

typedef struct UA_DataSocketFactory UA_DataSocketFactory;

typedef struct HookListEntry {
    UA_SocketHook hook;
    LIST_ENTRY(HookListEntry) pointers;
} HookListEntry;

struct UA_DataSocketFactory {
    LIST_HEAD(, HookListEntry) creationHooks;
    LIST_HEAD(, HookListEntry) deletionHooks;

    UA_Logger *logger;

    /**
     * This function can be used to build a socket.
     * After the socket is built, the creation hooks are called and the socket is also passed
     * the deletion hooks, which it will then later call when it is deleted, in order to perform
     * proper cleanup.
     *
     * @param factory the factory to perform the operation on.
     * @param listenerSocket the socket the DataSocket is created from (accepted from)
     * @return
     */
    UA_StatusCode (*buildSocket)(UA_DataSocketFactory *factory, UA_Socket *listenerSocket);

    UA_Socket_DataCallback socketDataCallback;
};

static inline UA_StatusCode
UA_DataSocketFactory_addCreationHook(UA_DataSocketFactory *factory, UA_SocketHook hook) {
    HookListEntry *hookListEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
    if(hookListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    hookListEntry->hook = hook;

    LIST_INSERT_HEAD(&factory->creationHooks, hookListEntry, pointers);
    UA_LOG_TRACE(factory->logger, UA_LOGCATEGORY_NETWORK, "Added deletion hook.");
    return UA_STATUSCODE_GOOD;
}

static inline UA_StatusCode
UA_DataSocketFactory_addDeletionHook(UA_DataSocketFactory *factory, UA_SocketHook hook) {
    HookListEntry *hookListEntry = (HookListEntry *)UA_malloc(sizeof(HookListEntry));
    if(hookListEntry == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    hookListEntry->hook = hook;

    LIST_INSERT_HEAD(&factory->deletionHooks, hookListEntry, pointers);
    UA_LOG_TRACE(factory->logger, UA_LOGCATEGORY_NETWORK, "Added deletion hook");
    return UA_STATUSCODE_GOOD;
}


/**
 * Convenience Wrapper for calling the dataCallback of a socket.
 */
static inline UA_StatusCode
UA_Socket_dataCallback(UA_Socket *socket, UA_ByteString *data) {
    if (socket == NULL || socket->dataCallback.callback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return socket->dataCallback.callback(socket, data, socket->dataCallback.callbackContext);
}

#endif //OPEN62541_UA_PLUGIN_SOCKET_H
