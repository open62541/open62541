/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_server_internal.h"
#include "ua_discovery_manager.h"
#include "ua_services.h"

#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)

typedef enum {
    UA_DISCOVERY_TCP,    /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

/* Create a mDNS Record for the given server info and adds it to the mDNS output
 * queue.
 *
 * Additionally this method also adds the given server to the internal
 * serversOnNetwork list so that a client finds it when calling
 * FindServersOnNetwork. */
static UA_StatusCode
UA_Discovery_addRecord(UA_DiscoveryManager *dm, const UA_String *servername,
                       const UA_String *hostname, UA_UInt16 port,
                       const UA_String *path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize,
                       UA_Boolean isSelf);

/* Create a mDNS Record for the given server info with TTL=0 and adds it to the
 * mDNS output queue.
 *
 * Additionally this method also removes the given server from the internal
 * serversOnNetwork list so that a client gets the updated data when calling
 * FindServersOnNetwork. */
static UA_StatusCode
UA_Discovery_removeRecord(UA_DiscoveryManager *dm, const UA_String *servername,
                          const UA_String *hostname, UA_UInt16 port,
                          UA_Boolean removeTxt);

static int
discovery_multicastQueryAnswer(mdns_answer_t *a, void *arg);

static void
multicastPolling(UA_Server *server, void *_) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return;

    struct timeval next_sleep = {.tv_sec = 0, .tv_usec = 0};
    fd_set fds;
    FD_ZERO(&fds);
    UA_fd_set(dm->mdnsSocket, &fds);
    select(dm->mdnsSocket + 1, &fds, 0, 0, &next_sleep);

    unsigned short retVal =
        mdnsd_step(dm->mdnsDaemon, dm->mdnsSocket,
                   FD_ISSET(dm->mdnsSocket, &fds),
                   true, &next_sleep);
    if(retVal == 1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                        "Multicast error: Can not read from socket. %s", errno_str));
    } else if(retVal == 2) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                        "Multicast error: Can not write to socket. %s", errno_str));
        }
}

static UA_StatusCode
addMdnsRecordForNetworkLayer(UA_DiscoveryManager *dm, const UA_String *appName,
                             const UA_String *discoveryUrl) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url is invalid: %.*s",
                       (int)discoveryUrl->length, discoveryUrl->data);
        return retval;
    }

    retval = UA_Discovery_addRecord(dm, appName, &hostname, port,
                                    &path, UA_DISCOVERY_TCP, true,
                                    dm->serverConfig->mdnsConfig.serverCapabilities,
                                    dm->serverConfig->mdnsConfig.serverCapabilitiesSize,
                                    true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Cannot add mDNS Record: %s", UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

#ifndef IN_ZERONET
#define IN_ZERONET(addr) ((addr & IN_CLASSA_NET) == 0)
#endif

/* Create multicast 224.0.0.251:5353 socket */
static UA_SOCKET
discovery_createMulticastSocket(UA_Server* server) {
    int flag = 1, ittl = 255;
    struct sockaddr_in in;
    struct ip_mreq mc;
    char ttl = (char)255; // publish to complete net, not only subnet. See:
                          // https://docs.oracle.com/cd/E23824_01/html/821-1602/sockets-137.html

    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_port = htons(5353);
    in.sin_addr.s_addr = 0;

    UA_SOCKET s = UA_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s == UA_INVALID_SOCKET)
        return UA_INVALID_SOCKET;

#ifdef SO_REUSEPORT
    UA_setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
    UA_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
    if(UA_bind(s, (struct sockaddr *)&in, sizeof(in))) {
        UA_close(s);
        return UA_INVALID_SOCKET;
    }

    /* Custom outbound multicast interface */
    size_t length = server->config.mdnsInterfaceIP.length;
    if(length > 0){
        char* interfaceName = (char*)UA_malloc(length+1);
        if(!interfaceName) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                         "Multicast DNS: cannot alloc memory for iface name");
            return 0;
        }
        struct in_addr ina;
        memset(&ina, 0, sizeof(ina));
        memcpy(interfaceName, server->config.mdnsInterfaceIP.data, length);
        interfaceName[length] = '\0';
        inet_pton(AF_INET, interfaceName, &ina);
        UA_free(interfaceName);
        /* Set interface for outbound multicast */
        if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&ina, sizeof(ina)) < 0)
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                         "Multicast DNS: failed setting IP_MULTICAST_IF to %s: %s",
                         inet_ntoa(ina), strerror(errno));
    }

    /* Check outbound multicast interface parameters */
    struct in_addr interface_addr;
    socklen_t addr_size = sizeof(struct in_addr);
    if(getsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&interface_addr, &addr_size) <  0) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                     "Multicast DNS: getsockopt(IP_MULTICAST_IF) failed");
    }

    if(IN_ZERONET(ntohl(interface_addr.s_addr))) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: outbound interface 0.0.0.0, it means that "
                       "the first OS interface is used (you can explicitly set the "
                       "interface by using 'discovery.mdnsInterfaceIP' config parameter)");
    } else {
        char buf[16];
        inet_ntop(AF_INET, &interface_addr, buf, 16);
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                    "Multicast DNS: outbound interface is %s", buf);
    }

    mc.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
    mc.imr_interface.s_addr = htonl(INADDR_ANY);
    UA_setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mc, sizeof(mc));
    UA_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
    UA_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl, sizeof(ittl));

    UA_socket_set_nonblocking(s); //TODO: check return value
    return s;
}

void
startMulticastDiscoveryServer(UA_Server *server) {
    /* Initialize the mdns daemon */
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return;

    if(!dm->mdnsDaemon) {
        dm->mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
        mdnsd_register_receive_callback(dm->mdnsDaemon,
                                        mdns_record_received, dm);
    }

    /* Open the mdns listen socket */
    UA_initialize_architecture_network();
    if(dm->mdnsSocket == UA_INVALID_SOCKET)
        dm->mdnsSocket = discovery_createMulticastSocket(server);
    if(dm->mdnsSocket == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                             "Could not create multicast socket. Error: %d - %s",
                             errno, errno_str));
        return;
    }

    /* Add record for the server itself */
    UA_String *appName = &server->config.mdnsConfig.mdnsServerName;
    for(size_t i = 0; i < server->config.serverUrlsSize; i++)
        addMdnsRecordForNetworkLayer(dm, appName,
                                     &server->config.serverUrls[i]);

    /* Send a multicast probe to find any other OPC UA server on the network
     * through mDNS */
    mdnsd_query(dm->mdnsDaemon, "_opcua-tcp._tcp.local.",
                QTYPE_PTR,discovery_multicastQueryAnswer, server);

    /* Start the cyclic polling callback */
    if(dm->mdnsCallbackId == 0) {
        UA_EventLoop *el = server->config.eventLoop;
        if(el) {
            el->addCyclicCallback(el, (UA_Callback)multicastPolling,
                                  server, NULL, 200, NULL,
                                  UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                  &dm->mdnsCallbackId);
        }
    }
}

void
stopMulticastDiscoveryServer(UA_Server *server) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return;

    for(size_t i = 0; i < server->config.serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;

        UA_StatusCode retval =
            UA_parseEndpointUrl(&server->config.serverUrls[i],
                                &hostname, &port, &path);

        if(retval != UA_STATUSCODE_GOOD || hostname.length == 0)
            continue;

        UA_Discovery_removeRecord(dm, &server->config.mdnsConfig.mdnsServerName,
                                  &hostname, port, true);
    }

    /* Stop the cyclic polling callback */
    if(dm->mdnsCallbackId != 0) {
        UA_EventLoop *el = server->config.eventLoop;
        if(el) {
            el->removeCyclicCallback(el, dm->mdnsCallbackId);
            dm->mdnsCallbackId = 0;
        }
    }

    /* Clean up mdns daemon */
    if(dm->mdnsDaemon) {
        mdnsd_shutdown(dm->mdnsDaemon);
        mdnsd_free(dm->mdnsDaemon);
        dm->mdnsDaemon = NULL;
    }

    /* Close the socket */
    if(dm->mdnsSocket != UA_INVALID_SOCKET) {
        UA_close(dm->mdnsSocket);
        dm->mdnsSocket = UA_INVALID_SOCKET;
    }
}

/* All filter criteria must be fulfilled in the list entry. The comparison is
 * case insensitive. Returns true if the entry matches the filter. */
static UA_Boolean
entryMatchesCapabilityFilter(size_t serverCapabilityFilterSize,
                             UA_String *serverCapabilityFilter,
                             serverOnNetwork_list_entry* current) {
    /* If the entry has less capabilities defined than the filter, there's no match */
    if(serverCapabilityFilterSize > current->serverOnNetwork.serverCapabilitiesSize)
        return false;
    for(size_t i = 0; i < serverCapabilityFilterSize; i++) {
        UA_Boolean capabilityFound = false;
        for(size_t j = 0; j < current->serverOnNetwork.serverCapabilitiesSize; j++) {
            if(UA_String_equal_ignorecase(&serverCapabilityFilter[i],
                               &current->serverOnNetwork.serverCapabilities[j])) {
                capabilityFound = true;
                break;
            }
        }
        if(!capabilityFound)
            return false;
    }
    return true;
}

void
Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                             const UA_FindServersOnNetworkRequest *request,
                             UA_FindServersOnNetworkResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    if(!server->config.mdnsEnabled) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    /* Set LastCounterResetTime */
    response->lastCounterResetTime =
        dm->serverOnNetworkRecordIdLastReset;

    /* Compute the max number of records to return */
    UA_UInt32 recordCount = 0;
    if(request->startingRecordId < dm->serverOnNetworkRecordIdCounter)
        recordCount = dm->serverOnNetworkRecordIdCounter - request->startingRecordId;
    if(request->maxRecordsToReturn && recordCount > request->maxRecordsToReturn)
        recordCount = UA_MIN(recordCount, request->maxRecordsToReturn);
    if(recordCount == 0) {
        response->serversSize = 0;
        return;
    }

    /* Iterate over all records and add to filtered list */
    UA_UInt32 filteredCount = 0;
    UA_STACKARRAY(UA_ServerOnNetwork*, filtered, recordCount);
    serverOnNetwork_list_entry* current;
    LIST_FOREACH(current, &dm->serverOnNetwork, pointers) {
        if(filteredCount >= recordCount)
            break;
        if(current->serverOnNetwork.recordId < request->startingRecordId)
            continue;
        if(!entryMatchesCapabilityFilter(request->serverCapabilityFilterSize,
                               request->serverCapabilityFilter, current))
            continue;
        filtered[filteredCount++] = &current->serverOnNetwork;
    }

    if(filteredCount == 0)
        return;

    /* Allocate the array for the response */
    response->servers = (UA_ServerOnNetwork*)
        UA_malloc(sizeof(UA_ServerOnNetwork)*filteredCount);
    if(!response->servers) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->serversSize = filteredCount;

    /* Copy the server names */
    for(size_t i = 0; i < filteredCount; i++)
        UA_ServerOnNetwork_copy(filtered[i], &response->servers[filteredCount-i-1]);
}

void
UA_Discovery_updateMdnsForDiscoveryUrl(UA_DiscoveryManager *dm, const UA_String *serverName,
                                       const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                       const UA_String *discoveryUrl,
                                       UA_Boolean isOnline, UA_Boolean updateTxt) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %.*s",
                       (int)discoveryUrl->length, discoveryUrl->data);
        return;
    }

    if(!isOnline) {
        UA_StatusCode removeRetval =
                UA_Discovery_removeRecord(dm, serverName, &hostname,
                                          port, updateTxt);
        if(removeRetval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                           "Could not remove mDNS record for hostname %.*s.",
                           (int)serverName->length, serverName->data);
        return;
    }

    UA_String *capabilities = NULL;
    size_t capabilitiesSize = 0;
    if(mdnsConfig) {
        capabilities = mdnsConfig->serverCapabilities;
        capabilitiesSize = mdnsConfig->serverCapabilitiesSize;
    }

    UA_StatusCode addRetval =
        UA_Discovery_addRecord(dm, serverName, &hostname,
                               port, &path, UA_DISCOVERY_TCP, updateTxt,
                               capabilities, capabilitiesSize,
                               false);
    if(addRetval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not add mDNS record for hostname %.*s.",
                       (int)serverName->length, serverName->data);
}

void
UA_Server_setServerOnNetworkCallback(UA_Server *server,
                                     UA_Server_serverOnNetworkCallback cb,
                                     void* data) {
    UA_LOCK(&server->serviceMutex);
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(dm) {
        dm->serverOnNetworkCallback = cb;
        dm->serverOnNetworkCallbackData = data;
    }
    UA_UNLOCK(&server->serviceMutex);
}

static void
UA_Discovery_multicastConflict(char *name, int type, void *arg) {
    /* In case logging is disabled */
    (void)name;
    (void)type;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*) arg;
    UA_LOG_ERROR(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                 "Multicast DNS name conflict detected: "
                 "'%s' for type %d", name, type);
}

/* Create a service domain with the format [servername]-[hostname]._opcua-tcp._tcp.local. */
static void
createFullServiceDomain(char *outServiceDomain, size_t maxLen,
                        const UA_String *servername, const UA_String *hostname) {
    size_t hostnameLen = hostname->length;
    size_t servernameLen = servername->length;

    maxLen -= 24; /* the length we have remaining before the opc ua postfix and
                   * the trailing zero */

    /* Can we use hostname and servername with full length? */
    if(hostnameLen + servernameLen + 1 > maxLen) {
        if(servernameLen + 2 > maxLen) {
            servernameLen = maxLen;
            hostnameLen = 0;
        } else {
            hostnameLen = maxLen - servernameLen - 1;
        }
    }

    size_t offset = 0;
    if(hostnameLen > 0) {
        UA_snprintf(outServiceDomain, maxLen + 1, "%.*s-%.*s",
                    (int) servernameLen, (char *) servername->data,
                    (int) hostnameLen, (char *) hostname->data);
        offset = servernameLen + hostnameLen + 1;
        //replace all dots with minus. Otherwise mDNS is not valid
        for(size_t i=servernameLen+1; i<offset; i++) {
            if(outServiceDomain[i] == '.')
                outServiceDomain[i] = '-';
        }
    } else {
        UA_snprintf(outServiceDomain, maxLen + 1, "%.*s",
                    (int) servernameLen, (char *) servername->data);
        offset = servernameLen;
    }
    UA_snprintf(&outServiceDomain[offset], 24, "._opcua-tcp._tcp.local.");
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
UA_Discovery_recordExists(UA_DiscoveryManager *dm, const char* fullServiceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(dm->mdnsDaemon, fullServiceDomain);
    while(r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if(data->type == QTYPE_SRV && (port == 0 || data->srv.port == port))
            return true;
        r = mdnsd_record_next(r);
    }
    return false;
}

static int
discovery_multicastQueryAnswer(mdns_answer_t *a, void *arg) {
    UA_Server *server = (UA_Server*) arg;
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return 0;

    if(a->type != QTYPE_PTR)
        return 0;

    if(a->rdname == NULL)
        return 0;

    /* Skip, if we already know about this server */
    UA_Boolean exists =
        UA_Discovery_recordExists(dm, a->rdname, 0, UA_DISCOVERY_TCP);
    if(exists == true)
        return 0;

    if(mdnsd_has_query(dm->mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_DISCOVERY,
                 "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(dm->mdnsDaemon, a->rdname, QTYPE_SRV,
                discovery_multicastQueryAnswer, server);
    mdnsd_query(dm->mdnsDaemon, a->rdname, QTYPE_TXT,
                discovery_multicastQueryAnswer, server);
    return 0;
}

static UA_StatusCode
UA_Discovery_addRecord(UA_DiscoveryManager *dm, const UA_String *servername,
                       const UA_String *hostname, UA_UInt16 port,
                       const UA_String *path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize,
                       UA_Boolean isSelf) {
    /* We assume that the hostname is not an IP address, but a valid domain
     * name. It is required by the OPC UA spec (see Part 12, DiscoveryURL to DNS
     * SRV mapping) to always use the hostname instead of the IP address. */

    if(capabilitiesSize > 0 && !capabilites)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t hostnameLen = hostname->length;
    size_t servernameLen = servername->length;
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    /* Use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    if(hostnameLen+servernameLen + 1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    } else if(hostnameLen > 63) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Hostname length exceeds maximum of 63 chars. "
                       "It will be truncated.");
    }

    if(!dm->mdnsMainSrvAdded) {
        mdns_record_t *r =
            mdnsd_shared(dm->mdnsDaemon, "_services._dns-sd._udp.local.",
                         QTYPE_PTR, 600);
        mdnsd_set_host(dm->mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        dm->mdnsMainSrvAdded = true;
    }

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomain[63+24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_Boolean exists = UA_Discovery_recordExists(dm, fullServiceDomain,
                                                  port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: add record for domain: %s", fullServiceDomain);

    if(isSelf && dm->selfFqdnMdnsRecord.length == 0) {
        dm->selfFqdnMdnsRecord = UA_STRING_ALLOC(fullServiceDomain);
        if(!dm->selfFqdnMdnsRecord.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    struct serverOnNetwork_list_entry *listEntry;
    /* The servername is servername + hostname. It is the same which we get
     * through mDNS and therefore we need to match servername */
    UA_StatusCode retval =
        UA_DiscoveryManager_addEntryToServersOnNetwork(dm, fullServiceDomain,
                                                       fullServiceDomain,
                                                       UA_MIN(63, (servernameLen+hostnameLen)+1),
                                                       &listEntry);
    if(retval != UA_STATUSCODE_GOOD &&
       retval != UA_STATUSCODE_BADALREADYEXISTS)
        return retval;

    /* If entry is already in list, skip initialization of capabilities and txt+srv */
    if(retval != UA_STATUSCODE_BADALREADYEXISTS) {
        /* if capabilitiesSize is 0, then add default cap 'NA' */
        listEntry->serverOnNetwork.serverCapabilitiesSize = UA_MAX(1, capabilitiesSize);
        listEntry->serverOnNetwork.serverCapabilities = (UA_String *)
            UA_Array_new(listEntry->serverOnNetwork.serverCapabilitiesSize,
                         &UA_TYPES[UA_TYPES_STRING]);
        if(!listEntry->serverOnNetwork.serverCapabilities)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(capabilitiesSize == 0) {
            UA_String na;
            na.length = 2;
            na.data = (UA_Byte *) (uintptr_t) "NA";
            UA_String_copy(&na, &listEntry->serverOnNetwork.serverCapabilities[0]);
        } else {
            for(size_t i = 0; i < capabilitiesSize; i++)
                UA_String_copy(&capabilites[i],
                               &listEntry->serverOnNetwork.serverCapabilities[i]);
        }

        listEntry->txtSet = true;

        UA_STACKARRAY(char, newUrl, 10 + hostnameLen + 8 + path->length + 1);
        UA_snprintf(newUrl, 10 + hostnameLen + 8 + path->length + 1,
                    "opc.tcp://%.*s:%d%s%.*s", (int) hostnameLen,
                    hostname->data, port, path->length > 0 ? "/" : "",
                    (int) path->length, path->data);
        listEntry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
        listEntry->srvSet = true;
    }

    /* _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local */

    /* check if there is already a PTR entry for the given service. */

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(dm->mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        r = mdnsd_shared(dm->mdnsDaemon, "_opcua-tcp._tcp.local.",
                         QTYPE_PTR, 600);
        mdnsd_set_host(dm->mdnsDaemon, r, fullServiceDomain);
    }

    /* The first 63 characters of the hostname (or less) */
    size_t maxHostnameLen = UA_MIN(hostnameLen, 63);
    char localDomain[65];
    memcpy(localDomain, hostname->data, maxHostnameLen);
    localDomain[maxHostnameLen] = '.';
    localDomain[maxHostnameLen+1] = '\0';

    /* [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname]. */
    r = mdnsd_unique(dm->mdnsDaemon, fullServiceDomain,
                     QTYPE_SRV, 600, UA_Discovery_multicastConflict, dm);
    mdnsd_set_srv(dm->mdnsDaemon, r, 0, 0, port, localDomain);

    /* A/AAAA record for all ip addresses.
     * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
     * [hostname]. A [ip]. */
    mdns_set_address_record(dm, fullServiceDomain, localDomain);

    /* TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,... */
    UA_STACKARRAY(char, pathChars, path->length + 1);
    if(createTxt) {
        if(path->length > 0)
            memcpy(pathChars, path->data, path->length);
        pathChars[path->length] = 0;
        mdns_create_txt(dm, fullServiceDomain, pathChars, capabilites,
                        capabilitiesSize, UA_Discovery_multicastConflict);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Discovery_removeRecord(UA_DiscoveryManager *dm, const UA_String *servername,
                          const UA_String *hostname, UA_UInt16 port,
                          UA_Boolean removeTxt) {
    /* use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    size_t hostnameLen = hostname->length;
    size_t servernameLen = servername->length;
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(hostnameLen+servernameLen+1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    }

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomain[63 + 24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_LOG_INFO(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: remove record for domain: %s",
                fullServiceDomain);

    UA_StatusCode retval =
        UA_DiscoveryManager_removeEntryFromServersOnNetwork(dm, fullServiceDomain,
                   fullServiceDomain, UA_MIN(63, (servernameLen+hostnameLen)+1));
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(dm->mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. "
                       "PTR Record not found for domain: %s", fullServiceDomain);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }
    mdnsd_done(dm->mdnsDaemon, r);

    /* looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5
     * port hostname.local. and TXT record:
     * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
     * and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip] */
    mdns_record_t *r2 =
        mdnsd_get_published(dm->mdnsDaemon, fullServiceDomain);
    if(!r2) {
        UA_LOG_WARNING(dm->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. Record not "
                       "found for domain: %s", fullServiceDomain);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }

    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if((removeTxt && data->type == QTYPE_TXT) ||
           (removeTxt && data->type == QTYPE_A) ||
           data->srv.port == port) {
            mdnsd_done(dm->mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST) */
