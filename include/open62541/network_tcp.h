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
