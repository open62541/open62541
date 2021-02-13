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

#include <linux/bpf.h>
#include "/usr/local/src/bpf-next/include/uapi/linux/if_xdp.h"
#include "/usr/local/src/bpf-next/tools/testing/selftests/bpf/bpf_util.h"
#include "/usr/local/src/bpf-next/tools/lib/bpf/bpf.h"
#include "/usr/local/src/bpf-next/tools/lib/bpf/libbpf.h"
#include "/usr/local/src/bpf-next/samples/bpf/xdpsock.h"
#include <sys/resource.h>
#include <sys/mman.h>

/* Theses structures shall be removed in the future XDP versions
 * (when RT Linux and XDP are mainlined and stable) */
typedef struct {
    UA_UInt32        cached_prod;
    UA_UInt32        cached_cons;
    UA_UInt32        mask;
    UA_UInt32        size;
    UA_UInt32        *producer;
    UA_UInt32        *consumer;
    UA_UInt64        *ring;
    void             *map;
} xdp_umem_uqueue;

typedef struct {
    char             *frames;
    xdp_umem_uqueue  fq;
    xdp_umem_uqueue  cq;
    UA_Int32         fd;
} xdp_umem;

typedef struct {
    UA_UInt32        cached_prod;
    UA_UInt32        cached_cons;
    UA_UInt32        mask;
    UA_UInt32        size;
    UA_UInt32        *producer;
    UA_UInt32        *consumer;
    struct xdp_desc  *ring;
    void             *map;
} xdp_uqueue;

typedef struct {
    xdp_uqueue       rx;
    xdp_uqueue       tx;
    UA_Int32         sfd;
    xdp_umem         *umem;
    UA_UInt32        outstanding_tx;
    UA_UInt32        rx_npkts;
    UA_UInt32        tx_npkts;
    UA_UInt32        prev_rx_npkts;
    UA_UInt32        prev_tx_npkts;
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
xskconfparam default_values = {131072, 2048, 1024, 1024, 1024};

/* Ethernet XDP layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
    UA_UInt32 xdp_flags;
    UA_UInt32 hw_receive_queue;
    xdpsock* xdpsocket;
} UA_PubSubChannelDataEthernetXDP;

/* Structure for Logical link control based on 802.2 */
typedef struct  {
    UA_Byte dsap; /* Destination Service Access Point */
    UA_Byte ssap; /* Source Service Access point */
    UA_Byte ctrl_1; /* Control Field */
    UA_Byte ctrl_2; /* Control Field */
} llc_pdu;


/* These defines shall be removed in the future when XDP and RT-Linux has been mainlined and stable */
#ifndef SOL_XDP
#define SOL_XDP        283
#endif

#ifndef AF_XDP
#define AF_XDP         44
#endif

// Receive buffer batch size
#define BATCH_SIZE     8

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
 * Calculates the number of entries using producer and consumer rings
 */
static UA_UInt32 xq_nb_avail(xdp_uqueue *queue, UA_UInt32 ndescs) {
    UA_UInt32 entries = queue->cached_prod - queue->cached_cons;
    if (entries == 0) {
        queue->cached_prod = *queue->producer;
        entries            = queue->cached_prod - queue->cached_cons;
    }
    return (entries > ndescs) ? ndescs : entries;
}

/**
 * Dequeueing of xdp queue
 * Returns the number of entries in dequeue
 */
static UA_UInt32 xq_deq(xdp_uqueue *queue, struct xdp_desc *descs,
                        UA_UInt32 ndescs) {
    struct xdp_desc *ring = queue->ring;
    UA_UInt32 iterator, index, entries;

    entries               = xq_nb_avail(queue, ndescs);
    u_smp_rmb();
    for (iterator = 0; iterator < entries; iterator++) {
        index             = queue->cached_cons++ & queue->mask;
        descs[iterator]   = ring[index];
    }
    if (entries > 0) {
        u_smp_wmb();
        *queue->consumer  = queue->cached_cons;
    }

    return entries;
}

/**
 * Gets data from xdpsocket to be processed via bpf functionality
 */
static inline void *xq_get_data(xdpsock *xdp_socket, UA_UInt64 addr) {
    return &xdp_socket->umem->frames[addr];
}

/**
 * Calculates the number of free entries in UMEM using producer and consumer rings
 */
static inline UA_UInt32 umem_nb_free(xdp_umem_uqueue *queue, UA_UInt32 numOfBytes) {
    UA_UInt32 free_entries = queue->cached_cons - queue->cached_prod;
    if (free_entries >= numOfBytes)
        return free_entries;

    /* Refresh the local tail pointer */
    queue->cached_cons     = *queue->consumer + queue->size;
    return queue->cached_cons - queue->cached_prod;
}

static inline int umem_fill_to_kernel_ex(xdp_umem_uqueue *fillQueue,
                                         struct xdp_desc *descriptor,
                                         UA_UInt32 numOfBytes) {
    UA_UInt32 iterator;

    if (umem_nb_free(fillQueue, numOfBytes) < numOfBytes)
        return EXIT_FAILURE;

    for (iterator = 0; iterator < numOfBytes; iterator++) {
        UA_UInt32 index        = fillQueue->cached_prod++ & fillQueue->mask;
        fillQueue->ring[index] = descriptor[iterator].addr;
    }
    u_smp_wmb();
    *fillQueue->producer       = fillQueue->cached_prod;

    return UA_STATUSCODE_GOOD;
}

/**
 * UMEM is associated to a netdev and a specific queue id of that netdev.
 * It is created and configured (chunk size, headroom, start address and size) by using the XDP_UMEM_REG setsockopt system call.
 * UMEM uses two rings: FILL and COMPLETION. Each socket associated with the UMEM must have an RX queue, TX queue or both.
 */
static xdp_umem *xdp_umem_configure(UA_Int32 sfd, xskconfparam *xskparam) {
    // Fill and completion ring queue size
    UA_UInt32 fq_size = xskparam->fill_queue_no_of_desc, cq_size = xskparam->com_queue_no_of_desc;
    struct xdp_mmap_offsets offset;
    struct xdp_umem_reg     memoryregister;
    xdp_umem *umem;
    socklen_t optlen;
    void *bufs;

    umem = (xdp_umem *)UA_calloc(1, sizeof(*umem));
    if (!umem) {
        return NULL;
    }

    if (posix_memalign(&bufs, (size_t)getpagesize(), /* PAGE_SIZE aligned */
                       xskparam->no_of_frames * xskparam->frame_size) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "buffer allocation of UMEM failed");
        return NULL;
    }

    memoryregister.addr       = (__u64)bufs;
    memoryregister.len        = xskparam->no_of_frames * xskparam->frame_size;
    memoryregister.chunk_size = xskparam->frame_size;
    memoryregister.headroom   = 0;

    /* Register UMEM to a socket */
    if (UA_setsockopt(sfd, SOL_XDP, XDP_UMEM_REG, &memoryregister, sizeof(memoryregister))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt UMEM memory registraton failed");
        return NULL;
    }

    /* Set the number of descriptors that FILL and COMPLETION rings should have */
    if (UA_setsockopt(sfd, SOL_XDP, XDP_UMEM_FILL_RING, &fq_size, sizeof(UA_UInt32))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt UMEM fill ring failed");
        return NULL;
    }
    if (UA_setsockopt(sfd, SOL_XDP, XDP_UMEM_COMPLETION_RING, &cq_size, sizeof(UA_UInt32))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt UMEM completion ring failed");
        return NULL;
    }

    optlen                    = sizeof(offset);
    if (getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &offset, &optlen)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "getsockopt failed");
        return NULL;
    }

    /* mmapp the rings to user-space using the appropriate offset */
    umem->fq.map = mmap(0, offset.fr.desc + fq_size * sizeof(UA_UInt64),
                        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, sfd,
                        XDP_UMEM_PGOFF_FILL_RING);
    if (umem->fq.map == MAP_FAILED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "memory mapping for fill ring failed");
        return NULL;
    }

    umem->fq.mask             = fq_size - 1;
    umem->fq.size             = fq_size;
    umem->fq.producer         = (UA_UInt32*)((unsigned char*) umem->fq.map + offset.fr.producer);
    umem->fq.consumer         = (UA_UInt32*)((unsigned char*) umem->fq.map + offset.fr.consumer);
    umem->fq.ring             = (UA_UInt64*)((unsigned char*)umem->fq.map + offset.fr.desc);
    umem->fq.cached_cons      = fq_size;

    umem->cq.map = mmap(0, offset.cr.desc + cq_size * sizeof(UA_UInt64),
                        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, sfd,
                        XDP_UMEM_PGOFF_COMPLETION_RING);
    if (umem->cq.map == MAP_FAILED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "memory mapping for completion ring failed");
        return NULL;
    }

    umem->cq.mask             = cq_size - 1;
    umem->cq.size             = cq_size;
    umem->cq.producer         = (UA_UInt32* )((unsigned char*) umem->cq.map + offset.cr.producer);
    umem->cq.consumer         = (UA_UInt32* )((unsigned char*) umem->cq.map + offset.cr.consumer);
    umem->cq.ring             = (UA_UInt64*)((unsigned char*) umem->cq.map + offset.cr.desc);
    umem->frames              = (char *)bufs;
    umem->fd                  = sfd;

    return umem;
}

/**
 * Transfer ownership of UMEM frames between the kernel and the user-space application
 * using producer and consumer rings
 */
static inline int umem_fill_to_kernel(xdp_umem_uqueue *fillQueue, UA_UInt64 *descriptor, UA_UInt32 nb) {
    UA_UInt32 iterator;
    if (umem_nb_free(fillQueue, nb) < nb)
        return EXIT_FAILURE;

    for (iterator = 0; iterator < nb; iterator++) {
        UA_UInt32 index        = fillQueue->cached_prod++ & fillQueue->mask;
        fillQueue->ring[index] = descriptor[iterator];
    }
    u_smp_wmb();
    *fillQueue->producer       = fillQueue->cached_prod;
    return UA_STATUSCODE_GOOD;
}

/**
 *  Configure AF_XDP socket to redirect frames to a memory buffer in a user-space application
 *  XSK has two rings: the RX ring and the TX ring
 *  A socket can receive packets on the RX ring and it can send packets on the TX ring
 */
static xdpsock *xsk_configure(xdp_umem *umem, int ifindex,
                              UA_UInt32 hw_receive_queue) {
    struct sockaddr_xdp sxdp;
    struct xdp_mmap_offsets offset;
    xdpsock *xdp_socket;
    static UA_UInt16 xdp_bind_flags;
    int sfd;
    UA_Boolean shared = true;
    socklen_t optlen;
    UA_UInt64 iterator;
    UA_UInt32 ndescs;

    xskconfparam *xskparam = (xskconfparam *)UA_calloc(1, (sizeof(xskconfparam)));
    memcpy(xskparam, &default_values, (sizeof(xskconfparam)));

    if((sfd = UA_socket(AF_XDP, SOCK_RAW, 0)) == UA_INVALID_SOCKET)
        return NULL;

    xdp_socket = (xdpsock *)UA_calloc(1, sizeof(*xdp_socket));
    if (!xdp_socket) {
        return NULL;
    }

    xdp_socket->sfd = sfd;
    xdp_socket->outstanding_tx = 0;

    if (!umem) {
        shared = false;
        xdp_socket->umem = xdp_umem_configure(sfd, xskparam);
    } else {
        xdp_socket->umem = umem;
    }

    ndescs = xskparam->no_of_desc;
    /* Set the number of descriptors that RX and TX rings should have */
    if (UA_setsockopt(sfd, SOL_XDP, XDP_RX_RING, &ndescs, sizeof(ndescs))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt for RX ring failed");
        return NULL;
    }

    if (UA_setsockopt(sfd, SOL_XDP, XDP_TX_RING, &ndescs, sizeof(ndescs))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt for TX ring failed");
        return NULL;
    }

    optlen = sizeof(offset);
    if (UA_getsockopt(sfd, SOL_XDP, XDP_MMAP_OFFSETS, &offset, &optlen)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "getsockopt failed");
        return NULL;
    }

    /* RX ring */
    xdp_socket->rx.map = mmap(NULL, offset.rx.desc + ndescs * sizeof(struct xdp_desc),
                              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, sfd,
                              XDP_PGOFF_RX_RING);
    if (xdp_socket->rx.map == MAP_FAILED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "RX mapping failed");
        return NULL;
    }

    if (!shared) {
        for (iterator = 0; iterator < ndescs * xskparam->frame_size; iterator += xskparam->frame_size)
             if (umem_fill_to_kernel(&xdp_socket->umem->fq, &iterator, 1) != 0) {
                 UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UMEM frame transfer failed");
                 return NULL;
             }
    }

    /* TX ring descriptor is filled (index, length and offset) and passed into the ring */
    xdp_socket->tx.map = mmap(NULL, offset.tx.desc + ndescs * sizeof(struct xdp_desc),
                              PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, sfd,
                              XDP_PGOFF_TX_RING);
    if (xdp_socket->tx.map == MAP_FAILED) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "TX mapping failed");
        return NULL;
    }

    /* RX ring descriptor contains UMEM offset (addr) and the length of the data (len)*/
    xdp_socket->rx.mask        = ndescs - 1;
    xdp_socket->rx.size        = ndescs;
    xdp_socket->rx.producer    = (UA_UInt32*)((unsigned char*)xdp_socket->rx.map + offset.rx.producer);
    xdp_socket->rx.consumer    = (UA_UInt32*)((unsigned char*)xdp_socket->rx.map + offset.rx.consumer);
    xdp_socket->rx.ring        = (struct xdp_desc*) ((unsigned char*)xdp_socket->rx.map + offset.rx.desc);

    xdp_socket->tx.mask        = ndescs - 1;
    xdp_socket->tx.size        = ndescs;
    xdp_socket->tx.producer    = (UA_UInt32*)((unsigned char*)xdp_socket->tx.map + offset.tx.producer);
    xdp_socket->tx.consumer    = (UA_UInt32*)((unsigned char*)xdp_socket->tx.map + offset.tx.consumer);
    xdp_socket->tx.ring        = (struct xdp_desc*) ((unsigned char*) xdp_socket->tx.map + offset.tx.desc);
    xdp_socket->tx.cached_cons = ndescs;

    sxdp.sxdp_family           = AF_XDP;
    sxdp.sxdp_ifindex          = (UA_UInt32)ifindex;
    sxdp.sxdp_queue_id         = hw_receive_queue;

    /* Share the UMEM between processes */
    if (shared) {
        sxdp.sxdp_flags          = XDP_SHARED_UMEM;
        sxdp.sxdp_shared_umem_fd = (UA_UInt32)umem->fd;
    } else {
        sxdp.sxdp_flags          = xdp_bind_flags;
    }

    /* Bind the socket to device and a specific queue id on that device */
    if (UA_bind(sfd, (struct sockaddr *)&sxdp, sizeof(sxdp))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "socket bind failed");
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

    int prog_fd, qidconf_map, xsks_map;
    int ret, key = 0;
    struct bpf_object *obj;
    char xdp_filename[256];
    struct bpf_map *map;

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
    UA_String xdpFlagParam = UA_STRING("xdpflag"), hwReceiveQueueParam = UA_STRING("hwreceivequeue");
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &xdpFlagParam)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataEthernetXDP->xdp_flags = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &hwReceiveQueueParam)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataEthernetXDP->hw_receive_queue = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub XDP Connection creation. Unknown connection parameter.");
        }
    }

    struct rlimit resourcelimit = {RLIM_INFINITY, RLIM_INFINITY};
    struct bpf_prog_load_attr prog_load_attr = {
        .prog_type    = BPF_PROG_TYPE_XDP,
    };

    if (setrlimit(RLIMIT_MEMLOCK, &resourcelimit)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Set limit on the consumption of resources failed");
        return NULL;
    }

    char *kernProg = "xdpsock";
    snprintf(xdp_filename, sizeof(xdp_filename), "/usr/local/src/bpf-next/samples/bpf/%s_kern.o", kernProg);
    prog_load_attr.file = xdp_filename;

    /* Loads the eBPF program specified by the prog_load_attr argument */
    if (bpf_prog_load_xattr(&prog_load_attr, &obj, &prog_fd))
        return NULL;

    /* Load the program inside the kernel via a newly allocated file descriptor */
    if (prog_fd < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Setting of new file descriptor failed");
        return NULL;
    }

    /* Fetch BPF match object using name */
    map = bpf_object__find_map_by_name(obj, "qidconf_map");
    qidconf_map = bpf_map__fd(map);
    if (qidconf_map < 0) {
         UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to obtain queue id from map");
         return NULL;
    }

     map = bpf_object__find_map_by_name(obj, "xsks_map");
     xsks_map = bpf_map__fd(map);
     if (xsks_map < 0) {
         UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "no xsks map found");
         return NULL;
    }

    /* eBPF program in xdpsock_kern.o is attached to interface and program runs each time a packet arrives on that interface */
    if (bpf_set_link_xdp_fd(channelDataEthernetXDP->ifindex, prog_fd, channelDataEthernetXDP->xdp_flags) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "ERROR: link set xdp fd failed");
        return NULL;
    }

    /* Overwrite the existing value, with a copy of the value supplied by the queue */
    ret = bpf_map_update_elem(qidconf_map, &key, &channelDataEthernetXDP->hw_receive_queue, 0);
    if (ret) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Update of value using qidconf failed");
        return NULL;
    }

    /* Create sockets... */
    channelDataEthernetXDP->xdpsocket = xsk_configure(NULL, channelDataEthernetXDP->ifindex, channelDataEthernetXDP->hw_receive_queue);
    newChannel->sockfd = channelDataEthernetXDP->xdpsocket->sfd;

    /* ...and insert them into the map. */
    ret = bpf_map_update_elem(xsks_map, &key, &channelDataEthernetXDP->xdpsocket->sfd, 0);
    if (ret) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Update of AF_XDP in xsk map failed");
        return NULL;
    }

    newChannel->handle = channelDataEthernetXDP;
    return newChannel;
}

/**
 * Dump the received value from queue to be processed as networkMessage
 */
static void receive_parse(UA_PubSubChannelDataEthernetXDP *channelDataEthernetXDP, void *pkt,
                          size_t length, UA_UInt64 addr,
                          UA_ByteString* message, size_t *offset) {
    size_t lenBuf;
    unsigned char* buffer;
    /* Make sure we match our target */
    if(memcmp((unsigned char *)pkt, channelDataEthernetXDP->targetAddress, ETH_ALEN) != 0)
        return;

    /* Without VLAN Tag */
    if (channelDataEthernetXDP->vid == 0 && channelDataEthernetXDP->prio == 0) {
        buffer = (unsigned char *)pkt + sizeof(struct ether_header) + sizeof(llc_pdu);
        lenBuf = sizeof(struct ether_header) + sizeof(llc_pdu);
    }
    else {
        /* With VLAN Tag */
        buffer = (unsigned char *)pkt + sizeof(struct ether_header) + sizeof(llc_pdu) + 4;
        lenBuf = sizeof(struct ether_header) + sizeof(llc_pdu) + 4;
    }

    /* TODO: Convert for loop to a well defined Linux APIs */
    if (length > 0) {
        for (size_t iterator = *offset, j = 0 ; iterator < (*offset + length - lenBuf); iterator++, j++) {
            message->data[iterator] = buffer [j];
        }
        *offset += (length - lenBuf);
        message->length = *offset;
    }
    else {
         message->length = 0;
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
                                    UA_ExtensionObject *transportSettings, UA_UInt32 timeout) {
    UA_PubSubChannelDataEthernetXDP *channelDataEthernetXDP =
        (UA_PubSubChannelDataEthernetXDP *) channel->handle;

    struct xdp_desc descs[BATCH_SIZE];
    UA_UInt32 receivedData;
    size_t    currentposition = 0;
    receivedData = xq_deq(&channelDataEthernetXDP->xdpsocket->rx, descs, BATCH_SIZE);
    if (!receivedData){
        message->length = 0;
        return UA_STATUSCODE_GOODNODATA;
    }

    for (UA_UInt16 i = 0; i < receivedData; i++) {
        unsigned char *pkt = (unsigned char *)xq_get_data(channelDataEthernetXDP->xdpsocket, descs[i].addr);
        receive_parse(channelDataEthernetXDP, pkt, descs[i].len, descs[i].addr, message, &currentposition);
        if (message->length == 0)
           break;
    }

    channelDataEthernetXDP->xdpsocket->rx_npkts += receivedData;

    umem_fill_to_kernel_ex(&channelDataEthernetXDP->xdpsocket->umem->fq, descs, receivedData);

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
    /* Detach xdpsock_kern.o from the interface */
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
        pubSubChannel->close = UA_PubSubChannelEthernetXDP_close;
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
