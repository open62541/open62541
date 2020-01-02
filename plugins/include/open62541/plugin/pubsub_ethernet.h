/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 *   Copyright 2019-2020 (c) Kalycito Infotech Private Limited
 */

#ifndef UA_NETWORK_PUBSUB_ETHERNET_H_
#define UA_NETWORK_PUBSUB_ETHERNET_H_

#include <open62541/plugin/pubsub.h>
#ifdef UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <open62541/plugin/pubsub_ethernet.h>
#endif

_UA_BEGIN_DECLS

/* Ethernet network layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
} UA_PubSubChannelDataEthernet;

#ifdef UA_ENABLE_PUBSUB_ETH_UADP_XDP_RECV
struct vlan_ethhdr {
    unsigned char    h_dest[ETH_ALEN];
    unsigned char    h_source[ETH_ALEN];
    __be16        h_vlan_proto;
    __be16        h_vlan_TCI;
    __be16        h_vlan_encapsulated_proto;
};
#endif

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerEthernet(void);

#ifdef UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
/* Function for real time pubsub - txtime calculation*/
UA_StatusCode
txtimecalc_ethernet(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettigns, void *bufSend, int lenBuf, struct sockaddr_ll sll);
#endif

#ifdef UA_ENABLE_PUBSUB_ETH_UADP_XDP_RECV
typedef struct {
    UA_UInt32 cached_prod;
    UA_UInt32 cached_cons;
    UA_UInt32 mask;
    UA_UInt32 size;
    UA_UInt32 *producer;
    UA_UInt32 *consumer;
    UA_UInt64 *ring;
    void *map;
} xdp_umem_uqueue;

typedef struct {
    char *frames;
    xdp_umem_uqueue fq;
    xdp_umem_uqueue cq;
    int fd;
} xdp_umem;

typedef struct {
    UA_UInt32 cached_prod;
    UA_UInt32 cached_cons;
    UA_UInt32 mask;
    UA_UInt32 size;
    UA_UInt32 *producer;
    UA_UInt32 *consumer;
    struct xdp_desc *ring;
    void *map;
} xdp_uqueue;

struct xdpsock {
    xdp_uqueue rx;
    xdp_uqueue tx;
    int sfd;
    xdp_umem *umem;
    UA_UInt32 outstanding_tx;
    unsigned long rx_npkts;
    unsigned long tx_npkts;
    unsigned long prev_rx_npkts;
    unsigned long prev_tx_npkts;
} xdpsock;

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerEthernet_XDPrecv(void);
#endif

_UA_END_DECLS

#endif /* UA_NETWORK_PUBSUB_ETHERNET_H_ */
