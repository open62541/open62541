/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>
#include <open62541/util.h>

#if defined(__vxworks) || defined(__VXWORKS__)
#include <netpacket/packet.h>
#include <netinet/if_ether.h>
#define ETH_ALEN ETHER_ADDR_LEN
#else
#include <linux/if_packet.h>
#include <netinet/ether.h>
#endif
#include "poll.h"
#include "linux/errqueue.h"
#include "time.h"

#ifndef ETHERTYPE_UADP
#define ETHERTYPE_UADP 0xb62c
#endif

#define SOCKET_PRIORITY 3
#define SHIFT_32BITS    32

#ifndef SOCKET_TRANSMISSION_TIME
#define SOCKET_TRANSMISSION_TIME                        61
#ifndef SCM_TXTIME
#define SCM_TXTIME                                      SOCKET_TRANSMISSION_TIME
#endif
#endif

#ifndef SOCKET_EE_ORIGIN_TRANSMISSION_TIME
#define SOCKET_EE_ORIGIN_TRANSMISSION_TIME              6
#define SOCKET_EE_CODE_TRANSMISSION_TIME_INVALID_PARAM  1
#define SOCKET_EE_CODE_TRANSMISSION_TIME_MISSED         2
#endif

#define TIMEOUT_REALTIME                                1

/* Ethernet network layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
} UA_PubSubChannelDataEthernet;

typedef struct {
    clockid_t clockIdentity;
    uint16_t flags;
} socket_transmission_time;

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

/**
 * Open communication socket based on the connectionConfig.
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelEthernetETF_open(const UA_PubSubConnectionConfig *connectionConfig) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Open PubSub ethernet connection.");

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

    /* Open a packet socket */
    int sockFd = UA_socket(PF_PACKET, SOCK_RAW, 0);
    if(sockFd < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot create socket.");
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }
    newChannel->sockfd = sockFd;

    /* allow the socket to be reused */
    int opt = 1;
    if(UA_setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot set socket reuse.");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }

    /* get interface index */
    struct ifreq ifreq;
    memset(&ifreq, 0, sizeof(struct ifreq));
    UA_UInt64 len = UA_MIN(address->networkInterface.length, sizeof(ifreq.ifr_name)-1);
    UA_snprintf(ifreq.ifr_name, sizeof(struct ifreq),
                "%.*s", (int)len,
                (char*)address->networkInterface.data);

    if(ioctl(sockFd, SIOCGIFINDEX, &ifreq) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
           "PubSub connection creation failed. Cannot get interface index.");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }
    channelDataEthernet->ifindex = ifreq.ifr_ifindex;

    /* determine own MAC address (source address for send) */
    if(ioctl(sockFd, SIOCGIFHWADDR, &ifreq) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot determine own MAC address.");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }
#if defined(__vxworks) || defined(__VXWORKS__)
    memcpy(channelDataEthernet->ifAddress, &ifreq.ifr_ifru.ifru_addr.sa_data, ETH_ALEN);
#else
    memcpy(channelDataEthernet->ifAddress, &ifreq.ifr_hwaddr.sa_data, ETH_ALEN);
#endif

    /* bind the socket to interface and ethertype */
    struct sockaddr_ll sll = { 0 };
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = channelDataEthernet->ifindex;
    sll.sll_protocol = htons(ETHERTYPE_UADP);

    if(UA_bind(sockFd, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot bind socket.");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }

    /* Setting the socket priority to the socket */
    UA_Int32 soPriority        = connectionConfig->etfConfiguration.socketPriority;
    if (setsockopt(sockFd, SOL_SOCKET, SO_PRIORITY, &soPriority, sizeof(int))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt SO_PRIORITY failed");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }

    /* Setting socket txtime with required flags to the socket */
    socket_transmission_time sk_txtime;
    memset(&sk_txtime, 0, sizeof(sk_txtime));
    clockid_t clockId          = CLOCK_TAI;
    sk_txtime.clockIdentity   = clockId;
    sk_txtime.flags           = (UA_UInt16)(connectionConfig->etfConfiguration.sotxtimeDeadlinemode | connectionConfig->etfConfiguration.sotxtimeReceiveerrors);
    if (connectionConfig->etfConfiguration.sotxtimeEnabled && setsockopt(sockFd, SOL_SOCKET, SOCKET_TRANSMISSION_TIME, &sk_txtime, sizeof(&sk_txtime))) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "setsockopt SOCKET_TRANSMISSION_TIME failed");
        UA_close(sockFd);
        UA_free(channelDataEthernet);
        UA_free(newChannel);
        return NULL;
    }

    newChannel->handle = channelDataEthernet;
    newChannel->state = UA_PUBSUB_CHANNEL_PUB;

    return newChannel;
}

static UA_Boolean
is_multicast_address(const UA_Byte *address) {
    /* check if it is a unicast address */
    if((address[0] & 1) == 0) {
        return UA_FALSE;
    }

    /* and exclude broadcast addresses */
    for(size_t i = 0; i < ETH_ALEN; i++) {
        if(address[i] != 0xff)
            return UA_TRUE;
    }

    /* reaching this point, we know it has to be a broadcast address */
    return UA_FALSE;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelEthernetETF_regist(UA_PubSubChannel *channel,
                                UA_ExtensionObject *transportSettings,
                                void (*notUsedHere)(UA_ByteString *encodedBuffer, UA_ByteString *topic)) {
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    if(!is_multicast_address(channelDataEthernet->targetAddress))
        return UA_STATUSCODE_GOOD;

    struct packet_mreq mreq;
    mreq.mr_ifindex = channelDataEthernet->ifindex;
    mreq.mr_type = PACKET_MR_MULTICAST;
    mreq.mr_alen = ETH_ALEN;
    memcpy(mreq.mr_address, channelDataEthernet->targetAddress, ETH_ALEN);

    if(UA_setsockopt(channel->sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed. %s", strerror(errno));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/**
 * Remove current subscription.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelEthernetETF_unregist(UA_PubSubChannel *channel,
                                  UA_ExtensionObject *transportSettings) {
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    if(!is_multicast_address(channelDataEthernet->targetAddress)) {
        return UA_STATUSCODE_GOOD;
    }

    struct packet_mreq mreq;
    mreq.mr_ifindex = channelDataEthernet->ifindex;
    mreq.mr_type = PACKET_MR_MULTICAST;
    mreq.mr_alen = ETH_ALEN;
    memcpy(mreq.mr_address, channelDataEthernet->targetAddress, ETH_ALEN);

    if(UA_setsockopt(channel->sockfd, SOL_PACKET, PACKET_DROP_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sendWithTxTime(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings, void *bufSend, size_t lenBuf) {
    UA_UInt64     transmission_time;
    UA_Boolean    txTimeEnable;
    /* Send the data packet with the tx time */
    char dataPacket[CMSG_SPACE(sizeof(transmission_time))] = {0};
    /* Structure for messages sent and received */
    struct msghdr message;
    /* Structure for scattering or gathering of input/output */
    struct iovec  inputOutputVec;
    ssize_t       msgCount;

    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    /* Structure for socket internet address */
    struct sockaddr_ll socketAddress = { 0 };

    socketAddress.sll_family   = AF_PACKET;
    socketAddress.sll_ifindex  = channelDataEthernet->ifindex;
    socketAddress.sll_protocol = htons(ETHERTYPE_UADP);

    inputOutputVec.iov_base   = bufSend;
    inputOutputVec.iov_len    = lenBuf;

    memset(&message, 0, sizeof(message));
    /* Provide message name / optional address */
    message.msg_name          = &socketAddress;
    /* Provide message address size in bytes */
    message.msg_namelen       = sizeof(socketAddress);
    /* Provide array of input/output buffers */
    message.msg_iov           = &inputOutputVec;
    /* Provide the number of elements in the array */
    message.msg_iovlen        = 1;

    /* Get ethernet ETF transport settings */
    UA_EthernetETFWriterGroupTransportDataType *ethernetETFtransportSettings;
    ethernetETFtransportSettings = (UA_EthernetETFWriterGroupTransportDataType *)transportSettings->content.decoded.data;
    transmission_time = ethernetETFtransportSettings->transmission_time;
    txTimeEnable = ethernetETFtransportSettings->txtime_enabled;

    /*
     * We specify the transmission time in the CMSG.
     */
    if (txTimeEnable) {
        /* Provide the necessary data */
        message.msg_control    = dataPacket;
        /* Provide the size of necessary bytes */
        message.msg_controllen = sizeof(dataPacket);
        /* Structure for storing the necessary data */
        struct cmsghdr*       controlMsg;
        /* Control message created for tx time */
        controlMsg                         = CMSG_FIRSTHDR(&message);
        controlMsg->cmsg_level             = SOL_SOCKET;
        controlMsg->cmsg_type              = SCM_TXTIME;
        controlMsg->cmsg_len               = CMSG_LEN(sizeof(__u64));
        *((__u64 *) CMSG_DATA(controlMsg)) = transmission_time;
    }

    msgCount = sendmsg(channel->sockfd, &message, 0);
    if ((msgCount < 1) && (msgCount != (UA_Int32)lenBuf)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/**
 * Send messages to the connection defined address
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelEthernetETF_send(UA_PubSubChannel *channel,
                                 UA_ExtensionObject *transportSettings,
                                 const UA_ByteString *buf) {
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    /* Allocate a buffer for the ethernet data which contains the ethernet
     * header (without VLAN tag), the VLAN tag and the OPC-UA/Ethernet data. */
    char *bufSend, *ptrCur;
    size_t lenBuf;
    struct ether_header* ethHdr;

    lenBuf = sizeof(*ethHdr) + 4 + buf->length;
    bufSend = (char*) UA_malloc(lenBuf);
    ethHdr = (struct ether_header*) bufSend;

    /* Set (own) source MAC address */
    memcpy(ethHdr->ether_shost, channelDataEthernet->ifAddress, ETH_ALEN);

    /* Set destination MAC address */
    memcpy(ethHdr->ether_dhost, channelDataEthernet->targetAddress, ETH_ALEN);

    /* Set ethertype */
    /* Either VLAN or Ethernet */
    ptrCur = bufSend + sizeof(*ethHdr);
    if(channelDataEthernet->vid == 0) {
        ethHdr->ether_type = htons(ETHERTYPE_UADP);
        lenBuf -= 4;  /* no VLAN tag */
    } else {
        ethHdr->ether_type = htons(ETHERTYPE_VLAN);
        /* set VLAN ID */
        UA_UInt16 vlanTag;
        vlanTag = (UA_UInt16) (channelDataEthernet->vid + (channelDataEthernet->prio << 13));
        *((UA_UInt16 *) ptrCur) = htons(vlanTag);
        ptrCur += sizeof(UA_UInt16);
        /* set Ethernet */
        *((UA_UInt16 *) ptrCur) = htons(ETHERTYPE_UADP);
        ptrCur += sizeof(UA_UInt16);
    }

    /* copy payload of ethernet message */
    memcpy(ptrCur, buf->data, buf->length);

    ssize_t rc;
    /* Send the packets at the given Txtime */
    rc = sendWithTxTime(channel, transportSettings, bufSend, lenBuf);
    if(rc != UA_STATUSCODE_GOOD) {
        printf("sendfailed");
        UA_free(bufSend);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_free(bufSend);

    return UA_STATUSCODE_GOOD;
}

/**
 * Receive messages.
 *
 * @param timeout in usec -> not used
 * @return
 */
static UA_StatusCode
UA_PubSubChannelEthernetETF_receive(UA_PubSubChannel *channel, UA_ByteString *message,
                                 UA_ExtensionObject *transportSettings, UA_UInt32 timeout) {
    UA_PubSubChannelDataEthernet *channelDataEthernet =
        (UA_PubSubChannelDataEthernet *) channel->handle;

    struct ether_header eth_hdr;
    struct msghdr msg;
    struct iovec iov[2];

    iov[0].iov_base = &eth_hdr;
    iov[0].iov_len = sizeof(eth_hdr);
    iov[1].iov_base = message->data;
    iov[1].iov_len = message->length;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    msg.msg_controllen = 0;

    /* TODO: timeout from receive API should be configurable.
     * The parameter should be inside the channel or transport settings.
     * For now, timeout value set to 1us as we run in a lower cycle time of 100us
     */
    timeout = TIMEOUT_REALTIME;

    /* Sleep in a select call if a timeout was set */
    if(timeout > 0) {
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(channel->sockfd, &fdset);
        struct timeval tmptv = {(long int)(timeout / 1000000),
                                (long int)(timeout % 1000000)};
        int resultsize = UA_select(channel->sockfd+1, &fdset, NULL, NULL, &tmptv);
        if(resultsize == 0) {
            message->length = 0;
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        }
        if(resultsize == -1) {
            message->length = 0;
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Read the current packet on the socket */
    ssize_t dataLen = recvmsg(channel->sockfd, &msg, 0);
    if(dataLen < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub connection receive failed. Receive message failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if((size_t)dataLen < sizeof(eth_hdr)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub connection receive failed. Packet too small.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(dataLen == 0)
        return UA_STATUSCODE_GOODNODATA;

    /* Make sure we match our target */
    if(memcmp(eth_hdr.ether_dhost, channelDataEthernet->targetAddress, ETH_ALEN) != 0)
        return UA_STATUSCODE_GOODNODATA;

    /* Set the message length */
    message->length = (size_t)dataLen - sizeof(eth_hdr);

    return UA_STATUSCODE_GOOD;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelEthernetETF_close(UA_PubSubChannel *channel) {
    UA_close(channel->sockfd);
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
TransportLayerEthernetETF_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelEthernetETF_open(connectionConfig);
    if(pubSubChannel) {
        pubSubChannel->regist = UA_PubSubChannelEthernetETF_regist;
        pubSubChannel->unregist = UA_PubSubChannelEthernetETF_unregist;
        pubSubChannel->send = UA_PubSubChannelEthernetETF_send;
        pubSubChannel->receive = UA_PubSubChannelEthernetETF_receive;
        pubSubChannel->close = UA_PubSubChannelEthernetETF_close;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

UA_PubSubTransportLayer
UA_PubSubTransportLayerEthernetETF() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerEthernetETF_addChannel;
    return pubSubTransportLayer;
}
