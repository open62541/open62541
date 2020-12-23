/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#include <open62541/server_pubsub.h>
#include <open62541/util.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet_xdp.h>

#if defined(__vxworks) || defined(__VXWORKS__)
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#define ETH_ALEN ETHER_ADDR_LEN
#else
#include <linux/if_packet.h>
#include <netinet/ether.h>
#endif

/* RLIMIT */
#include <sys/time.h>
#include <sys/resource.h>

/* XSK kernel headers */
#include <linux/bpf.h>
#include <linux/if_xdp.h>

/* Libbpf headers */
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <bpf/xsk.h>

/* getpagesize() */
#include <unistd.h>

/* if_nametoindex() */
#include <net/if.h>

#ifndef ETHERTYPE_UADP
#define ETHERTYPE_UADP 0xb62c
#endif

#define VLAN_SHIFT 13
#define XDP_FRAME_SHIFT 11 //match the frame size 2048

/* Theses structures shall be removed in the future XDP versions
 * (when RT Linux and XDP are mainlined and stable) */
typedef struct {
    char                    *frames;
    struct xsk_ring_prod    fq;
    struct xsk_ring_cons    cq;
    struct xsk_umem *       umem;
    void                    *buffer;
    UA_Int32                fd;
} xdp_umem;

typedef struct {
    struct xsk_ring_cons rx_ring;
    struct xsk_ring_prod tx_ring;

    struct xsk_socket *xskfd;
    UA_UInt32 bpf_prog_id;

    UA_UInt32 cur_tx;
    UA_UInt32 cur_rx;

    xdp_umem         *umem;
    UA_UInt32        outstanding_tx;
    UA_UInt64        rx_npkts;
    UA_UInt64        tx_npkts;
    UA_UInt64        prev_rx_npkts;
    UA_UInt64        prev_tx_npkts;
    UA_UInt32        idx_rx;
    UA_UInt32        idx_fq;
} xdpsock;

/* Configuration parameters for xdp socket */
typedef struct {
    UA_UInt32        no_of_frames;
    UA_UInt32        frame_size;
    UA_UInt32        no_of_desc;
    UA_UInt32        fill_queue_no_of_desc;
    UA_UInt32        com_queue_no_of_desc;
} xskconfparam;

/* Default values for xdp socket parameters */
xskconfparam default_values = {2048, 2048, 2048, 2048, 2048};

/* Ethernet XDP layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
    UA_UInt32 xdp_flags;
    UA_UInt32 hw_receive_queue;
    UA_UInt16 xdp_bind_flags;
    xdpsock* xdpsocket;
} UA_PubSubChannelDataEthernetXDP;

/* These defines shall be removed in the future when XDP and RT-Linux has been mainlined and stable */
#ifndef SOL_XDP
#define SOL_XDP        283
#endif

#ifndef AF_XDP
#define AF_XDP         44
#endif

// Receive buffer batch size
#define BATCH_SIZE                           8
#define VLAN_HEADER_SIZE                     4

#define barrier() __asm__ __volatile__("": : :"memory")
#ifdef __aarch64__
#define u_smp_rmb() __asm__ __volatile__("dmb ishld": : :"memory")
#define u_smp_wmb() __asm__ __volatile__("dmb ishst": : :"memory")
#else
#define u_smp_rmb() barrier()
#define u_smp_wmb() barrier()
#endif
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/*
 * OPC-UA specification Part 14:
 *
 * "The target is a MAC address, an IP address or a registered name like a
 *  hostname. The format of a MAC address is six groups of hexadecimal digits,
 *  separated by hyphens (e.g. 01-23-45-67-89-ab). A system may also accept
 *  hostnames and/or IP addresses if it provides means to resolve it to a MAC
 *  address (e.g. DNS and Reverse-ARP)."
 *
 * We do not support currently IP addresses or hostnames.
 */

static UA_StatusCode
UA_parseHardwareAddress(UA_String* target, UA_Byte* destinationMac) {
    size_t curr = 0, index = 0;
    for(; index < ETH_ALEN; index++) {
        UA_UInt32 value;
        size_t progress       = UA_readNumberWithBase(&target->data[curr],
                                                      target->length - curr, &value, 16);
        if(progress == 0 || value > (long)0xff)
            return UA_STATUSCODE_BADINTERNALERROR;
        destinationMac[index] = (UA_Byte) value;
        curr                 += progress;
        if(curr == target->length)
            break;

        if(target->data[curr] != '-')
            return UA_STATUSCODE_BADINTERNALERROR;
        curr++; /* skip '-' */
    }

    if(index != (ETH_ALEN-1))
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

/**
 * UMEM is associated to a netdev and a specific queue id of that netdev.
 * It is created and configured (chunk size, headroom, start address and size) by using the XDP_UMEM_REG setsockopt system call.
 * UMEM uses two rings: FILL and COMPLETION. Each socket associated with the UMEM must have an RX queue, TX queue or both.
 */
static xdp_umem *xdp_umem_configure(xskconfparam *xskparam) {
    struct xsk_umem_config uconfig = {
        .fill_size = xskparam->fill_queue_no_of_desc,
        .comp_size = xskparam->com_queue_no_of_desc,
        .frame_size = xskparam->frame_size,
        .frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
    };
    xdp_umem *umem;
    int ret;
    size_t sret;
    UA_UInt32 frames_per_ring;
    UA_UInt32 idx;

    umem = (xdp_umem *)UA_calloc(1, sizeof(*umem));
    if (!umem) {
        return NULL;
    }

    if (posix_memalign(&umem->buffer, (size_t)getpagesize(),
                       xskparam->no_of_frames * xskparam->frame_size) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "buffer allocation of UMEM failed");
        return NULL;
    }

    ret = xsk_umem__create(&umem->umem, umem->buffer,
                           xskparam->no_of_frames * xskparam->frame_size,
                           &umem->fq, &umem->cq,
                           &uconfig);
    if (ret) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub XSK UMEM creation failed. Out of memory.");
        return NULL;
    }

    /* Populate rx fill ring with addresses */
    frames_per_ring = xskparam->fill_queue_no_of_desc;

    sret = xsk_ring_prod__reserve(&umem->fq, frames_per_ring, &idx);
    if (sret != frames_per_ring)
        return NULL;

    for (UA_UInt64 i = 0; i < frames_per_ring; i++)
        *xsk_ring_prod__fill_addr(&umem->fq, idx++) = i * frames_per_ring;

    xsk_ring_prod__submit(&umem->fq, frames_per_ring);

    return umem;
}


/**
 *  Configure AF_XDP socket to redirect frames to a memory buffer in a user-space application
 *  XSK has two rings: the RX ring and the TX ring
 *  A socket can receive packets on the RX ring and it can send packets on the TX ring
 */
static xdpsock *xsk_configure(xdp_umem *umem, UA_UInt32 hw_receive_queue,
                              int ifindex, char *ifname,
                              UA_UInt32 xdp_flags, UA_UInt16 xdp_bind_flags) {
    xdpsock *xdp_socket;
    int ret;
    struct xsk_socket_config cfg;

    xskconfparam *xskparam = (xskconfparam *)UA_calloc(1, (sizeof(xskconfparam)));
    memcpy(xskparam, &default_values, (sizeof(xskconfparam)));

    xdp_socket = (xdpsock *)UA_calloc(1, sizeof(*xdp_socket));
    if (!xdp_socket) {
        return NULL;
    }

    if (umem)
        xdp_socket->umem = umem;
    else
        xdp_socket->umem = xdp_umem_configure(xskparam);

    if (!xdp_socket->umem) {
        UA_close(xsk_socket__fd(xdp_socket->xskfd));
        bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
        return NULL;
    }

    xdp_socket->outstanding_tx = 0;
    xdp_socket->idx_rx = 0;
    xdp_socket->idx_fq = 0;

    cfg.rx_size = xskparam->fill_queue_no_of_desc;
    cfg.tx_size = xskparam->com_queue_no_of_desc;

    cfg.libbpf_flags = 0;
    cfg.xdp_flags = xdp_flags;
    cfg.bind_flags = xdp_bind_flags;

    ret = xsk_socket__create(&xdp_socket->xskfd,
                             ifname,
                             hw_receive_queue,
                             xdp_socket->umem->umem,
                             &xdp_socket->rx_ring,
                             &xdp_socket->tx_ring,
                             &cfg);

    if (ret == ENOTSUP) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                      "PubSub connection creation failed."
                      " xsk_socket__create not supported.");
        return NULL;
    } else if (ret < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "PubSub connection creation failed."
                    " xsk_socket__create failed: %s", strerror(errno));
        UA_close(xsk_socket__fd(xdp_socket->xskfd));
        bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
        return NULL;
    }

    ret = bpf_get_link_xdp_id(ifindex, &xdp_socket->bpf_prog_id, xdp_flags);
    if (ret) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Unable to retrieve XDP program.");
        UA_close(xsk_socket__fd(xdp_socket->xskfd));
        bpf_set_link_xdp_fd(ifindex, -1, xdp_flags);
        return NULL;
    }

    UA_free(xskparam);
    return xdp_socket;
}

/**
 * Open communication socket based on the connectionConfig.
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelEthernetXDP_open(const UA_PubSubConnectionConfig *connectionConfig) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Open PubSub ethernet connection with XDP.");

    /* allocate and init memory for the ethernet specific internal data */
    UA_PubSubChannelDataEthernetXDP* channelDataEthernetXDP =
            (UA_PubSubChannelDataEthernetXDP*) UA_calloc(1, sizeof(*channelDataEthernetXDP));
    if(!channelDataEthernetXDP) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    /* handle specified network address */
    UA_NetworkAddressUrlDataType *address;
    if(UA_Variant_hasScalarType(&connectionConfig->address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        address = (UA_NetworkAddressUrlDataType *) connectionConfig->address.data;
    } else {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Invalid Address.");
        UA_free(channelDataEthernetXDP);
        return NULL;
    }
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Interface Name = %.*s",
         (int) address->networkInterface.length, address->networkInterface.data);
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Network Url = %.*s",
         (int)address->url.length, address->url.data);

    UA_String target;
    /* encode the URL and store information in internal structure */
    if(UA_parseEndpointUrlEthernet(&address->url, &target, &channelDataEthernetXDP->vid,
                                   &channelDataEthernetXDP->prio)) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Invalid Address URL.");
        UA_free(channelDataEthernetXDP);
        return NULL;
    }

    /* Get a valid MAC address from target definition */
    if(UA_parseHardwareAddress(&target, channelDataEthernetXDP->targetAddress) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid destination MAC address.");
        UA_free(channelDataEthernetXDP);
        return NULL;
    }

    /* generate a new Pub/Sub channel and open a related socket */
    UA_PubSubChannel *newChannel = (UA_PubSubChannel*)UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        UA_free(channelDataEthernetXDP);
        return NULL;
    }

    /* get interface index */
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(struct ifreq));
    UA_UInt64 length = UA_MIN(address->networkInterface.length, sizeof(ifreq.ifr_name)-1);
    UA_snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name), "%.*s", (int)length,
                (char*)address->networkInterface.data);

    /* TODO: ifreq has to be checked with ioctl commands */
    channelDataEthernetXDP->ifindex = (int)if_nametoindex(ifreq.ifr_name);

    //iterate over the given KeyValuePair paramters
    UA_String xdpFlagParam = UA_STRING("xdpflag"), hwReceiveQueueParam = UA_STRING("hwreceivequeue"), xdpBindFlagParam = UA_STRING("xdpbindflag");
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &xdpFlagParam)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataEthernetXDP->xdp_flags = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &hwReceiveQueueParam)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataEthernetXDP->hw_receive_queue = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &xdpBindFlagParam)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT16])){
                channelDataEthernetXDP->xdp_bind_flags = *(UA_UInt16 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub XDP Connection creation. Unknown connection parameter.");
        }
    }

    struct rlimit resourcelimit = {RLIM_INFINITY, RLIM_INFINITY};

    if (setrlimit(RLIMIT_MEMLOCK, &resourcelimit)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Set limit on the consumption of resources failed");
        return NULL;
    }

    /* Create the UMEM and XDP sockets */
    channelDataEthernetXDP->xdpsocket = xsk_configure(NULL,
                                                      channelDataEthernetXDP->hw_receive_queue,
                                                      channelDataEthernetXDP->ifindex,
                                                      (char *)address->networkInterface.data,
                                                      channelDataEthernetXDP->xdp_flags,
                                                      channelDataEthernetXDP->xdp_bind_flags);
    if(!channelDataEthernetXDP->xdpsocket) {
        bpf_set_link_xdp_fd(channelDataEthernetXDP->ifindex, -1,
                             channelDataEthernetXDP->xdp_flags);
        return NULL;
    }

    newChannel->sockfd = xsk_socket__fd(channelDataEthernetXDP->xdpsocket->xskfd);
    newChannel->handle = channelDataEthernetXDP;
    return newChannel;
}

/**
 * Receive messages.
 *
 * @param timeout in usec -> not used
 * @return
 */
static UA_StatusCode
UA_PubSubChannelEthernetXDP_receive(UA_PubSubChannel *channel, UA_ByteString *message,
                                    UA_ExtensionObject *transportSettings, UA_UInt32 timeout) {
    UA_PubSubChannelDataEthernetXDP *channelDataEthernetXDP =
        (UA_PubSubChannelDataEthernetXDP *) channel->handle;

    struct ether_header *eth_hdr;
    xdpsock *xdp_socket;
    UA_UInt64 addr;
    UA_UInt64 ret;
    UA_Byte *pkt;
    ssize_t len;

    xdp_socket = channelDataEthernetXDP->xdpsocket;
    message->length = 0;

    /* Sleep in a select call if a timeout was set */
    if(timeout > 0) {
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(channel->sockfd, &fdset);
        struct timeval tmptv = {(long int)(timeout / 1000000),
                                (long int)(timeout % 1000000)};
        int resultsize = UA_select(channel->sockfd+1, &fdset, NULL, NULL, &tmptv);
        if(resultsize == 0)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        if(resultsize == -1)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    ret = xsk_ring_cons__peek(&xdp_socket->rx_ring, 1, &xdp_socket->idx_rx);
    if (!ret)
        return UA_STATUSCODE_GOODNODATA; //No packets even after select/poll

    ret = xsk_ring_prod__reserve(&xdp_socket->umem->fq, 1, &xdp_socket->idx_fq);
    if(ret != 1)
        return UA_STATUSCODE_BADINTERNALERROR; //Got data but failed to reserve, something's wrong

    addr = xsk_ring_cons__rx_desc(&xdp_socket->rx_ring, xdp_socket->idx_rx)->addr;
    len  = (UA_UInt32) xsk_ring_cons__rx_desc(&xdp_socket->rx_ring, xdp_socket->idx_rx++)->len;
    pkt  = (UA_Byte *) xsk_umem__get_data(xdp_socket->umem->buffer, addr);

    /* AF_XDP does not do any filtering on ethertype or protocol.
     * Manually check for VLAN headers and ETH UADP ethertype.
     * Note: we use UA_UInt16 to compare ethertype, which is 2-bytes
     */
    UA_UInt16 *pkt_proto = (UA_UInt16 *) (pkt + (ETH_ALEN * 2));
    if(channelDataEthernetXDP->vid > 0 && *pkt_proto == htons(ETHERTYPE_VLAN))
        pkt_proto += 2;

    if(*pkt_proto != htons(ETHERTYPE_UADP))
        return UA_STATUSCODE_GOODNODATA;

    eth_hdr = (struct ether_header *) pkt;

    /* Make sure we match our target */
    if(memcmp(eth_hdr->ether_dhost, channelDataEthernetXDP->targetAddress, ETH_ALEN) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Invalid hardware address.");
        return UA_STATUSCODE_GOODNODATA;
    }

    message->data   = (UA_Byte *) pkt + sizeof(struct ether_header);
    message->length = (size_t) len - sizeof(struct ether_header);

    if(channelDataEthernetXDP->vid > 0) {
        message->data += 4;
        message->length -= 4;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubChannelEthernetXDP_release(UA_PubSubChannel *channel) {
    UA_PubSubChannelDataEthernetXDP *channelDataEthernetXDP;
    xdpsock *xdp_socket;

    channelDataEthernetXDP = (UA_PubSubChannelDataEthernetXDP *) channel->handle;
    xdp_socket = channelDataEthernetXDP->xdpsocket;

    xsk_ring_prod__submit(&xdp_socket->umem->fq, 1);
    xsk_ring_cons__release(&xdp_socket->rx_ring, 1);
    xdp_socket->rx_npkts += 1;

    return UA_STATUSCODE_GOOD;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelEthernetXDP_close(UA_PubSubChannel *channel) {
    UA_close(channel->sockfd);
    UA_PubSubChannelDataEthernetXDP *channelDataEthernetXDP =
        (UA_PubSubChannelDataEthernetXDP *) channel->handle;
    /* Detach XDP program from the interface */
    bpf_set_link_xdp_fd(channelDataEthernetXDP->ifindex, -1, channelDataEthernetXDP->xdp_flags);
    UA_free(channelDataEthernetXDP->xdpsocket->umem);
    UA_free(channelDataEthernetXDP->xdpsocket);
    UA_free(channelDataEthernetXDP);
    UA_free(channel);
    return UA_STATUSCODE_GOOD;
}

/**
 * Generate a new channel. based on the given configuration.
 *
 * @param connectionConfig connection configuration
 * @return  ref to created channel, NULL on error
 */
static UA_PubSubChannel *
TransportLayerEthernetXDP_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelEthernetXDP_open(connectionConfig);
    if(pubSubChannel) {
        pubSubChannel->receive = UA_PubSubChannelEthernetXDP_receive;
        pubSubChannel->close   = UA_PubSubChannelEthernetXDP_close;
        pubSubChannel->release = UA_PubSubChannelEthernetXDP_release;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

UA_PubSubTransportLayer
UA_PubSubTransportLayerEthernetXDP() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerEthernetXDP_addChannel;
    return pubSubTransportLayer;
}
