/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#ifndef UA_NETWORK_PUBSUB_ETHERNET_H_
#define UA_NETWORK_PUBSUB_ETHERNET_H_

#include <open62541/plugin/pubsub.h>
#if defined(__vxworks) || defined(__VXWORKS__)
#include <netinet/if_ether.h>
#define ETH_ALEN ETHER_ADDR_LEN
#endif
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <netinet/if_ether.h>
#endif

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#ifndef   ETHERTYPE_UADP
#define   ETHERTYPE_UADP 0xb62c
#endif

/* Ethernet network layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
} UA_PubSubChannelDataEthernet;
#endif

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerEthernet(void);

_UA_END_DECLS

#endif /* UA_NETWORK_PUBSUB_ETHERNET_H_ */
