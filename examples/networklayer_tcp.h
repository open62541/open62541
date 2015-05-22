/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef NETWORKLAYERTCP_H_
#define NETWORKLAYERTCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOT_AMALGATED
#include "ua_server.h"
#include "ua_client.h"
#else
#include "open62541.h"
#endif

/** @brief Create the TCP networklayer and listen to the specified port */
UA_EXPORT UA_ServerNetworkLayer ServerNetworkLayerTCP_new(UA_ConnectionConfig conf, UA_UInt32 port);
UA_EXPORT UA_Connection ClientNetworkLayerTCP_connect(char *endpointUrl, UA_Logger *logger);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NETWORKLAYERTCP_H_ */
