/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef NETWORKLAYERUDP_H_
#define NETWORKLAYERUDP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOT_AMALGATED
# include "ua_server.h"
#else
# include "open62541.h"
#endif

/** @brief Create the UDP networklayer and listen to the specified port */
UA_ServerNetworkLayer ServerNetworkLayerUDP_new(UA_ConnectionConfig conf, UA_UInt32 port);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NETWORKLAYERUDP_H_ */
