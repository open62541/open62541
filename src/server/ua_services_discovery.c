/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 */

#include "ua_server_internal.h"
#include "ua_services.h"

#ifdef UA_ENABLE_DISCOVERY

#include "ua_client_internal.h"

static UA_StatusCode
setApplicationDescriptionFromRegisteredServer(const UA_FindServersRequest *request,
                                              UA_ApplicationDescription *target,
                                              const UA_RegisteredServer *registeredServer) {
    UA_ApplicationDescription_init(target);
    UA_StatusCode retval = UA_String_copy(&registeredServer->serverUri, &target->applicationUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_String_copy(&registeredServer->productUri, &target->productUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    // if the client requests a specific locale, select the corresponding server name
    if(request->localeIdsSize) {
        UA_Boolean appNameFound = false;
        for(size_t i =0; i<request->localeIdsSize && !appNameFound; i++) {
            for(size_t j =0; j<registeredServer->serverNamesSize; j++) {
                if(UA_String_equal(&request->localeIds[i], &registeredServer->serverNames[j].locale)) {
                    retval = UA_LocalizedText_copy(&registeredServer->serverNames[j],
                                                   &target->applicationName);
                    if(retval != UA_STATUSCODE_GOOD)
                        return retval;
                    appNameFound = true;
                    break;
                }
            }
        }

        // server does not have the requested local, therefore we can select the
        // most suitable one
        if(!appNameFound && registeredServer->serverNamesSize) {
            retval = UA_LocalizedText_copy(&registeredServer->serverNames[0],
                                           &target->applicationName);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    } else if(registeredServer->serverNamesSize) {
        // just take the first name
        retval = UA_LocalizedText_copy(&registeredServer->serverNames[0], &target->applicationName);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    target->applicationType = registeredServer->serverType;
    retval = UA_String_copy(&registeredServer->gatewayServerUri, &target->gatewayServerUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    // TODO where do we get the discoveryProfileUri for application data?

    target->discoveryUrlsSize = registeredServer->discoveryUrlsSize;
    if(registeredServer->discoveryUrlsSize) {
        size_t duSize = sizeof(UA_String) * registeredServer->discoveryUrlsSize;
        target->discoveryUrls = (UA_String *)UA_malloc(duSize);
        if(!target->discoveryUrls)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < registeredServer->discoveryUrlsSize; i++) {
            retval = UA_String_copy(&registeredServer->discoveryUrls[i], &target->discoveryUrls[i]);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    return retval;
}
#endif

static UA_StatusCode
setApplicationDescriptionFromServer(UA_ApplicationDescription *target, const UA_Server *server) {
    /* Copy ApplicationDescription from the config */
    UA_StatusCode result = UA_ApplicationDescription_copy(&server->config.applicationDescription, target);
    if(result != UA_STATUSCODE_GOOD)
        return result;

    /* add the discoveryUrls from the networklayers */
    size_t discSize = sizeof(UA_String) * (target->discoveryUrlsSize + server->config.networkLayersSize);
    UA_String* disc = (UA_String *)UA_realloc(target->discoveryUrls, discSize);
    if(!disc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    size_t existing = target->discoveryUrlsSize;
    target->discoveryUrls = disc;
    target->discoveryUrlsSize += server->config.networkLayersSize;

    // TODO: Add nl only if discoveryUrl not already present
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_ServerNetworkLayer* nl = &server->config.networkLayers[i];
        UA_String_copy(&nl->discoveryUrl, &target->discoveryUrls[existing + i]);
    }
    return UA_STATUSCODE_GOOD;
}

void Service_FindServers(UA_Server *server, UA_Session *session,
                         const UA_FindServersRequest *request,
                         UA_FindServersResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing FindServersRequest");

    /* Return the server itself? */
    UA_Boolean foundSelf = false;
    if(request->serverUrisSize) {
        for(size_t i = 0; i < request->serverUrisSize; i++) {
            if(UA_String_equal(&request->serverUris[i],
                               &server->config.applicationDescription.applicationUri)) {
                foundSelf = true;
                break;
            }
        }
    } else {
        foundSelf = true;
    }

#ifndef UA_ENABLE_DISCOVERY
    if(!foundSelf)
        return;

    UA_ApplicationDescription *ad = UA_ApplicationDescription_new();
    if(!ad) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    UA_StatusCode retval = setApplicationDescriptionFromServer(ad, server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ApplicationDescription_delete(ad);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->servers = ad;
    response->serversSize = 1;
    return;

#else

    /* Temporarily store all the pointers which we found to avoid reiterating
     * through the list */
    size_t foundServersSize = 0;
    UA_STACKARRAY(UA_RegisteredServer*, foundServers, server->discoveryManager.registeredServersSize+1);

    registeredServer_list_entry* current;
    LIST_FOREACH(current, &server->discoveryManager.registeredServers, pointers) {
        if(request->serverUrisSize) {
            /* If client only requested a specific set of servers */
            for(size_t i = 0; i < request->serverUrisSize; i++) {
                if(UA_String_equal(&current->registeredServer.serverUri, &request->serverUris[i])) {
                    foundServers[foundServersSize] = &current->registeredServer;
                    foundServersSize++;
                    break;
                }
            }
        } else {
            /* Return all registered servers */
            foundServers[foundServersSize] = &current->registeredServer;
            foundServersSize++;
        }
    }

    size_t allocSize = foundServersSize;
    if(foundSelf)
        allocSize++;

    /* Nothing to do? */
    if(allocSize == 0)
        return;

    /* Allocate memory */
    response->servers = (UA_ApplicationDescription*)UA_Array_new(allocSize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    if(!response->servers) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->serversSize = allocSize;

    /* Copy into the response. TODO: Evaluate return codes */
    size_t pos = 0;
    if(foundSelf) {
        setApplicationDescriptionFromServer(&response->servers[pos++], server);
    }
    for(size_t i = 0; i < foundServersSize; i++) {
        setApplicationDescriptionFromRegisteredServer(request, &response->servers[pos++], foundServers[i]);
    }

#endif
}

void
Service_GetEndpoints(UA_Server *server, UA_Session *session,
                     const UA_GetEndpointsRequest *request,
                     UA_GetEndpointsResponse *response) {
    /* If the client expects to see a specific endpointurl, mirror it back. If
       not, clone the endpoints with the discovery url of all networklayers. */
    const UA_String *endpointUrl = &request->endpointUrl;
    if(endpointUrl->length > 0) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Processing GetEndpointsRequest with endpointUrl "
                             UA_PRINTF_STRING_FORMAT, UA_PRINTF_STRING_DATA(*endpointUrl));
    } else {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Processing GetEndpointsRequest with an empty endpointUrl");
    }

    /* test if the supported binary profile shall be returned */
    size_t reSize = sizeof(UA_Boolean) * server->config.endpointsSize;
    UA_STACKARRAY(UA_Boolean, relevant_endpoints, reSize);
    memset(relevant_endpoints, 0, reSize);
    size_t relevant_count = 0;
    if(request->profileUrisSize == 0) {
        for(size_t j = 0; j < server->config.endpointsSize; ++j)
            relevant_endpoints[j] = true;
        relevant_count = server->config.endpointsSize;
    } else {
        for(size_t j = 0; j < server->config.endpointsSize; ++j) {
            for(size_t i = 0; i < request->profileUrisSize; ++i) {
                if(!UA_String_equal(&request->profileUris[i],
                                    &server->config.endpoints[j].transportProfileUri))
                    continue;
                relevant_endpoints[j] = true;
                ++relevant_count;
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

    response->endpoints =
        (UA_EndpointDescription*)UA_Array_new(relevant_count * clone_times,
                                              &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->endpointsSize = relevant_count * clone_times;

    size_t k = 0;
    UA_StatusCode retval;
    for(size_t i = 0; i < clone_times; ++i) {
        if(nl_endpointurl)
            endpointUrl = &server->config.networkLayers[i].discoveryUrl;
        for(size_t j = 0; j < server->config.endpointsSize; ++j) {
            if(!relevant_endpoints[j])
                continue;
            retval = UA_EndpointDescription_copy(&server->config.endpoints[j],
                                                 &response->endpoints[k]);
            if(retval != UA_STATUSCODE_GOOD)
                goto error;
            retval = UA_String_copy(endpointUrl, &response->endpoints[k].endpointUrl);
            if(retval != UA_STATUSCODE_GOOD)
                goto error;
            ++k;
        }
    }

    return;
error:
    response->responseHeader.serviceResult = retval;
    UA_Array_delete(response->endpoints, response->endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    response->endpoints = NULL;
    response->endpointsSize = 0;
}

#ifdef UA_ENABLE_DISCOVERY

static void
process_RegisterServer(UA_Server *server, UA_Session *session,
                       const UA_RequestHeader* requestHeader,
                       const UA_RegisteredServer *requestServer,
                       const size_t requestDiscoveryConfigurationSize,
                       const UA_ExtensionObject *requestDiscoveryConfiguration,
                       UA_ResponseHeader* responseHeader,
                       size_t *responseConfigurationResultsSize,
                       UA_StatusCode **responseConfigurationResults,
                       size_t *responseDiagnosticInfosSize,
                       UA_DiagnosticInfo *responseDiagnosticInfos) {
    /* Find the server from the request in the registered list */
    registeredServer_list_entry* current;
    registeredServer_list_entry *registeredServer_entry = NULL;
    LIST_FOREACH(current, &server->discoveryManager.registeredServers, pointers) {
        if(UA_String_equal(&current->registeredServer.serverUri, &requestServer->serverUri)) {
            registeredServer_entry = current;
            break;
        }
    }

    UA_MdnsDiscoveryConfiguration *mdnsConfig = NULL;

    const UA_String* mdnsServerName = NULL;
    if(requestDiscoveryConfigurationSize) {
        *responseConfigurationResults =
            (UA_StatusCode *)UA_Array_new(requestDiscoveryConfigurationSize,
                                          &UA_TYPES[UA_TYPES_STATUSCODE]);
        if(!(*responseConfigurationResults)) {
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        *responseConfigurationResultsSize = requestDiscoveryConfigurationSize;

        for(size_t i = 0; i < requestDiscoveryConfigurationSize; i++) {
            const UA_ExtensionObject *object = &requestDiscoveryConfiguration[i];
            if(!mdnsConfig && (object->encoding == UA_EXTENSIONOBJECT_DECODED ||
                               object->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
               (object->content.decoded.type == &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION])) {
                mdnsConfig = (UA_MdnsDiscoveryConfiguration *)object->content.decoded.data;
                mdnsServerName = &mdnsConfig->mdnsServerName;
                (*responseConfigurationResults)[i] = UA_STATUSCODE_GOOD;
            } else {
                (*responseConfigurationResults)[i] = UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }
    }

    if(!mdnsServerName && requestServer->serverNamesSize)
        mdnsServerName = &requestServer->serverNames[0].text;

    if(!mdnsServerName) {
        responseHeader->serviceResult = UA_STATUSCODE_BADSERVERNAMEMISSING;
        return;
    }

    if(requestServer->discoveryUrlsSize == 0) {
        responseHeader->serviceResult = UA_STATUSCODE_BADDISCOVERYURLMISSING;
        return;
    }

    if(requestServer->semaphoreFilePath.length) {
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        char* filePath = (char*)
            UA_malloc(sizeof(char)*requestServer->semaphoreFilePath.length+1);
        if(!filePath) {
            UA_LOG_ERROR_SESSION(&server->config.logger, session,
                                 "Cannot allocate memory for semaphore path. Out of memory.");
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        memcpy(filePath, requestServer->semaphoreFilePath.data, requestServer->semaphoreFilePath.length );
        filePath[requestServer->semaphoreFilePath.length] = '\0';
        if(!UA_fileExists( filePath )) {
            responseHeader->serviceResult = UA_STATUSCODE_BADSEMPAHOREFILEMISSING;
            UA_free(filePath);
            return;
        }
        UA_free(filePath);
#else
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Ignoring semaphore file path. open62541 not compiled "
                       "with UA_ENABLE_DISCOVERY_SEMAPHORE=ON");
#endif
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.discovery.mdnsEnable) {
        for(size_t i = 0; i < requestServer->discoveryUrlsSize; i++) {
            /* create TXT if is online and first index, delete TXT if is offline and last index */
            UA_Boolean updateTxt = (requestServer->isOnline && i==0) ||
                (!requestServer->isOnline && i==requestServer->discoveryUrlsSize);
            UA_Server_updateMdnsForDiscoveryUrl(server, mdnsServerName, mdnsConfig,
                                                &requestServer->discoveryUrls[i],
                                                requestServer->isOnline, updateTxt);
        }
    }
#endif

    if(!requestServer->isOnline) {
        // server is shutting down. Remove it from the registered servers list
        if(!registeredServer_entry) {
            // server not found, show warning
            UA_LOG_WARNING_SESSION(&server->config.logger, session,
                                   "Could not unregister server %.*s. Not registered.",
                                   (int)requestServer->serverUri.length, requestServer->serverUri.data);
            responseHeader->serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
            return;
        }

        if(server->discoveryManager.registerServerCallback)
            server->discoveryManager.
                registerServerCallback(requestServer,
                                       server->discoveryManager.registerServerCallbackData);

        // server found, remove from list
        LIST_REMOVE(registeredServer_entry, pointers);
        UA_RegisteredServer_deleteMembers(&registeredServer_entry->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
        UA_free(registeredServer_entry);
        server->discoveryManager.registeredServersSize--;
#else
        UA_atomic_subSize(&server->discoveryManager.registeredServersSize, 1);
        registeredServer_entry->delayedCleanup.callback = NULL; /* only free the structure */
        UA_WorkQueue_enqueueDelayed(&server->workQueue, &registeredServer_entry->delayedCleanup);
#endif
        responseHeader->serviceResult = UA_STATUSCODE_GOOD;
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!registeredServer_entry) {
        // server not yet registered, register it by adding it to the list
        UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Registering new server: %.*s",
                             (int)requestServer->serverUri.length, requestServer->serverUri.data);

        registeredServer_entry =
            (registeredServer_list_entry *)UA_malloc(sizeof(registeredServer_list_entry));
        if(!registeredServer_entry) {
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        LIST_INSERT_HEAD(&server->discoveryManager.registeredServers, registeredServer_entry, pointers);
#ifndef UA_ENABLE_MULTITHREADING
        server->discoveryManager.registeredServersSize++;
#else
        UA_atomic_addSize(&server->discoveryManager.registeredServersSize, 1);
#endif

        if(server->discoveryManager.registerServerCallback)
            server->discoveryManager.
                registerServerCallback(requestServer,
                                       server->discoveryManager.registerServerCallbackData);
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
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing RegisterServerRequest");
    process_RegisterServer(server, session, &request->requestHeader, &request->server, 0,
                           NULL, &response->responseHeader, 0, NULL, 0, NULL);
}

void Service_RegisterServer2(UA_Server *server, UA_Session *session,
                            const UA_RegisterServer2Request *request,
                             UA_RegisterServer2Response *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing RegisterServer2Request");
    process_RegisterServer(server, session, &request->requestHeader, &request->server,
                           request->discoveryConfigurationSize, request->discoveryConfiguration,
                           &response->responseHeader, &response->configurationResultsSize,
                           &response->configurationResults, &response->diagnosticInfosSize,
                           response->diagnosticInfos);
}

/* Cleanup server registration: If the semaphore file path is set, then it just
 * checks the existence of the file. When it is deleted, the registration is
 * removed. If there is no semaphore file, then the registration will be removed
 * if it is older than 60 minutes. */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic) {
    UA_DateTime timedOut = nowMonotonic;
    // registration is timed out if lastSeen is older than 60 minutes (default
    // value, can be modified by user).
    if(server->config.discovery.cleanupTimeout)
        timedOut -= server->config.discovery.cleanupTimeout*UA_DATETIME_SEC;

    registeredServer_list_entry* current, *temp;
    LIST_FOREACH_SAFE(current, &server->discoveryManager.registeredServers, pointers, temp) {
        UA_Boolean semaphoreDeleted = false;

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        if(current->registeredServer.semaphoreFilePath.length) {
            size_t fpSize = sizeof(char)*current->registeredServer.semaphoreFilePath.length+1;
            // todo: malloc may fail: return a statuscode
            char* filePath = (char *)UA_malloc(fpSize);
            if(filePath) {
                memcpy(filePath, current->registeredServer.semaphoreFilePath.data,
                       current->registeredServer.semaphoreFilePath.length );
                filePath[current->registeredServer.semaphoreFilePath.length] = '\0';
                semaphoreDeleted = UA_fileExists(filePath) == false;
                UA_free(filePath);
            } else {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Cannot check registration semaphore. Out of memory");
            }
        }
#endif

        if(semaphoreDeleted || (server->config.discovery.cleanupTimeout &&
                                current->lastSeen < timedOut)) {
            if(semaphoreDeleted) {
                UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s is removed because "
                            "the semaphore file '%.*s' was deleted.",
                            (int)current->registeredServer.serverUri.length,
                            current->registeredServer.serverUri.data,
                            (int)current->registeredServer.semaphoreFilePath.length,
                            current->registeredServer.semaphoreFilePath.data);
            } else {
                // cppcheck-suppress unreadVariable
                UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s has timed out and is removed.",
                            (int)current->registeredServer.serverUri.length,
                            current->registeredServer.serverUri.data);
            }
            LIST_REMOVE(current, pointers);
            UA_RegisteredServer_deleteMembers(&current->registeredServer);
#ifndef UA_ENABLE_MULTITHREADING
            UA_free(current);
            server->discoveryManager.registeredServersSize--;
#else
            UA_atomic_subSize(&server->discoveryManager.registeredServersSize, 1);
            current->delayedCleanup.callback = NULL; /* Only free the structure */
            UA_WorkQueue_enqueueDelayed(&server->workQueue, &current->delayedCleanup);
#endif
        }
    }
}

struct PeriodicServerRegisterCallback {
    UA_UInt64 id;
    UA_Double this_interval;
    UA_Double default_interval;
    UA_Boolean registered;
    UA_Client* client;
    const char* discovery_server_url;
};

/* Called by the UA_Server callback. The OPC UA specification says:
 *
 * > If an error occurs during registration (e.g. the Discovery Server is not running) then the Server
 * > must periodically re-attempt registration. The frequency of these attempts should start at 1 second
 * > but gradually increase until the registration frequency is the same as what it would be if not
 * > errors occurred. The recommended approach would double the period each attempt until reaching the maximum.
 *
 * We will do so by using the additional data parameter which holds information
 * if the next interval is default or if it is a repeated call. */
static void
periodicServerRegister(UA_Server *server, void *data) {
    UA_assert(data != NULL);

    struct PeriodicServerRegisterCallback *cb = (struct PeriodicServerRegisterCallback *)data;

    /* Which URL to register on */
    // fixme: remove magic url
    const char * server_url;
    if(cb->discovery_server_url != NULL)
        server_url = cb->discovery_server_url;
    else
        server_url = "opc.tcp://localhost:4840";
    UA_StatusCode retval = UA_Client_connect_noSession(cb->client, server_url);
    if (retval == UA_STATUSCODE_GOOD) {
        /* Register
           You can also use a semaphore file. That file must exist. When the file is
           deleted, the server is automatically unregistered. The semaphore file has
           to be accessible by the discovery server

           UA_StatusCode retval = UA_Server_register_discovery(server,
           "opc.tcp://localhost:4840", "/path/to/some/file");
        */
        retval = UA_Server_register_discovery(server, cb->client, NULL);
    }
    if (cb->client->state == UA_CLIENTSTATE_CONNECTED) {
        UA_StatusCode retval1 = UA_Client_disconnect(cb->client);
        if(retval1 != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Could not disconnect client from register server. StatusCode %s",
                         UA_StatusCode_name(retval));
        }
    }
    /* Registering failed */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not register server with discovery server. "
                     "Is the discovery server started? StatusCode %s",
                     UA_StatusCode_name(retval));

        /* If the server was previously registered, retry in one second,
         * else, double the previous interval */
        UA_Double nextInterval = 1000.0;
        if(!cb->registered)
            nextInterval = cb->this_interval * 2;

        /* The interval should be smaller than the default interval */
        if(nextInterval > cb->default_interval)
            nextInterval = cb->default_interval;

        cb->this_interval = nextInterval;
        UA_Server_changeRepeatedCallbackInterval(server, cb->id, nextInterval);
        return;
    }

    /* Registering succeeded */
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Server successfully registered. Next periodical register will be in %d seconds",
                 (int)(cb->default_interval/1000));

    if(!cb->registered) {
        retval = UA_Server_changeRepeatedCallbackInterval(server, cb->id, cb->default_interval);
        /* If changing the interval fails, try again after the next registering */
        if(retval == UA_STATUSCODE_GOOD)
            cb->registered = true;
    }
}

UA_StatusCode
UA_Server_addPeriodicServerRegisterCallback(UA_Server *server,
                                            struct UA_Client *client,
                                            const char* discoveryServerUrl,
                                            UA_Double intervalMs,
                                            UA_Double delayFirstRegisterMs,
                                            UA_UInt64 *periodicCallbackId) {

    /* No valid server URL */
    if(!discoveryServerUrl) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "No discovery server URL provided");
        return UA_STATUSCODE_BADINTERNALERROR;
    }


    if (client->connection.state != UA_CONNECTION_CLOSED)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* check if we are already registering with the given discovery url and remove the old periodic call */
    {
        periodicServerRegisterCallback_entry *rs, *rs_tmp;
        LIST_FOREACH_SAFE(rs, &server->discoveryManager.
                          periodicServerRegisterCallbacks, pointers, rs_tmp) {
            if(strcmp(rs->callback->discovery_server_url, discoveryServerUrl) == 0) {
                UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "There is already a register callback for '%s' in place. Removing the older one.", discoveryServerUrl);
                UA_Server_removeRepeatedCallback(server, rs->callback->id);
                LIST_REMOVE(rs, pointers);
                UA_free(rs->callback);
                UA_free(rs);
                break;
            }
        }
    }

    /* Allocate and initialize */
    struct PeriodicServerRegisterCallback* cb =
        (struct PeriodicServerRegisterCallback*)
        UA_malloc(sizeof(struct PeriodicServerRegisterCallback));
    if(!cb)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Start repeating a failed register after 1s, then increase the delay. Set
     * to 500ms, as the delay is doubled before changing the callback
     * interval.*/
    cb->this_interval = 500.0;
    cb->default_interval = intervalMs;
    cb->registered = false;
    cb->client = client;
    cb->discovery_server_url = discoveryServerUrl;



    /* Add the callback */
    UA_StatusCode retval =
        UA_Server_addRepeatedCallback(server, periodicServerRegister,
                                      cb, delayFirstRegisterMs, &cb->id);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. "
                     "StatusCode %s", UA_StatusCode_name(retval));
        UA_free(cb);
        return retval;
    }

#ifndef __clang_analyzer__
    // the analyzer reports on LIST_INSERT_HEAD a use after free false positive
    periodicServerRegisterCallback_entry *newEntry =
            (periodicServerRegisterCallback_entry *)UA_malloc(sizeof(periodicServerRegisterCallback_entry));
    if(!newEntry) {
        UA_Server_removeRepeatedCallback(server, cb->id);
        UA_free(cb);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    newEntry->callback = cb;
    LIST_INSERT_HEAD(&server->discoveryManager.periodicServerRegisterCallbacks, newEntry, pointers);
#endif

    if(periodicCallbackId)
        *periodicCallbackId = cb->id;
    return UA_STATUSCODE_GOOD;
}

void
UA_Server_setRegisterServerCallback(UA_Server *server,
                                    UA_Server_registerServerCallback cb,
                                    void* data) {
    server->discoveryManager.registerServerCallback = cb;
    server->discoveryManager.registerServerCallbackData = data;
}

#endif /* UA_ENABLE_DISCOVERY */
