/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021-2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"

#define IPV4_PREFIX_MASK 0xF0
#define IPV4_MULTICAST_PREFIX 0xE0
#ifdef UA_IPV6
#   define IPV6_PREFIX_MASK 0xFF
#   define IPV6_MULTICAST_PREFIX 0xFF
#endif

#define UA_MAXBACKLOG 100
#define UA_MAXHOSTNAME_LENGTH 256
#define UA_MAXNETIFNAME_LENGTH 256
#define UA_MAXPORTSTR_LENGTH 6

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

/* A registered file descriptor with an additional method pointer */
typedef struct {
    UA_RegisteredFD fd;
    UA_ConnectionManager_connectionCallback connectionCallback;
} UDP_FD;

typedef struct {
    UA_ConnectionManager cm;

    size_t fdsSize;
    LIST_HEAD(, UA_RegisteredFD) fds;

    UA_ByteString rxBuffer; /* Reuse the receiver buffer. The size is configured
                             * via the recv-bufsize parameter.*/
} UDPConnectionManager;

typedef union {
    struct ip_mreq ipv4;
#if UA_IPV6
    struct ipv6_mreq ipv6;
#endif
} IpMulticastRequest;
/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
static UA_StatusCode
UDP_setNonBlocking(UA_FD sockfd) {
#ifndef _WIN32
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/* Don't have the socket create interrupt signals */
static UA_StatusCode
UDP_setNoSigPipe(UA_FD sockfd) {
#ifdef SO_NOSIGPIPE
    int val = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/**
 * Retrieves hostname and port from given key value parameters.
 * @param[in] paramsSize the size of the parameter list
 * @param[in] params the parameter list to retrieve from
 * @param[out] hostname the retrieved hostname when present, NULL otherwise
 * @param[out] portStr the retrieved port when present, NULL otherwise
 * @param[in] logger the logger to log information
 * @return -1 upon error, 0 if there was no host or port parameter, 1 if
 *         host and port are present
 */
static int
getHostAndPortFromParams(size_t paramsSize, const UA_KeyValuePair *params,
                         char *hostname, char *portStr, const UA_Logger *logger) {
    /* Prepare the port parameter as a string */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| No port found");
        return 0;
    }
    UA_snprintf(portStr, UA_MAXPORTSTR_LENGTH, "%d", *port);

    /* Prepare the hostname string */
    const UA_String *host = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "hostname"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!host) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| No hostname found");
        return 0;
    }
    if(host->length >= UA_MAXHOSTNAME_LENGTH) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Open UDP Connection: Hostname too long, aborting");
        return -1;
    }
    strncpy(hostname, (const char*)host->data, host->length);
    hostname[host->length] = 0;
    return 1;
}

static int
getNetworkInterfaceFromParams(size_t paramsSize, const UA_KeyValuePair *params,
                              UA_String *outNetworkInterface, const UA_Logger *logger) {
    /* Prepare the networkinterface string */
    const UA_String *networkInterface = (const UA_String*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "networkInterface"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!networkInterface) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| No network interface found");
        return 0;
    }
    // if(networkInterface->length >= UA_MAXNETIFNAME_LENGTH) {
    //     UA_LOG_ERROR(logger, UA_LOGCATEGORY_EVENTLOOP,
    //                  "UDP\t| Open UDP Connection: network inteface name too long, aborting");
    //     return -1;
    // }
    UA_String_copy(networkInterface, outNetworkInterface);
    // strncpy(networkInterfaceName, (const char*)networkInterface->data, networkInterface->length);
    // networkInterfaceName[networkInterface->length] = 0;
    return 1;
}

static int
getConnectionInfoFromParams(size_t paramsSize, const UA_KeyValuePair *params, char *hostname, char *portStr,
                            struct addrinfo **info, const UA_Logger *logger) {

    int foundParams = getHostAndPortFromParams(paramsSize, params, hostname, portStr, logger);
    if (foundParams < 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Failed retrieving host and port parameters");
        return -1;
    }
    if(foundParams == 0) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Hostname or Port not present");
        return 0;
    }
    /* Create the socket description from the connectString
    * TODO: Make this non-blocking */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    int error = getaddrinfo(hostname, portStr, &hints, info);
    if(error != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Lookup of %s failed with error %d - %s",
                           hostname, error, errno_str));
        return -1;
    }
    return 1;
}
static UA_INLINE UA_StatusCode
setLoopBackData(UA_SOCKET sockfd,
                UA_Boolean enableLoopback,
                int ai_family,
                const UA_Logger *logger) {
    /* Set loop back data to your host */
    /* The Linux Kernel IPv6 socket code checks for optlen to be at least the
     * size of an integer. However, channelDataUDPMC->enableLoopback is a
     * boolean. In order for the code to work for IPv4 and IPv6 propagate it to
     * a temporary integer here. */
    UA_Int32 enable = enableLoopback;
#if UA_IPV6
    if(UA_setsockopt(sockfd,
                     ai_family == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     ai_family == AF_INET6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP,
                     (const char *)&enable,
                     sizeof (enable)) < 0)
#else
        if(UA_setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
                     (const char *)&enable,
                     sizeof (enable)) < 0)
#endif
    {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "PubSub Connection creation failed. Loopback setup failed: "
                         "Cannot set socket option IP_MULTICAST_LOOP. Error: %s",
                         errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_INLINE void
setTimeToLive(UA_SOCKET sockfd,
              UA_UInt32 messageTTL,
              int ai_family,
              const UA_Logger *logger) {
    /* Set Time to live (TTL). Value of 1 prevent forward beyond the local network. */
#if UA_IPV6
    if(UA_setsockopt(sockfd,
                     ai_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     ai_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL,
                     (const char *)&messageTTL,
                     sizeof(messageTTL)) < 0)
#else
        if(UA_setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                     (const char *)&messageTTL,
                     sizeof(messageTTL)) < 0)
#endif
    {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "PubSub Connection creation problem. Time to live setup failed: "
                           "Cannot set socket option IP_MULTICAST_TTL. Error: %s",
                           errno_str));
    }
}

static UA_INLINE void
setReuseAddress(UA_SOCKET sockfd, UA_Boolean enableReuse, const UA_Logger *logger) {
    /* Set reuse address -> enables sharing of the same listening address on
    * different sockets */
    if(enableReuse){
        int enableReuseVal = 1;
        if(UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                         (const char*)&enableReuseVal, sizeof(enableReuseVal)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                               "PubSub Connection creation problem. Reuse address setup failed: "
                               "Cannot set socket option SO_REUSEADDR. Error: %s",
                               errno_str));
        }
    }
}

#ifdef __linux__
static UA_StatusCode
setSocketPriority(UA_SOCKET sockfd, UA_UInt32 *socketPriority, const UA_Logger *logger) {/* Setting the socket priority to the socket */
    if(socketPriority != NULL) {
        if (UA_setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY,
                          socketPriority, sizeof(*socketPriority)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection creation problem. Priority setup failed: "
                             "Cannot set socket option SO_PRIORITY. Error: %s", errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
bindChannelSocket(UA_SOCKET sockfd, struct sockaddr_storage *ai_addr, int ai_family, const UA_Logger *logger) {
    if(ai_family == AF_INET) { //IPv4 handling
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        memcpy(&addr, ai_addr, sizeof(struct sockaddr_in));
        addr.sin_addr.s_addr = INADDR_ANY;
        if(UA_bind(sockfd, (const struct sockaddr *)&addr,
                   sizeof(struct sockaddr_in)) != 0){
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "PubSub connection creation failed (IPv4). Cannot bind socket: "
                             "Error: %s", errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#if UA_IPV6
    } else if(ai_family == AF_INET6) {//IPv6 handling
        struct sockaddr_in6 addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        memcpy(&addr, ai_addr, sizeof(struct sockaddr_in6));
        addr.sin6_addr = in6addr_any;
        if(UA_bind(sockfd, (const struct sockaddr *)&addr,
                   sizeof(struct sockaddr_in6)) != 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "PubSub connection creation failed (IPv6). Cannot bind socket: "
                             "Error: %s", errno_str));

            return UA_STATUSCODE_BADINTERNALERROR;
        }
#endif
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setMulticastOption(UA_SOCKET sockfd, IpMulticastRequest ipMulticastRequest, int ai_family, const UA_Logger *logger) {
#if UA_IPV6
    if(UA_setsockopt(sockfd,
                     ai_family == AF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP,
                     ai_family == AF_INET6 ? IPV6_MULTICAST_IF : IP_MULTICAST_IF,
                     ai_family == AF_INET6 ? (const void *) &ipMulticastRequest.ipv6.ipv6mr_interface : &ipMulticastRequest.ipv4.imr_interface,
                     ai_family == AF_INET6 ? sizeof(ipMulticastRequest.ipv6.ipv6mr_interface) : sizeof(struct in_addr)) < 0)
#else
        if(UA_setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF,
                     &ipMulticastRequest.ipv4.imr_interface, sizeof(struct in_addr)) < 0)
#endif
    {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "PubSub Connection creation problem. Interface selection failed: "
                         "Cannot set socket option IP_MULTICAST_IF. Error: %s",
                         errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}
#ifdef UA_IPV6
static UA_INLINE UA_StatusCode
setMulticastInfoIPV6(const char *addressAsChar, IpMulticastRequest *ipMulticastRequest) {
    int convertTextAddressToBinarySuccessful = UA_inet_pton(AF_INET6, addressAsChar,
                                                            &ipMulticastRequest->ipv6.ipv6mr_multiaddr);
    if(convertTextAddressToBinarySuccessful != 1) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    ipMulticastRequest->ipv6.ipv6mr_interface = 0; // default configuration
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
setMulticastInfoIPv4(const char *addressAsChar, IpMulticastRequest *ipMulticastRequest) {
    int convertTextAddressToBinarySuccessful = UA_inet_pton(AF_INET, addressAsChar,
                                                            &ipMulticastRequest->ipv4.imr_multiaddr);
    if(convertTextAddressToBinarySuccessful != 1) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    ipMulticastRequest->ipv4.imr_interface.s_addr = UA_htonl(INADDR_ANY);
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
isMulticastAddress(const UA_Byte *address, int domain, UA_Byte mask, UA_Byte prefix, UA_Boolean *isMulticast) {
    *isMulticast = (address[0] & mask) == prefix;
    return UA_STATUSCODE_GOOD;
}


static UA_Boolean
isIPv4MulticastAddress(const UA_Byte *address) {
    UA_Boolean isMulticast;
    isMulticastAddress(address, AF_INET, IPV4_PREFIX_MASK,  IPV4_MULTICAST_PREFIX, &isMulticast);

    return isMulticast;
}

static UA_Boolean
isIPv6MulticastAddress(const UA_Byte *address) {
    UA_Boolean isMulticast;
    isMulticastAddress(address, AF_INET6, IPV6_PREFIX_MASK, IPV6_MULTICAST_PREFIX, &isMulticast);

    return isMulticast;
}

static UA_StatusCode
setupNetworkInterface(IpMulticastRequest *ipMulticastRequest,
                      struct sockaddr_storage *intf_addr,
                      int ai_family, UA_String *networkInterface, const UA_Logger *logger) {/* Set configured interface */
    UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * (*networkInterface).length + 1);
    memcpy(interfaceAsChar, (*networkInterface).data, (*networkInterface).length);
    interfaceAsChar[(*networkInterface).length] = 0;

    if(ai_family == AF_INET) {
        if(UA_inet_pton(AF_INET, interfaceAsChar, &ipMulticastRequest->ipv4.imr_interface) <= 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Connection creation problem. "
                         "Interface configuration preparation failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        memcpy(intf_addr, &ipMulticastRequest->ipv4.imr_interface,
               sizeof(ipMulticastRequest->ipv4.imr_interface));
#if UA_IPV6
    } else if(ai_family == AF_INET6) {
        ipMulticastRequest->ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
        if(ipMulticastRequest->ipv6.ipv6mr_interface == 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Connection creation problem. "
                         "Interface configuration preparation failed.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
#endif
    } else {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "PubSub Connection creation failed. Multicast setup failed: "
                     "unknown address family");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
configureMulticast(IpMulticastRequest *ipMulticastRequest,
                   int ai_family,
                   UA_String networkInterface,
                   const char *addressAsChar,
                   const UA_Logger *logger) {

    struct sockaddr_storage intf_addr;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* Prepare for following socket options:
     * - Set the physical interface for in- and outgoing traffic (if configured)
     * - Join multicast group */
    memset(ipMulticastRequest, 0, sizeof(IpMulticastRequest));
    /* Check if the ip address is a multicast address */
    if(ai_family == AF_INET) {
        res = setMulticastInfoIPv4(addressAsChar, ipMulticastRequest);
#ifdef UA_IPV6
    } else if(ai_family == AF_INET6) {
        res = setMulticastInfoIPV6(addressAsChar, ipMulticastRequest);
#endif
    } else {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "PubSub Connection creation failed. Multicast setup failed: "
                     "unknown address family");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                     "PubSub Connection creation failed. Multicast setup failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(networkInterface.length > 0) {
        res = setupNetworkInterface(ipMulticastRequest, &intf_addr, ai_family, &networkInterface, logger);
        if(res != UA_STATUSCODE_GOOD) {
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
setConnectionConfigurationProperties(UA_FD socket,
                                     const UA_KeyValuePair *connectionProperties,
                                     const size_t connectionPropertiesSize,
                                     int ai_family,
                                     const UA_Logger *logger) {
    // channelDataUDPMC->messageTTL = 255;
    // channelDataUDPMC->enableLoopback = true;
    // channelDataUDPMC->enableReuse = true;
    // channelDataUDPMC->isMulticast = true;

    /* Iterate over the given KeyValuePair parameters */
    UA_String ttlParam = UA_STRING("ttl");
    UA_String loopbackParam = UA_STRING("loopback");
    UA_String reuseParam = UA_STRING("reuse");

    UA_String hostnameParam = UA_STRING("hostname");
    UA_String portParam = UA_STRING("port");
    UA_String listenHostnamesParam = UA_STRING("listen-hostnames");
    UA_String listenPortParam= UA_STRING("listen-port");

#ifdef __linux__
    UA_String socketPriorityParam = UA_STRING("sockpriority");
#endif
    for(size_t i = 0; i < connectionPropertiesSize; i++) {
        const UA_KeyValuePair *prop = &connectionProperties[i];
        if(UA_String_equal(&prop->key.name, &ttlParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_UINT32])){
                UA_UInt32 messageTTL = *(UA_UInt32*)prop->value.data;
                setTimeToLive(socket, messageTTL, ai_family, logger);
            }
        } else if(UA_String_equal(&prop->key.name, &loopbackParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                UA_Boolean enableLoopback = *(UA_Boolean*)prop->value.data;
                UA_StatusCode res = setLoopBackData(socket, enableLoopback, ai_family, logger);
                if (res != UA_STATUSCODE_GOOD) return res;
            }
        } else if(UA_String_equal(&prop->key.name, &reuseParam)) {
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                UA_Boolean enableReuse = *(UA_Boolean *)prop->value.data;
                setReuseAddress(socket, enableReuse, logger);
            }
#ifdef __linux__
        } else if(UA_String_equal(&prop->key.name, &socketPriorityParam)){
            if(UA_Variant_hasScalarType(&prop->value, &UA_TYPES[UA_TYPES_UINT32])){
                UA_UInt32 *socketPriority = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32));
                if(!socketPriority) {
                    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SERVER,
                                   "PubSub Connection creation. Could not set socket priority due to out of memory.");
                    continue;
                }
                UA_UInt32_copy((UA_UInt32 *) prop->value.data, socketPriority);
                setSocketPriority(socket, socketPriority, logger);
                UA_free(socketPriority);
            }
#endif
        } else if (UA_String_equal(&prop->key.name, &hostnameParam) ||
                   UA_String_equal(&prop->key.name, &portParam) ||
                   UA_String_equal(&prop->key.name, &listenHostnamesParam) ||
                   UA_String_equal(&prop->key.name, &listenPortParam)) {
            /* ignore there are handled elsewhere */
        } else {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Connection creation. Unknown connection parameter.");
        }
    }
    /* Set the socket options
    * also there might be external requirements for socket options
    * how to handle and set those
    * either via: passed in parameters
    * or via: specific callback that gives access to the allocated socket
    * */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UDP_setNonBlocking(socket);
    res |= UDP_setNoSigPipe(socket);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not set socket options: %s", errno_str));
        UA_close(socket);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
configureMulticastInterfaceIPv4(UA_FD socket,
                                socklen_t addrlen, struct sockaddr_in *addr,
                                size_t paramsSize, const UA_KeyValuePair *params, const UA_Logger *logger){
    struct in_addr address = addr->sin_addr;
    UA_Byte *addressVal = (UA_Byte *) &addr->sin_addr;

    if(isIPv4MulticastAddress(addressVal)) {

        IpMulticastRequest ipMulticastRequest;
        ipMulticastRequest.ipv4.imr_multiaddr = address;
        /* default outgoing interface ANY */
        ipMulticastRequest.ipv4.imr_interface.s_addr = UA_htonl(INADDR_ANY);

        UA_String netif = UA_STRING_NULL;
        int foundInterface = getNetworkInterfaceFromParams(paramsSize, params, &netif, logger);
        if(foundInterface < 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP\t| Opening a connection failed");
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        if(netif.length > 0) {
            UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * (netif).length + 1);
            memcpy(interfaceAsChar, netif.data, netif.length);
            interfaceAsChar[netif.length] = 0;

            if(UA_inet_pton(AF_INET, interfaceAsChar, &ipMulticastRequest.ipv4.imr_interface) <= 0) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation problem. "
                             "Interface configuration preparation failed.");
                return UA_STATUSCODE_BADINTERNALERROR;
            }
        }
        setsockopt(socket, IPPROTO_IP, IP_MULTICAST_IF,
                   &ipMulticastRequest.ipv4.imr_interface, sizeof(struct in_addr));
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addMulticastInterfaceIPv4(UA_FD socket,
                          socklen_t addrlen, struct sockaddr_in *addr,
                          const UA_Logger *logger){
    struct in_addr address = addr->sin_addr;
    UA_Byte *addressVal = (UA_Byte *) &addr->sin_addr;

    if(isIPv4MulticastAddress(addressVal)) {

        IpMulticastRequest ipMulticastRequest;
        ipMulticastRequest.ipv4.imr_multiaddr = address;
        /* default outgoing interface ANY */
        ipMulticastRequest.ipv4.imr_interface.s_addr = UA_htonl(INADDR_ANY);

        // UA_String netif = UA_STRING_NULL;
        // int foundInterface = getNetworkInterfaceFromParams(paramsSize, params, &netif, logger);
        // if(foundInterface < 0) {
        //     UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
        //                  "UDP\t| Opening a connection failed");
        //     return UA_STATUSCODE_BADINTERNALERROR;
        // }

        // if(netif.length > 0) {
        //     UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * (netif).length + 1);
        //     memcpy(interfaceAsChar, netif.data, netif.length);
        //     interfaceAsChar[netif.length] = 0;

        //     if(UA_inet_pton(AF_INET, interfaceAsChar, &ipMulticastRequest.ipv4.imr_interface) <= 0) {
        //         UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
        //                      "PubSub Connection creation problem. "
        //                      "Interface configuration preparation failed.");
        //         return UA_STATUSCODE_BADINTERNALERROR;
        //     }
        // }
        if(UA_setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                         &ipMulticastRequest.ipv4, sizeof(ipMulticastRequest.ipv4)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection regist failed. IP membership setup failed: "
                             "Cannot set socket option IP_ADD_MEMBERSHIP. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_IPV6
static UA_StatusCode
addMulticastInterfaceIPv6(UA_FD socket, socklen_t addrlen, struct sockaddr_in6 *addr,
                          const UA_Logger *logger) {

    struct in6_addr address = addr->sin6_addr;
    UA_Byte *addressVal = (UA_Byte *) &addr->sin6_addr;

    if(isIPv6MulticastAddress(addressVal)) {

        IpMulticastRequest ipMulticastRequest;
        ipMulticastRequest.ipv6.ipv6mr_multiaddr = address;
        /* default outgoing interface ANY */
        ipMulticastRequest.ipv6.ipv6mr_interface = 0;

        // UA_String netif = UA_STRING_NULL;
        // int foundInterface = getNetworkInterfaceFromParams(paramsSize, params, &netif, logger);
        // if(foundInterface < 0) {
        //     UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
        //                  "UDP\t| Opening a connection failed");
        //     return UA_STATUSCODE_BADINTERNALERROR;
        // }

        // if(netif.length > 0) {
        //     UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * (netif).length + 1);
        //     memcpy(interfaceAsChar, netif.data, netif.length);
        //     interfaceAsChar[netif.length] = 0;

        //     ipMulticastRequest.ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
        //     if(ipMulticastRequest.ipv6.ipv6mr_interface == 0) {
        //         UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
        //                      "PubSub Connection creation problem. "
        //                      "Interface configuration preparation failed.");
        //         return UA_STATUSCODE_BADINTERNALERROR;
        //     }
        // }
        if(UA_setsockopt(socket, IPPROTO_IPV6,IPV6_ADD_MEMBERSHIP,
                         &ipMulticastRequest.ipv6,sizeof(ipMulticastRequest.ipv6)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "PubSub Connection regist failed. IP membership setup failed: "
                             "Cannot set socket option IP_ADD_MEMBERSHIP. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
configureMulticastInterfaceIPv6(UA_FD socket, socklen_t addrlen, struct sockaddr_in6 *addr,
                                size_t paramsSize, const UA_KeyValuePair *params, const UA_Logger *logger) {
    struct in6_addr address = addr->sin6_addr;
    UA_Byte *addressVal = (UA_Byte *) &addr->sin6_addr;

    if(isIPv6MulticastAddress(addressVal)) {

        IpMulticastRequest ipMulticastRequest;
        ipMulticastRequest.ipv6.ipv6mr_multiaddr = address;
        /* default outgoing interface ANY */
        ipMulticastRequest.ipv6.ipv6mr_interface = 0;

        UA_String netif = UA_STRING_NULL;
        int foundInterface = getNetworkInterfaceFromParams(paramsSize, params, &netif, logger);
        if(foundInterface < 0) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                         "UDP\t| Opening a connection failed");
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        if(netif.length > 0) {
            UA_STACKARRAY(char, interfaceAsChar, sizeof(char) * (netif).length + 1);
            memcpy(interfaceAsChar, netif.data, netif.length);
            interfaceAsChar[netif.length] = 0;

            ipMulticastRequest.ipv6.ipv6mr_interface = UA_if_nametoindex(interfaceAsChar);
            if(ipMulticastRequest.ipv6.ipv6mr_interface == 0) {
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation problem. "
                             "Interface configuration preparation failed.");
                return UA_STATUSCODE_BADINTERNALERROR;
            }
        }

        if(setsockopt(socket, IPPROTO_IPV6, IP_MULTICAST_IF,
                      &ipMulticastRequest.ipv6.ipv6mr_interface, sizeof(ipMulticastRequest.ipv6.ipv6mr_interface)) < 0) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(logger, UA_LOGCATEGORY_NETWORK,
                             "Opening UDP connection failed: "
                             "Cannot set socket option IP_MULTICAST_IF. Error: %s",
                             errno_str));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
UDPConnectionManager_register(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, rfd);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    LIST_INSERT_HEAD(&ucm->fds, rfd, es_pointers);
    ucm->fdsSize++;
    return UA_STATUSCODE_GOOD;
}

static void
UDPConnectionManager_deregister(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;
    UA_EventLoopPOSIX_deregisterFD(el, rfd);
    LIST_REMOVE(rfd, es_pointers);
    UA_assert(ucm->fdsSize > 0);
    ucm->fdsSize--;
}

static UA_StatusCode
UDP_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                       UA_ByteString *buf, size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
UDP_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                      UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}


/* Test if the ConnectionManager can be stopped */
static void
UDP_checkStopped(UDPConnectionManager *ucm) {
    if(ucm->fdsSize == 0 && ucm->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING) {
        UA_LOG_DEBUG(ucm->cm.eventSource.eventLoop->logger,
                     UA_LOGCATEGORY_NETWORK,
                     "UDP\t| All sockets closed, the EventLoop has stopped");

        UA_ByteString_clear(&ucm->rxBuffer);
        ucm->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
    }
}

/* This method must not be called from the application directly, but from within
 * the EventLoop. Otherwise we cannot be sure whether the file descriptor is
 * still used after calling close. */
static void
UDP_close(UDPConnectionManager *ucm, UA_RegisteredFD *rfd) {
    UDP_FD *udpfd = (UDP_FD*)rfd;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)ucm->cm.eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Closing connection", (unsigned)rfd->fd);

    /* Deregister from the EventLoop */
    UDPConnectionManager_deregister(ucm, rfd);

    /* Signal closing to the application */
    udpfd->connectionCallback(&ucm->cm, (uintptr_t)rfd->fd,
                              rfd->application, &rfd->context,
                              UA_STATUSCODE_BADCONNECTIONCLOSED,
                              0, NULL, UA_BYTESTRING_NULL);

    /* Close the socket */
    int ret = UA_close(rfd->fd);
    if(ret == 0) {
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP %u\t| Socket closed", (unsigned)rfd->fd);
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Could not close the socket (%s)",
                          (unsigned)rfd->fd, errno_str));
    }

    /* Don't call free here. This might be done automatically via the delayed
     * callback that calls UDP_close. */
    /* UA_free(rfd); */

    /* Stop if the ucm is stopping and this was the last open socket */
    UDP_checkStopped(ucm);
}

static void
UDP_delayedClose(void *application, void *context) {
    UA_ConnectionManager *cm = (UA_ConnectionManager*)application;
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_RegisteredFD* rfd = (UA_RegisteredFD *)context;
    UA_LOG_DEBUG(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "UDP %u\t| Delayed closing of the connection", (unsigned)rfd->fd);
    UDP_close(ucm, rfd);
    /* Don't call free here. This is done automatically via the delayed callback
     * mechanism. */
}

/* Gets called when a socket receives data or closes */
static void
UDP_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                             short event) {
    UDP_FD *udpfd = (UDP_FD*)rfd;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Activity on the socket", (unsigned)rfd->fd);

    /* Write-Event, a new connection for sending has opened.  */
    if(event == UA_FDEVENT_OUT) {
        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP %u\t| Opening a new connection", (unsigned)rfd->fd);

        /* A new socket has opened. Signal it to the application. */
        udpfd->connectionCallback(cm, (uintptr_t)rfd->fd,
                                  rfd->application, &rfd->context,
                                  UA_STATUSCODE_GOOD, 0, NULL,
                                  UA_BYTESTRING_NULL);

        /* Now we are interested in read-events. */
        rfd->listenEvents = UA_FDEVENT_IN;
        UA_EventLoopPOSIX_modifyFD(el, rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Allocate receive buffer", (unsigned)rfd->fd);

    /* Use the already allocated receive-buffer */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_ByteString response = ucm->rxBuffer;

    /* Receive */
#ifndef _WIN32
    ssize_t ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#else
    int ret = UA_recv(rfd->fd, (char*)response.data, response.length, MSG_DONTWAIT);
#endif

    /* Receive has failed */
    if(ret <= 0) {
        if(UA_ERRNO == UA_INTERRUPTED)
            return;

        /* Orderly shutdown of the socket. We can immediately close as no method
         * "below" in the call stack will use the socket in this iteration of
         * the EventLoop. */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                        "UDP %u\t| recv signaled the socket was shutdown (%s)",
                        (unsigned)rfd->fd, errno_str));
        UDP_close(ucm, rfd);
        UA_free(rfd);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Received message of size %u",
                 (unsigned)rfd->fd, (unsigned)ret);

    /* Callback to the application layer */
    response.length = (size_t)ret; /* Set the length of the received buffer */
    udpfd->connectionCallback(cm, (uintptr_t)rfd->fd,
                              rfd->application, &rfd->context,
                              UA_STATUSCODE_GOOD, 0, NULL, response);
}

static void
UDP_registerListenSocket(UA_ConnectionManager *cm, UA_UInt16 port,
                         struct addrinfo *ai, void *application, void *context,
                         UA_ConnectionManager_connectionCallback connectionCallback) {
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Get logging information */
    char hoststr[UA_MAXHOSTNAME_LENGTH];
    int get_res = UA_getnameinfo(ai->ai_addr, ai->ai_addrlen,
                                 hoststr, sizeof(hoststr),
                                 NULL, 0, NI_NUMERICHOST);
    if(get_res != 0) {
        hoststr[0] = 0;
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP\t| getnameinfo(...) could not resolve the hostname (%s)",
                          errno_str));
    }
    
    /* Create the server socket */
    UA_FD listenSocket = UA_socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if(listenSocket == UA_INVALID_FD) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error opening the listen socket for "
                          "\"%s\" on port %u (%s)",
                          (unsigned)listenSocket, hoststr, port, errno_str));
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New server socket for \"%s\" on port %u",
                (unsigned)listenSocket, hoststr, port);

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(ai->ai_family == AF_INET) {
        res = addMulticastInterfaceIPv4(listenSocket, ai->ai_addrlen, (struct sockaddr_in *)ai->ai_addr,
                                              el->eventLoop.logger);
#ifdef UA_IPV6
    } else if(ai->ai_family == AF_INET6) {
        res = addMulticastInterfaceIPv6(listenSocket, ai->ai_addrlen, (struct sockaddr_in6 *)ai->ai_addr, el->eventLoop.logger);
#endif
    } else {
        UA_close(listenSocket);
        freeaddrinfo(ai);
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Opening a connection failed");
        return;
    }

    /* Some Linux distributions have net.ipv6.bindv6only not activated. So
     * sockets can double-bind to IPv4 and IPv6. This leads to problems. Use
     * AF_INET6 sockets only for IPv6. */
    int optval = 1;
#if UA_IPV6
    if(ai->ai_family == AF_INET6 &&
       UA_setsockopt(listenSocket, IPPROTO_IPV6, IPV6_V6ONLY,
                     (const char*)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not set an IPv6 socket to IPv6 only, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }
#endif

    /* Allow rebinding to the IP/port combination. Eg. to restart the server. */
    if(UA_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                     (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not make the socket reusable, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Set the socket non-blocking */
    if(UDP_setNonBlocking(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not set the socket non-blocking, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Supress interrupts from the socket */
    if(UDP_setNoSigPipe(listenSocket) != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Could not disable SIGPIPE, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    /* Bind socket to address */
    int ret = UA_bind(listenSocket, ai->ai_addr, (socklen_t)ai->ai_addrlen);
    if(ret < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "UDP %u\t| Error binding the socket to the address (%s), closing",
                          (unsigned)listenSocket, errno_str));
        UA_close(listenSocket);
        return;
    }

    /* Allocate the UA_RegisteredFD */
    UDP_FD *newudpfd = (UDP_FD*)UA_calloc(1, sizeof(UDP_FD));
    if(!newudpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error allocating memory for the socket, closing",
                       (unsigned)listenSocket);
        UA_close(listenSocket);
        return;
    }

    newudpfd->fd.fd = listenSocket;
    newudpfd->fd.es = &cm->eventSource;
    newudpfd->fd.callback = (UA_FDCallback)UDP_connectionSocketCallback;
    newudpfd->fd.application = application;
    newudpfd->fd.context = context;
    newudpfd->fd.listenEvents = UA_FDEVENT_IN;
    newudpfd->connectionCallback = connectionCallback;

    /* Register in the EventLoop */
    res = UDPConnectionManager_register(ucm, &newudpfd->fd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error registering the socket, closing",
                       (unsigned)listenSocket);
        UA_free(newudpfd);
        UA_close(listenSocket);
        return;
    }

    connectionCallback(cm, (uintptr_t)newudpfd->fd.fd,
                       application, &newudpfd->fd.context,
                       UA_STATUSCODE_GOOD, 0, NULL, UA_BYTESTRING_NULL);
}

static UA_StatusCode
UDP_registerListenSockets(UA_ConnectionManager *cm, const char *hostname,
                          UA_UInt16 port, void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback) {
    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
#if UA_IPV6
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
#else
    hints.ai_family = AF_INET;   /* IPv4 only */
#endif
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE; // | AI_NUMERICHOST;
#ifdef AI_ADDRCONFIG
    hints.ai_flags |= AI_ADDRCONFIG; /* Only return IPv4/IPv6 if at least one
                                      * such address is configured */
#endif

    /* Set up the port string */
    char portstr[6];
    UA_snprintf(portstr, 6, "%d", port);

    int retcode = UA_getaddrinfo(hostname, portstr, &hints, &res);
    if(retcode != 0) {
        UA_LOG_SOCKET_ERRNO_GAI_WRAP(
           UA_LOG_WARNING(cm->eventSource.eventLoop->logger,
                          UA_LOGCATEGORY_NETWORK,
                          "UDP\t| getaddrinfo lookup for \"%s\" on port %u failed (%s)",
                          hostname, port, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add listen sockets */
    struct addrinfo *ai = res;
    while(ai) {
        UDP_registerListenSocket(cm, port, ai, application, context, connectionCallback);
        ai = ai->ai_next;
    }
    UA_freeaddrinfo(res);
    return UA_STATUSCODE_GOOD;
}

static
UA_RegisteredFD *
UDP_findRegisteredFD(UDPConnectionManager *ucm, uintptr_t connectionId) {
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &ucm->fds, es_pointers) {
        if(rfd->fd == (UA_FD)connectionId)
            return rfd;
    }
    return NULL;
}

/* Close the connection via a delayed callback */
static void
UDP_shutdown(UA_ConnectionManager *cm, UA_RegisteredFD *rfd) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    if(rfd->dc.callback) {
        UA_LOG_INFO(el->logger, UA_LOGCATEGORY_NETWORK,
                    "UDP %u\t| Cannot close - already closing",
                    (unsigned)rfd->fd);
        return;
    }

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_NETWORK,
                 "UDP %u\t| Shutdown called", (unsigned)rfd->fd);

    UA_DelayedCallback *dc = &rfd->dc;
    dc->callback = UDP_delayedClose;
    dc->application = cm;
    dc->context = rfd;
    el->addDelayedCallback(el, dc);
}

static UA_StatusCode
UDP_shutdownConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_EventLoop *el = cm->eventSource.eventLoop;
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_RegisteredFD *rfd = UDP_findRegisteredFD(ucm, connectionId);
    if(!rfd) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Cannot close UDP connection %u - not found",
                       (unsigned)connectionId);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UDP_shutdown(cm, rfd);

    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
UDP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                       size_t paramsSize, const UA_KeyValuePair *params,
                       UA_ByteString *buf) {
    /* Prevent OS signals when sending to a closed socket */
    int flags = MSG_NOSIGNAL;

    struct pollfd tmp_poll_fd;
    tmp_poll_fd.fd = (UA_FD)connectionId;
    tmp_poll_fd.events = UA_POLLOUT;

    /* Send the full buffer. This may require several calls to send */
    size_t nWritten = 0;

    do {
        ssize_t n = 0;
        do {
            UA_LOG_DEBUG(cm->eventSource.eventLoop->logger,
                         UA_LOGCATEGORY_NETWORK,
                         "UDP %u\t| Attempting to send", (unsigned)connectionId);
            size_t bytes_to_send = buf->length - nWritten;
            n = UA_send((UA_FD)connectionId,
                        (const char*)buf->data + nWritten,
                        bytes_to_send, flags);
            if(n < 0) {
                /* An error we cannot recover from? */
                if(UA_ERRNO != UA_INTERRUPTED &&
                   UA_ERRNO != UA_WOULDBLOCK &&
                   UA_ERRNO != UA_AGAIN) {
                    UA_LOG_SOCKET_ERRNO_WRAP(
                       UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                                    UA_LOGCATEGORY_NETWORK,
                                    "UDP %u\t| Send failed with error %s",
                                    (unsigned)connectionId, errno_str));
                    UDP_shutdownConnection(cm, connectionId);
                    UA_ByteString_clear(buf);
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }

                /* Poll for the socket resources to become available and retry
                 * (blocking) */
                int poll_ret;
                do {
                    poll_ret = UA_poll(&tmp_poll_fd, 1, 100);
                    if(poll_ret < 0 && UA_ERRNO != UA_INTERRUPTED) {
                        UA_LOG_SOCKET_ERRNO_WRAP(
                           UA_LOG_ERROR(cm->eventSource.eventLoop->logger,
                                        UA_LOGCATEGORY_NETWORK,
                                        "UDP %u\t| Send failed with error %s",
                                        (unsigned)connectionId, errno_str));
                        UDP_shutdownConnection(cm, connectionId);
                        UA_ByteString_clear(buf);
                        return UA_STATUSCODE_BADCONNECTIONCLOSED;
                    }
                } while(poll_ret <= 0);
            }
        } while(n < 0);
        nWritten += (size_t)n;
    } while(nWritten < buf->length);

    /* Free the buffer */
    // UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
UDP_openSendConnection(UA_ConnectionManager *cm,
                       size_t paramsSize, const UA_KeyValuePair *params,
                       void *application, void *context,
                       UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)cm->eventSource.eventLoop;

    /* Get the connection parameters */
    char hostname[UA_MAXHOSTNAME_LENGTH];
    char portStr[UA_MAXPORTSTR_LENGTH];
    struct addrinfo *info = NULL;

    int error = getConnectionInfoFromParams(paramsSize, params, hostname, portStr, &info, el->eventLoop.logger);
    if(error < 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* Create a socket */
    UA_FD newSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(newSock == UA_INVALID_FD) {
        freeaddrinfo(info);
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Could not create socket to connect to %s (%s)",
                           hostname, errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }
    UA_StatusCode res = setConnectionConfigurationProperties(newSock, params, paramsSize, info->ai_family, el->eventLoop.logger);
    if(res != UA_STATUSCODE_GOOD) {
        return res;
    }

    if(info->ai_family == AF_INET) {
        res = configureMulticastInterfaceIPv4(newSock, info->ai_addrlen, (struct sockaddr_in *)info->ai_addr,
                                              paramsSize,
                                              params, el->eventLoop.logger);
#ifdef UA_IPV6
    } else if(info->ai_family == AF_INET6) {
        res = configureMulticastInterfaceIPv6(newSock, info->ai_addrlen, (struct sockaddr_in6 *)info->ai_addr, paramsSize,
                                              params, el->eventLoop.logger);
#endif
    } else {
        freeaddrinfo(info);
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Opening a connection failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* Non-blocking connect */
    error = UA_connect(newSock, info->ai_addr, info->ai_addrlen);
    freeaddrinfo(info);
    if(error != 0 &&
       UA_ERRNO != UA_INPROGRESS &&
       UA_ERRNO != UA_WOULDBLOCK) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                           "UDP\t| Connecting the socket failed (%s)",
                           errno_str));
        return UA_STATUSCODE_BADDISCONNECT;
    }

    /* Allocate the UA_RegisteredFD */
    UDP_FD *newudpfd = (UDP_FD*)UA_calloc(1, sizeof(UDP_FD));
    if(!newudpfd) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP %u\t| Error allocating memory for the socket, closing",
                       (unsigned)newSock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newudpfd->fd.fd = newSock;
    newudpfd->fd.es = &cm->eventSource;
    newudpfd->fd.callback = (UA_FDCallback)UDP_connectionSocketCallback;
    newudpfd->fd.application = application;
    newudpfd->fd.context = context;
    newudpfd->fd.listenEvents = UA_FDEVENT_OUT; /* Switched to _IN once the
                                                 * connection is open */
    newudpfd->connectionCallback = connectionCallback;

    /* Register the fd to trigger when output is possible (the connection is open) */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    res = UDPConnectionManager_register(ucm, &newudpfd->fd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "UDP\t| Registering the socket failed");
        UA_close(newSock);
        UA_free(newudpfd);
        return res;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                "UDP %u\t| New connection", (unsigned)newSock);
    // "to \"%s\" on port %s",
    // (unsigned)newSock, hostname, portStr);

    /* Don't signal the application that the connection has opened. This will
     * happen in the next iteration of the EventLoop. */
    /* connectionCallback(cm, (uintptr_t)newSock, application, &newudpfd->fd.context,
                          UA_STATUSCODE_GOOD, 0, NULL, UA_BYTESTRING_NULL); */

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UDP_openReceiveConnection(UA_ConnectionManager *cm,
                          size_t paramsSize, const UA_KeyValuePair *params,
                          void *application, void *context,
                          UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Get the socket */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, paramsSize,
                                 UA_QUALIFIEDNAME(0, "listen-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    if(!port) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "UDP\t| Port information required for UDP listening");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the hostnames configuration */
    const UA_Variant *hostNames =
        UA_KeyValueMap_get(params, paramsSize,
                           UA_QUALIFIEDNAME(0, "listen-hostnames"));
    if(!hostNames) {
        /* No hostnames configured -> listen on all interfaces*/
        UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                    "UDP\t| Listening on all interfaces");
        return UDP_registerListenSockets(cm, NULL, *port, application,
                                         context, connectionCallback);
    }

    /* Correct datatype for the hostnames? */
    if(hostNames->type != &UA_TYPES[UA_TYPES_STRING] ||
       UA_Variant_isScalar(hostNames)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| The hostnames have to be an array of string");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    size_t interfaces = hostNames->arrayLength;
    if(interfaces == 0) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| Listening on all interfaces");
        return UDP_registerListenSockets(cm, NULL, *port, application,
                                         context, connectionCallback);
    }
    
    /* Iterate over the configured hostnames */
    UA_String *hostStrings = (UA_String*)hostNames->data;
    for(size_t i = 0; i < hostNames->arrayLength; i++) {
        char hostname[512];
        if(hostStrings[i].length >= sizeof(hostname))
            continue;
        memcpy(hostname, hostStrings[i].data, hostStrings->length);
        hostname[hostStrings->length] = '\0';
        UDP_registerListenSockets(cm, hostname, *port, application,
                                  context, connectionCallback);
    }

    return UA_STATUSCODE_GOOD;
}

/**
 *
 * @param cm
 * @param paramSize
 * @param params the parameters to be passed for opening the connection
 *               hostname
 *               port
 *               sockpriority: uint32
 *               mc-ttl: uint32 - set the multicast time to live
 *               mc-loopback: bool - whether or not multicast datagrams are looped back
 *                                   (enable local interfaces belonging to the multicast group to receive packages)
 *               reuse: bool -
 *
 * @param application
 * @param context
 * @param connectionCallback
 * @return
 */
static UA_StatusCode
UDP_openConnection(UA_ConnectionManager *cm,
                   size_t paramsSize, const UA_KeyValuePair *params,
                   void *application, void *context,
                   UA_ConnectionManager_connectionCallback connectionCallback) {
    /* If the "port"-parameter is defined, then try to open a send connection.
     * Otherwise try to open a socket that listens for incoming TCP
     * connections. */
    const UA_Variant *val = UA_KeyValueMap_get(params, paramsSize,
                                               UA_QUALIFIEDNAME(0, "port"));
    if(val) {
        return UDP_openSendConnection(cm, paramsSize, params,
                                      application, context, connectionCallback);
    } else {
        return UDP_openReceiveConnection(cm, paramsSize, params,
                                         application, context, connectionCallback);
    }
}

/* Asynchronously register the listenSocket */
static UA_StatusCode
UDP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)cm->eventSource.eventLoop;

    /* Check the state */
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "To start the UDP ConnectionManager, "
                     "it has to be registered in an EventLoop and not started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* The receive buffersize was configured? */
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_UInt32 rxBufSize = 2u << 16; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(cm->eventSource.params,
                                 cm->eventSource.paramsSize,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    UA_StatusCode res = UA_ByteString_allocBuffer(&ucm->rxBuffer, rxBufSize);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Set the EventSource to the started state */
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;

    return UA_STATUSCODE_GOOD;
}

static void
UDP_eventSourceStop(UA_ConnectionManager *cm) {
    UDPConnectionManager *ucm = (UDPConnectionManager*)cm;
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                "UDP\t| Shutting down the ConnectionManager");

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;

    /* Shut down all registered fd. The cm is set to "stopped" when the last fd
     * is closed (from within UDP_close). */
    UA_RegisteredFD *rfd;
    LIST_FOREACH(rfd, &ucm->fds, es_pointers) {
        UDP_shutdown(cm, rfd);
    }

    /* Check if stopped once more (also checking inside UDP_close, but there we
     * don't check if there is no rfd at all) */
    UDP_checkStopped(ucm);
}

static UA_StatusCode
UDP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "UDP\t| The EventSource must be stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Delete the parameters */
    UA_Array_delete(cm->eventSource.params,
                    cm->eventSource.paramsSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    cm->eventSource.params = NULL;
    cm->eventSource.paramsSize = 0;

    UA_String_clear(&cm->eventSource.name);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

static const char *udpName = "udp";

UA_ConnectionManager *
UA_ConnectionManager_new_POSIX_UDP(const UA_String eventSourceName) {
    UDPConnectionManager *cm = (UDPConnectionManager*)
        UA_calloc(1, sizeof(UDPConnectionManager));
    if(!cm)
        return NULL;

    cm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_String_copy(&eventSourceName, &cm->cm.eventSource.name);
    cm->cm.eventSource.start = (UA_StatusCode (*)(UA_EventSource *)) UDP_eventSourceStart;
    cm->cm.eventSource.stop = (void (*)(UA_EventSource *))UDP_eventSourceStop;
    cm->cm.eventSource.free = (UA_StatusCode (*)(UA_EventSource *))UDP_eventSourceDelete;
    cm->cm.protocol = UA_STRING((char*)(uintptr_t)udpName);
    cm->cm.openConnection = UDP_openConnection;
    cm->cm.allocNetworkBuffer = UDP_allocNetworkBuffer;
    cm->cm.freeNetworkBuffer = UDP_freeNetworkBuffer;
    cm->cm.sendWithConnection = UDP_sendWithConnection;
    cm->cm.closeConnection = UDP_shutdownConnection;
    return &cm->cm;
}

