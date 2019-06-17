/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_SOCKETS_H
#define OPEN62541_SOCKETS_H

#include "open62541/plugin/socket.h"

_UA_BEGIN_DECLS

UA_StatusCode
UA_TCP_ListenerSocketFromAddrinfo(struct addrinfo *addrinfo, const UA_SocketConfig *socketConfig,
                                  UA_SocketCallbackFunction const onAccept,
                                  UA_Socket **p_socket);

typedef struct {
    UA_SocketCallbackFunction onAccept;
} UA_TCP_ListenerSockets_AdditionalParameters;

/**
 * Creates all listener sockets that can be created for the specified port.
 * For example if the underlying network architecture has a IPv4/IPv6 dual
 * stack, a socket will be created for each IP version.
 * \param socketConfig The configuration data for the socket.
 * \param creationCallback Because multiple sockets may be created by this function,
 *                     instead of creating an array, we call a callback function for
 *                     each new socket. This avoids memory allocation and potential
 *                     resource leaks.
 */
UA_StatusCode
UA_TCP_ListenerSockets(const UA_SocketConfig *socketConfig, UA_SocketCallbackFunction const creationCallback);

typedef struct {
    UA_Socket *listenerSocket;
} UA_TCP_DataSocket_AcceptFrom_AdditionalParameters;
/**
 * Creates a data socket by accepting an incoming connection from the listenerSocket.
 *
 * \param listenerSocket
 * \param networkManager
 * \param sendBufferSize
 * \param recvBufferSize
 * \param creationCallback
 */
UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(const UA_SocketConfig *parameters, UA_SocketCallbackFunction const creationCallback);

typedef struct {
    UA_String targetEndpointUrl;
    UA_UInt32 timeout;
} UA_TCP_ClientDataSocket_AdditionalParameters;

/**
 * Creates a new client socket according to the socket config
 *
 * \param socketParameters The parameters of the socket.
 * \param creationCallback The creation callback that will be called once the socket has been created.
 */
UA_StatusCode
UA_TCP_ClientDataSocket(const UA_SocketConfig *socketParameters, UA_SocketCallbackFunction const creationCallback);

_UA_END_DECLS

#endif //OPEN62541_SOCKETS_H
