/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_server_internal.h"
#include "ua_services.h"

#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)

#ifdef UA_ENABLE_MULTITHREADING

static void *
multicastWorkerLoop(UA_Server *server) {
    struct timeval next_sleep = {.tv_sec = 0, .tv_usec = 0};
    volatile UA_Boolean *running = &server->discoveryManager.mdnsRunning;
    fd_set fds;

    while(*running) {
        FD_ZERO(&fds);
        UA_fd_set(server->discoveryManager.mdnsSocket, &fds);
        select(server->discoveryManager.mdnsSocket + 1, &fds, 0, 0, &next_sleep);

        if(!*running)
            break;

        unsigned short retVal =
            mdnsd_step(server->discoveryManager.mdnsDaemon, server->discoveryManager.mdnsSocket,
                       FD_ISSET(server->discoveryManager.mdnsSocket, &fds), true, &next_sleep);
        if(retVal == 1) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                          "Multicast error: Can not read from socket. %s", errno_str));
            break;
        } else if (retVal == 2) {
            UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Multicast error: Can not write to socket. %s", errno_str));
            break;
        }
    }
    return NULL;
}

static UA_StatusCode
multicastListenStart(UA_Server* server) {
    int err = pthread_create(&server->discoveryManager.mdnsThread, NULL,
                             (void* (*)(void*))multicastWorkerLoop, server);
    if(err != 0) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not create multicast thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
multicastListenStop(UA_Server* server) {
    mdnsd_shutdown(server->discoveryManager.mdnsDaemon);
    // wake up select
    if (write(server->discoveryManager.mdnsSocket, "\0", 1)) {
        // TODO: if makes no sense here?
    }  // TODO: move to arch?
    if (pthread_join(server->discoveryManager.mdnsThread, NULL)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast error: Can not stop thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

# endif /* UA_ENABLE_MULTITHREADING */

static UA_StatusCode
addMdnsRecordForNetworkLayer(UA_Server *server, const UA_String *appName,
                             const UA_ServerNetworkLayer* nl) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval = UA_parseEndpointUrl(&nl->discoveryUrl, &hostname,
                                               &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Server url is invalid: %.*s",
                       (int)nl->discoveryUrl.length, nl->discoveryUrl.data);
        return retval;
    }

    retval = UA_Discovery_addRecord(server, appName, &hostname, port,
                                    &path, UA_DISCOVERY_TCP, true,
                                    server->config.discovery.mdns.serverCapabilities,
                                    server->config.discovery.mdns.serverCapabilitiesSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Cannot add mDNS Record: %s",
                       UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

void startMulticastDiscoveryServer(UA_Server *server) {
    UA_String *appName = &server->config.discovery.mdns.mdnsServerName;
    for(size_t i = 0; i < server->config.networkLayersSize; i++)
        addMdnsRecordForNetworkLayer(server, appName, &server->config.networkLayers[i]);

    /* find any other server on the net */
    UA_Discovery_multicastQuery(server);

# ifdef UA_ENABLE_MULTITHREADING
    multicastListenStart(server);
# endif
}

void
stopMulticastDiscoveryServer(UA_Server *server) {
    if (!server->discoveryManager.mdnsDaemon)
        return;

    char hostname[256];
    if(UA_gethostname(hostname, 255) == 0) {
        UA_String hnString = UA_STRING(hostname);
        UA_Discovery_removeRecord(server, &server->config.discovery.mdns.mdnsServerName,
                                  &hnString, 4840, true);
    } else {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not get hostname for multicast discovery.");
    }

# ifdef UA_ENABLE_MULTITHREADING
    multicastListenStop(server);
# else
    // send out last package with TTL = 0
    iterateMulticastDiscoveryServer(server, NULL, false);
# endif
}

/* All filter criteria must be fulfilled */
static UA_Boolean
filterServerRecord(size_t serverCapabilityFilterSize, UA_String *serverCapabilityFilter,
                   serverOnNetwork_list_entry* current) {
    for(size_t i = 0; i < serverCapabilityFilterSize; i++) {
        for(size_t j = 0; j < current->serverOnNetwork.serverCapabilitiesSize; j++)
            if(!UA_String_equal(&serverCapabilityFilter[i],
                                &current->serverOnNetwork.serverCapabilities[j]))
                return false;
    }
    return true;
}

void Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                                  const UA_FindServersOnNetworkRequest *request,
                                  UA_FindServersOnNetworkResponse *response) {
    if (!server->config.discovery.mdnsEnable) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    /* Set LastCounterResetTime */
    UA_DateTime_copy(&server->discoveryManager.serverOnNetworkRecordIdLastReset,
                     &response->lastCounterResetTime);

    /* Compute the max number of records to return */
    UA_UInt32 recordCount = 0;
    if(request->startingRecordId < server->discoveryManager.serverOnNetworkRecordIdCounter)
        recordCount = server->discoveryManager.serverOnNetworkRecordIdCounter -
            request->startingRecordId;
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
    LIST_FOREACH(current, &server->discoveryManager.serverOnNetwork, pointers) {
        if(filteredCount >= recordCount)
            break;
        if(current->serverOnNetwork.recordId < request->startingRecordId)
            continue;
        if(!filterServerRecord(request->serverCapabilityFilterSize,
                               request->serverCapabilityFilter, current))
            continue;
        filtered[filteredCount++] = &current->serverOnNetwork;
    }

    if(filteredCount == 0)
        return;

    /* Allocate the array for the response */
    response->servers =
        (UA_ServerOnNetwork*)UA_malloc(sizeof(UA_ServerOnNetwork)*filteredCount);
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
UA_Server_updateMdnsForDiscoveryUrl(UA_Server *server, const UA_String *serverName,
                                    const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                    const UA_String *discoveryUrl,
                                    UA_Boolean isOnline, UA_Boolean updateTxt) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval = UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Server url invalid: %.*s",
                       (int)discoveryUrl->length, discoveryUrl->data);
        return;
    }

    if(!isOnline) {
        UA_StatusCode removeRetval =
                UA_Discovery_removeRecord(server, serverName, &hostname,
                                          port, updateTxt);
        if(removeRetval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
        UA_Discovery_addRecord(server, serverName, &hostname,
                               port, &path, UA_DISCOVERY_TCP, updateTxt,
                               capabilities, capabilitiesSize);
    if(addRetval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Could not add mDNS record for hostname %.*s.",
                       (int)serverName->length, serverName->data);
}

void
UA_Server_setServerOnNetworkCallback(UA_Server *server,
                                     UA_Server_serverOnNetworkCallback cb,
                                     void* data) {
    server->discoveryManager.serverOnNetworkCallback = cb;
    server->discoveryManager.serverOnNetworkCallbackData = data;
}

static void
UA_Discovery_multicastConflict(char *name, int type, void *arg) {
    // cppcheck-suppress unreadVariable
    UA_Server *server = (UA_Server*) arg;
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
    if (hostnameLen > 0) {
        UA_snprintf(outServiceDomain, maxLen + 1, "%.*s-%.*s",
                    (int) servernameLen, (char *) servername->data,
                    (int) hostnameLen, (char *) hostname->data);
        offset = servernameLen + hostnameLen + 1;
    }
    else {
        UA_snprintf(outServiceDomain, maxLen + 1, "%.*s",
                    (int) servernameLen, (char *) servername->data);
        offset = servernameLen;
    }
    UA_snprintf(&outServiceDomain[offset], 24, "._opcua-tcp._tcp.local.");
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
UA_Discovery_recordExists(UA_Server* server, const char* fullServiceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(server->discoveryManager.mdnsDaemon, fullServiceDomain);
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
    if(a->type != QTYPE_PTR)
        return 0;

    if(a->rdname == NULL)
        return 0;

    /* Skip, if we already know about this server */
    UA_Boolean exists =
        UA_Discovery_recordExists(server, a->rdname, 0, UA_DISCOVERY_TCP);
    if(exists == true)
        return 0;

    if(mdnsd_has_query(server->discoveryManager.mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(server->discoveryManager.mdnsDaemon, a->rdname, QTYPE_SRV,
                discovery_multicastQueryAnswer, server);
    mdnsd_query(server->discoveryManager.mdnsDaemon, a->rdname, QTYPE_TXT,
                discovery_multicastQueryAnswer, server);
    return 0;
}

UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server) {
    mdnsd_query(server->discoveryManager.mdnsDaemon, "_opcua-tcp._tcp.local.",
                QTYPE_PTR,discovery_multicastQueryAnswer, server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_addRecord(UA_Server *server, const UA_String *servername,
                       const UA_String *hostname, UA_UInt16 port,
                       const UA_String *path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize) {
    // we assume that the hostname is not an IP address, but a valid domain name
    // It is required by the OPC UA spec (see Part 12, DiscoveryURL to DNS SRV mapping)
    // to always use the hostname instead of the IP address

    if(capabilitiesSize > 0 && !capabilites)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t hostnameLen = hostname->length;
    size_t servernameLen = servername->length;
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    // use a limit for the hostname length to make sure full string fits into 63
    // chars (limited by DNS spec)
    if(hostnameLen+servernameLen + 1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    } else if(hostnameLen > 63) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Hostname length exceeds maximum of 63 chars. "
                       "It will be truncated.");
    }

    if(!server->discoveryManager.mdnsMainSrvAdded) {
        mdns_record_t *r =
            mdnsd_shared(server->discoveryManager.mdnsDaemon, "_services._dns-sd._udp.local.",
                         QTYPE_PTR, 600);
        mdnsd_set_host(server->discoveryManager.mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        server->discoveryManager.mdnsMainSrvAdded = true;
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char fullServiceDomain[63+24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_Boolean exists = UA_Discovery_recordExists(server, fullServiceDomain, port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: add record for domain: %s", fullServiceDomain);

    // _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local

    // check if there is already a PTR entry for the given service.

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    mdns_record_t *r = mdns_find_record(server->discoveryManager.mdnsDaemon, QTYPE_PTR,
                                        "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        r = mdnsd_shared(server->discoveryManager.mdnsDaemon, "_opcua-tcp._tcp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(server->discoveryManager.mdnsDaemon, r, fullServiceDomain);
    }

    /* The first 63 characters of the hostname (or less) */
    size_t maxHostnameLen = UA_MIN(hostnameLen, 63);
    char localDomain[65];
    memcpy(localDomain, hostname->data, maxHostnameLen);
    localDomain[maxHostnameLen] = '.';
    localDomain[maxHostnameLen+1] = '\0';

    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    r = mdnsd_unique(server->discoveryManager.mdnsDaemon, fullServiceDomain, QTYPE_SRV, 600,
                     UA_Discovery_multicastConflict, server);
    mdnsd_set_srv(server->discoveryManager.mdnsDaemon, r, 0, 0, port, localDomain);

    // A/AAAA record for all ip addresses.
    // [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
    // [hostname]. A [ip].
    mdns_set_address_record(server, fullServiceDomain, localDomain);

    // TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    UA_STACKARRAY(char, pathChars, path->length + 1);
    if(createTxt) {
        if(path->length > 0)
            memcpy(pathChars, path->data, path->length);
        pathChars[path->length] = 0;
        mdns_create_txt(server, fullServiceDomain, pathChars, capabilites,
                        capabilitiesSize, UA_Discovery_multicastConflict);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_removeRecord(UA_Server *server, const UA_String *servername,
                          const UA_String *hostname, UA_UInt16 port,
                          UA_Boolean removeTxt) {
    // use a limit for the hostname length to make sure full string fits into 63
    // chars (limited by DNS spec)
    size_t hostnameLen = hostname->length;
    size_t servernameLen = servername->length;
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(hostnameLen+servernameLen+1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char fullServiceDomain[63 + 24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: remove record for domain: %s", fullServiceDomain);

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    mdns_record_t *r = mdns_find_record(server->discoveryManager.mdnsDaemon, QTYPE_PTR,
                                        "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: could not remove record. "
                       "PTR Record not found for domain: %s", fullServiceDomain);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }
    mdnsd_done(server->discoveryManager.mdnsDaemon, r);

    // looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port hostname.local.
    // and TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    // and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]
    mdns_record_t *r2 = mdnsd_get_published(server->discoveryManager.mdnsDaemon, fullServiceDomain);
    if(!r2) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
            mdnsd_done(server->discoveryManager.mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
iterateMulticastDiscoveryServer(UA_Server* server, UA_DateTime *nextRepeat,
                                UA_Boolean processIn) {
    struct timeval next_sleep = { 0, 0 };
    unsigned short retval = mdnsd_step(server->discoveryManager.mdnsDaemon,
                                       (int)server->discoveryManager.mdnsSocket,
                                       processIn, true, &next_sleep);
    if(retval == 1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
               UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not read from socket. %s", errno_str));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    } else if(retval == 2) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not write to socket. %s", errno_str));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    }

    if(nextRepeat)
        *nextRepeat = UA_DateTime_now() +
            (UA_DateTime)((next_sleep.tv_sec * UA_DATETIME_SEC) +
                          (next_sleep.tv_usec * UA_DATETIME_USEC));
    return UA_STATUSCODE_GOOD;
}

#endif /* defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST) */
