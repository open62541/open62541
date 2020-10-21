/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2018 (c) Jose Cabral, fortiss GmbH
 * Copyright (c) 2020 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2020 Kalycito Infotech Private Limited
 */

#include <open62541/server_pubsub.h>
#include <open62541/util.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>

/* UDP multicast network layer specific internal data */
typedef struct {
    int ai_family;                    /* Protocol family for socket. IPv4/IPv6 */
    struct sockaddr_storage *ai_addr; /* https://msdn.microsoft.com/de-de/library/windows/desktop/ms740496(v=vs.85).aspx */
    UA_UInt32 messageTTL;
    UA_Boolean enableLoopback;
    UA_Boolean enableReuse;
} UA_PubSubChannelDataUDPMC;

/**
 * Open communication socket based on the connectionConfig. Protocol specific parameters are
 * provided within the connectionConfig as KeyValuePair.
 * Currently supported options: "ttl" , "loopback", "reuse"
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelUDPMC_open(const UA_PubSubConnectionConfig *connectionConfig) {
    UA_initialize_architecture_network();

    UA_NetworkAddressUrlDataType address;
    if(UA_Variant_hasScalarType(&connectionConfig->address,
                                &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        address = *(UA_NetworkAddressUrlDataType *)connectionConfig->address.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid Address.");
        return NULL;
    }

    /* Allocate and init memory for the UDP multicast specific internal data */
    UA_PubSubChannelDataUDPMC * channelDataUDPMC = (UA_PubSubChannelDataUDPMC *)
        UA_calloc(1, (sizeof(UA_PubSubChannelDataUDPMC)));
    if(!channelDataUDPMC){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    /* Set default values */
    UA_PubSubChannelDataUDPMC defaultValues = {0, NULL, 255, UA_TRUE, UA_TRUE};
    memcpy(channelDataUDPMC, &defaultValues, sizeof(UA_PubSubChannelDataUDPMC));
    /* Iterate over the given KeyValuePair parameters */
    UA_String ttlParam = UA_STRING("ttl");
    UA_String loopbackParam = UA_STRING("loopback");
    UA_String reuseParam = UA_STRING("reuse");
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++) {
        UA_KeyValuePair *prop = &connectionConfig->connectionProperties[i];
        if(UA_String_equal(&prop->key.name, &ttlParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataUDPMC->messageTTL = *(UA_UInt32*)prop->value.data;
            }
        } else if(UA_String_equal(&prop->key.name, &loopbackParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                channelDataUDPMC->enableLoopback = *(UA_Boolean*)prop->value.data;
            }
        } else if(UA_String_equal(&prop->key.name, &reuseParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                channelDataUDPMC->enableReuse = *(UA_Boolean*)prop->value.data;
            }
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection creation. Unknown connection parameter.");
        }
    }

    UA_PubSubChannel *newChannel = (UA_PubSubChannel *) UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        UA_free(channelDataUDPMC);
        return NULL;
    }
    struct addrinfo hints, *rp, *requestResult = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    UA_String hostname, path;
    UA_UInt16 networkPort;
    if(UA_parseEndpointUrl(&address.url, &hostname, &networkPort, &path) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid URL.");
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }
    if(hostname.length > 512) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. URL maximum length is 512.");
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    UA_STACKARRAY(char, addressAsChar, sizeof(char) * hostname.length +1);
    memcpy(addressAsChar, hostname.data, hostname.length);
    addressAsChar[hostname.length] = 0;
    char port[6];
    sprintf(port, "%u", networkPort);

    if(UA_getaddrinfo(addressAsChar, port, &hints, &requestResult) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Internal error.");
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    bool isMulticast = true;
    /* Check if the ip address is a multicast address */
    if(requestResult->ai_family == PF_INET) {
        struct in_addr imr_interface;
        memset(&imr_interface, 0, sizeof(imr_interface));
        if(UA_inet_pton(AF_INET, addressAsChar, &imr_interface) == 1 &&
           (UA_ntohl(imr_interface.s_addr) & 0xF0000000) != 0xE0000000) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "PubSub Connection is created for a unicast address (IPv4)");
            isMulticast = false;
        }
    } else {
        struct in6_addr imr_interface;
        memset(&imr_interface, 0, sizeof(imr_interface));
        if(UA_inet_pton(AF_INET6, addressAsChar, &imr_interface) == 1 &&
           (imr_interface.s6_addr[0] != 0xFF)) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "PubSub Connection is created for a unicast address (IPv6)");
            isMulticast = false;
        }
    }

    for(rp = requestResult; rp != NULL; rp = rp->ai_next){
        newChannel->sockfd = UA_socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(newChannel->sockfd != UA_INVALID_SOCKET)
            break; /*success*/
    }
    if(!rp) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Internal error.");
        UA_freeaddrinfo(requestResult);
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    channelDataUDPMC->ai_family = rp->ai_family;
    channelDataUDPMC->ai_addr = (struct sockaddr_storage *)
        UA_calloc(1, sizeof(struct sockaddr_storage));
    if(!channelDataUDPMC->ai_addr){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        UA_close(newChannel->sockfd);
        UA_freeaddrinfo(requestResult);
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }
    memcpy(channelDataUDPMC->ai_addr, rp->ai_addr, sizeof(*rp->ai_addr));
    newChannel->handle = channelDataUDPMC; /* Link channel and internal channel data */

    /* Set loop back data to your host */
#if UA_IPV6
    if(UA_setsockopt(newChannel->sockfd,
                     requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     requestResult->ai_family == PF_INET6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP,
                     (const char *)&channelDataUDPMC->enableLoopback,
                     sizeof (channelDataUDPMC->enableLoopback)) < 0)
#else
    if(UA_setsockopt(newChannel->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
                     (const char *)&channelDataUDPMC->enableLoopback,
                     sizeof (channelDataUDPMC->enableLoopback)) < 0)
#endif
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Loopback setup failed.");
        UA_close(newChannel->sockfd);
        UA_freeaddrinfo(requestResult);
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    /* Set Time to live (TTL). Value of 1 prevent forward beyond the local network. */
#if UA_IPV6
    if(UA_setsockopt(newChannel->sockfd,
                     requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     requestResult->ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
                     (const char *)&channelDataUDPMC->messageTTL,
                     sizeof(channelDataUDPMC->messageTTL)) < 0)
#else
    if(UA_setsockopt(newChannel->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                     (const char *)&channelDataUDPMC->messageTTL,
                     sizeof(channelDataUDPMC->messageTTL)) < 0)
#endif
    {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection creation problem. Time to live setup failed.");
    }

    /* Set reuse address -> enables sharing of the same listening address on
     * different sockets */
    if(channelDataUDPMC->enableReuse){
        int enableReuse = 1;
        if(UA_setsockopt(newChannel->sockfd, SOL_SOCKET, SO_REUSEADDR,
                         (const char*)&enableReuse, sizeof(enableReuse)) < 0) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection creation problem. Reuse address setup failed.");
        }
    }

    /* Prepare for following socket options:
     * - Set the physical interface for in- and outgoing traffic (if configured)
     * - Join multicast group */
    enum{
        IPv4,
#if UA_IPV6
        IPv6,
#endif
    } ipVersion;
    union {
        struct ip_mreq ipv4;
#if UA_IPV6
        struct ipv6_mreq ipv6;
#endif
    } group;
    memset(&group, 0, sizeof(group));

    if(requestResult->ai_family == AF_INET &&
       UA_inet_pton(AF_INET, addressAsChar, &group.ipv4.imr_multiaddr) > 0) {
        ipVersion = IPv4;
        /* default configuration: multihomed hosts can join several groups on
         * different IF, INADDR_ANY -> kernel decides */
        group.ipv4.imr_interface.s_addr = htonl(INADDR_ANY);
#if UA_IPV6
    } else if(requestResult->ai_family == AF_INET6 &&
              UA_inet_pton(AF_INET6, addressAsChar, &group.ipv6.ipv6mr_multiaddr) > 0) {
        ipVersion = IPv6;
        group.ipv6.ipv6mr_interface = 0; // default configuration
#endif
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation problem."
                     "Preparing the socket multicast configuration failed");
        goto cleanup;
    }

    if(address.networkInterface.length > 0) {
        /* Set configured interface */
        UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * address.networkInterface.length + 1);
        memcpy(interfaceAsChar, address.networkInterface.data, address.networkInterface.length);
        interfaceAsChar[address.networkInterface.length] = 0;

        if(ipVersion == IPv4) {
            if(UA_inet_pton(AF_INET, interfaceAsChar, &group.ipv4.imr_interface) <= 0) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation problem. "
                             "Interface configuration preparation failed.");
                goto cleanup;
            }
        }
#if UA_IPV6
        else {
            group.ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
            if(group.ipv6.ipv6mr_interface == 0) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation problem. "
                             "Interface configuration preparation failed.");
                goto cleanup;
            }
        }
#endif
    }

#if UA_IPV6
    if(UA_setsockopt(newChannel->sockfd,
          requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
          requestResult->ai_family == PF_INET6 ? IPV6_MULTICAST_IF : IP_MULTICAST_IF,
          ipVersion == IPv6 ? (const void *) &group.ipv6.ipv6mr_interface : &group.ipv4.imr_interface,
          ipVersion == IPv6 ? sizeof(group.ipv6.ipv6mr_interface) : sizeof(struct in_addr)) < 0)
#else
    if(UA_setsockopt(newChannel->sockfd, IPPROTO_IP, IP_MULTICAST_IF,
                     &group.ipv4.imr_interface, sizeof(struct in_addr)) < 0)
#endif
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation problem. Interface selection failed.");
        goto cleanup;
    }

    if(isMulticast){
#if UA_IPV6
        if(UA_setsockopt(newChannel->sockfd,
            requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
            requestResult->ai_family == PF_INET6 ? IPV6_JOIN_GROUP : IP_ADD_MEMBERSHIP,
            ipVersion == IPv6 ? (const void *) &group.ipv6 : &group.ipv4,
            ipVersion == IPv6 ? sizeof(group.ipv6) : sizeof(group.ipv4)) < 0)
#else
        if(UA_setsockopt(newChannel->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        &group.ipv4, sizeof(group.ipv4)) < 0)
#endif
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "PubSub Connection creation problem. Join multicast group failed.");
            goto cleanup;
        }
    }

    UA_freeaddrinfo(requestResult);
    newChannel->state = UA_PUBSUB_CHANNEL_PUB;
    return newChannel;

 cleanup:
    UA_freeaddrinfo(requestResult);
    UA_close(newChannel->sockfd);
    UA_free(channelDataUDPMC);
    UA_free(newChannel);
    return NULL;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDPMC_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                             void (*notUsedHere)(UA_ByteString *encodedBuffer,
                                                 UA_ByteString *topic)) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_RDY)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubChannelDataUDPMC * connectionConfig = (UA_PubSubChannelDataUDPMC *) channel->handle;
    if(connectionConfig->ai_family == PF_INET){//IPv4 handling
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        memcpy(&addr, connectionConfig->ai_addr, sizeof(struct sockaddr_in));
        addr.sin_addr.s_addr = INADDR_ANY;
        if(UA_bind(channel->sockfd, (const struct sockaddr *)&addr,
                    sizeof(struct sockaddr_in)) != 0){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "PubSub Connection regist failed. (IPv4)");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#if UA_IPV6
    } else if(connectionConfig->ai_family == PF_INET6) {//IPv6 handling
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        memcpy(&addr, connectionConfig->ai_addr, sizeof(struct sockaddr_in6));
        addr.sin6_addr = in6addr_any;
        if(UA_bind(channel->sockfd, (const struct sockaddr *)&addr,
                   sizeof(struct sockaddr_in6)) != 0){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "PubSub Connection regist failed. (IPv6)");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#endif
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
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
UA_PubSubChannelUDPMC_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB_SUB || channel->state == UA_PUBSUB_CHANNEL_SUB)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubChannelDataUDPMC * connectionConfig = (UA_PubSubChannelDataUDPMC *) channel->handle;
    if(connectionConfig->ai_family == PF_INET){//IPv4 handling
        struct ip_mreq groupV4;
        memcpy(&groupV4.imr_multiaddr,
               &((const struct sockaddr_in *)connectionConfig->ai_addr)->sin_addr,
               sizeof(struct ip_mreq));
        groupV4.imr_interface.s_addr = UA_htonl(INADDR_ANY);

        if(UA_setsockopt(channel->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                         (char *) &groupV4, sizeof(groupV4)) != 0){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#if UA_IPV6
    } else if (connectionConfig->ai_family == PF_INET6) {//IPv6 handling
        //TODO implement unregist for IPv6
#endif
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
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
UA_PubSubChannelUDPMC_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                           const UA_ByteString *buf) {
    UA_PubSubChannelDataUDPMC *channelConfigUDPMC = (UA_PubSubChannelDataUDPMC *) channel->handle;
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_PUB_SUB)){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection sending failed. Invalid state.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    //TODO evalute: chunk messages or check against MTU?
    long nWritten = 0;
    while (nWritten < (long)buf->length) {
        long n = (long)UA_sendto(channel->sockfd, buf->data, buf->length, 0,
                                 (struct sockaddr *) channelConfigUDPMC->ai_addr,
                                 sizeof(struct sockaddr_storage));
        if(n == -1L) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection sending failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nWritten += n;
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Receive messages. The regist function should be called before.
 *
 * @param timeout in usec | on windows platforms are only multiples of 1000usec possible
 * @return
 */
static UA_StatusCode
UA_PubSubChannelUDPMC_receive(UA_PubSubChannel *channel, UA_ByteString *message,
                              UA_ExtensionObject *transportSettings, UA_UInt32 timeout) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_PUB_SUB)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection receive failed. Invalid state.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubChannelDataUDPMC *channelConfigUDPMC = (UA_PubSubChannelDataUDPMC *) channel->handle;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_UInt16 rcvCount = 0;
    struct timeval tmptv;
    struct timeval receiveTime;
    fd_set fdset;
    size_t remainingMessageLength = 0;
    size_t dataLength = 0;

    memset(&tmptv, 0, sizeof(tmptv));
    memset(&receiveTime, 0, sizeof(receiveTime));
    FD_ZERO(&fdset);
    tmptv.tv_sec  = (long int)(timeout / 1000000);
    tmptv.tv_usec = (long int)(timeout % 1000000);
    remainingMessageLength = message->length;
    do {
        if(timeout > 0) {
            UA_fd_set(channel->sockfd, &fdset);
            /* Select API will return the remaining time in the struct
             * timeval */
            int resultsize = UA_select(channel->sockfd+1, &fdset, NULL,
                                       NULL, &tmptv);
            if(resultsize == 0) {
                retval = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
                if(rcvCount > 0)
                    retval = UA_STATUSCODE_GOOD;
                break;
            }

            if (resultsize == -1) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
        }

        UA_DateTime now = UA_DateTime_nowMonotonic();
        if(channelConfigUDPMC->ai_family == PF_INET){
            ssize_t messageLength = UA_recvfrom(channel->sockfd, (message->data + dataLength), remainingMessageLength, 0, NULL, NULL);
            if(messageLength > 0){
                dataLength += (size_t)messageLength;
                remainingMessageLength -= (size_t)dataLength;
            } else {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
#if UA_IPV6
        } else {
        //TODO implement recieve for IPv6
#endif
        }

        rcvCount++;
        UA_DateTime endtTime = UA_DateTime_nowMonotonic();
        UA_DateTime dataReceiveTime = endtTime - now;
        receiveTime.tv_sec = (long int)(dataReceiveTime / UA_DATETIME_SEC);
        receiveTime.tv_usec = (long int)(dataReceiveTime % UA_DATETIME_SEC);
        UA_DateTime receiveTimeoutValue = (receiveTime.tv_sec * 1000000) + receiveTime.tv_usec;
        UA_DateTime remainingTimeoutValue = (tmptv.tv_sec * 1000000) + tmptv.tv_usec;
        if(remainingTimeoutValue < receiveTimeoutValue) {
            retval = UA_STATUSCODE_GOOD;
            break;
        }

        UA_DateTime newTimeoutValue = remainingTimeoutValue - receiveTimeoutValue;
        tmptv.tv_sec = (long int)(newTimeoutValue  / 1000000);
        tmptv.tv_usec = (long int)(newTimeoutValue % 1000000);
    } while(remainingMessageLength >= 1472); /* TODO:Need to handle for jumbo frames*/
                                             /* 1518 bytes is the maximum size of ethernet packet
                                              * where 18 bytes used for header size, 28 bytes of header
                                              * used for IP and UDP header so remaining length is 1472 */
    message->length = dataLength;
    return retval;
}
/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelUDPMC_close(UA_PubSubChannel *channel) {
    if(UA_close(channel->sockfd) != 0){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection delete failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_deinitialize_architecture_network();
    //cleanup the internal NetworkLayer data
    UA_PubSubChannelDataUDPMC *networkLayerData = (UA_PubSubChannelDataUDPMC *) channel->handle;
    UA_free(networkLayerData->ai_addr);
    UA_free(networkLayerData);
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
TransportLayerUDPMC_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelUDPMC_open(connectionConfig);
    if(pubSubChannel){
        pubSubChannel->regist = UA_PubSubChannelUDPMC_regist;
        pubSubChannel->unregist = UA_PubSubChannelUDPMC_unregist;
        pubSubChannel->send = UA_PubSubChannelUDPMC_send;
        pubSubChannel->receive = UA_PubSubChannelUDPMC_receive;
        pubSubChannel->close = UA_PubSubChannelUDPMC_close;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

//UDPMC channel factory
UA_PubSubTransportLayer
UA_PubSubTransportLayerUDPMP() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerUDPMC_addChannel;
    return pubSubTransportLayer;
}
