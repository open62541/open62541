/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_SOCKETS_H
#define OPEN62541_UA_SOCKETS_H

#include "ua_plugin_socket.h"

UA_StatusCode
UA_TCP_ListenerSocketFromAddrinfo(struct addrinfo *addrinfo, UA_SocketConfig *socketConfig,
                                  UA_Socket **p_socket);

/**
 * Creates all listener sockets that can be created for the specified port.
 * For example if the underlying network architecture has a IPv4/IPv6 dual
 * stack, a socket will be created for each IP version.
 * @param socketConfig The configuration data for the socket.
 * @param creationHook Because multiple sockets may be created by this function,
 *                     instead of creating an array, we call a hook function for
 *                     each new socket. This avoids memory allocation and potential
 *                     resource leaks.
 */
UA_StatusCode
UA_TCP_ListenerSockets(UA_SocketConfig *socketConfig, UA_SocketHook creationHook);

/**
 * Creates a data socket by accepting an incoming connection from the listenerSocket.
 *
 * @param listenerSocket
 * @param p_sock
 * @param creationHook
 * @param deletionHook
 * @param logger
 */
UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(UA_Socket *listenerSocket, UA_Logger *logger, UA_UInt32 sendBufferSize,
                             UA_UInt32 recvBufferSize, UA_SocketHook creationHook, UA_SocketHook deletionHook,
                             UA_Socket_DataCallback dataCallback);

#endif //OPEN62541_UA_SOCKETS_H
