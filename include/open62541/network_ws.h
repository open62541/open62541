/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_NETWORK_WS_H_
#define UA_NETWORK_WS_H_

#include <open62541/client.h>
#include <open62541/plugin/log.h>
#include <open62541/server.h>

_UA_BEGIN_DECLS

UA_ServerNetworkLayer UA_EXPORT
UA_ServerNetworkLayerWS(UA_ConnectionConfig config, UA_UInt16 port,
                        const UA_ByteString* certificate,
                        const UA_ByteString* privateKey);

_UA_END_DECLS

#endif /* UA_NETWORK_WS_H_ */
