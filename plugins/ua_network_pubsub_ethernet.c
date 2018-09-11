/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright 2018 (c) Kontron Europe GmbH (Author: Rudolf Hoyler)
 */

#include "ua_plugin_network.h"
#include "ua_log_stdout.h"
#include "ua_util.h"

/* **NOTE: Currently ONLY for LINUX** */
#include <netpacket/packet.h>
#include <netinet/ether.h>

#include "ua_network_pubsub_ethernet.h"

#define ETHERTYPE_UADP  0xB62C

/* internal structure */
#define MAX_INTERFACE_NAME 64
#define MAX_MAC_ADDR       18

/* ethernet network layer specific internal data */
typedef struct {
    char interfaceName[MAX_INTERFACE_NAME];
    char destinationMac[MAX_MAC_ADDR];
    UA_UInt16 vid;
    UA_Byte prio;
    u_int8_t etherSourceAddr[ETH_ALEN];
} UA_PubSubChannelDataEth;

/* for UDP/TCP exists 'UA_parseEndpointUrl' (ua_util.c).
 * This is adapted for ethernet here locally.
 */

static UA_StatusCode
UA_parseEndpointUrlEthernet(const UA_String *endpointUrl, UA_PubSubChannelDataEth *channelDataEth) {
    /* related to OPC-UA specification the URL for ethernet should be:
     *    opc.eth://<host>[:<vid>[.<pcp>]]
     * with <host> .. MAC address, IP address or name
     *      <vid>  .. VLAN ID
     *      <pcp>  .. priority
     */
    char urlStr[64];
    char *ptr, *ptrHost;
    char *ptrVid = NULL;
    char *ptrPrio = NULL;
    int i;

    if (endpointUrl->length > sizeof(urlStr)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    sprintf(urlStr, "%.*s", (int) endpointUrl->length, endpointUrl->data);

    if (strncmp(urlStr, "opc.eth://", 10) != 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* set host address */
    ptrHost = urlStr + 10;

    /* check for VLAN Id and priority */
    ptr = strchr(ptrHost, ':');
    if (ptr != NULL) {
        *ptr = 0;
        ptrVid = ptr + 1;

        /* check for VLAN priority */
        ptr = strchr(ptrVid, '.');
        if (ptr != NULL) {
            *ptr = 0;
            ptrPrio = ptr + 1;
        }
    }
    snprintf(channelDataEth->destinationMac, MAX_MAC_ADDR, "%s", ptrHost);
    for (i = 0; channelDataEth->destinationMac[i] != 0; i++) {
        if (channelDataEth->destinationMac[i] == ' ')
            channelDataEth->destinationMac[i] = ':';
    }
    channelDataEth->vid  = (UA_UInt16) ((ptrVid == NULL) ? 0 : atoi(ptrVid));
    channelDataEth->prio = (UA_Byte) ((ptrPrio == NULL) ? 0 : atoi(ptrPrio));

    return UA_STATUSCODE_GOOD;
}

/*****************************************************************************
 * Plugin Functions
 ****************************************************************************/

/**
 * Open communication socket based on the connectionConfig.
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelEthernet_open(const UA_PubSubConnectionConfig *connectionConfig) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Open PubSub ethernet connection.");

    /* allocate and init memory for the ethernet specific internal data */
    UA_PubSubChannelDataEth* channelDataEth =
            (UA_PubSubChannelDataEth*) UA_malloc(sizeof(*channelDataEth));
    if (!channelDataEth) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }
    memset(channelDataEth, 0, sizeof(*channelDataEth));

    /* handle specified network address */
    UA_NetworkAddressUrlDataType address;
    if (UA_Variant_hasScalarType (&connectionConfig->address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        address = *(UA_NetworkAddressUrlDataType *) connectionConfig->address.data;
    } else {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Invalid Address.");
        UA_free(channelDataEth);
        return NULL;
    }
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Interface Name = %.*s",
         (int) address.networkInterface.length, address.networkInterface.data);
    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Specified Network Url = %.*s",
         (int)address.url.length, address.url.data);

    snprintf(channelDataEth->interfaceName,
             sizeof(channelDataEth->interfaceName), "%.*s",
             (int) address.networkInterface.length,
             address.networkInterface.data);

    /* encode the URL and store information in internal structure */
    if (UA_parseEndpointUrlEthernet(&address.url, channelDataEth)) {
        UA_LOG_ERROR (UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub Connection creation failed. Invalid Address URL.");
        UA_free(channelDataEth);
        return NULL;
    }

    /* generate a new Pub/Sub channel and open a related socket */
    UA_PubSubChannel *newChannel;
    newChannel = (UA_PubSubChannel*)UA_calloc(1, sizeof(UA_PubSubChannel));
    if (!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        UA_free(channelDataEth);
        return NULL;
    }

    /* open a RAW socket */
    int sockFd;
    if ((sockFd = UA_socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot create socket.");
                UA_free(channelDataEth);
        UA_free(channelDataEth);
        UA_free(newChannel);
        return NULL;
    }
    newChannel->sockfd = sockFd;

    /* allow the socket to be reused */
    int opt = 1;
    if (UA_setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot set socket reuse.");
        UA_close(sockFd);
        UA_free(channelDataEth);
        UA_free(newChannel);
        return NULL;
    }

    /* set interface */
    struct ifreq ifopts;
    strncpy(ifopts.ifr_name, channelDataEth->interfaceName, sizeof(ifopts.ifr_name));
    ifopts.ifr_name[sizeof(ifopts.ifr_name)-1] = 0;

    if (ioctl(sockFd, SIOCGIFINDEX, &ifopts) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
           "PubSub connection creation failed. Cannot set interface.");
        UA_close(sockFd);
        UA_free(channelDataEth);
        UA_free(newChannel);
        return NULL;
    }

    struct sockaddr_ll sll;
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifopts.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    /* bind the socket to interface */
    if (UA_bind(sockFd, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot bind socket.");
        UA_close(sockFd);
        UA_free(channelDataEth);
        UA_free(newChannel);
        return NULL;
    }

    /* determine own MAC address (source address for send) */
    memset(&ifopts, 0, sizeof(struct ifreq));
    strncpy(ifopts.ifr_name, channelDataEth->interfaceName, sizeof(ifopts.ifr_name));
    if (ioctl(sockFd, SIOCGIFHWADDR, &ifopts) < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection creation failed. Cannot determine own MAC address.");
        UA_close(sockFd);
        UA_free(channelDataEth);
        UA_free(newChannel);
        return NULL;
    }
    memcpy(channelDataEth->etherSourceAddr, &ifopts.ifr_hwaddr.sa_data,
           sizeof(channelDataEth->etherSourceAddr));

    newChannel->handle = channelDataEth;
    newChannel->state = UA_PUBSUB_CHANNEL_PUB;

    return newChannel;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelEthernet_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    (void) channel;
    (void) transportSettings;
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                 "UA_PubSubChannelEth_regist NOT applicable");
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

/**
 * Remove current subscription.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelEthernet_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    (void) channel;
    (void) transportSettings;
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                 "UA_PubSubChannelEth_unregist NOT applicable");
    return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
}

/**
 * Send messages to the connection defined address
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelEthernet_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings, const UA_ByteString *buf) {
    (void) transportSettings;

    UA_PubSubChannelDataEth *channelDataEth;
    channelDataEth = (UA_PubSubChannelDataEth *) channel->handle;

    /* allocate a buffer for the ethernet data which contains the ethernet
     * header (without VLAN tag), the VLAN tag and the OPC-UA/UADP data.
     */
    char *bufSend, *ptrCur;
    size_t lenBuf;
    struct ether_header* ethHdr;

    lenBuf = sizeof(*ethHdr) + 4 + buf->length;
    bufSend = (char*) UA_malloc(lenBuf);
    ethHdr = (struct ether_header*) bufSend;

    /* set (own) source MAC address */
    memcpy(ethHdr->ether_shost, channelDataEth->etherSourceAddr, ETH_ALEN);

    /* set destination MAC address */
    if (ether_aton_r(channelDataEth->destinationMac,
            (struct ether_addr*)&ethHdr->ether_dhost) == (struct ether_addr*) 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection send failed. Cannot set destination MAC address.");
        UA_free(bufSend);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* set ethertype */
    /* either VLAN or UADP */
    ptrCur = bufSend + sizeof(*ethHdr);
    if (channelDataEth->vid == 0) {
        ethHdr->ether_type = htons(ETHERTYPE_UADP);
        lenBuf -= 4;  /* no VLAN tag */
    } else {
        ethHdr->ether_type = htons(ETHERTYPE_VLAN);
        /* set VLAN ID */
        UA_UInt16 vlanTag;
        vlanTag = (UA_UInt16) (channelDataEth->vid + (channelDataEth->prio << 13));
        *((UA_UInt16 *) ptrCur) = htons(vlanTag);
        ptrCur += sizeof(UA_UInt16);
        /* set UADP */
        *((UA_UInt16 *) ptrCur) = htons(ETHERTYPE_UADP);
        ptrCur += sizeof(UA_UInt16);
    }

    /* copy payload of ethernet message */
    memcpy(ptrCur, buf->data, buf->length);

    ssize_t rc;
    rc = UA_send(channel->sockfd, bufSend, lenBuf, 0);
    if (rc  < 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection send failed. Send message failed.");
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
UA_PubSubChannelEthernet_receive(UA_PubSubChannel *channel, UA_ByteString *message, UA_ExtensionObject *transportSettings, UA_UInt32 timeout) {
    static unsigned char buffer[2048];
    ssize_t dataLen;

    (void) transportSettings;
    (void) timeout;

    dataLen = UA_recv(channel->sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (dataLen < 0) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            return UA_STATUSCODE_GOODNODATA;
        }
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection receive failed. Receive message failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    } else if (dataLen == 0) {
        return UA_STATUSCODE_GOODNODATA;
    }

    /* handle VLAN tag */
    struct ether_header *ethHdr;
    ssize_t ethHdrLen;
    UA_UInt16 etherType;

    ethHdr = (struct ether_header*) buffer;
    ethHdrLen = sizeof(*ethHdr);
    etherType = ntohs(ethHdr->ether_type);
    if (etherType == ETHERTYPE_VLAN) {
        ethHdrLen += 4;
        etherType = *((UA_UInt16 *) (ethHdr + ethHdrLen + 2));
    }
    if (etherType != ETHERTYPE_UADP) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection receive failed. Not UADP format.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (dataLen < ethHdrLen) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection receive failed. Packet too small.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (dataLen > (ssize_t) message->length) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
            "PubSub connection receive failed. Message too long.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* set UADP message bytes */
    dataLen -= ethHdrLen;
    memcpy(message->data, &buffer[ethHdrLen], (size_t) dataLen);
    message->length = (size_t) dataLen;

    return UA_STATUSCODE_GOOD;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelEthernet_close(UA_PubSubChannel *channel) {
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
TransportLayerEthernet_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelEthernet_open(connectionConfig);
    if (pubSubChannel) {
        pubSubChannel->regist = UA_PubSubChannelEthernet_regist;
        pubSubChannel->unregist = UA_PubSubChannelEthernet_unregist;
        pubSubChannel->send = UA_PubSubChannelEthernet_send;
        pubSubChannel->receive = UA_PubSubChannelEthernet_receive;
        pubSubChannel->close = UA_PubSubChannelEthernet_close;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

//ETH channel factory
UA_PubSubTransportLayer
UA_PubSubTransportLayerEthernet() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI_ETHERNET);
    pubSubTransportLayer.createPubSubChannel = &TransportLayerEthernet_addChannel;
    return pubSubTransportLayer;
}

/* (end of file) */
