/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#ifdef UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */
#endif

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/util.h>
#ifdef UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING
#include <linux/if_packet.h>
#include <netinet/ether.h>

#include <open62541/plugin/pubsub_udp.h>
#include <linux/types.h>
#include <time.h>
#include <linux/errqueue.h>
#include <poll.h>
#endif

#include <linux/bpf.h>
#include <linux/if_link.h>
#include "/usr/src/bpf-next/include/uapi/linux/if_xdp.h"
#include "/usr/src/bpf-next/tools/testing/selftests/bpf/bpf_util.h"
#include "/usr/src/bpf-next/tools/lib/bpf/bpf.h"
#include "/usr/src/bpf-next/tools/lib/bpf/libbpf.h"
#include "/usr/src/bpf-next/samples/bpf/xdpsock.h"
#include <sys/resource.h>
#include <sys/mman.h>

/*******XDP related defines*******/
#ifndef SOL_XDP
#define SOL_XDP 283
#endif

#ifndef AF_XDP
#define AF_XDP 44
#endif

#ifndef PF_XDP
#define PF_XDP AF_XDP
#endif

#define NUM_FRAMES 131072
#define FRAME_HEADROOM 0
#define FRAME_SHIFT 11
#define FRAME_SIZE 2048
#define NUM_DESCS 1024
#define BATCH_SIZE 16

#define FQ_NUM_DESCS 1024
#define CQ_NUM_DESCS 1024

typedef UA_UInt64 us64;
typedef UA_UInt32 us32;
typedef UA_UInt32 u32;
typedef UA_UInt64 u64;
typedef UA_UInt16 u16;

static us32 opt_xdp_flags;
static us32 opt_queue = 2;
static UA_UInt16 opt_xdp_bind_flags;

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
    size_t curr = 0, idx = 0;
    for(; idx < ETH_ALEN; idx++) {
        UA_UInt32 value;
        size_t progress =
            UA_readNumberWithBase(&target->data[curr],
                                  target->length - curr, &value, 16);
        if(progress == 0 || value > (long)0xff)
            return UA_STATUSCODE_BADINTERNALERROR;

        destinationMac[idx] = (UA_Byte) value;

        curr += progress;
        if(curr == target->length)
            break;

        if(target->data[curr] != '-')
            return UA_STATUSCODE_BADINTERNALERROR;

        curr++; /* skip '-' */
    }

    if(idx != (ETH_ALEN-1))
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_STATUSCODE_GOOD;
}

static us32 xq_nb_avail(xdp_uqueue *q, us32 ndescs)
{
    us32 entries = q->cached_prod - q->cached_cons;

    if (entries == 0) {
        q->cached_prod = *q->producer;
        entries = q->cached_prod - q->cached_cons;
    }

    return (entries > ndescs) ? ndescs : entries;
}

static u32 xq_deq(xdp_uqueue *uq,
             struct xdp_desc *descs,
             us32 ndescs)
{
    struct xdp_desc *r = uq->ring;
    unsigned int idx;
    us32 i, entries;

    entries = xq_nb_avail(uq, ndescs);

    u_smp_rmb();

    for (i = 0; i < entries; i++) {
        idx = uq->cached_cons++ & uq->mask;
        descs[i] = r[idx];
    }

    if (entries > 0) {
        u_smp_wmb();

        *uq->consumer = uq->cached_cons;
    }

    return entries;
}

static inline void *xq_get_data(struct xdpsock *xsk, us64 addr)
{
    return &xsk->umem->frames[addr];
}

static inline us32 umem_nb_free(xdp_umem_uqueue *q, us32 nb)
{
    us32 free_entries = q->cached_cons - q->cached_prod;

    if (free_entries >= nb)
        return free_entries;

    /* Refresh the local tail pointer */
    q->cached_cons = *q->consumer + q->size;

    return q->cached_cons - q->cached_prod;
}

static inline int umem_fill_to_kernel_ex(xdp_umem_uqueue *fq,
                                         struct xdp_desc *d,
                                         us32 nb)
{
    us32 i;

    if (umem_nb_free(fq, nb) < nb)
        return -ENOSPC;

    for (i = 0; i < nb; i++) {
        us32 idx = fq->cached_prod++ & fq->mask;

        fq->ring[idx] = d[i].addr;
    }

    u_smp_wmb();

    *fq->producer = fq->cached_prod;

    return 0;
}

static xdp_umem *xdp_umem_configure(int sfd)
{
    int fq_size = FQ_NUM_DESCS, cq_size = CQ_NUM_DESCS;
    struct xdp_mmap_offsets off;
    struct xdp_umem_reg mr;
    xdp_umem *umem;
    socklen_t optlen;
    void *bufs;

    umem = (xdp_umem *)calloc(1, sizeof(*umem));
    UA_assert(umem);

    UA_assert(posix_memalign(&bufs, (size_t)getpagesize(), /* PAGE_SIZE aligned */
                   NUM_FRAMES * FRAME_SIZE) == 0);

    mr.addr = (__u64)bufs;
    mr.len = NUM_FRAMES * FRAME_SIZE;
    mr.chunk_size = FRAME_SIZE;
    mr.headroom = FRAME_HEADROOM;

    UA_assert(setsockopt(sfd, SOL_XDP, XDP_UMEM_REG, &mr, sizeof(mr)) == 0);
    UA_assert(setsockopt(sfd, SOL_XDP, XDP_UMEM_FILL_RING, &fq_size,
               sizeof(int)) == 0);
    UA_assert(setsockopt(sfd, SOL_XDP, XDP_UMEM_COMPLETION_RING, &cq_size,
               sizeof(int)) == 0);

    optlen = sizeof(off);
    UA_assert(getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &off,
               &optlen) == 0);

    umem->fq.map = mmap(0, off.fr.desc +
                FQ_NUM_DESCS * sizeof(us64),
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, sfd,
                XDP_UMEM_PGOFF_FILL_RING);
    UA_assert(umem->fq.map != MAP_FAILED);

    umem->fq.mask = FQ_NUM_DESCS - 1;
    umem->fq.size = FQ_NUM_DESCS;
    umem->fq.producer = (u32*)((unsigned char*) umem->fq.map + off.fr.producer);
    umem->fq.consumer = (u32*)((unsigned char*) umem->fq.map + off.fr.consumer);
    umem->fq.ring = (us64*)((unsigned char*)umem->fq.map + off.fr.desc);
    umem->fq.cached_cons = FQ_NUM_DESCS;

    umem->cq.map = mmap(0, off.cr.desc +
                 CQ_NUM_DESCS * sizeof(us64),
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_POPULATE, sfd,
                 XDP_UMEM_PGOFF_COMPLETION_RING);
    UA_assert(umem->cq.map != MAP_FAILED);

    umem->cq.mask = CQ_NUM_DESCS - 1;
    umem->cq.size = CQ_NUM_DESCS;
    umem->cq.producer = (u32* )((unsigned char*) umem->cq.map + off.cr.producer);
    umem->cq.consumer = (u32* )((unsigned char*) umem->cq.map + off.cr.consumer);
    umem->cq.ring = (us64*)((unsigned char*) umem->cq.map + off.cr.desc);

    umem->frames = (char *)bufs;
    umem->fd = sfd;

    return umem;
}

static inline int umem_fill_to_kernel(xdp_umem_uqueue *fq, us64 *d,
                      us32 nb)
{
    us32 i;

    if (umem_nb_free(fq, nb) < nb)
        return -ENOSPC;

    for (i = 0; i < nb; i++) {
        us32 idx = fq->cached_prod++ & fq->mask;

        fq->ring[idx] = d[i];
    }

    u_smp_wmb();

    *fq->producer = fq->cached_prod;

    return 0;
}

static struct xdpsock *xsk_configure(xdp_umem *umem, UA_PubSubChannel *channel, struct ifreq* ifreq)
{
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    struct sockaddr_xdp sxdp;
    struct xdp_mmap_offsets off;
    int sfd, ndescs = NUM_DESCS;
    struct xdpsock *xsk;
    bool shared = true;
    socklen_t optlen;
    us64 i;

    sfd = socket(PF_XDP, SOCK_RAW, 0);
    UA_assert(sfd >= 0);

    xsk = (struct xdpsock *)UA_calloc(1, sizeof(*xsk));
    UA_assert(xsk);

    xsk->sfd = sfd;
    xsk->outstanding_tx = 0;

    channel->sockfd = sfd;

    if (!umem) {
        shared = false;
        xsk->umem = xdp_umem_configure(sfd);
    } else {
        xsk->umem = umem;
    }

    UA_assert(setsockopt(sfd, SOL_XDP, XDP_RX_RING,
               &ndescs, sizeof(int)) == 0);
    UA_assert(setsockopt(sfd, SOL_XDP, XDP_TX_RING,
               &ndescs, sizeof(int)) == 0);
    optlen = sizeof(off);
    UA_assert(getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &off,
               &optlen) == 0);

    /* Rx */
    xsk->rx.map = mmap(NULL,
               off.rx.desc +
               NUM_DESCS * sizeof(struct xdp_desc),
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_POPULATE, sfd,
               XDP_PGOFF_RX_RING);
    UA_assert(xsk->rx.map != MAP_FAILED);

    if (!shared) {
        for (i = 0; i < NUM_DESCS * FRAME_SIZE; i += FRAME_SIZE)
             UA_assert(umem_fill_to_kernel(&xsk->umem->fq, &i, 1)
                == 0);
    }

    /* Tx */
    xsk->tx.map = mmap(NULL,
               off.tx.desc +
               NUM_DESCS * sizeof(struct xdp_desc),
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_POPULATE, sfd,
               XDP_PGOFF_TX_RING);
    UA_assert(xsk->tx.map != MAP_FAILED);

    xsk->rx.mask = NUM_DESCS - 1;
    xsk->rx.size = NUM_DESCS;
    xsk->rx.producer = (u32*)((unsigned char*)xsk->rx.map + off.rx.producer);
    xsk->rx.consumer = (u32*)((unsigned char*)xsk->rx.map + off.rx.consumer);
    xsk->rx.ring = (struct xdp_desc*) ((unsigned char*)xsk->rx.map + off.rx.desc);

    xsk->tx.mask = NUM_DESCS - 1;
    xsk->tx.size = NUM_DESCS;
    xsk->tx.producer = (u32*)((unsigned char*)xsk->tx.map + off.tx.producer);
    xsk->tx.consumer = (u32*)((unsigned char*)xsk->tx.map + off.tx.consumer);
    xsk->tx.ring = (struct xdp_desc*) ((unsigned char*) xsk->tx.map + off.tx.desc);
    xsk->tx.cached_cons = NUM_DESCS;

    sxdp.sxdp_family = PF_XDP;
    sxdp.sxdp_ifindex = (u32)channelDataEthernet->ifindex;
    sxdp.sxdp_queue_id = opt_queue;

    if (shared) {
        sxdp.sxdp_flags = XDP_SHARED_UMEM;
        sxdp.sxdp_shared_umem_fd = (u32)umem->fd;
    } else {
        sxdp.sxdp_flags = opt_xdp_bind_flags;
    }

    UA_assert(bind(sfd, (struct sockaddr *)&sxdp, sizeof(sxdp)) == 0);

    return xsk;
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
    UA_PubSubChannelDataEthernet* channelDataEthernet =
            (UA_PubSubChannelDataEthernet*) UA_calloc(1, sizeof(*channelDataEthernet));
    if(!channelDataEthernet) {
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
        UA_free(channelDataEthernet);
        return NULL;
    }
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Interface Name = %.*s",
         (int) address->networkInterface.length, address->networkInterface.data);
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Network Url = %.*s",
         (int)address->url.length, address->url.data);

    UA_String target;
    /* encode the URL and store information in internal structure */
    if(UA_parseEndpointUrlEthernet(&address->url, &target, &channelDataEthernet->vid,
                                   &channelDataEthernet->prio)) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Invalid Address URL.");
        UA_free(channelDataEthernet);
        return NULL;
    }

    /* Get a valid MAC address from target definition */
    if(UA_parseHardwareAddress(&target, channelDataEthernet->targetAddress) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid destination MAC address.");
        UA_free(channelDataEthernet);
        return NULL;
    }

    /* generate a new Pub/Sub channel and open a related socket */
    UA_PubSubChannel *newChannel = (UA_PubSubChannel*)UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        UA_free(channelDataEthernet);
        return NULL;
    }

    /* get interface index */
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, (char*)address->networkInterface.data,
            UA_MIN(address->networkInterface.length, sizeof(ifreq.ifr_name)-1));

    /* TODO: ifreq has to be checked with ioctl commands */
    channelDataEthernet->ifindex = (int)if_nametoindex(ifreq.ifr_name);

    struct rlimit r = {RLIM_INFINITY, RLIM_INFINITY};
    struct bpf_prog_load_attr prog_load_attr = {
        .prog_type    = BPF_PROG_TYPE_XDP,
    };

    int prog_fd, qidconf_map, xsks_map;
    int ret, key = 0;
    struct bpf_object *obj;
    char xdp_filename[256];
    struct bpf_map *map;

    if (setrlimit(RLIMIT_MEMLOCK, &r)) {
        fprintf(stderr, "ERROR: setrlimit(RLIMIT_MEMLOCK) \"%s\"\n",
                strerror(errno));
        return NULL;
    }

    char *a = "xdpsock";
    snprintf(xdp_filename, sizeof(xdp_filename), "/usr/src/bpf-next/samples/bpf/%s_kern.o", a);
    prog_load_attr.file = xdp_filename;

    if (bpf_prog_load_xattr(&prog_load_attr, &obj, &prog_fd))
        return NULL;
    if (prog_fd < 0) {
        fprintf(stderr, "ERROR: no program found: %s\n",
                strerror(prog_fd));
        return NULL;
    }

    map = bpf_object__find_map_by_name(obj, "qidconf_map");
    qidconf_map = bpf_map__fd(map);
    if (qidconf_map < 0) {
         fprintf(stderr, "ERROR: no qidconf map found: %s\n",
                        strerror(qidconf_map));
         exit(EXIT_FAILURE);
    }

            map = bpf_object__find_map_by_name(obj, "xsks_map");
            xsks_map = bpf_map__fd(map);
            if (xsks_map < 0) {
                fprintf(stderr, "ERROR: no xsks map found: %s\n",
                    strerror(xsks_map));
                return NULL;
            }

            if (bpf_set_link_xdp_fd(channelDataEthernet->ifindex, prog_fd, opt_xdp_flags) < 0) {
                fprintf(stderr, "ERROR: link set xdp fd failed\n");
                return NULL;
            }

             ret = bpf_map_update_elem(qidconf_map, &key, &opt_queue, 0);
            if (ret) {
                fprintf(stderr, "ERROR: bpf_map_update_elem qidconf\n");
                exit(EXIT_FAILURE);
            }

            newChannel->handle = channelDataEthernet;
            newChannel->state = UA_PUBSUB_CHANNEL_PUB;

           /* Create sockets... */
            newChannel->xdpsocket = xsk_configure(NULL, newChannel, &ifreq);

           /* ...and insert them into the map. */
                    ret = bpf_map_update_elem(xsks_map, &key, &newChannel->sockfd, 0);
                    if (ret) {
                        fprintf(stderr, "ERROR: bpf_map_update_elem %d\n", key);
                        return NULL;
                    }
                return newChannel;
}

static void hex_dump(void *pkt, size_t length, u64 addr, UA_ByteString* message, u32 messageNumber) {

    unsigned char * buffer = (unsigned char *)pkt + sizeof(struct vlan_ethhdr);
    /* TODO: Convert for loop to a well defined Linux APIs */
    if (length > 0) {
        for (size_t i = 0 ; i < length - sizeof(struct vlan_ethhdr); i++) {
            message[messageNumber].data[i] = buffer [i];
        }
        message[messageNumber].length = length - sizeof(struct vlan_ethhdr);
    }
    else {
        message[messageNumber].length = 0;
    }
}

/**
 * Receive messages.
 *
 * @param timeout in usec -> not used
 * @return
 */
static UA_StatusCode
UA_PubSubChannelEthernetXDP_receive(UA_PubSubChannel *channel, UA_ByteString *message,
                                    UA_ExtensionObject *transportSettings, UA_UInt32 *numOfMsgsRcvd) {

        struct xdp_desc descs[BATCH_SIZE];
        u32 rcvd, i;
        rcvd = xq_deq(&channel->xdpsocket->rx, descs, BATCH_SIZE);
        if (!rcvd){
                message->length = 0;
                return UA_STATUSCODE_GOODNODATA;
        }
        for (i = 0; i < rcvd; i++) {
            unsigned char *pkt = (unsigned char *)xq_get_data(channel->xdpsocket, descs[i].addr);
            hex_dump(pkt, descs[i].len, descs[i].addr, message, i);
        }

        *numOfMsgsRcvd = rcvd;
        channel->xdpsocket->rx_npkts += rcvd;

        umem_fill_to_kernel_ex(&channel->xdpsocket->umem->fq, descs, rcvd);

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
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;
    bpf_set_link_xdp_fd(channelDataEthernet->ifindex, -1, opt_xdp_flags);
    UA_free(channel->handle);
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
    /* TODO: Integrate with PubSub channel */
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelEthernetXDP_open(connectionConfig);
    if(pubSubChannel) {
        pubSubChannel->receiveXDP = UA_PubSubChannelEthernetXDP_receive;
        pubSubChannel->close = UA_PubSubChannelEthernetXDP_close;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

UA_PubSubTransportLayer
UA_PubSubTransportLayerEthernet_XDPrecv() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerEthernetXDP_addChannel;
    return pubSubTransportLayer;
}
