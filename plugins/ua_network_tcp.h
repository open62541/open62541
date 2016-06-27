/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_NETWORK_TCP_H_
#define UA_NETWORK_TCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_client.h"

UA_ServerNetworkLayer UA_EXPORT
UA_ServerNetworkLayerTCP(UA_ConnectionConfig conf, UA_UInt16 port);

UA_Connection UA_EXPORT
UA_ClientConnectionTCP(UA_ConnectionConfig conf, const char *endpointUrl, UA_Logger logger);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_TCP_H_ */
