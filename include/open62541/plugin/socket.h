/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */


#ifndef OPEN62541_SOCKET_H
#define OPEN62541_SOCKET_H

#include <open62541/types.h>
#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

typedef struct UA_Socket UA_Socket;
typedef struct UA_SocketConfig UA_SocketConfig;
typedef struct UA_ClientSocketConfig UA_ClientSocketConfig;
typedef struct UA_ListenerSocketConfig UA_ListenerSocketConfig;
typedef struct UA_SocketFactory UA_SocketFactory;
typedef struct UA_NetworkManager UA_NetworkManager;

/**
     * The signature of the dataCallback that needs to be implemented.
     *
     * \param data the data buffer the socket received the data to.
     *             Data in this buffer will be lost after the call returns.
     * \param socket the socket that the data was received on.
     */
typedef UA_StatusCode (*UA_Socket_DataCallbackFunction)(UA_ByteString *data,
                                                        UA_Socket *socket);

/**
 * This is a convenience typedef to easily cast functions to a socketCallback.
 * The first argument then doesn't need to be cast in the function itself.
 */
typedef UA_StatusCode (*UA_SocketCallbackFunction)(UA_Socket *);

typedef enum {
    UA_SOCKETSTATE_NEW,
    UA_SOCKETSTATE_OPEN,
    UA_SOCKETSTATE_CLOSED,
} UA_Socket_State;

struct UA_Socket {
    /**
     * The socket id. Used by the NetworkManager to map to an internal representation (e.g. file descriptor)
     */
    UA_UInt64 id;

    UA_Socket_State socketState;

    /**
     * If set to true, the network manager will call the activity function if the socket is writeable.
     */
    UA_Boolean waitForWriteActivity;

    /**
     * If set to true, the networkm manager will call the activity function if the socket is readable.
     */
    UA_Boolean waitForReadActivity;

    UA_NetworkManager *networkManager;

    /**
     * This callback is called when the socket->open function successfully returns.
     */
    UA_SocketCallbackFunction openCallback;

    /**
     * This callback is called when the socket is freed with the socket->free function.
     */
    UA_SocketCallbackFunction freeCallback;

    /**
     * The dataCallback is called by the socket once it has sufficient data
     * that it can pass on to be processed.
     * The data ByteString will be reused or deallocated once the callback returns.
     * If the data is needed beyond the call, it needs to be copied, otherwise
     * it will be lost.
     */
    UA_Socket_DataCallbackFunction dataCallback;

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
     * \param socket the socket to perform the operation on.
     */
    UA_StatusCode (*open)(UA_Socket *socket);

    /**
     * Closes the socket. This typically also signals the mayDelete function to return true,
     * indicating that the socket can be safely deleted on the next NetworkManager iteration.
     * \param socket the socket to perform the operation on.
     */
    UA_StatusCode (*close)(UA_Socket *socket);

    /**
     * Checks if the socket can be deleted because it has been closed by the local application,
     * or if it was closed remotely.
     * \param socket the socket to perform the operation on.
     * \return true, if the socket can be deleted, false otherwise.
     */
    UA_Boolean (*mayDelete)(UA_Socket *socket);

    /**
     * This function deletes the socket and frees all resources allocated by it.
     * After calling this function the behavior for all following calls is undefined.
     *
     * Because a socket might be kept in several places, the free function
     * will call the registered freeCallback so that the references to this
     * socket can be properly cleaned up.
     *
     * \param socket the socket to perform the operation on.
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
     * appropriate creation callbacks.
     *
     * \param socket The socket to perform the operation on.
     */
    UA_StatusCode (*activity)(UA_Socket *socket, UA_Boolean readActivity, UA_Boolean writeActivity);

    /**
     * Sends the data contained in the send buffer. The data in the buffer is lost
     * after calling send.
     * Always call getSendBuffer to get a buffer and write the data to that buffer.
     * The length needs to be set to the amount of bytes to send.
     * The length may not exceed the originally allocated length.
     * \param socket the socket to perform the operation on.
     */
    UA_StatusCode (*send)(UA_Socket *socket, UA_ByteString *buffer);

    /**
     * This function can be used to get a send buffer from the socket implementation.
     * To send data, directly write to this buffer. Calling send, will send the
     * contained data. The length of the buffer determines the bytes sent.
     *
     * \param socket the socket to perform the operation on.
     * \param buffer the pointer the the allocated buffer
     */
    UA_StatusCode (*acquireSendBuffer)(UA_Socket *socket, size_t bufferSize, UA_ByteString **p_buffer);

    /**
     * Releases a previously acquired send buffer.
     *
     * \param socket the socket to perform the operation on.
     * \param buffer the pointer to the buffer that will be released.
     */
    UA_StatusCode (*releaseSendBuffer)(UA_Socket *socket, UA_ByteString *buffer);

    /**
     * The application pointer points to the application the socket is associated with. (e.g server/client)
     */
    void *application;

    /**
     * The context can be a pointer to any kind of data that is needed for example for the data and free callbacks.
     */
    void *context;
};


/**
 * Configuration parameters for sockets created at startup.
 */
struct UA_SocketConfig {
    UA_UInt32 recvBufferSize;
    UA_UInt32 sendBufferSize;
    UA_UInt16 port;
    UA_NetworkManager *networkManager;
    UA_ByteString customHostname;

    /**
     * This function is called by the server to create all configured listener sockets.
     * The delayed configuration makes sure, that initialization is done during
     * server startup. Also, only the server will have ownership of the Sockets.
     * \param parameters
     * \param creationCallback The socketCallback is called once for each socket that is created.
     */
    UA_StatusCode (*createSocket)(const UA_SocketConfig *parameters, const UA_SocketCallbackFunction creationCallback);

    /**
     * The following parameters will be automatically filled by the application.
     * ########################
     */

    /**
     * The application that is associated with the created socket.
     */
    void *application;
    void *additionalParameters;
    /**
     * This function, if set by the networkmanager, is called on socket creation.
     */
    UA_SocketCallbackFunction networkManagerCallback;
};

struct UA_ClientSocketConfig {
    UA_SocketConfig socketConfig;

    UA_String targetEndpointUrl;
    UA_UInt32 timeout;
};

struct UA_ListenerSocketConfig {
    UA_SocketConfig socketConfig;

    /**
     * The onAccept function is called when a new socket is accepted by a listener socket.
     * This function may never be called if the socket is a data socket.
     */
    UA_SocketCallbackFunction onAccept;
};

/**
 * Convenience wrapper for calling socket callbacks.
 * Does a sanity check before calling the callback.
 * Returns good if both callback data and function are null,
 * which means no callback was configured.
 *
 * \param callback the callback to call
 * \param sock the socket parameter of the callback.
 */
static UA_INLINE UA_StatusCode
UA_SocketCallback_call(UA_SocketCallbackFunction callback, UA_Socket *sock) {
    if(callback == NULL)
        return UA_STATUSCODE_GOOD;
    if(sock == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return callback(sock);
}


/**
 * Convenience Wrapper for calling the dataCallback of a socket.
 */
static UA_INLINE UA_StatusCode
UA_Socket_DataCallback_call(UA_Socket *socket, UA_ByteString *data) {
    if (socket == NULL || data == NULL || socket->dataCallback == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return socket->dataCallback(data, socket);
}

_UA_END_DECLS

#endif //OPEN62541_SOCKET_H
