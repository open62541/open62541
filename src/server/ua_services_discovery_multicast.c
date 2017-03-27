/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_mdns_internal.h"

#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)

#ifdef _MSC_VER
# ifndef UNDER_CE
#  include <io.h> //access
#  define access _access
# endif
#else
# include <unistd.h> //access
#endif

#include <fcntl.h>
#include <errno.h>
#ifdef _WIN32
# define CLOSESOCKET(S) closesocket((SOCKET)S)
#else
# define CLOSESOCKET(S) close(S)
#endif

/* All filter criteria must be fulfilled */
static UA_Boolean
filterServerRecord(size_t serverCapabilityFilterSize, UA_String *serverCapabilityFilter,
                   serverOnNetwork_list_entry* current) {
    for(size_t i = 0; i < request->serverCapabilityFilterSize; i++) {
        for(size_t j = 0; j < current->serverOnNetwork.serverCapabilitiesSize; j++)
            if(!UA_String_equal(&request->serverCapabilityFilter[i],
                                &current->serverOnNetwork.serverCapabilities[j]))
                return false;
    }
    return true;
}

void Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                                  const UA_FindServersOnNetworkRequest *request,
                                  UA_FindServersOnNetworkResponse *response) {
    /* Set LastCounterResetTime */
    UA_DateTime_copy(&server->serverOnNetworkRecordIdLastReset, &response->lastCounterResetTime);

    /* Compute the max number of records to return */
    UA_UInt32 recordCount = 0;
    if(request->startingRecordId < server->serverOnNetworkRecordIdCounter)
        recordCount = server->serverOnNetworkRecordIdCounter - request->startingRecordId;
    if(request->maxRecordsToReturn && recordCount > request->maxRecordsToReturn)
        recordCount = recordCount > request->maxRecordsToReturn ? request->maxRecordsToReturn : recordCount;
    if(recordCount == 0) {
        response->serversSize = 0;
        return;
    }

    /* Iterate over all records and add to filtered list */
    UA_UInt32 filteredCount = 0;
    UA_ServerOnNetwork** filtered =
        (UA_ServerOnNetwork**)UA_alloca(sizeof(UA_ServerOnNetwork*) * recordCount);
    serverOnNetwork_list_entry* current;
    LIST_FOREACH(current, &server->serverOnNetwork, pointers) {
        if(filteredCount >= recordCount)
            break;
        if(current->serverOnNetwork.recordId < request->startingRecordId)
            continue;
        if(!filterServerRecord(request->serverCapabilityFilterSize,
                               request->serverCapabilityFilter, current))
            continue;
        filtered[filteredCount++] = &current->serverOnNetwork;
    }

    /* Allocate the array for the response */
    response->servers = (UA_ServerOnNetwork*)UA_malloc(sizeof(UA_ServerOnNetwork)*filteredCount);
    if(!response->servers) {
        response->serversSize = -1;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    }
    response->serversSize = filteredCount;

    /* Copy the server names */
    for(size_t i = 0; i < filteredCount; i++)
        UA_ServerOnNetwork_copy(filtered[i], &response->servers[filteredCount-i-1]);
}

void
UA_Discovery_update_MdnsForDiscoveryUrl(UA_Server *server, const char *serverName,
                                        UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                        const UA_String discoveryUrl, UA_Boolean isOnline,
                                        UA_Boolean updateTxt) {
    UA_UInt16 port = 0;
    char hostname[256]; hostname[0] = '\0';
    const char *path = NULL;

    size_t uriSize = sizeof(char) * discoveryUrl.length + 1;

    // todo: malloc may fail: return a statuscode
    char* uri = (char*)UA_malloc(uriSize);
    strncpy(uri, (char*)discoveryUrl.data, discoveryUrl.length);
    uri[discoveryUrl.length] = '\0';

    UA_StatusCode retval = UA_EndpointUrl_split(uri, hostname, &port, &path);
    if (retval != UA_STATUSCODE_GOOD) {
        hostname[0] = '\0';
        if (retval == UA_STATUSCODE_BADOUTOFRANGE)
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK,
                           "Server url size invalid");
        else if (retval == UA_STATUSCODE_BADATTRIBUTEIDINVALID)
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK,
                           "Server url does not begin with opc.tcp://");
    }
    free(uri);

    if(!isOnline) {
        UA_StatusCode removeRetval =
                UA_Discovery_removeRecord(server, serverName, hostname,
                                          (unsigned short) port, updateTxt);
        if(removeRetval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Could not remove mDNS record for hostname %s.", serverName);
        }
    } else {
        UA_String *capabilities = NULL;
        size_t capabilitiesSize = 0;
        if(mdnsConfig) {
            capabilities = mdnsConfig->serverCapabilities;
            capabilitiesSize = mdnsConfig->serverCapabilitiesSize;
        }
        UA_StatusCode addRetval =
                UA_Discovery_addRecord(server, serverName, hostname,
                                       (unsigned short) port, path,
                                       UA_DISCOVERY_TCP, updateTxt,
                                       capabilities, &capabilitiesSize);
        if(addRetval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Could not add mDNS record for hostname %s.", serverName);
        }
    }
}

void
UA_Server_setServerOnNetworkCallback(UA_Server *server, UA_Server_serverOnNetworkCallback cb,
                                     void* data) {
    server->serverOnNetworkCallback = cb;
    server->serverOnNetworkCallbackData = data;
}

static void
socket_mdns_set_nonblocking(int sockfd) {
#ifdef _WIN32
    u_long iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);
#else
    int opts = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, opts|O_NONBLOCK);
#endif
}

/* Create multicast 224.0.0.251:5353 socket */
static int
discovery_createMulticastSocket(void) {
    int s, flag = 1, ittl = 255;
    struct sockaddr_in in;
    struct ip_mreq mc;
    char ttl = (char)255; // publish to complete net, not only subnet. See:
                          // https://docs.oracle.com/cd/E23824_01/html/821-1602/sockets-137.html

    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_port = htons(5353);
    in.sin_addr.s_addr = 0;

    if ((s = (int)socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return 0;

#ifdef SO_REUSEPORT
    setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
    if (bind(s, (struct sockaddr *)&in, sizeof(in))) {
        CLOSESOCKET(s);
        return 0;
    }

    mc.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
    mc.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mc, sizeof(mc));
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl, sizeof(ittl));

    socket_mdns_set_nonblocking(s);
    return s;
}

UA_StatusCode
UA_Discovery_multicastInit(UA_Server* server) {
    server->mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
    if((server->mdnsSocket = discovery_createMulticastSocket()) == 0) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not create multicast socket. Error: %s", strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    mdnsd_register_receive_callback(server->mdnsDaemon, mdns_record_received, server);
    return UA_STATUSCODE_GOOD;
}

void UA_Discovery_multicastDestroy(UA_Server* server) {
    mdnsd_shutdown(server->mdnsDaemon);
    mdnsd_free(server->mdnsDaemon);
}

static void
UA_Discovery_multicastConflict(char *name, int type, void *arg) {
    // cppcheck-suppress unreadVariable
    UA_Server *server = (UA_Server*) arg;
    UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS name conflict detected: '%s' for type %d", name, type);
}

static char*
create_fullServiceDomain(const char* servername, const char* hostname, size_t maxLen) {
    size_t hostnameLen = strlen(hostname);
    size_t servernameLen = strlen(servername);
    // [servername]-[hostname]._opcua-tcp._tcp.local.

    if(hostnameLen+servernameLen+1 > maxLen) {
        if (servernameLen+2 > maxLen) {
            servernameLen = maxLen;
            hostnameLen = 0;
        } else {
            hostnameLen = maxLen - servernameLen - 1;
        }
    }

    char *fullServiceDomain = (char*)UA_malloc(servernameLen + 1 + hostnameLen + 23 + 2);
    if (!fullServiceDomain)
        return NULL;

    if (hostnameLen > 0)
        sprintf(fullServiceDomain, "%.*s-%.*s._opcua-tcp._tcp.local.",
                (int)servernameLen, servername, (int)hostnameLen, hostname);
    else
        sprintf(fullServiceDomain, "%.*s._opcua-tcp._tcp.local.",
                (int)servernameLen, servername);
    return fullServiceDomain;
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
UA_Discovery_recordExists(UA_Server* server, const char* fullServiceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(server->mdnsDaemon, fullServiceDomain);
    while (r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if (data->type == QTYPE_SRV && (port == 0 || data->srv.port == port))
            return UA_TRUE;
        r = mdnsd_record_next(r);
    }
    return UA_FALSE;
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
    if(exists == UA_TRUE)
        return 0;

    if(mdnsd_has_query(server->mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(server->mdnsDaemon, a->rdname, QTYPE_SRV,
                discovery_multicastQueryAnswer, server);
    mdnsd_query(server->mdnsDaemon, a->rdname, QTYPE_TXT,
                discovery_multicastQueryAnswer, server);
    return 0;
}

/* Send a multicast probe to find any other OPC UA server on the network through mDNS. */
UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server) {
    mdnsd_query(server->mdnsDaemon, "_opcua-tcp._tcp.local.",
                QTYPE_PTR,discovery_multicastQueryAnswer, server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_addRecord(UA_Server* server, const char* servername,
                       const char* hostname, unsigned short port,
                       const char* path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t *capabilitiesSize) {
    if(!capabilitiesSize || (*capabilitiesSize > 0 && !capabilites))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    size_t hostnameLen = strlen(hostname);
    size_t servernameLen = strlen(servername);
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    // use a limit for the hostname length to make sure full string fits into 63
    // chars (limited by DNS spec)
    if(hostnameLen+servernameLen + 1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Combination of hostname+servername exceeds maximum "
                       "of 62 chars. It will be truncated.");
    } else if(hostnameLen > 63) {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Hostname length exceeds maximum of 63 chars. "
                       "It will be truncated.");
    }

    if(!server->mdnsMainSrvAdded) {
        mdns_record_t *r = mdnsd_shared(server->mdnsDaemon, "_services._dns-sd._udp.local.",
                                        QTYPE_PTR, 600);
        mdnsd_set_host(server->mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        server->mdnsMainSrvAdded = UA_TRUE;
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char *fullServiceDomain = create_fullServiceDomain(servername, hostname, 63);
    if(!fullServiceDomain)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_Boolean exists = UA_Discovery_recordExists(server, fullServiceDomain, port, protocol);
    if(exists == UA_TRUE) {
        free(fullServiceDomain);
        return UA_STATUSCODE_GOOD;
    }

    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: add record for domain: %s", fullServiceDomain);

    // _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local

    // check if there is already a PTR entry for the given service.

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    mdns_record_t *r = mdns_find_record(server->mdnsDaemon, QTYPE_PTR,
                                        "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        r = mdnsd_shared(server->mdnsDaemon, "_opcua-tcp._tcp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(server->mdnsDaemon, r, fullServiceDomain);
    }

    // hostname.
    size_t maxHostnameLen = hostnameLen < 63 ? hostnameLen : 63;
    char *localDomain = (char*)UA_malloc(maxHostnameLen+2);
    if(!localDomain) {
        free(fullServiceDomain);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sprintf(localDomain, "%.*s.",(int)(maxHostnameLen), hostname);

    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    r = mdnsd_unique(server->mdnsDaemon, fullServiceDomain, QTYPE_SRV, 600,
                     UA_Discovery_multicastConflict, server);
    // r = mdnsd_shared(server->mdnsDaemon, fullServiceDomain, QTYPE_SRV, 600);
    mdnsd_set_srv(server->mdnsDaemon, r, 0, 0, port, localDomain);

    // A/AAAA record for all ip addresses.
    // [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
    // [hostname]. A [ip].
    mdns_set_address_record(server, fullServiceDomain, localDomain);

    // TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    if(createTxt) {
        mdns_create_txt(server, fullServiceDomain, path, capabilites, capabilitiesSize,
                        UA_Discovery_multicastConflict);
    }

    free(fullServiceDomain);
    free(localDomain);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_removeRecord(UA_Server* server, const char* servername, const char* hostname,
                          unsigned short port, UA_Boolean removeTxt) {
    size_t hostnameLen = strlen(hostname);
    size_t servernameLen = strlen(servername);
    // use a limit for the hostname length to make sure full string fits into 63
    // chars (limited by DNS spec)
    if(hostnameLen == 0 || servernameLen == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(hostnameLen+servernameLen+1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char *fullServiceDomain = create_fullServiceDomain(servername, hostname, 63);
    if(!fullServiceDomain)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: remove record for domain: %s", fullServiceDomain);

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    mdns_record_t *r = mdns_find_record(server->mdnsDaemon, QTYPE_PTR,
                                        "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: could not remove record. "
                       "PTR Record not found for domain: %s", fullServiceDomain);
        free(fullServiceDomain);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    mdnsd_done(server->mdnsDaemon, r);

    // looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port hostname.local.
    // and TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    // and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]
    mdns_record_t *r2 = mdnsd_get_published(server->mdnsDaemon, fullServiceDomain);
    if(!r2) {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: could not remove record. Record not "
                       "found for domain: %s", fullServiceDomain);
        free(fullServiceDomain);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if((removeTxt && data->type == QTYPE_TXT) ||
           (removeTxt && data->type == QTYPE_A) ||
           data->srv.port == port) {
            mdnsd_done(server->mdnsDaemon, r2);
        }
        r2 = next;
    }

    free(fullServiceDomain);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_multicastIterate(UA_Server* server, UA_DateTime *nextRepeat,
                              UA_Boolean processIn) {
    struct timeval next_sleep = { 0, 0 };
    unsigned short retVal = mdnsd_step(server->mdnsDaemon, server->mdnsSocket,
                                       processIn, true, &next_sleep);
    if(retVal == 1) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not read from socket. %s",
                     strerror(errno));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    } else if(retVal == 2) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not write to socket. %s",
                     strerror(errno));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    }

    if(nextRepeat)
        *nextRepeat = UA_DateTime_now() +
            (UA_DateTime)(next_sleep.tv_sec * UA_SEC_TO_DATETIME +
                          next_sleep.tv_usec * UA_USEC_TO_DATETIME);
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_MULTITHREADING

static void *
multicastWorkerLoop(UA_Server *server) {
    struct timeval next_sleep = {.tv_sec = 0, .tv_usec = 0};
    volatile UA_Boolean *running = &server->mdnsRunning;
    fd_set fds;

    while(*running) {
        FD_ZERO(&fds);
        FD_SET(server->mdnsSocket, &fds);
        select(server->mdnsSocket + 1, &fds, 0, 0, &next_sleep);

        if(!*running)
            break;

        unsigned short retVal =
            mdnsd_step(server->mdnsDaemon, server->mdnsSocket,
                       FD_ISSET(server->mdnsSocket, &fds), true, &next_sleep);
        if (retVal == 1) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Multicast error: Can not read from socket. %s", strerror(errno));
            break;
        } else if (retVal == 2) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Multicast error: Can not write to socket. %s", strerror(errno));
            break;
        }
    }
    return NULL;
}

UA_StatusCode
UA_Discovery_multicastListenStart(UA_Server* server) {
    int err = pthread_create(&server->mdnsThread, NULL,
                             (void* (*)(void*))multicastWorkerLoop, server);
    if(err != 0) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not create multicast thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_multicastListenStop(UA_Server* server) {
    mdnsd_shutdown(server->mdnsDaemon);
    // wake up select
    write(server->mdnsSocket, "\0", 1);
    if(pthread_join(server->mdnsThread, NULL)) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not stop thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

# endif /* UA_ENABLE_MULTITHREADING */

#endif /* defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST) */
