/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPEN62541_PUBSUB_TEST_HELPERS_H
#define OPEN62541_PUBSUB_TEST_HELPERS_H

#include <open62541/types.h>

#ifdef __APPLE__
# include <ifaddrs.h>
# include <net/if.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <string.h>
#endif

static UA_INLINE char *
UA_PubSubTest_getUdpMulticastUrl4840(void) {
    static char url[] = "opc.udp://224.0.0.22:4840/";
    return url;
}

static UA_INLINE char *
UA_PubSubTest_getUdpMulticastUrl4801(void) {
    static char url[] = "opc.udp://224.0.0.22:4801/";
    return url;
}

static UA_INLINE char *
UA_PubSubTest_getUdpMulticastUrlIPv6_4840(void) {
    static char url[] = "opc.udp://[ff00::1:5]:4840/";
    return url;
}

#define UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840 UA_PubSubTest_getUdpMulticastUrl4840()
#define UA_PUBSUB_TEST_UDP_MULTICAST_URL_4801 UA_PubSubTest_getUdpMulticastUrl4801()
#define UA_PUBSUB_TEST_UDP_MULTICAST_URL_IPV6_4840 UA_PubSubTest_getUdpMulticastUrlIPv6_4840()

static UA_INLINE UA_String
UA_PubSubTest_getMulticastInterface(void) {
#ifdef __APPLE__
    static char interfaceName[IF_NAMESIZE];
    struct ifaddrs *ifaddr = NULL;

    if(interfaceName[0] != '\0')
        return UA_STRING(interfaceName);

    if(getifaddrs(&ifaddr) != 0)
        return UA_STRING_NULL;

    struct ifaddrs *fallback = NULL;
    for(struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if(!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_MULTICAST) == 0)
            continue;

        if((ifa->ifa_flags & IFF_LOOPBACK) == 0) {
            fallback = ifa;
            break;
        }
        if(!fallback)
            fallback = ifa;
    }

    if(fallback) {
        strncpy(interfaceName, fallback->ifa_name, sizeof(interfaceName) - 1);
        interfaceName[sizeof(interfaceName) - 1] = '\0';
    }

    freeifaddrs(ifaddr);
    return interfaceName[0] != '\0' ? UA_STRING(interfaceName) : UA_STRING_NULL;
#else
    return UA_STRING_NULL;
#endif
}

#define UA_PUBSUB_TEST_NETWORKADDRESSURL(URL) \
    {UA_PubSubTest_getMulticastInterface(), UA_STRING(URL)}

static UA_INLINE void
UA_PubSubTest_initNetworkAddressUrl(UA_NetworkAddressUrlDataType *address,
                                    char *url) {
    UA_NetworkAddressUrlDataType_init(address);
    address->networkInterface = UA_PubSubTest_getMulticastInterface();
    address->url = UA_STRING(url);
}

static UA_INLINE UA_StatusCode
UA_PubSubTest_initNetworkAddressUrlAlloc(UA_NetworkAddressUrlDataType *address,
                                         char *url) {
    UA_NetworkAddressUrlDataType_init(address);
    UA_String networkInterface = UA_PubSubTest_getMulticastInterface();
    if(networkInterface.length > 0) {
        address->networkInterface =
            UA_String_fromChars((char*)networkInterface.data);
        if(address->networkInterface.length == 0)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    address->url = UA_String_fromChars(url);
    if(address->url.length == 0 && url && url[0] != '\0')
        return UA_STATUSCODE_BADOUTOFMEMORY;
    return UA_STATUSCODE_GOOD;
}

#endif /* OPEN62541_PUBSUB_TEST_HELPERS_H */
