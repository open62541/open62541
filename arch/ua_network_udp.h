/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_NETWORK_UDP_H_
#define UA_NETWORK_UDP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_client.h"

/* Create the UDP networklayer and listen to the specified port */
UA_ServerNetworkLayer UA_EXPORT
UA_ServerNetworkLayerUDP(UA_ConnectionConfig conf, UA_UInt16 port);

UA_Connection UA_EXPORT
UA_ClientConnectionUDP(UA_ConnectionConfig conf, const char *endpointUrl, UA_Logger logger);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_UDP_H_ */
