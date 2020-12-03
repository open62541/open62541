/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#include <open62541/server_pubsub.h>
#include <open62541/util.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>

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

#define TIMEOUT_REALTIME                                15

/* Ethernet network layer specific internal data */
typedef struct {
    int ifindex;
    UA_UInt16 vid;
    UA_Byte prio;
    UA_Byte ifAddress[ETH_ALEN];
    UA_Byte targetAddress[ETH_ALEN];
} UA_PubSubChannelDataEthernet;

/* Structure for Logical link control based on 802.2 */
typedef struct  {
    UA_Byte dsap;   /* Destination Service Access Point */
    UA_Byte ssap;   /* Source Service Access point */
    UA_Byte ctrl_1; /* Control Field */
    UA_Byte ctrl_2; /* Control Field */
} llc_pdu;

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
    UA_snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name),
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
    sll.sll_protocol = htons(ETH_P_802_2);

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
     * header (without VLAN tag), the VLAN tag, the LLC header and the OPC-UA/Ethernet data. */
    char *bufSend, *ptrCur;
    size_t lenBuf;
    struct ether_header* ethHdr;
    llc_pdu* llcData;

    /* Below added 4 bytes for the size of VLAN tag */
    lenBuf = sizeof(*ethHdr) + 4 + sizeof(*llcData) + buf->length;
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
        ethHdr->ether_type = htons((UA_UInt16)(buf->length + sizeof(*llcData)));
        lenBuf -= 4;  /* no VLAN tag */
    } else {
        ethHdr->ether_type = htons(ETHERTYPE_VLAN);
        /* set VLAN ID */
        UA_UInt16 vlanTag;
        vlanTag = (UA_UInt16) (channelDataEthernet->vid + (channelDataEthernet->prio << 13));
        *((UA_UInt16 *) ptrCur) = htons(vlanTag);
        ptrCur += sizeof(UA_UInt16);
        /* set Ethernet */
        *((UA_UInt16 *) ptrCur) = htons((UA_UInt16)(buf->length + sizeof(*llcData)));
        ptrCur += sizeof(UA_UInt16);
    }

    llcData = (llc_pdu*)ptrCur;
    /* Set 802.3 with 802.2(Logical Link Control)*/
    llcData->dsap = 0;
    llcData->ssap = 0;
    llcData->ctrl_1 = 0;
    llcData->ctrl_2 = 0;
    ptrCur += sizeof(*llcData);

    /* copy payload of ethernet message */
    memcpy(ptrCur, buf->data, buf->length);

    /* Send the packets at the given Txtime */
    UA_StatusCode rc = sendWithTxTime(channel, transportSettings, bufSend, lenBuf);
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

    struct timeval  tmptv;
    struct timespec currentTime;
    struct timespec maxTime;
    UA_UInt64       currentTimeValue = 0;
    UA_UInt64       maxTimeValue = 0;
    UA_Int32        receiveFlags;
    UA_StatusCode   retval;
    UA_UInt16       rcvCount = 0;

    memset(&tmptv, 0, sizeof(tmptv));

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
        tmptv.tv_sec = (long int)(timeout / 1000000);
        tmptv.tv_usec = (long int)(timeout % 1000000);
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
    clock_gettime(CLOCK_TAI, &currentTime);
    currentTimeValue = (UA_UInt64)((currentTime.tv_sec * 1000000000) + currentTime.tv_nsec);
    maxTime.tv_sec   = currentTime.tv_sec + tmptv.tv_sec;
    /* UA_Select uses timespec which accpets value in microseconds
     * but etf code requires precision of nanoseconds */
    maxTime.tv_nsec  = currentTime.tv_nsec + (tmptv.tv_usec * 1000);
    maxTimeValue     = (UA_UInt64)((maxTime.tv_sec * 1000000000)+ maxTime.tv_nsec);
    /* Receive flags set to Zero which indicates it will wait inside recvmsg API untill
     * first packet received */
    receiveFlags     = 0;
    size_t messageLength = 0;
    size_t remainingMessageLength = 0;
    remainingMessageLength = message->length;

    do {
        if(maxTimeValue < currentTimeValue) {
             retval = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
             break;
        }

        struct ether_header eth_hdr;
        struct iovec        iov[3];
        struct msghdr       msg;
        ssize_t             dataLen;
        llc_pdu             llcData;
        UA_UInt16           payloadLength;
        size_t              paddingBytes;
        memset(&dataLen, 0, sizeof(dataLen));
        memset(&msg, 0, sizeof(msg));

        iov[0].iov_base = &eth_hdr;
        iov[0].iov_len  = sizeof(eth_hdr);
        iov[1].iov_base = &llcData;
        iov[1].iov_len  = sizeof(llcData);
        iov[2].iov_base = message->data + messageLength;
        iov[2].iov_len  = remainingMessageLength;
        msg.msg_iov     = iov;
        msg.msg_iovlen  = 3;

        dataLen = recvmsg(channel->sockfd, &msg, receiveFlags);
        if(dataLen < 0) {
            if(rcvCount == 0) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub connection receive failed. Receive message failed.");
                retval = UA_STATUSCODE_BADINTERNALERROR;
            }
            else {
                retval = UA_STATUSCODE_GOOD;
            }
            break;
        }

        if((size_t)(dataLen) < sizeof(struct ether_header)) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "PubSub connection receive failed. Packet too small.");
            retval = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        if(dataLen == 0) {
            retval = UA_STATUSCODE_GOODNODATA;
            break;
        }

        /* Make sure we match our target */
        if(memcmp(eth_hdr.ether_dhost, channelDataEthernet->targetAddress, ETH_ALEN) != 0) {
            retval = UA_STATUSCODE_GOODNODATA;
            break;
        }

        payloadLength = (UA_UInt16)((htons(eth_hdr.ether_type)) - sizeof(llcData));
        paddingBytes  = (size_t)dataLen - sizeof(struct ether_header) - sizeof(llcData) - payloadLength;
        messageLength = messageLength + ((size_t)dataLen - sizeof(struct ether_header) - sizeof(llcData) - paddingBytes);
        remainingMessageLength -= messageLength;
        rcvCount++;
        clock_gettime(CLOCK_TAI, &currentTime);
        currentTimeValue = (UA_UInt64)((currentTime.tv_sec * 1000000000) + currentTime.tv_nsec);
        /* Receive flags set to MSG_DONTWAIT for the 2nd packet */
        /* The recvmsg API with MSG_DONTWAIT flag will not wait for the next packet */
        receiveFlags = MSG_DONTWAIT;

    } while(remainingMessageLength >= 1496); /* 1518 bytes is the maximum size of ethernet packet
                                              * where 18 bytes used for header size, 4 bytes of LLC
                                              * so remaining length is 1496 */
    message->length = messageLength;
    return retval;

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
