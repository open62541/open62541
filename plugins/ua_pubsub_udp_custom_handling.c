/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2018 (c) Jose Cabral, fortiss GmbH
 * Copyright 2019 (c) Kalycito Infotech Private Limited
 */

/* Enable POSIX features */
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <open62541/plugin/network.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/util.h>
#include <open62541/plugin/log_stdout.h>

#include <linux/errqueue.h>
#include <poll.h>
#include <linux/types.h>

#include "time.h"

#define       SECONDS                            1000 * 1000 * 1000
#define       ONE                                1
#define       HOST_NAME_LENGTH                   512
#define       FAILURE_EXIT                       -1
#define       SHIFT_32BITS                       32

#ifndef       SO_TXTIME
#define       SO_TXTIME                          61
#define       SCM_TXTIME                         SO_TXTIME
#define       SCM_DROP_IF_LATE                   62
#define       SCM_CLOCKID                        63
#endif

#ifndef       SO_EE_ORIGIN_TXTIME
#define       SO_EE_ORIGIN_TXTIME                6
#define       SO_EE_CODE_TXTIME_INVALID_PARAM    1
#define       SO_EE_CODE_TXTIME_MISSED           2
#endif

#define       MULTICAST_ADDRESS                  "224.0.0.32"
#define       PUBSUB_IP_ADDRESS                  "192.168.9.10"

#define       PRINT_ERROR(ERROR_INFO)            fprintf(stderr, ERROR_INFO "\n")

extern struct timespec        nextCycleStartTime;
clockid_t     clockId         = CLOCK_TAI;
struct        timespec        timeSpecCalculation;
ssize_t       dataCount;
UA_Int32      errorCount;
__u64         txtime;
/* Qbv offset is 5us for i5. For Mbox, qbv offset is 25us */
__u64              qbv_offset      = 25 * 1000;
UA_Int32           waketx_delay    = 50000;
static UA_Int32    txTimeEnable    = 1;
static UA_Int32    soPriority      = 3;
static struct sock_txtime     txtimeSocket;
static unsigned char txBuffer[256];

/* The API for SO_TXTIME is the below struct and enum, which will be
 * provided by uapi/linux/net_tstamp.h in the near future.
 */
struct sock_txtime {
    clockid_t clockid;
    uint16_t flags;
};

enum txtime_flags {
    SOF_TXTIME_DEADLINE_MODE = (1 << 0),
    SOF_TXTIME_REPORT_ERRORS = (1 << 1),

    SOF_TXTIME_FLAGS_LAST = SOF_TXTIME_REPORT_ERRORS,
    SOF_TXTIME_FLAGS_MASK = (SOF_TXTIME_FLAGS_LAST - 1) |
                 SOF_TXTIME_FLAGS_LAST
};

/* UDP multicast network layer specific internal data */
typedef struct {
    /*Protocol family for socket.  IPv4/IPv6*/
    UA_Int32   ai_family;
    /*https://msdn.microsoft.com/de-de/library/windows/desktop/ms740496(v=vs.85).aspx*/
    struct sockaddr_storage *ai_addr;
    UA_UInt32  messageTTL;
    UA_Boolean enableLoopback;
    UA_Boolean enableReuse;
} UA_PubSubChannelDataUDPMC;

static ssize_t udp_send(UA_Int32 fd, void *buf, UA_Int32 len, __u64 tx_time, clockid_t clk_id) {
    /* Send the data packet with the tx time */
    char dataPacket[CMSG_SPACE(sizeof(tx_time))] = {0};
    /* Structure for socket internet address */
    struct sockaddr_in    socketAddress;
    /* Structure for storing the necessary data */
    struct cmsghdr*       controlMsg;
    /* Structure for messages sent and received */
    struct msghdr         message;
    /* Structure for scattering or gathering of input/output */
    struct iovec          inputOutputVec;
    ssize_t               msgCount;
    static struct in_addr mcast_addr;

    if(!inet_aton(MULTICAST_ADDRESS, &mcast_addr)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    memset(&socketAddress, 0, sizeof(socketAddress));
    /* Provide the socket family, port and multicast address*/
    socketAddress.sin_family  = AF_INET;
    socketAddress.sin_addr    = mcast_addr;
    socketAddress.sin_port    = htons(4840);

    /* Provide the base address and length */
    inputOutputVec.iov_base   = buf;
    inputOutputVec.iov_len    = (size_t)len;

    memset(&message, 0, sizeof(message));
    /* Provide message name / optional address */
    message.msg_name          = &socketAddress;
    /* Provide message address size in bytes */
    message.msg_namelen       = sizeof(socketAddress);
    /* Provide array of input/output buffers */
    message.msg_iov           = &inputOutputVec;
    /* Provide the number of elements in the array */
    message.msg_iovlen        = 1;

    /*
     * We specify the transmission time in the CMSG.
     */
    if(txTimeEnable) {
        /* Provide the necessary data */
        message.msg_control    = dataPacket;
        /* Provide the size of necessary bytes */
        message.msg_controllen = sizeof(dataPacket);

        /* Control message created for tx time */
        controlMsg                         = CMSG_FIRSTHDR(&message);
        controlMsg->cmsg_level             = SOL_SOCKET;
        controlMsg->cmsg_type              = SCM_TXTIME;
        controlMsg->cmsg_len               = CMSG_LEN(sizeof(__u64));
        *((__u64 *) CMSG_DATA(controlMsg)) = tx_time;
    }

    msgCount = sendmsg(fd, &message, 0);
    if(msgCount < 1) {
        printf("sendmsg failed: ");
        return msgCount;
    }

    return msgCount;
}

static int sockErrorQueueProcess(int fd) {
    uint8_t dataErrorPacket[CMSG_SPACE(sizeof(struct sock_extended_err))];
    unsigned char errorBuffer[sizeof(txBuffer)];
    /* Structure for storing the error queue*/
    struct sock_extended_err* sockErrorQueue;
    /* Structure for storing the error data */
    struct cmsghdr*           controlErrorMsg;
    __u64 timeStamp           = 0;

    /* Structure for gathering of input/output error buffers */
    struct iovec inputOutputErrorVec = {
            .iov_base = errorBuffer,
            .iov_len  = sizeof(errorBuffer)
    };

    /* Structure for error messages sent and received */
    struct msghdr messageError = {
            .msg_iov        = &inputOutputErrorVec,
            .msg_iovlen     = 1,
            .msg_control    = dataErrorPacket,
            .msg_controllen = sizeof(dataErrorPacket)
    };

    if(recvmsg(fd, &messageError, MSG_ERRQUEUE) == FAILURE_EXIT) {
        PRINT_ERROR("recvmsg failed");
            return FAILURE_EXIT;
    }

    controlErrorMsg = CMSG_FIRSTHDR(&messageError);
    while(controlErrorMsg != NULL) {
        sockErrorQueue = (void *) CMSG_DATA(controlErrorMsg);
        if(sockErrorQueue->ee_origin == SO_EE_ORIGIN_TXTIME) {
            timeStamp = ((__u64) sockErrorQueue->ee_data << SHIFT_32BITS) + sockErrorQueue->ee_info;
            switch(sockErrorQueue->ee_code) {
            case SO_EE_CODE_TXTIME_INVALID_PARAM:
                fprintf(stderr, "packet with timeStamp %llu dropped due to invalid params\n", timeStamp);
                return 0;
            case SO_EE_CODE_TXTIME_MISSED:
                fprintf(stderr, "packet with timeStamp %llu dropped due to missed deadline\n", timeStamp);
                return 0;
                default:
                    return -1;
            }
        }

        controlErrorMsg = CMSG_NXTHDR(&messageError, controlErrorMsg);
    }

    return UA_STATUSCODE_GOOD;
}

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
    if(UA_Variant_hasScalarType(&connectionConfig->address, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        address = *(UA_NetworkAddressUrlDataType *)connectionConfig->address.data;
    }
    else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Invalid Address.");
        return NULL;
    }

    /* allocate and init memory for the UDP multicast specific internal data */
    UA_PubSubChannelDataUDPMC * channelDataUDPMC =
            (UA_PubSubChannelDataUDPMC *) UA_calloc(1, (sizeof(UA_PubSubChannelDataUDPMC)));
    if(!channelDataUDPMC) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    /* set default values */
    UA_PubSubChannelDataUDPMC defaultValues = {0, NULL, 255, UA_TRUE, UA_TRUE};
    memcpy(channelDataUDPMC, &defaultValues, sizeof(UA_PubSubChannelDataUDPMC));
    /* iterate over the given KeyValuePair paramters */
    UA_String ttlParam = UA_STRING("ttl"), loopbackParam = UA_STRING("loopback"), reuseParam = UA_STRING("reuse");
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++) {
        if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &ttlParam)) {
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])) {
                channelDataUDPMC->messageTTL = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }

        }
        else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &loopbackParam)) {
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                channelDataUDPMC->enableLoopback = *(UA_Boolean *) connectionConfig->connectionProperties[i].value.data;
            }

        }
        else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &reuseParam)) {
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                channelDataUDPMC->enableReuse = *(UA_Boolean *) connectionConfig->connectionProperties[i].value.data;
            }

        }
        else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation. Unknown connection parameter.");
        }

    }

    UA_PubSubChannel *newChannel = (UA_PubSubChannel *) UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
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
    if(UA_parseEndpointUrl(&address.url, &hostname, &networkPort, &path) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid URL.");
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    if(hostname.length > HOST_NAME_LENGTH) {
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

    /* check if the ip address is a multicast address */
    if(requestResult->ai_family == PF_INET) {
        struct in_addr imr_interface;
        UA_inet_pton(AF_INET, addressAsChar, &imr_interface);
        if((UA_ntohl(imr_interface.s_addr) & 0xF0000000) != 0xE0000000){
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection creation failed. No multicast address.");
        }
    }
    else {
        /* TODO check if ipv6 addrr is multicast address. */
    }

    for(rp = requestResult; rp != NULL; rp = rp->ai_next) {
        newChannel->sockfd = UA_socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(newChannel->sockfd != UA_INVALID_SOCKET) {
            /* success */
            break;
        }

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
    channelDataUDPMC->ai_addr = (struct sockaddr_storage *) UA_calloc(1, sizeof(struct sockaddr_storage));
    if(!channelDataUDPMC->ai_addr) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        UA_close(newChannel->sockfd);
        UA_freeaddrinfo(requestResult);
        UA_free(channelDataUDPMC);
        UA_free(newChannel);
        return NULL;
    }

    memcpy(channelDataUDPMC->ai_addr, rp->ai_addr, sizeof(*rp->ai_addr));
    /* link channel and internal channel data */
    newChannel->handle = channelDataUDPMC;
    /* Set loop back data to your host */
#if UA_IPV6
    if(UA_setsockopt(newChannel->sockfd,
                     requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     requestResult->ai_family == PF_INET6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP,
                     (const char *)&channelDataUDPMC->enableLoopback, sizeof (channelDataUDPMC->enableLoopback))
#else
    if(UA_setsockopt(newChannel->sockfd,
                     IPPROTO_IP,
                     IP_MULTICAST_LOOP,
                     (const char *)&channelDataUDPMC->enableLoopback, sizeof (channelDataUDPMC->enableLoopback))
#endif
                      < 0) {
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
                     (const char *)&channelDataUDPMC->messageTTL, sizeof(channelDataUDPMC->messageTTL))
#else
    if(UA_setsockopt(newChannel->sockfd,
                     IPPROTO_IP,
                     IP_MULTICAST_TTL,
                     (const char *)&channelDataUDPMC->messageTTL, sizeof(channelDataUDPMC->messageTTL))
#endif

                      < 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection creation problem. Time to live setup failed.");
    }

    /* Set reuse address -> enables sharing of the same listening address on different sockets. */
    if(channelDataUDPMC->enableReuse) {
        int enableReuse = 1;
        if(UA_setsockopt(newChannel->sockfd,
                      SOL_SOCKET, SO_REUSEADDR,
                      (const char*)&enableReuse, sizeof(enableReuse)) < 0) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection creation problem. Reuse address setup failed.");
        }
    }

    /* Set the physical interface for outgoing traffic */
    if(address.networkInterface.length > 0) {
        UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * address.networkInterface.length + 1);
        memcpy(interfaceAsChar, address.networkInterface.data, address.networkInterface.length);
        interfaceAsChar[address.networkInterface.length] = 0;
        enum{
            IPv4,
    #if UA_IPV6
    IPv6,
    #endif

    INVALID
    } ipVersion;
    union {
    struct ip_mreq ipv4;
    #if UA_IPV6
    struct ipv6_mreq ipv6;
    #endif

    } group;

    if(UA_inet_pton(AF_INET, interfaceAsChar, &group.ipv4.imr_interface)) {
        ipVersion = IPv4;
    #if UA_IPV6
    }
    else if (UA_inet_pton(AF_INET6, interfaceAsChar, &group.ipv6.ipv6mr_multiaddr)) {
        group.ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
        ipVersion = IPv6;
    #endif

    }
    else {
        ipVersion = INVALID;
    }

    if(ipVersion == INVALID ||
    #if UA_IPV6
        UA_setsockopt(newChannel->sockfd,
        requestResult->ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
        requestResult->ai_family == PF_INET6 ? IPV6_MULTICAST_IF : IP_MULTICAST_IF,
        ipVersion == IPv6 ? (const void *) &group.ipv6.ipv6mr_interface : &group.ipv4.imr_interface,
        ipVersion == IPv6 ? sizeof(group.ipv6.ipv6mr_interface) : sizeof(struct in_addr))
     #else
     UA_setsockopt(newChannel->sockfd,
              IPPROTO_IP,
              IP_MULTICAST_IF,
              &group.ipv4.imr_interface,
              sizeof(struct in_addr))
     #endif
     < 0) {
      UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation problem. Interface selection failed.");
    };
}

    /* TTS - changes done for time triggered send usage  */
    txtimeSocket.clockid = clockId;
    /* Flag to use deadline mode or receive error mode */
    txtimeSocket.flags   = 0;
    if(setsockopt(newChannel->sockfd, SOL_SOCKET, SO_TXTIME, &txtimeSocket, sizeof(txtimeSocket))) {
        PRINT_ERROR("setsockopt SO_TXTIME failed");
    }

    if(setsockopt(newChannel->sockfd, SOL_SOCKET, SO_PRIORITY, &soPriority, sizeof(int))) {
        perror("setsockopt SO_PRIORITY failed: %m");
    }

    UA_freeaddrinfo(requestResult);
    newChannel->state = UA_PUBSUB_CHANNEL_PUB;
    return newChannel;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDPMC_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
        void (*notUsedHere)(UA_ByteString *encodedBuffer, UA_ByteString *topic)) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_RDY)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubChannelDataUDPMC * connectionConfig = (UA_PubSubChannelDataUDPMC *) channel->handle;
    /* IPV4 Handling */
    if(connectionConfig->ai_family == PF_INET){
        struct sockaddr_in addr;
        memcpy(&addr, connectionConfig->ai_addr, sizeof(struct sockaddr_in));
        addr.sin_addr.s_addr = INADDR_ANY;
        if (UA_bind(channel->sockfd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        struct ip_mreq groupV4;
        memcpy(&groupV4.imr_multiaddr, &((const struct sockaddr_in *)connectionConfig->ai_addr)->sin_addr, sizeof(struct ip_mreq));
        groupV4.imr_interface.s_addr = inet_addr(PUBSUB_IP_ADDRESS);

        if(UA_setsockopt(channel->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &groupV4, sizeof(groupV4)) != 0) {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection not on multicast");
        }
    #if UA_IPV6
    }
    else if (connectionConfig->ai_family == PF_INET6) {//IPv6 handling
        /* TODO implement regist for IPv6 */
    #endif

    }
    else {
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
    /* IPV4 Handling */
    if(connectionConfig->ai_family == PF_INET) {
        struct ip_mreq groupV4;
        memcpy(&groupV4.imr_multiaddr, &((const struct sockaddr_in *)connectionConfig->ai_addr)->sin_addr, sizeof(struct ip_mreq));
        groupV4.imr_interface.s_addr = UA_htonl(INADDR_ANY);

        if(UA_setsockopt(channel->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &groupV4, sizeof(groupV4)) != 0){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }

#if UA_IPV6
    }
    /* IPv6 handling */
    else if (connectionConfig->ai_family == PF_INET6) {
        /* TODO implement unregist for IPv6 */
#endif

    }
    else {
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
UA_PubSubChannelUDPMC_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettigns, const UA_ByteString *buf) {
    UA_Int32 errorQueueCheck;
    struct   pollfd p_fd = {
    .fd = channel->sockfd,
    };

    /* Calculate the txtime and use txtime to publish the packet at the configured time */
    txtime = (long long unsigned int)nextCycleStartTime.tv_sec * SECONDS + (long long unsigned int)nextCycleStartTime.tv_nsec;
    txtime += qbv_offset;
    if(errorCount == 0) {
        dataCount = udp_send(channel->sockfd, buf->data, (UA_Int32)buf->length, txtime, clockId);
        if(dataCount != (UA_Int32)(buf->length)) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }

    /* Check if errors are pending on the error queue. */
    errorQueueCheck = poll(&p_fd, 1, 0);
    if(errorQueueCheck == 1 && p_fd.revents & POLLERR) {
        if(!sockErrorQueueProcess(channel->sockfd))
            return UA_STATUSCODE_BADINTERNALERROR; // Modified the return value. Need to change this
        }

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
UA_PubSubChannelUDPMC_receive(UA_PubSubChannel *channel, UA_ByteString *message, UA_ExtensionObject *transportSettigns, UA_UInt32 timeout){
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_PUB_SUB)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection receive failed. Invalid state.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubChannelDataUDPMC *channelConfigUDPMC = (UA_PubSubChannelDataUDPMC *) channel->handle;

    if(timeout > 0) {
        fd_set fdset;
        FD_ZERO(&fdset);
        UA_fd_set(channel->sockfd, &fdset);
        struct timeval tmptv = {(long int)(timeout / 1000000),
                                (long int)(timeout % 1000000)};
        int resultsize = UA_select(channel->sockfd+1, &fdset, NULL,
                                NULL, &tmptv);
        if(resultsize == 0) {
            message->length = 0;
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        }
        if (resultsize == -1) {
            message->length = 0;
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    if(channelConfigUDPMC->ai_family == PF_INET){
        ssize_t messageLength;
        messageLength = UA_recvfrom(channel->sockfd, message->data, message->length, 0, NULL, NULL);
        if(messageLength > 0){
            message->length = (size_t) messageLength;
        }
        else {
            message->length = 0;
        }
#if UA_IPV6
    } else {
        /* TODO implement recieve for IPv6 */
#endif
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelUDPMC_close(UA_PubSubChannel *channel) {
    if(UA_close(channel->sockfd) != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection delete failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_deinitialize_architecture_network();
    /* cleanup the internal NetworkLayer data */
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

/* UDPMC channel factory */
UA_PubSubTransportLayer
UA_PubSubTransportLayerUDPMP() {
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerUDPMC_addChannel;
    return pubSubTransportLayer;
}
