/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2020 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#ifndef UA_NETWORK_TCP_H_
#define UA_NETWORK_TCP_H_

#include <open62541/client.h>
#include <open62541/plugin/log.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Initializes a TCP network layer.
 *
 * @param config The connection config.
 * @param port The TCP port to listen to.
 * @param maxConnections Maximum number of TCP connections this network layer
 *                       instance is allowed to allocate. Set to 0 for unlimited
 *                       number of connections.
 * @param logger Pointer to a logger
 * @return Returns the network layer instance */
UA_ServerNetworkLayer UA_EXPORT
UA_ServerNetworkLayerTCP(UA_ConnectionConfig config, UA_UInt16 port,
                         UA_UInt16 maxConnections);

/* Open a non-blocking client TCP socket. The connection might not be fully
 * opened yet. Drop into the _poll function withe a timeout to complete the
 * connection. */
UA_Connection UA_EXPORT
UA_ClientConnectionTCP_init(UA_ConnectionConfig config, const UA_String endpointUrl,
                            UA_UInt32 timeout, const UA_Logger *logger);

/* Wait for a half-opened connection to fully open. Returns UA_STATUSCODE_GOOD
 * even if the timeout was hit. Returns UA_STATUSCODE_BADDISCONNECT if the
 * connection is lost. */
UA_StatusCode UA_EXPORT
UA_ClientConnectionTCP_poll(UA_Connection *connection, UA_UInt32 timeout,
                            const UA_Logger *logger);

_UA_END_DECLS

#endif /* UA_NETWORK_TCP_H_ */
