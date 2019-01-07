/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_SOCKETS_H
#define OPEN62541_UA_SOCKETS_H

#include "ua_plugin_socket.h"

UA_StatusCode
UA_TCP_ListenerSocketFromAddrinfo(struct addrinfo *addrinfo,
                                  UA_DataSocketFactory *dataSocketFactory,
                                  UA_Logger *logger,
                                  UA_ByteString *customHostname,
                                  UA_Socket **p_socket);

/**
 * Creates all listener sockets that can be created for the specified port.
 * For example if the underlying network architecture has a IPv4/IPv6 dual
 * stack, a socket will be created for each IP version.
 * @param port the port to listen on.
 * @param dataSocketFactory the DataSocketFactory that the listener sockets
 *                          will use when a new DataSocket is created/accepted.
 * @param logger the logger the socket will use for its logging.
 * @param customHostname this can be used to set a custom hostname for the socket.
 *                       If customHostname is NULL, the hostname configured in the
 *                       system will be used.
 * @param sockets a pointer to the location where the array will be stored.
 */
UA_StatusCode
UA_TCP_ListenerSockets(UA_UInt32 port,
                       UA_DataSocketFactory *dataSocketFactory,
                       UA_Logger *logger,
                       UA_ByteString *customHostname,
                       UA_Socket **sockets[],
                       size_t *sockets_size);

/**
 * Creates a data socket by accepting an incoming connection from the listenerSocket.
 *
 * @param listenerSocket
 * @param p_sock
 * @param creationHooks
 * @param deletionHooks
 * @param logger
 */
UA_StatusCode
UA_TCP_DataSocket_AcceptFrom(UA_Socket *listenerSocket, UA_Logger *logger, UA_SocketConfig socketConfig,
                             HookList creationHooks, HookList deletionHooks, UA_Socket **p_sock);

#endif //OPEN62541_UA_SOCKETS_H
