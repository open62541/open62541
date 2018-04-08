/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_NETWORK_TCP_H_
#define UA_NETWORK_TCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_client.h"
#include "ua_plugin_log.h"

UA_ServerNetworkLayer UA_EXPORT
UA_ServerNetworkLayerTCP(UA_ConnectionConfig conf, UA_UInt16 port, UA_Logger logger);

UA_Connection UA_EXPORT
UA_ClientConnectionTCP(UA_ConnectionConfig conf, const char *endpointUrl, const UA_UInt32 timeout,
                       UA_Logger logger);

UA_StatusCode UA_ClientConnectionTCP_poll(UA_Client *client, void *data);
UA_Connection UA_EXPORT UA_ClientConnectionTCP_init(UA_ConnectionConfig conf,
                const char *endpointUrl, const UA_UInt32 timeout, UA_Logger logger);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_TCP_H_ */
