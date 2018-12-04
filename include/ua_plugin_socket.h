/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */


#ifndef OPEN62541_UA_PLUGIN_SOCKET_H
#define OPEN62541_UA_PLUGIN_SOCKET_H

#include "ua_types.h"

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
    UA_StatusCode (*deleteMembers)(UA_Socket *socket);

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
     * This opaque pointer points to implementation specific internal data.
     * It is initialized and allocated when a new socket is created and deallocated
     * when the socket is deleted.
     */
    void *internalData;
};

/**
 * Convenience Wrapper for calling the dataCallback of a socket.
 */
inline UA_StatusCode
UA_Socket_dataCallback(UA_Socket *socket, UA_ByteString *data) {
    return socket->dataCallback.callback(socket, data, socket->dataCallback.callbackContext);
}

#endif //OPEN62541_UA_PLUGIN_SOCKET_H
