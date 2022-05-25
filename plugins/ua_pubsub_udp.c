/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2018 (c) Jose Cabral, fortiss GmbH
 * Copyright (c) 2020 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2020 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Linutronix GmbH (Author: Kurt Kanzenbach)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/server_pubsub.h>
#include <open62541/util.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>

#define RECEIVE_MSG_BUFFER_SIZE   4096
#define UA_DEFAULT_PARAM_SIZE 3

#define UA_MULTICAST_TTL_NO_LIMIT 255

#define IPV4_PREFIX_MASK 0xF0000000
#define IPV4_MULTICAST_PREFIX 0xE0000000
#ifdef UA_IPV6
#   define IPV6_MULTICAST_PREFIX 0xFF
#endif

static UA_THREAD_LOCAL UA_Byte ReceiveMsgBufferUDP[RECEIVE_MSG_BUFFER_SIZE];

typedef union {
    struct ip_mreq ipv4;
#if UA_IPV6
    struct ipv6_mreq ipv6;
#endif
} IpMulticastRequest;

/* UDP multicast network layer specific internal data */
typedef struct {
    int ai_family;                   /* Protocol family for socket. IPv4/IPv6 */
    struct sockaddr_storage ai_addr; /* https://msdn.microsoft.com/de-de/library/windows/desktop/ms740496(v=vs.85).aspx */
    socklen_t ai_addrlen;            /* Address length */
    struct sockaddr_storage intf_addr;
    UA_UInt32 messageTTL;
    UA_Boolean enableLoopback;
    UA_Boolean enableReuse;
    UA_Boolean isMulticast;
#ifdef __linux__
    UA_UInt32* socketPriority;
#endif
    IpMulticastRequest ipMulticastRequest;
} UA_PubSubChannelDataUDP;

#define MAX_URL_LENGTH 512
#define MAX_PORT_CHARACTER_COUNT 6

static UA_StatusCode
getRawAddressAndPortValues(const UA_NetworkAddressUrlDataType *address, char *outAddress, UA_UInt16 *outPort) {

    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = UA_parseEndpointUrl(&address->url, &hostname, outPort, &path);
    if(res != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid URL.");
        return res;
    }

    if(hostname.length >= MAX_URL_LENGTH) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. URL maximum length is 512.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    memcpy(outAddress, hostname.data, hostname.length);
    outAddress[hostname.length] = 0;

    return UA_STATUSCODE_GOOD;
}

typedef struct UA_UDPConnectionContext {
    UA_ConnectionManager *connectionManager;
    uintptr_t connectionId;
} UA_UDPConnectionContext;

/* Callback of a TCP socket (server socket or an active connection) */
static void
UA_PubSub_udpCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                      void *application, void **connectionContext,
                      UA_StatusCode state,
                      size_t paramsSize, const UA_KeyValuePair *params,
                      UA_ByteString msg) {
    UA_PubSubChannel *channel = (UA_PubSubChannel *) application;
    channel->sockfd = (int) connectionId;

    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) *connectionContext;
    ctx->connectionId = connectionId;
    ctx->connectionManager = cm;

    UA_EventSource es = ctx->connectionManager->eventSource;
    // ctx->connectionId = connectionId;
    // ctx->

    // UA_StatusCode retval = receiveCallback(channel, receiveCallbackContext, msg);
    // if(retval != UA_STATUSCODE_GOOD) {
    //     UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
    //                    "PubSub Connection decode and process failed.");
    //     return;
    // }

}

static size_t
UA_KeyValueMap_countMergedMembers(UA_KeyValuePair *lhs, size_t lhsCount, UA_KeyValuePair *rhs, size_t rhsCount) {
    size_t disjointCountInRhs = rhsCount;

    for(size_t i = 0; i < lhsCount; ++i) {
        UA_KeyValuePair lhsPair = lhs[i];
        for(size_t j = 0; j < rhsCount; ++j) {
            UA_KeyValuePair rhsPair = rhs[j];
            if(UA_String_equal(&lhsPair.key.name, &rhsPair.key.name)) {
                disjointCountInRhs--;
            }
        }
    }
    return lhsCount + disjointCountInRhs;
}

static UA_Boolean
UA_KeyValueMap_contains(UA_KeyValuePair *map, size_t mapSize, UA_String key) {
    for(size_t i = 0; i < mapSize; ++i) {
        if(UA_String_equal(&map[i].key.name, &key)) {
            return true;
        }
    }
    return false;
}

static void
UA_KeyValueMap_merge(UA_KeyValuePair *dst, UA_KeyValuePair *lhs, size_t lhsCount, UA_KeyValuePair *rhs, size_t rhsCount) {
    size_t outIndex = 0;
    for(size_t i = 0; i < lhsCount; ++i) {
        UA_KeyValuePair_copy(&lhs[i], &dst[outIndex]);
        for(size_t j = 0; j < rhsCount; ++j) {
            if(UA_String_equal(&lhs[i].key.name, &rhs[j].key.name)) {
                UA_KeyValuePair_copy(&rhs[i], &dst[outIndex]);
                break;
            }
        }
        outIndex++;
    }
    for(size_t i = 0; i < rhsCount; ++i) {
        if(!UA_KeyValueMap_contains(dst, outIndex, rhs[i].key.name)) {
            UA_KeyValuePair_copy(&rhs[i], &dst[outIndex++]);
        }
    }
}

/**
 * Open communication socket based on the connectionConfig. Protocol specific parameters are
 * provided within the connectionConfig as KeyValuePair.
 * Currently supported options: "ttl" , "loopback", "reuse"
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelUDP_open(UA_ConnectionManager *connectionManager, const UA_PubSubConnectionConfig *connectionConfig) {
    UA_initialize_architecture_network();

    if(!UA_Variant_hasScalarType(&connectionConfig->address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid Address.");
        return NULL;
    }
    UA_PubSubChannel *newChannel = (UA_PubSubChannel *)
        UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    UA_NetworkAddressUrlDataType *address =
        (UA_NetworkAddressUrlDataType *)connectionConfig->address.data;

    size_t sizeForAdditionalParams = 2;
    if(address->networkInterface.length > 0) {
        sizeForAdditionalParams = 3;
    }

    UA_KeyValuePair defaultParams[UA_DEFAULT_PARAM_SIZE];
    defaultParams[0].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = UA_MULTICAST_TTL_NO_LIMIT;
    UA_Variant_setScalar(&defaultParams[0].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);

    defaultParams[1].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = true;
    UA_Variant_setScalar(&defaultParams[1].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);

    defaultParams[2].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = true;
    UA_Variant_setScalar(&defaultParams[2].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);

    size_t mergeSize = UA_KeyValueMap_countMergedMembers(defaultParams, UA_DEFAULT_PARAM_SIZE,
                                                         connectionConfig->connectionProperties, connectionConfig->connectionPropertiesSize);
    UA_KeyValuePair mergedParams[mergeSize + sizeForAdditionalParams];

    UA_KeyValueMap_merge(mergedParams + sizeForAdditionalParams,
                         defaultParams, UA_DEFAULT_PARAM_SIZE,
                         connectionConfig->connectionProperties, connectionConfig->connectionPropertiesSize);

    char addressAsChar[MAX_URL_LENGTH];
    UA_UInt16 port;
    UA_StatusCode res = getRawAddressAndPortValues(address, addressAsChar, &port);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(newChannel);
        return NULL;
    }

    UA_String targetHost = UA_STRING(addressAsChar);
    mergedParams[0].key = UA_QUALIFIEDNAME(0, "hostname");
    UA_Variant_setScalar(&mergedParams[0].value, &targetHost, &UA_TYPES[UA_TYPES_STRING]);

    UA_UInt16 targetPort = port;
    mergedParams[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&mergedParams[1].value, &targetPort, &UA_TYPES[UA_TYPES_UINT16]);

    if(address->networkInterface.length > 0) {
        UA_String targetInterface = address->networkInterface;
        mergedParams[2].key = UA_QUALIFIEDNAME(0, "networkInterface");
        UA_Variant_setScalar(&mergedParams[2].value, &targetInterface, &UA_TYPES[UA_TYPES_STRING]);
    }
    UA_UDPConnectionContext *context = (UA_UDPConnectionContext *) UA_calloc(1, sizeof(UA_UDPConnectionContext));
    newChannel->handle = context; /* Link channel and internal channel data */
    // void *application = NULL;
    connectionManager->openConnection(connectionManager,
                                      mergeSize + sizeForAdditionalParams, mergedParams,
                                      newChannel, context, UA_PubSub_udpCallback);
    for(size_t i = 0; i < mergeSize + sizeForAdditionalParams; ++i) {
        UA_KeyValuePair_clear(&mergedParams[i]);
    }
    return newChannel;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDP_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                           void (*notUsedHere)(UA_ByteString *encodedBuffer,
                                                 UA_ByteString *topic)) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_RDY)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubChannelDataUDP * connectionConfig = (UA_PubSubChannelDataUDP *) channel->handle;
    struct ip_mreq groupV4;
    memset(&groupV4, 0, sizeof(struct ip_mreq));
    memcpy(&groupV4.imr_multiaddr,
           &((const struct sockaddr_in *) &connectionConfig->ai_addr)->sin_addr,
           sizeof(struct in_addr));
    memcpy(&groupV4.imr_interface, &connectionConfig->intf_addr, sizeof(struct in_addr));

    if(connectionConfig->isMulticast){
#if UA_IPV6
        struct ipv6_mreq groupV6 = { 0 };

        memcpy(&groupV6.ipv6mr_multiaddr,
               &((const struct sockaddr_in6 *) &connectionConfig->ai_addr)->sin6_addr,
               sizeof(struct in6_addr));

        if(UA_setsockopt(channel->sockfd,
            connectionConfig->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
            connectionConfig->ai_family == PF_INET6 ? IPV6_ADD_MEMBERSHIP : IP_ADD_MEMBERSHIP,
            connectionConfig->ai_family == PF_INET6 ? (const void *) &groupV6 : &groupV4,
            connectionConfig->ai_family == PF_INET6 ? sizeof(groupV6) : sizeof(groupV4)) < 0)
#else
        if(UA_setsockopt(channel->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        &groupV4, sizeof(groupV4)) < 0)
#endif
        {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection regist failed. IP membership setup failed: "
                             "Cannot set socket option IP_ADD_MEMBERSHIP. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Remove current subscription.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDP_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB_SUB || channel->state == UA_PUBSUB_CHANNEL_SUB)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubChannelDataUDP * connectionConfig = (UA_PubSubChannelDataUDP *) channel->handle;
    if(connectionConfig->ai_family == PF_INET){//IPv4 handling
        struct ip_mreq groupV4 = { 0 };

        memcpy(&groupV4.imr_multiaddr,
               &((const struct sockaddr_in *) &connectionConfig->ai_addr)->sin_addr,
               sizeof(struct in_addr));
        groupV4.imr_interface.s_addr = UA_htonl(INADDR_ANY);

        if(UA_setsockopt(channel->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                         (char *) &groupV4, sizeof(groupV4)) != 0){
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection unregist failed. IP membership setup failed: "
                             "Cannot set socket option IP_DROP_MEMBERSHIP. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#if UA_IPV6
    } else if (connectionConfig->ai_family == PF_INET6) {//IPv6 handling
        struct ipv6_mreq groupV6 = { 0 };

        memcpy(&groupV6.ipv6mr_multiaddr,
               &((const struct sockaddr_in6 *) &connectionConfig->ai_addr)->sin6_addr,
               sizeof(struct in6_addr));

        if(UA_setsockopt(channel->sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
                         (char *) &groupV6, sizeof(groupV6)) != 0){
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection unregist failed. IP membership setup failed: "
                             "Cannot set socket option IPV6_DROP_MEMBERSHIP. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
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
UA_PubSubChannelUDP_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                         UA_ByteString *buf) {

    UA_UDPConnectionContext *udpContext = (UA_UDPConnectionContext *) channel->handle;
    UA_ConnectionManager *cm = udpContext->connectionManager;

    uintptr_t connectionId = udpContext->connectionId;

    UA_EventSource es = cm->eventSource;

    return cm->sendWithConnection(cm, connectionId, 0, NULL, buf);
}

static
UA_INLINE
UA_DateTime timevalToDateTime(struct timeval val) {
    return val.tv_sec * UA_DATETIME_SEC + val.tv_usec / 100;
}

/**
 * Receive messages. The regist function should be called before.
 *
 * @param timeout in usec | on windows platforms are only multiples of 1000usec possible
 * @return
 */
static UA_StatusCode
UA_PubSubChannelUDP_receive(UA_PubSubChannel *channel,
                            UA_ExtensionObject *transportSettings,
                            UA_PubSubReceiveCallback receiveCallback,
                            void *receiveCallbackContext,
                            UA_UInt32 timeout) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB ||
         channel->state == UA_PUBSUB_CHANNEL_PUB_SUB)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection receive failed. Invalid state.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_UInt16 rcvCount = 0;
    struct timeval timeoutValue;
    struct timeval receiveTime;
    fd_set fdset;

    memset(&timeoutValue, 0, sizeof(timeoutValue));
    memset(&receiveTime, 0, sizeof(receiveTime));
    FD_ZERO(&fdset);
    timeoutValue.tv_sec  = (long int)(timeout / 1000000);
    timeoutValue.tv_usec = (long int)(timeout % 1000000);
    do {
        if(timeout > 0) {
            UA_fd_set(channel->sockfd, &fdset);
            /* Select API will return the remaining time in the struct
             * timeval */
            int resultsize = UA_select(channel->sockfd+1, &fdset, NULL,
                                       NULL, &timeoutValue);
            if(resultsize == 0) {
                retval = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
                if(rcvCount > 0)
                    retval = UA_STATUSCODE_GOOD;
                break;
            }

            if (resultsize == -1) {
                UA_LOG_SOCKET_ERRNO_WRAP(
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                                   "PubSub Connection receiving failed: "
                                   "select failed. Error: %s", errno_str));
                retval = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
        }
        UA_ByteString buffer;
        buffer.length = RECEIVE_MSG_BUFFER_SIZE;
        buffer.data = ReceiveMsgBufferUDP;

        UA_DateTime beforeRecvTime = UA_DateTime_nowMonotonic();
        ssize_t messageLength = UA_recvfrom(channel->sockfd, buffer.data,
                                            RECEIVE_MSG_BUFFER_SIZE, 0, NULL, NULL);
        if(messageLength > 0){
            buffer.length = (size_t) messageLength;
            retval = receiveCallback(channel, receiveCallbackContext, &buffer);
            if(retval != UA_STATUSCODE_GOOD) {
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                                   "PubSub Connection decode and process failed.");

            }

        } else {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                               "PubSub Connection receiving failed: "
                               "recvfrom failed. Error: %s", errno_str));
            retval = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        rcvCount++;
        UA_DateTime endTime = UA_DateTime_nowMonotonic();
        UA_DateTime receiveDuration = endTime - beforeRecvTime;

        UA_DateTime remainingTimeoutValue = timevalToDateTime(timeoutValue);
        if(remainingTimeoutValue < receiveDuration) {
            retval = UA_STATUSCODE_GOOD;
            break;
        }

        UA_DateTime newTimeoutValue = remainingTimeoutValue - receiveDuration;
        timeoutValue.tv_sec = (long int)(newTimeoutValue  / UA_DATETIME_SEC);
        timeoutValue.tv_usec = (long int)((newTimeoutValue % UA_DATETIME_SEC) * 100);

    } while(true); /* TODO:Need to handle for jumbo frames*/
                                             /* 1518 bytes is the maximum size of ethernet packet
                                              * where 18 bytes used for header size, 28 bytes of header
                                              * used for IP and UDP header so remaining length is 1472 */
    // message->length = dataLength;
    return retval;
}
/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelUDP_close(UA_PubSubChannel *channel) {
    //cleanup the internal NetworkLayer data
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
TransportLayerUDP_addChannel(UA_PubSubTransportLayer *tl, UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelUDP_open(tl->connectionManager, connectionConfig);
    if(pubSubChannel){
        pubSubChannel->regist = UA_PubSubChannelUDP_regist;
        pubSubChannel->unregist = UA_PubSubChannelUDP_unregist;
        pubSubChannel->send = UA_PubSubChannelUDP_send;
        pubSubChannel->receive = UA_PubSubChannelUDP_receive;
        pubSubChannel->close = UA_PubSubChannelUDP_close;
        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

//UDP channel factory
UA_PubSubTransportLayer
UA_PubSubTransportLayerUDP(UA_EventLoop* el) {

    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.connectionManager = UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp-cm"));
    el->registerEventSource(el, &pubSubTransportLayer.connectionManager->eventSource);

    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerUDP_addChannel;
    return pubSubTransportLayer;
}
