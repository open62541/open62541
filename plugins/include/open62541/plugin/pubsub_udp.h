/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017-2018 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_NETWORK_UDPMC_H_
#define UA_NETWORK_UDPMC_H_

#include <open62541/plugin/pubsub.h>

_UA_BEGIN_DECLS

// UDP multicast network layer specific internal data
typedef struct {
    int ai_family;                        //Protocol family for socket.  IPv4/IPv6
    struct sockaddr_storage *ai_addr;     //https://msdn.microsoft.com/de-de/library/windows/desktop/ms740496(v=vs.85).aspx
    UA_UInt32 messageTTL;
    UA_Boolean enableLoopback;
    UA_Boolean enableReuse;
} UA_PubSubChannelDataUDPMC;

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerUDPMP(void);

_UA_END_DECLS

#endif /* UA_NETWORK_UDPMC_H_ */
