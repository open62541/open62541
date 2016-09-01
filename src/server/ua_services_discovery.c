#include "ua_server_internal.h"
#include "ua_services.h"

#ifdef UA_ENABLE_DISCOVERY
# ifdef _MSC_VER
#  include <io.h> //access
#  define access _access
# else
#  include <unistd.h> //access
# endif
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
#  include <fcntl.h>
#  include <errno.h>
#  include <stdio.h>
#  ifndef UA_ENABLE_AMALGAMATION
#   include "mdnsd/libmdnsd/xht.h"
#   include "mdnsd/libmdnsd/sdtxt.h"
#  endif
#  ifdef _WIN32
#   define _WINSOCK_DEPRECATED_NO_WARNINGS /* inet_ntoa is deprecated on MSVC but used for compatibility */
#   include <winsock2.h>
#   include <iphlpapi.h>
#   include <ws2tcpip.h>
#   define CLOSESOCKET(S) closesocket((SOCKET)S)
#  else
#   define CLOSESOCKET(S) close(S)
#   include <sys/time.h> // for struct timeval
#   include <netinet/in.h> // for struct ip_mreq
#   include <ifaddrs.h>
#   include <net/if.h> /* for IFF_RUNNING */
#  endif
# endif
#endif

#ifndef STRDUP
# if defined(__MINGW32__)
static char *ua_strdup(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if(p) { strcpy(p, s); }
    return p;
}
# define STRDUP ua_strdup
# elif defined(_WIN32)
# define STRDUP _strdup
# else
# define STRDUP strdup
# endif
#endif

#ifdef UA_ENABLE_DISCOVERY
static UA_StatusCode
copyRegisteredServerToApplicationDescription(const UA_FindServersRequest *request, UA_ApplicationDescription *target,
                                             const UA_RegisteredServer* registeredServer) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ApplicationDescription_init(target);

    retval |= UA_String_copy(&registeredServer->serverUri, &target->applicationUri);
    retval |= UA_String_copy(&registeredServer->productUri, &target->productUri);

    // if the client requests a specific locale, select the corresponding server name
    if (request->localeIdsSize) {
        UA_Boolean appNameFound = UA_FALSE;
        for (size_t i =0; i<request->localeIdsSize && !appNameFound; i++) {
            for (size_t j =0; j<registeredServer->serverNamesSize; j++) {
                if (UA_String_equal(&request->localeIds[i], &registeredServer->serverNames[j].locale)) {
                    retval |= UA_LocalizedText_copy(&registeredServer->serverNames[j], &target->applicationName);
                    appNameFound = UA_TRUE;
                    break;
                }
            }
        }
        if (!appNameFound && registeredServer->serverNamesSize) {
            // server does not have the requested local, therefore we can select the most suitable one
            retval |= UA_LocalizedText_copy(&registeredServer->serverNames[0], &target->applicationName);
        }
    } else if (registeredServer->serverNamesSize){
        // just take the first name
        retval |= UA_LocalizedText_copy(&registeredServer->serverNames[0], &target->applicationName);
    }

    target->applicationType = registeredServer->serverType;
    retval |= UA_String_copy(&registeredServer->gatewayServerUri, &target->gatewayServerUri);
    // TODO where do we get the discoveryProfileUri for application data?

    target->discoveryUrlsSize = registeredServer->discoveryUrlsSize;
    if (registeredServer->discoveryUrlsSize) {
        target->discoveryUrls = UA_malloc(sizeof(UA_String) * registeredServer->discoveryUrlsSize);
        if (!target->discoveryUrls) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for (size_t i = 0; i<registeredServer->discoveryUrlsSize; i++) {
            retval |= UA_String_copy(&registeredServer->discoveryUrls[i], &target->discoveryUrls[i]);
        }
    }

    return retval;
}
#endif

void Service_FindServers(UA_Server *server, UA_Session *session,
                         const UA_FindServersRequest *request,
                         UA_FindServersResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing FindServersRequest");

    size_t foundServersSize = 0;
    UA_ApplicationDescription *foundServers = NULL;

    UA_Boolean addSelf = UA_FALSE;
    // temporarily store all the pointers which we found to avoid reiterating through the list
    UA_RegisteredServer **foundServerFilteredPointer = NULL;

#ifdef UA_ENABLE_DISCOVERY
    // check if client only requested a specific set of servers
    if (request->serverUrisSize) {

        foundServerFilteredPointer = UA_malloc(sizeof(UA_RegisteredServer*) * server->registeredServersSize);
        if(!foundServerFilteredPointer) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        for (size_t i=0; i<request->serverUrisSize; i++) {
            if (!addSelf && UA_String_equal(&request->serverUris[i], &server->config.applicationDescription.applicationUri)) {
                addSelf = UA_TRUE;
            } else {
                registeredServer_list_entry* current;
                LIST_FOREACH(current, &server->registeredServers, pointers) {
                    if (UA_String_equal(&current->registeredServer.serverUri, &request->serverUris[i])) {
                        foundServerFilteredPointer[foundServersSize++] = &current->registeredServer;
                        break;
                    }
                }
            }
        }

        if (addSelf)
            foundServersSize++;

    } else {
        addSelf = true;

        // self + registered servers
        foundServersSize = 1 + server->registeredServersSize;
    }
#else
    if (request->serverUrisSize) {
        for (size_t i=0; i<request->serverUrisSize; i++) {
            if (UA_String_equal(&request->serverUris[i], &server->config.applicationDescription.applicationUri)) {
                addSelf = UA_TRUE;
                foundServersSize = 1;
                break;
            }
        }
    } else {
        addSelf = UA_TRUE;
        foundServersSize = 1;
    }
#endif

    if (foundServersSize) {
        foundServers = UA_malloc(sizeof(UA_ApplicationDescription) * foundServersSize);
        if (!foundServers) {
            if (foundServerFilteredPointer)
                UA_free(foundServerFilteredPointer);
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        if (addSelf) {
            /* copy ApplicationDescription from the config */

            response->responseHeader.serviceResult |= UA_ApplicationDescription_copy(&server->config.applicationDescription,
                                                                                     &foundServers[0]);
            if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                UA_free(foundServers);
                if (foundServerFilteredPointer)
                    UA_free(foundServerFilteredPointer);
                return;
            }

            /* add the discoveryUrls from the networklayers */
            UA_String* disc = UA_realloc(foundServers[0].discoveryUrls, sizeof(UA_String) *
                                                                                   (foundServers[0].discoveryUrlsSize +
                                                                                    server->config.networkLayersSize));
            if (!disc) {
                response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
                UA_free(foundServers);
                if (foundServerFilteredPointer)
                    UA_free(foundServerFilteredPointer);
                return;
            }
            size_t existing = foundServers[0].discoveryUrlsSize;
            foundServers[0].discoveryUrls = disc;
            foundServers[0].discoveryUrlsSize += server->config.networkLayersSize;

            // TODO: Add nl only if discoveryUrl not already present
            for (size_t i = 0; i < server->config.networkLayersSize; i++) {
                UA_ServerNetworkLayer* nl = &server->config.networkLayers[i];
                UA_String_copy(&nl->discoveryUrl, &foundServers[0].discoveryUrls[existing + i]);
            }
        }
#ifdef UA_ENABLE_DISCOVERY

        size_t currentIndex = 0;
        if (addSelf)
            currentIndex++;

        // add all the registered servers to the list

        if (foundServerFilteredPointer) {
            // use filtered list because client only requested specific uris
            // -1 because foundServersSize also includes this self server
            size_t iterCount = addSelf ? foundServersSize - 1 : foundServersSize;
            for (size_t i = 0; i < iterCount; i++) {
                response->responseHeader.serviceResult =
                    copyRegisteredServerToApplicationDescription(request, &foundServers[currentIndex++],
                                                                 foundServerFilteredPointer[i]);
                if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                    UA_free(foundServers);
                    UA_free(foundServerFilteredPointer);
                    return;
                }
            }
            UA_free(foundServerFilteredPointer);
            foundServerFilteredPointer = NULL;
        } else {
            registeredServer_list_entry* current;
            LIST_FOREACH(current, &server->registeredServers, pointers) {
                response->responseHeader.serviceResult =
                    copyRegisteredServerToApplicationDescription(request, &foundServers[currentIndex++],
                                                                 &current->registeredServer);
                if (response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
                    UA_free(foundServers);
                    return;
                }
            }
        }
#endif
    }

    if (foundServerFilteredPointer)
        UA_free(foundServerFilteredPointer);

    response->servers = foundServers;
    response->serversSize = foundServersSize;
}

void Service_GetEndpoints(UA_Server *server, UA_Session *session, const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse *response) {
    /* If the client expects to see a specific endpointurl, mirror it back. If
       not, clone the endpoints with the discovery url of all networklayers. */
    const UA_String *endpointUrl = &request->endpointUrl;
    if(endpointUrl->length > 0) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with endpointUrl " \
                             UA_PRINTF_STRING_FORMAT, UA_PRINTF_STRING_DATA(*endpointUrl));
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with an empty endpointUrl");
    }

    /* test if the supported binary profile shall be returned */
#ifdef NO_ALLOCA
    UA_Boolean relevant_endpoints[server->endpointDescriptionsSize];
#else
    UA_Boolean *relevant_endpoints = UA_alloca(sizeof(UA_Boolean) * server->endpointDescriptionsSize);
#endif
    memset(relevant_endpoints, 0, sizeof(UA_Boolean) * server->endpointDescriptionsSize);
    size_t relevant_count = 0;
    if(request->profileUrisSize == 0) {
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++)
            relevant_endpoints[j] = true;
        relevant_count = server->endpointDescriptionsSize;
    } else {
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++) {
            for(size_t i = 0; i < request->profileUrisSize; i++) {
                if(!UA_String_equal(&request->profileUris[i], &server->endpointDescriptions[j].transportProfileUri))
                    continue;
                relevant_endpoints[j] = true;
                relevant_count++;
                break;
            }
        }
    }

    if(relevant_count == 0) {
        response->endpointsSize = 0;
        return;
    }

    /* Clone the endpoint for each networklayer? */
    size_t clone_times = 1;
    UA_Boolean nl_endpointurl = false;
    if(endpointUrl->length == 0) {
        clone_times = server->config.networkLayersSize;
        nl_endpointurl = true;
    }

    response->endpoints = UA_Array_new(relevant_count * clone_times, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->endpointsSize = relevant_count * clone_times;

    size_t k = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < clone_times; i++) {
        if(nl_endpointurl)
            endpointUrl = &server->config.networkLayers[i].discoveryUrl;
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++) {
            if(!relevant_endpoints[j])
                continue;
            retval |= UA_EndpointDescription_copy(&server->endpointDescriptions[j], &response->endpoints[k]);
            retval |= UA_String_copy(endpointUrl, &response->endpoints[k].endpointUrl);
            k++;
        }
    }

    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        UA_Array_delete(response->endpoints, response->endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        response->endpoints = NULL;
        response->endpointsSize = 0;
        return;
    }
}

#ifdef UA_ENABLE_DISCOVERY

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

/* Generates a hash code for a string.
 * This function uses the ELF hashing algorithm as reprinted in
 * Andrew Binstock, "Hashing Rehashed," Dr. Dobb's Journal, April 1996.
 */
static int mdns_hash_record(const char *s)
{
    /* ELF hash uses unsigned chars and unsigned arithmetic for portability */
    const unsigned char *name = (const unsigned char *)s;
    unsigned long h = 0;

    while(*name) {
        /* do some fancy bitwanking on the string */
        h = (h << 4) + (unsigned long)(*name++);
        unsigned long g;
        if ((g = (h & 0xF0000000UL)) != 0)
            h ^= (g >> 24);
        h &= ~g;
    }

    return (int)h;
}

static struct serverOnNetwork_list_entry*
mdns_record_add_or_get(UA_Server *server, const char* record, const char* serverName,
                       size_t serverNameLen, UA_Boolean createNew) {
    int hashIdx = mdns_hash_record(record) % SERVER_ON_NETWORK_HASH_PRIME;
    struct serverOnNetwork_hash_entry* hash_entry = server->serverOnNetworkHash[hashIdx];

    while (hash_entry) {
        size_t maxLen = serverNameLen > hash_entry->entry->serverOnNetwork.serverName.length ? hash_entry->entry->serverOnNetwork.serverName.length : serverNameLen;
        if (strncmp((char*)hash_entry->entry->serverOnNetwork.serverName.data, serverName,maxLen)==0) {
            return hash_entry->entry;
        }
        hash_entry = hash_entry->next;
    }

    if (!createNew)
        return NULL;

    // not yet in list, create new one
    struct serverOnNetwork_list_entry* listEntry = malloc(sizeof(struct serverOnNetwork_list_entry));
    listEntry->created = UA_DateTime_now();
    listEntry->pathTmp = NULL;
    listEntry->txtSet = UA_FALSE;
    listEntry->srvSet = UA_FALSE;
    UA_ServerOnNetwork_init(&listEntry->serverOnNetwork);
    listEntry->serverOnNetwork.recordId = server->serverOnNetworkRecordIdCounter;
    listEntry->serverOnNetwork.serverName.length = serverNameLen;
    listEntry->serverOnNetwork.serverName.data = malloc(serverNameLen);
    memcpy(listEntry->serverOnNetwork.serverName.data, serverName, serverNameLen);
    #  ifndef UA_ENABLE_MULTITHREADING
    server->serverOnNetworkRecordIdCounter++;
    #  else
    server->serverOnNetworkRecordIdCounter = uatomic_add_return(&server->serverOnNetworkRecordIdCounter, 1);
    #  endif
    if (server->serverOnNetworkRecordIdCounter == 0)
        server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();

    // add to hash
    struct serverOnNetwork_hash_entry* newHashEntry = malloc(sizeof(struct serverOnNetwork_hash_entry));
    newHashEntry->next = server->serverOnNetworkHash[hashIdx];
    server->serverOnNetworkHash[hashIdx] = newHashEntry;
    newHashEntry->entry = listEntry;

    LIST_INSERT_HEAD(&server->serverOnNetwork, listEntry, pointers);

    return listEntry;
}

static void mdns_record_remove(UA_Server *server, const char* record,
                               struct serverOnNetwork_list_entry* entry) {

    // remove from hash

    int hashIdx = mdns_hash_record(record) % SERVER_ON_NETWORK_HASH_PRIME;
    struct serverOnNetwork_hash_entry* hash_entry = server->serverOnNetworkHash[hashIdx];
    struct serverOnNetwork_hash_entry* prevEntry = hash_entry;
    while (hash_entry) {
        if (hash_entry->entry == entry) {
            if (server->serverOnNetworkHash[hashIdx] == hash_entry)
                server->serverOnNetworkHash[hashIdx] = hash_entry->next;
            else if (prevEntry){
                prevEntry->next = hash_entry->next;
            }
            break;
        }
        prevEntry = hash_entry;
        hash_entry = hash_entry->next;
    }
    free(hash_entry);

    if (server->serverOnNetworkCallback) {
        server->serverOnNetworkCallback(&entry->serverOnNetwork, UA_FALSE, entry->txtSet, server->serverOnNetworkCallbackData);
    }

    // remove from list

    LIST_REMOVE(entry, pointers);
    UA_ServerOnNetwork_deleteMembers(&entry->serverOnNetwork);
    if (entry->pathTmp)
        free(entry->pathTmp);

#ifndef UA_ENABLE_MULTITHREADING
    UA_free(entry);
    server->serverOnNetworkSize--;
#else
    server->serverOnNetworkSize = uatomic_add_return(&server->serverOnNetworkSize, -1);
    UA_Server_delayedFree(server, entry);
#endif
}

static void mdns_append_path_to_url(UA_String* url, const char* path) {
    size_t pathLen = strlen(path);
    char *newUrl = malloc(url->length + pathLen);
    memcpy(newUrl,url->data, url->length);
    memcpy(newUrl+url->length,path,pathLen);
    url->length = url->length + pathLen;
    url->data = (UA_Byte*)newUrl;
}

/**
 * This will be called by the mDNS library on every record which is received.
 * @param r
 * @param data
 */
static void mdns_record_received(const struct resource* r, void* data) {
    UA_Server *server = (UA_Server *) data;
    // we only need SRV and TXT records
    if ((r->class != QCLASS_IN && r->class != QCLASS_IN + 32768) || (r->type != QTYPE_SRV && r->type != QTYPE_TXT))
        return;

    // we only handle '_opcua-tcp._tcp.' records
    char *opcStr = strstr(r->name, "_opcua-tcp._tcp.");
    if (opcStr == NULL)
        return;

    size_t servernameLen = (size_t)(opcStr - r->name);
    if (servernameLen == 0)
        return;
    servernameLen--; // remove point

    // opcStr + strlen("_opcua-tcp._tcp.")
    //char *hostname = opcStr + 16;

    struct serverOnNetwork_list_entry* entry = mdns_record_add_or_get(server, r->name, r->name, servernameLen, r->ttl > 0);

    if (entry == NULL)
        // TTL is 0 and entry not yet in list
        return;
    else if (r->ttl == 0) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Multicast DNS: remove server (TTL=0): %.*s",
                    entry->serverOnNetwork.discoveryUrl.length,
                    entry->serverOnNetwork.discoveryUrl.data);
        mdns_record_remove(server, r->name, entry);
        return;
    }

    entry->lastSeen = UA_DateTime_nowMonotonic();

    if (entry->txtSet && entry->srvSet)
        return;

    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    // TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    if (r->type == QTYPE_TXT && !entry->txtSet) {
        entry->txtSet = UA_TRUE;


        xht_t* x = txt2sd((unsigned char*)r->rdata, r->rdlength);
        char* path = (char*)xht_get(x, "path");
        char* caps = (char*)xht_get(x, "caps");

        if (path && strlen(path) > 1) {
            if (!entry->srvSet) {
                // txt arrived before SRV, thus cache path entry
                entry->pathTmp = STRDUP(path);
            } else {
                // SRV already there and discovery URL set. Add path to discovery URL
                mdns_append_path_to_url(&entry->serverOnNetwork.discoveryUrl, path);
            }
        }

        if (caps && strlen(caps) > 0) {
            size_t capsCount = 1;
            // count comma in caps
            for (size_t i=0; caps[i]; i++) {
                if (caps[i]==',')
                    capsCount++;
            }
            // set capabilities
            entry->serverOnNetwork.serverCapabilitiesSize = capsCount;
            entry->serverOnNetwork.serverCapabilities = UA_Array_new(capsCount, &UA_TYPES[UA_TYPES_STRING]);
            for (size_t i=0; i<capsCount; i++) {

                char* nextStr = strchr(caps, ',');

                size_t len = nextStr ? (size_t)(nextStr - caps) : strlen(caps);
                entry->serverOnNetwork.serverCapabilities[i].length = len;
                entry->serverOnNetwork.serverCapabilities[i].data = malloc(len);
                memcpy(entry->serverOnNetwork.serverCapabilities[i].data, caps, len);
                if (nextStr)
                    caps = nextStr+1;
                else
                    break;
            }
        }
        xht_free(x);
    } else if (r->type == QTYPE_SRV && !entry->srvSet){
        entry->srvSet = UA_TRUE;

        // opc.tcp://[servername]:[port][path]
        size_t srvNameLen = strlen(r->known.srv.name);
        if (srvNameLen > 0 && r->known.srv.name[srvNameLen-1] == '.')
            srvNameLen--;
        char *newUrl = malloc(10 + srvNameLen + 8);
        sprintf(newUrl, "opc.tcp://%.*s:%d",(int)srvNameLen, r->known.srv.name, r->known.srv.port);
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: found server: %s", newUrl);
        entry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
        free(newUrl);

        if (entry->pathTmp) {
            mdns_append_path_to_url(&entry->serverOnNetwork.discoveryUrl, entry->pathTmp);
            free(entry->pathTmp);
        }

    }

    if (entry->srvSet && server->serverOnNetworkCallback) {
        server->serverOnNetworkCallback(&entry->serverOnNetwork, UA_TRUE, entry->txtSet, server->serverOnNetworkCallbackData);
    }
}

void Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                                  const UA_FindServersOnNetworkRequest *request,
                                  UA_FindServersOnNetworkResponse *response) {
    UA_UInt32 recordCount;
    if (request->startingRecordId < server->serverOnNetworkRecordIdCounter)
        recordCount = server->serverOnNetworkRecordIdCounter - request->maxRecordsToReturn;
    else
        recordCount = 0;

    if (request->maxRecordsToReturn && recordCount > request->maxRecordsToReturn) {
        recordCount = request->maxRecordsToReturn;
    }

    UA_ServerOnNetwork** filtered = NULL;
    if (recordCount > 0) {
        filtered = UA_malloc(sizeof(UA_ServerOnNetwork*) * recordCount);
        if (!filtered) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        memset(filtered, 0, sizeof(UA_ServerOnNetwork*) * recordCount);

        if (request->serverCapabilityFilterSize) {
            UA_UInt32 filteredCount = 0;
            // iterate over all records and add to filtered list
            serverOnNetwork_list_entry* current;
            LIST_FOREACH(current, &server->serverOnNetwork, pointers) {
                if (current->serverOnNetwork.recordId < request->startingRecordId)
                    continue;
                UA_Boolean foundAll = UA_TRUE;
                for (size_t i = 0; i < request->serverCapabilityFilterSize && foundAll; i++) {
                    UA_Boolean foundSingle = UA_FALSE;
                    for (size_t j = 0; j < current->serverOnNetwork.serverCapabilitiesSize && !foundSingle; j++) {
                        foundSingle |= UA_String_equal(&request->serverCapabilityFilter[i],
                                                       &current->serverOnNetwork.serverCapabilities[j]);
                    }
                    foundAll &= foundSingle;
                }
                if (foundAll) {
                    filtered[filteredCount++] = &current->serverOnNetwork;
                    if (filteredCount >= recordCount)
                        break;
                }
            }
            recordCount = filteredCount;
        } else {
            UA_UInt32 filteredCount = 0;
            serverOnNetwork_list_entry* current;
            LIST_FOREACH(current, &server->serverOnNetwork, pointers) {
                if (current->serverOnNetwork.recordId < request->startingRecordId)
                    continue;
                filtered[filteredCount++] = &current->serverOnNetwork;
                if (filteredCount >= recordCount)
                    break;
            }
            recordCount = filteredCount;
        }
    }
    response->serversSize = recordCount;
    if (recordCount > 0) {
        response->servers = UA_malloc(sizeof(UA_ServerOnNetwork)*recordCount);
        if (!response->servers) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            free(filtered);
            return;
        }
        for (size_t i=0; i<recordCount; i++) {
            UA_ServerOnNetwork_copy(filtered[i],&response->servers[recordCount - i -1]);
        }
    }
    free(filtered);

    UA_DateTime_copy(&server->serverOnNetworkRecordIdLastReset, &response->lastCounterResetTime);
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
}
# endif

static void
process_RegisterServer(UA_Server *server, UA_Session *session, const UA_RequestHeader* requestHeader,
                       const UA_RegisteredServer *requestServer, const size_t requestDiscoveryConfigurationSize,
                       const UA_ExtensionObject *requestDiscoveryConfiguration, UA_ResponseHeader* responseHeader,
                       size_t *responseConfigurationResultsSize, UA_StatusCode **responseConfigurationResults,
                       size_t *responseDiagnosticInfosSize, UA_DiagnosticInfo *responseDiagnosticInfos) {
    registeredServer_list_entry *registeredServer_entry = NULL;

    // find the server from the request in the registered list
    registeredServer_list_entry* current;
    LIST_FOREACH(current, &server->registeredServers, pointers) {
        if (UA_String_equal(&current->registeredServer.serverUri, &requestServer->serverUri)) {
            registeredServer_entry = current;
            break;
        }
    }

    UA_MdnsDiscoveryConfiguration *mdnsConfig = NULL;

    const UA_String* mdnsServerName = NULL;
    if (requestDiscoveryConfigurationSize) {
        *responseConfigurationResultsSize = requestDiscoveryConfigurationSize;
        *responseConfigurationResults = UA_Array_new(requestDiscoveryConfigurationSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
        for (size_t i =0; i<requestDiscoveryConfigurationSize; i++) {
            const UA_ExtensionObject *object = &requestDiscoveryConfiguration[i];
            if (!mdnsConfig && (object->encoding == UA_EXTENSIONOBJECT_DECODED || object->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE)
                && (object->content.decoded.type == &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION])) {
                // yayy, we have a known extension object type
                mdnsConfig = object->content.decoded.data;

                mdnsServerName = &mdnsConfig->mdnsServerName;

                *responseConfigurationResults[i] = UA_STATUSCODE_GOOD;
            } else {
                *responseConfigurationResults[i] = UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }
    }
    if (!mdnsServerName && requestServer->serverNamesSize) {
        mdnsServerName = &requestServer->serverNames[0].text;
    }

    if (!mdnsServerName) {
        responseHeader->serviceResult = UA_STATUSCODE_BADSERVERNAMEMISSING;
        return;
    }


    if (requestServer->discoveryUrlsSize == 0) {
        responseHeader->serviceResult = UA_STATUSCODE_BADDISCOVERYURLMISSING;
        return;
    }

    if (requestServer->semaphoreFilePath.length) {
        char* filePath = malloc(sizeof(char)*requestServer->semaphoreFilePath.length+1);
        memcpy( filePath, requestServer->semaphoreFilePath.data, requestServer->semaphoreFilePath.length );
        filePath[requestServer->semaphoreFilePath.length] = '\0';
        if (access( filePath, 0 ) == -1) {
            responseHeader->serviceResult = UA_STATUSCODE_BADSEMPAHOREFILEMISSING;
            free(filePath);
            return;
        }
        free(filePath);
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if (server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {

        char* mdnsServer = malloc(sizeof(char) * mdnsServerName->length + 1);
        memcpy(mdnsServer, mdnsServerName->data, mdnsServerName->length);
        mdnsServer[mdnsServerName->length] = '\0';

        for (size_t i=0; i<requestServer->discoveryUrlsSize; i++) {
            UA_UInt16 port = 0;
            char hostname[256]; hostname[0] = '\0';
            const char *path = NULL;
            {
                char* uri = malloc(sizeof(char) * requestServer->discoveryUrls[i].length + 1);
                strncpy(uri, (char*) requestServer->discoveryUrls[i].data, requestServer->discoveryUrls[i].length);
                uri[requestServer->discoveryUrls[i].length] = '\0';
                UA_StatusCode retval;
                if ((retval = UA_EndpointUrl_split(uri, hostname, &port, &path)) != UA_STATUSCODE_GOOD) {
                    hostname[0] = '\0';
                    if (retval == UA_STATUSCODE_BADOUTOFRANGE)
                        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK, "Server url size invalid");
                    else if (retval == UA_STATUSCODE_BADATTRIBUTEIDINVALID)
                        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK, "Server url does not begin with opc.tcp://");
                }
                free(uri);
            }

            if (!requestServer->isOnline) {
                if (UA_Discovery_removeRecord(server, mdnsServer, hostname, (unsigned short) port, i==requestServer->discoveryUrlsSize) != UA_STATUSCODE_GOOD) {
                    UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Could not remove mDNS record for hostname %s.%s", mdnsServer);
                }
            }
            else {
                UA_String *capabilities = NULL;
                size_t capabilitiesSize = 0;
                if (mdnsConfig) {
                    capabilities = mdnsConfig->serverCapabilities;
                    capabilitiesSize = mdnsConfig->serverCapabilitiesSize;
                }
                if (UA_Discovery_addRecord(server, mdnsServer, hostname, (unsigned short) port, path, UA_DISCOVERY_TCP, i==0, capabilities, &capabilitiesSize) != UA_STATUSCODE_GOOD) {
                    UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Could not add mDNS record for hostname %s.%s", mdnsServer);
                }
            }
        }
        free(mdnsServer);
    }
#endif


    if (!requestServer->isOnline) {
        // server is shutting down. Remove it from the registered servers list
        if (!registeredServer_entry) {
            // server not found, show warning
            UA_LOG_WARNING_SESSION(server->config.logger, session,
                                   "Could not unregister server %.*s. Not registered.",
                                   (int)requestServer->serverUri.length, requestServer->serverUri.data);
            responseHeader->serviceResult = UA_STATUSCODE_BADNOTFOUND;
            return;
        }

        if (server->registerServerCallback) {
            server->registerServerCallback(&registeredServer_entry->registeredServer, server->registerServerCallbackData);
        }

        // server found, remove from list
        LIST_REMOVE(registeredServer_entry, pointers);
        UA_RegisteredServer_deleteMembers(&registeredServer_entry->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
        UA_free(registeredServer_entry);
        server->registeredServersSize--;
#else
        server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, -1);
        UA_Server_delayedFree(server, registeredServer_entry);
#endif
        responseHeader->serviceResult = UA_STATUSCODE_GOOD;
        return;
    }


    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if (!registeredServer_entry) {
        // server not yet registered, register it by adding it to the list


        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Registering new server: %.*s",
                             (int)requestServer->serverUri.length, requestServer->serverUri.data);

        registeredServer_entry = UA_malloc(sizeof(registeredServer_list_entry));
        if(!registeredServer_entry) {
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        LIST_INSERT_HEAD(&server->registeredServers, registeredServer_entry, pointers);
#ifndef UA_ENABLE_MULTITHREADING
        server->registeredServersSize++;
#else
        server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, 1);
#endif

        if (server->registerServerCallback) {
            server->registerServerCallback(&registeredServer_entry->registeredServer, server->registerServerCallbackData);
        }

    } else {
        UA_RegisteredServer_deleteMembers(&registeredServer_entry->registeredServer);
    }

    // copy the data from the request into the list
    UA_RegisteredServer_copy(requestServer, &registeredServer_entry->registeredServer);
    registeredServer_entry->lastSeen = UA_DateTime_nowMonotonic();

    responseHeader->serviceResult = retval;
}

void Service_RegisterServer(UA_Server *server, UA_Session *session,
                            const UA_RegisterServerRequest *request,
                            UA_RegisterServerResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RegisterServerRequest");
    process_RegisterServer(server, session, &request->requestHeader, &request->server, 0,
                           NULL, &response->responseHeader, 0, NULL, 0, NULL);
}

void Service_RegisterServer2(UA_Server *server, UA_Session *session,
                            const UA_RegisterServer2Request *request,
                             UA_RegisterServer2Response *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RegisterServer2Request");
    process_RegisterServer(server, session, &request->requestHeader, &request->server,
                           request->discoveryConfigurationSize, request->discoveryConfiguration,
                           &response->responseHeader, &response->configurationResultsSize,
                           &response->configurationResults, &response->diagnosticInfosSize,
                           response->diagnosticInfos);
}

/**
 * Cleanup server registration:
 * If the semaphore file path is set, then it just checks the existence of the file.
 * When it is deleted, the registration is removed.
 * If there is no semaphore file, then the registration will be removed if it is older than 60 minutes.
 */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic) {
    UA_DateTime timedOut = nowMonotonic;
    // registration is timed out if lastSeen is older than 60 minutes (default value, can be modified by user).
    if (server->config.discoveryCleanupTimeout) {
        timedOut -= server->config.discoveryCleanupTimeout*UA_SEC_TO_DATETIME;
    }

    registeredServer_list_entry* current, *temp;
    LIST_FOREACH_SAFE(current, &server->registeredServers, pointers, temp) {

        UA_Boolean semaphoreDeleted = UA_FALSE;

        if (current->registeredServer.semaphoreFilePath.length) {
            char* filePath = malloc(sizeof(char)*current->registeredServer.semaphoreFilePath.length+1);
            memcpy( filePath, current->registeredServer.semaphoreFilePath.data, current->registeredServer.semaphoreFilePath.length );
            filePath[current->registeredServer.semaphoreFilePath.length] = '\0';

#ifdef _MSC_VER
            semaphoreDeleted = _access( filePath, 0 ) == -1;
#else
            semaphoreDeleted = access( filePath, 0 ) == -1;
#endif
            free(filePath);
        }

        if (semaphoreDeleted || (server->config.discoveryCleanupTimeout && current->lastSeen < timedOut)) {
            if (semaphoreDeleted) {
                UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s is removed because the semaphore file '%.*s' was deleted.",
                            (int)current->registeredServer.serverUri.length, current->registeredServer.serverUri.data,
                            (int)current->registeredServer.semaphoreFilePath.length, current->registeredServer.semaphoreFilePath.data);
            } else {
                // cppcheck-suppress unreadVariable
                UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Registration of server with URI %.*s has timed out and is removed.",
                            (int)current->registeredServer.serverUri.length, current->registeredServer.serverUri.data);
            }
            LIST_REMOVE(current, pointers);
            UA_RegisteredServer_deleteMembers(&current->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
            UA_free(current);
            server->registeredServersSize--;
#else
            server->registeredServersSize = uatomic_add_return(&server->registeredServersSize, -1);
            UA_Server_delayedFree(server, current);
#endif

        }
    }
}

struct PeriodicServerRegisterJob {
    UA_Boolean is_main_job;
    UA_UInt32 default_interval;
    UA_Guid job_id;
    UA_Job *job;
    UA_UInt32 this_interval;
    const char* discovery_server_url;
};

/**
 * Called by the UA_Server job.
 * The OPC UA specification says:
 *
 * > If an error occurs during registration (e.g. the Discovery Server is not running) then the Server
 * > must periodically re-attempt registration. The frequency of these attempts should start at 1 second
 * > but gradually increase until the registration frequency is the same as what it would be if not
 * > errors occurred. The recommended approach would double the period each attempt until reaching the maximum.
 *
 * We will do so by using the additional data parameter which holds information if the next interval
 * is default or if it is a repeaded call.
 */
static void periodicServerRegister(UA_Server *server, void *data) {

    if (data == NULL) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Data parameter must be not NULL for periodic server register");
        return;
    }

    struct PeriodicServerRegisterJob *retryJob = (struct PeriodicServerRegisterJob *)data;

    if (!retryJob->is_main_job) {
        // remove the retry job because we don't want to fire it again.
        UA_Server_removeRepeatedJob(server, retryJob->job_id);
    }

    UA_StatusCode retval = UA_Server_register_discovery(server, retryJob->discovery_server_url != NULL ? retryJob->discovery_server_url : "opc.tcp://localhost:4840", NULL);
    // You can also use a semaphore file. That file must exist. When the file is deleted, the server is automatically unregistered.
    // The semaphore file has to be accessible by the discovery server
    // UA_StatusCode retval = UA_Server_register_discovery(server, "opc.tcp://localhost:4840", "/path/to/some/file");
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not register server with discovery server. "
                     "Is the discovery server started? StatusCode 0x%08x", retval);

        // first retry in 1 second
        UA_UInt32 nextInterval = 1;

        if (!retryJob->is_main_job) {
            nextInterval = retryJob->this_interval*2;
        }

        // as long as next retry is smaller than default interval, retry
        if (nextInterval < retryJob->default_interval) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Retrying registration in %d seconds", nextInterval);
            struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
            newRetryJob->job = malloc(sizeof(UA_Job));
            newRetryJob->default_interval = retryJob->default_interval;
            newRetryJob->is_main_job = UA_FALSE;
            newRetryJob->this_interval = nextInterval;
            newRetryJob->discovery_server_url = retryJob->discovery_server_url;

            newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
            newRetryJob->job->job.methodCall.method = periodicServerRegister;
            newRetryJob->job->job.methodCall.data = newRetryJob;

            UA_Server_addRepeatedJob(server, *newRetryJob->job, nextInterval*1000, &newRetryJob->job_id);
        }
    } else {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Server successfully registered. Next periodical register will be in %d seconds",
                    (int)(retryJob->default_interval/1000));
    }
    if (!retryJob->is_main_job) {
        UA_free(retryJob->job);
        UA_free(retryJob);
    }

}

UA_StatusCode UA_Server_addPeriodicServerRegisterJob(UA_Server *server, const char* discoveryServerUrl, const UA_UInt32 intervalMs,
                                                     const UA_UInt32 delayFirstRegisterMs, UA_Guid* periodicJobId) {

    if (server->periodicServerRegisterJob != NULL) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    // registering the server should be done periodically. Approx. every 10 minutes. The first call will be in 10 Minutes.

    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
            .job.methodCall = {.method = periodicServerRegister, .data = NULL} };

    server->periodicServerRegisterJob = UA_malloc(sizeof(struct PeriodicServerRegisterJob));
    server->periodicServerRegisterJob->job = &job;
    server->periodicServerRegisterJob->this_interval = 0;
    server->periodicServerRegisterJob->is_main_job = UA_TRUE;
    server->periodicServerRegisterJob->default_interval = intervalMs;
    server->periodicServerRegisterJob->discovery_server_url = discoveryServerUrl;
    job.job.methodCall.data = server->periodicServerRegisterJob;


    UA_StatusCode retval = UA_Server_addRepeatedJob(server, *server->periodicServerRegisterJob->job, intervalMs, &server->periodicServerRegisterJob->job_id);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode 0x%08x", retval);
        return retval;
    }
    if (periodicJobId) {
        UA_Guid_copy(&server->periodicServerRegisterJob->job_id, periodicJobId);
    }

    if (delayFirstRegisterMs>0) {
        // Register the server with the discovery server.
        // Delay this first registration until the server is fully initialized
        // will be freed in the callback
        struct PeriodicServerRegisterJob *newRetryJob = malloc(sizeof(struct PeriodicServerRegisterJob));
        newRetryJob->job = malloc(sizeof(UA_Job));
        newRetryJob->this_interval = 1;
        newRetryJob->is_main_job = UA_FALSE;
        newRetryJob->default_interval = intervalMs;
        newRetryJob->job->type = UA_JOBTYPE_METHODCALL;
        newRetryJob->job->job.methodCall.method = periodicServerRegister;
        newRetryJob->job->job.methodCall.data = newRetryJob;
        newRetryJob->discovery_server_url = discoveryServerUrl;
        retval = UA_Server_addRepeatedJob(server, *newRetryJob->job, delayFirstRegisterMs, &newRetryJob->job_id);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Could not create first job for server register. StatusCode 0x%08x", retval);
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

void UA_Server_setRegisterServerCallback(UA_Server *server, UA_Server_registerServerCallback cb, void* data) {
    server->registerServerCallback = cb;
    server->registerServerCallbackData = data;
}

# ifdef UA_ENABLE_DISCOVERY_MULTICAST

void UA_Server_setServerOnNetworkCallback(UA_Server *server,    UA_Server_serverOnNetworkCallback cb, void* data) {
    server->serverOnNetworkCallback = cb;
    server->serverOnNetworkCallbackData = data;
}


static void socket_mdns_set_nonblocking(int sockfd) {
#ifdef _WIN32
    u_long iMode = 1;
    ioctlsocket(sockfd, FIONBIO, &iMode);
#else
    int opts = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, opts|O_NONBLOCK);
#endif
}

/* Create multicast 224.0.0.251:5353 socket */
static int discovery_createMulticastSocket(void) {
    int s, flag = 1, ittl = 255;
    struct sockaddr_in in;
    struct ip_mreq mc;
    char ttl = (char)255; // publish to complete net, not only subnet. See: https://docs.oracle.com/cd/E23824_01/html/821-1602/sockets-137.html

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
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&mc, sizeof(mc));
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ttl, sizeof(ttl));
    setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&ittl, sizeof(ittl));

    socket_mdns_set_nonblocking(s);
    return s;
}

UA_StatusCode UA_Discovery_multicastInit(UA_Server* server) {
    server->mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
    if ((server->mdnsSocket = discovery_createMulticastSocket()) == 0) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Could not create multicast socket. Error: %s", strerror(errno));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    mdnsd_register_receive_callback(server->mdnsDaemon, mdns_record_received, server);
    return UA_STATUSCODE_GOOD;
}

void UA_Discovery_multicastDestroy(UA_Server* server) {
    mdnsd_shutdown(server->mdnsDaemon);
    mdnsd_free(server->mdnsDaemon);
}

static void UA_Discovery_multicastConflict(char *name, int type, void *arg) {
    // cppcheck-suppress unreadVariable
    UA_Server *server = (UA_Server*) arg;
    UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS name conflict detected: '%s' for type %d", name, type);
}

static char* create_fullServiceDomain(const char* servername, const char* hostname, size_t maxLen) {
    size_t hostnameLen = strlen(hostname);
    size_t servernameLen = strlen(servername);
    // [servername]-[hostname]._opcua-tcp._tcp.local.

    if (hostnameLen+servernameLen+1 > maxLen) {
        if (servernameLen+2 > maxLen) {
            servernameLen = maxLen;
            hostnameLen = 0;
        } else {
            hostnameLen = maxLen - servernameLen - 1;
        }
    }

    char *fullServiceDomain = malloc(servernameLen + 1 + hostnameLen + 23 + 2);
    if (!fullServiceDomain) {
        return NULL;
    }
    if (hostnameLen > 0)
        sprintf(fullServiceDomain, "%.*s-%.*s._opcua-tcp._tcp.local.", (int)servernameLen, servername, (int)hostnameLen, hostname);
    else
        sprintf(fullServiceDomain, "%.*s._opcua-tcp._tcp.local.", (int)servernameLen, servername);
    return fullServiceDomain;
}

/**
 * Check if mDNS already has an entry for given hostname and port combination.
 * @param server
 * @param hostname
 * @param port
 * @param protocol
 * @return
 */
static UA_StatusCode
UA_Discovery_recordExists(UA_Server* server, const char* fullServiceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    unsigned short found = 0;

    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(server->mdnsDaemon, fullServiceDomain);
    if (r) {
        while (r) {
            const mdns_answer_t *data = mdnsd_record_data(r);
            if (data->type == QTYPE_SRV && (port == 0 || data->srv.port == port)) {
                found = 1;
                break;
            }
            r = mdnsd_record_next(r);
        }
    }
    return found ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}

static int discovery_multicastQueryAnswer(mdns_answer_t *a, void *arg) {
    UA_Server *server = (UA_Server*) arg;
    if (a->type != QTYPE_PTR)
        return 0;

    if (a->rdname == NULL)
        return 0;

    if (UA_Discovery_recordExists(server, a->rdname, 0, UA_DISCOVERY_TCP) == UA_STATUSCODE_GOOD) {
        // we already know about this server. So skip.
        return 0;
    }

    if (mdnsd_has_query(server->mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER, "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(server->mdnsDaemon, a->rdname,QTYPE_SRV,discovery_multicastQueryAnswer, server);
    mdnsd_query(server->mdnsDaemon, a->rdname,QTYPE_TXT,discovery_multicastQueryAnswer, server);

    return 0;
}

/**
 * Send a multicast probe to find any other OPC UA server on the network through mDNS.
 *
 * @param server
 * @return
 */
UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server) {
    mdnsd_query(server->mdnsDaemon, "_opcua-tcp._tcp.local.",QTYPE_PTR,discovery_multicastQueryAnswer, server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Discovery_addRecord(UA_Server* server, const char* servername, const char* hostname,
                       unsigned short port, const char* path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites, const size_t *capabilitiesSize) {
    if (capabilitiesSize == NULL || (*capabilitiesSize>0 && capabilites == NULL)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    size_t hostnameLen = strlen(hostname);
    size_t servernameLen = strlen(servername);
    // use a limit for the hostname length to make sure full string fits into 63 chars (limited by DNS spec)
    if (hostnameLen == 0 || servernameLen == 0) {
        return UA_STATUSCODE_BADOUTOFRANGE;
    } else if (hostnameLen+servernameLen+1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: Combination of hostname+servername exceeds maximum of 62 chars. It will be truncated.");
    } else if (hostnameLen > 63) {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: Hostname length exceeds maximum of 63 chars. It will be truncated.");
    }

    if (!server->mdnsMainSrvAdded) {
        mdns_record_t *r = mdnsd_shared(server->mdnsDaemon, "_services._dns-sd._udp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(server->mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        server->mdnsMainSrvAdded = 1;
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char *fullServiceDomain;
    if (!(fullServiceDomain = create_fullServiceDomain(servername, hostname, 63))) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    if (UA_Discovery_recordExists(server, fullServiceDomain, port, protocol) == UA_STATUSCODE_GOOD) {
        free(fullServiceDomain);
        return UA_STATUSCODE_GOOD;
    }

    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: add record for domain: %s", fullServiceDomain);


    // _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local

    // check if there is already a PTR entry for the given service.
    mdns_record_t *r = mdnsd_get_published(server->mdnsDaemon, "_opcua-tcp._tcp.local.");
    unsigned short found = 0;
    // search for the record with the correct ptr hostname
    while (r) {
        const mdns_answer_t *data = mdnsd_record_data(r);

        if (data->type == QTYPE_PTR && strcmp(data->rdname, fullServiceDomain)==0) {
            found = 1;
            break;
        }
        r = mdnsd_record_next(r);
    }

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    if (!found) {
        r = mdnsd_shared(server->mdnsDaemon, "_opcua-tcp._tcp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(server->mdnsDaemon, r, fullServiceDomain);
    }

    // hostname.
    size_t maxHostnameLen = hostnameLen < 63 ? hostnameLen : 63;
    char *localDomain = malloc(maxHostnameLen+2);
    if (!localDomain) {
        free(fullServiceDomain);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sprintf(localDomain, "%.*s.",(int)(maxHostnameLen), hostname);


    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    r = mdnsd_unique(server->mdnsDaemon, fullServiceDomain, QTYPE_SRV, 600, UA_Discovery_multicastConflict, server);
    // r = mdnsd_shared(server->mdnsDaemon, fullServiceDomain, QTYPE_SRV, 600);
    mdnsd_set_srv(server->mdnsDaemon, r, 0, 0, port, localDomain);

    // A/AAAA record for all ip addresses.
    // [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
    // [hostname]. A [ip].
#ifdef _WIN32
    // see http://stackoverflow.com/a/10838854/869402
    IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;
    IP_ADAPTER_ADDRESSES* adapter = NULL;

    // Start with a 16 KB buffer and resize if needed -
    // multiple attempts in case interfaces change while
    // we are in the middle of querying them.
    DWORD adapter_addresses_buffer_size = 16 * 1024;
    for (int attempts = 0; attempts != 3; ++attempts)
    {
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(adapter_addresses_buffer_size);
        assert(adapter_addresses);

        DWORD error = GetAdaptersAddresses(
            AF_UNSPEC, 
            GAA_FLAG_SKIP_ANYCAST |
                GAA_FLAG_SKIP_DNS_SERVER |
                GAA_FLAG_SKIP_FRIENDLY_NAME, 
            NULL, 
            adapter_addresses,
            &adapter_addresses_buffer_size);

        if (ERROR_SUCCESS == error)
        {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,"GetAdaptersAddresses returned an error. Not setting mDNS A records.");
            adapter_addresses = NULL;
            break;
        }
        else if (ERROR_BUFFER_OVERFLOW == error)
        {
            // Try again with the new size
            free(adapter_addresses);
            adapter_addresses = NULL;

            continue;
        }
        else
        {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,"GetAdaptersAddresses returned an unexpected error. Not setting mDNS A records.");
            // Unexpected error code - log and throw
            free(adapter_addresses);
            adapter_addresses = NULL;

            break;
        }
    }

    // Iterate through all of the adapters
    for (adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next)
    {
        // Skip loopback adapters
        if (IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)
        {
            continue;
        }

        // Parse all IPv4 and IPv6 addresses
        for (
            IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; 
            NULL != address;
            address = address->Next)
        {
            int family = address->Address.lpSockaddr->sa_family;
            if (AF_INET == family)
            {
                // IPv4
                SOCKADDR_IN* ipv4 = (SOCKADDR_IN*)(address->Address.lpSockaddr);


                // [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
                r = mdnsd_shared(server->mdnsDaemon, fullServiceDomain, QTYPE_A, 600);
                mdnsd_set_raw(server->mdnsDaemon, r,(char *)&ipv4->sin_addr , 4);

                // [hostname]. A [ip].
                r = mdnsd_shared(server->mdnsDaemon, localDomain, QTYPE_A, 600);
                mdnsd_set_raw(server->mdnsDaemon, r,(char *)&ipv4->sin_addr , 4);
            }
            /*else if (AF_INET6 == family)
            {
                // IPv6
                SOCKADDR_IN6* ipv6 = (SOCKADDR_IN6*)(address->Address.lpSockaddr);

                char str_buffer[INET6_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET6, &(ipv6->sin6_addr), str_buffer, INET6_ADDRSTRLEN);

                std::string ipv6_str(str_buffer);

                // Detect and skip non-external addresses
                bool is_link_local(false);
                bool is_special_use(false);

                if (0 == ipv6_str.find("fe"))
                {
                    char c = ipv6_str[2];
                    if (c == '8' || c == '9' || c == 'a' || c == 'b')
                    {
                        is_link_local = true;
                    }
                }
                else if (0 == ipv6_str.find("2001:0:"))
                {
                    is_special_use = true;
                }

                if (! (is_link_local || is_special_use))
                {
                    ipAddrs.mIpv6.push_back(ipv6_str);
                }
            }*/
            else
            {
                // Skip all other types of addresses
                continue;
            }
        }
    }

    // Cleanup
    free(adapter_addresses);
    adapter_addresses = NULL;
#else
    {
        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == -1) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,"getifaddrs returned an unexpected error. Not setting mDNS A records.");
        } else {
            /* Walk through linked list, maintaining head pointer so we can free list later */
            int n;
            for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
                if (ifa->ifa_addr == NULL)
                    continue;

                if ((strcmp("lo", ifa->ifa_name) == 0) ||
                    !(ifa->ifa_flags & (IFF_RUNNING))||
                    !(ifa->ifa_flags & (IFF_MULTICAST)))
                    continue;

                if (ifa->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in* sa = (struct sockaddr_in*) ifa->ifa_addr;
                    // [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
                    r = mdnsd_shared(server->mdnsDaemon, fullServiceDomain, QTYPE_A, 600);
                    mdnsd_set_raw(server->mdnsDaemon, r,(char *)&sa->sin_addr.s_addr , 4);
                    // [hostname]. A [ip].
                    r = mdnsd_shared(server->mdnsDaemon, localDomain, QTYPE_A, 600);
                    mdnsd_set_raw(server->mdnsDaemon, r,(char *)&sa->sin_addr.s_addr , 4);
                } /*else if (ifa->ifa_addr->sa_family == AF_INET6) {
                    // IPv6 not implemented yet
                }*/
            }

            freeifaddrs(ifaddr);
        }

    }
#endif

    // TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    if (createTxt) {
        r = mdnsd_unique(server->mdnsDaemon, fullServiceDomain, QTYPE_TXT, 600, UA_Discovery_multicastConflict, server);
        xht_t* h = xht_new(11);
        char* allocPath = NULL;
        if (!path || strlen(path) == 0)
            xht_set(h, "path", "/");
        else {
            // path does not contain slash, so add it here
            if (path[0]=='/')
                allocPath = STRDUP(path);
            else {
                allocPath = malloc(strlen(path)+2);
                allocPath[0] = '/';
                memcpy(allocPath+1, path, strlen(path));
                allocPath[strlen(path)+1] = '\0';
            }
            xht_set(h, "path", allocPath);
        }


        // calculate max string length:
        size_t capsLen = 0;

        for (size_t i = 0; i < *capabilitiesSize; i++) {
            // add comma or last \0
            capsLen += capabilites[i].length + 1;
        }

        char* caps = NULL;
        if (capsLen) {
            // freed when xht_free is called
            caps = malloc(sizeof(char) * capsLen);
            size_t idx = 0;
            for (size_t i = 0; i < *capabilitiesSize; i++) {
                strncpy(caps + idx, (const char *)capabilites[i].data, capabilites[i].length);
                idx += capabilites[i].length + 1;
                caps[idx - 1] = ',';
            }
            caps[idx - 1] = '\0';

            xht_set(h, "caps", caps);
        } else {
            xht_set(h, "caps", "NA");
        }

        int txtRecordLength;
        unsigned char* packet = sd2txt(h, &txtRecordLength);
        if (allocPath)
            free(allocPath);
        if (caps)
            free(caps);
        xht_free(h);
        mdnsd_set_raw(server->mdnsDaemon, r, (char*) packet, (unsigned short) txtRecordLength);
        free(packet);
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
    // use a limit for the hostname length to make sure full string fits into 63 chars (limited by DNS spec)
    if (hostnameLen == 0 || servernameLen == 0) {
        return UA_STATUSCODE_BADOUTOFRANGE;
    } else if (hostnameLen+servernameLen+1 > 63) { // include dash between servername-hostname
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: Combination of hostname+servername exceeds maximum of 62 chars. It will be truncated.");
    }

    // [servername]-[hostname]._opcua-tcp._tcp.local.
    char *fullServiceDomain;
    if (!(fullServiceDomain = create_fullServiceDomain(servername, hostname, 63))) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast DNS: remove record for domain: %s", fullServiceDomain);

    // _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local.
    mdns_record_t *r = mdnsd_get_published(server->mdnsDaemon, "_opcua-tcp._tcp.local.");
    if (r) {
        while (r) {
            const mdns_answer_t *data = mdnsd_record_data(r);
            if (data->type == QTYPE_PTR && strcmp(data->rdname, fullServiceDomain)==0) {
                mdnsd_done(server->mdnsDaemon,r);
                break;
            }
            r = mdnsd_record_next(r);
        }
    } else {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: could not remove record. PTR Record not found for domain: %s",
                       fullServiceDomain);
        free(fullServiceDomain);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    // looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port hostname.local.
    // and TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
    // and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]
    r = mdnsd_get_published(server->mdnsDaemon, fullServiceDomain);
    if (r) {
        while (r) {
            const mdns_answer_t *data = mdnsd_record_data(r);
            mdns_record_t *next = mdnsd_record_next(r);
            if ((removeTxt && data->type == QTYPE_TXT) || (removeTxt && data->type == QTYPE_A) || data->srv.port == port) {
                mdnsd_done(server->mdnsDaemon,r);
            }
            r = next;
        }
    } else {
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Multicast DNS: could not remove record. Record not found for domain: %s",
                       fullServiceDomain);
        free(fullServiceDomain);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    free(fullServiceDomain);
    return UA_STATUSCODE_GOOD;
}

#  ifdef UA_ENABLE_MULTITHREADING

static void * multicastWorkerLoop(UA_Server *server) {
    struct timeval next_sleep = {.tv_sec = 0, .tv_usec = 0};

    volatile UA_Boolean *running = &server->mdnsRunning;
    fd_set fds;

    while (*running) {

        FD_ZERO(&fds);
        FD_SET(server->mdnsSocket, &fds);
        select(server->mdnsSocket + 1, &fds, 0, 0, &next_sleep);

        if (!*running)
            break;


        unsigned short retVal = mdnsd_step(server->mdnsDaemon, server->mdnsSocket,
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

        if (!*running)
            break;
    }

    return NULL;
}

UA_StatusCode UA_Discovery_multicastListenStart(UA_Server* server) {
    if (pthread_create(&server->mdnsThread, NULL, (void* (*)(void*))multicastWorkerLoop, server)) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast error: Can not create multicast thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Discovery_multicastListenStop(UA_Server* server) {
    mdnsd_shutdown(server->mdnsDaemon);
    // wake up select
    write(server->mdnsSocket, "\0", 1);
    if(pthread_join(server->mdnsThread, NULL)) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER, "Multicast error: Can not stop thread.");
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

#  endif // UA_ENABLE_DISCOVERY_MULTICAST && UA_ENABLE_MULTITHREADING

UA_StatusCode UA_Discovery_multicastIterate(UA_Server* server, UA_DateTime *nextRepeat, UA_Boolean processIn) {
    struct timeval next_sleep = {.tv_sec = 0, .tv_usec = 0};
    unsigned short retVal = mdnsd_step(server->mdnsDaemon, server->mdnsSocket, processIn, true, &next_sleep);
    if (retVal == 1) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not read from socket. %s", strerror(errno));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    } else if (retVal == 2) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Multicast error: Can not write to socket. %s", strerror(errno));
        return UA_STATUSCODE_BADNOCOMMUNICATION;
    }
    if (nextRepeat)
        *nextRepeat = UA_DateTime_now() + (UA_DateTime)(next_sleep.tv_sec * UA_SEC_TO_DATETIME + next_sleep.tv_usec * UA_USEC_TO_DATETIME);
    return UA_STATUSCODE_GOOD;
}

# endif // UA_ENABLE_DISCOVERY_MULTICAST

#endif // UA_ENABLE_DISCOVERY
