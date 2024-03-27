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
#include "ua_discovery.h"
#include "ua_services.h"

#ifdef UA_ENABLE_DISCOVERY

#include <open62541/client.h>

static UA_StatusCode
setApplicationDescriptionFromRegisteredServer(const UA_FindServersRequest *request,
                                              UA_ApplicationDescription *target,
                                              const UA_RegisteredServer *rs) {
    UA_ApplicationDescription_init(target);
    UA_StatusCode retval =
        UA_String_copy(&rs->serverUri, &target->applicationUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_String_copy(&rs->productUri, &target->productUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    // if the client requests a specific locale, select the corresponding server name
    if(request->localeIdsSize) {
        UA_Boolean appNameFound = false;
        for(size_t i =0; i<request->localeIdsSize && !appNameFound; i++) {
            for(size_t j =0; j<rs->serverNamesSize; j++) {
                if(UA_String_equal(&request->localeIds[i],
                                   &rs->serverNames[j].locale)) {
                    retval = UA_LocalizedText_copy(&rs->serverNames[j],
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
        if(!appNameFound && rs->serverNamesSize) {
            retval = UA_LocalizedText_copy(&rs->serverNames[0],
                                           &target->applicationName);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    } else if(rs->serverNamesSize) {
        // just take the first name
        retval = UA_LocalizedText_copy(&rs->serverNames[0],
                                       &target->applicationName);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    target->applicationType = rs->serverType;
    retval = UA_String_copy(&rs->gatewayServerUri, &target->gatewayServerUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    // TODO where do we get the discoveryProfileUri for application data?

    target->discoveryUrlsSize = rs->discoveryUrlsSize;
    if(rs->discoveryUrlsSize) {
        size_t duSize = sizeof(UA_String) * rs->discoveryUrlsSize;
        target->discoveryUrls = (UA_String *)UA_malloc(duSize);
        if(!target->discoveryUrls)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < rs->discoveryUrlsSize; i++) {
            retval = UA_String_copy(&rs->discoveryUrls[i],
                                    &target->discoveryUrls[i]);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    return retval;
}
#endif

void Service_FindServers(UA_Server *server, UA_Session *session,
                         const UA_FindServersRequest *request,
                         UA_FindServersResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing FindServersRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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

    response->responseHeader.serviceResult =
        UA_Array_copy(&server->config.applicationDescription, 1,
                      (void**)&response->servers,
                      &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    response->serversSize = 1;
#else
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Allocate enough memory, including memory for the "self" response */
    size_t maxResults = dm->registeredServersSize + 1;
    response->servers = (UA_ApplicationDescription*)
        UA_Array_new(maxResults, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    if(!response->servers) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Copy into the response. TODO: Evaluate return codes */
    size_t pos = 0;
    if(foundSelf)
        UA_ApplicationDescription_copy(&server->config.applicationDescription,
                                       &response->servers[pos++]);

    registeredServer *current;
    LIST_FOREACH(current, &dm->registeredServers, pointers) {
        UA_Boolean usable = (request->serverUrisSize == 0);
        if(!usable) {
            /* If client only requested a specific set of servers */
            for(size_t i = 0; i < request->serverUrisSize; i++) {
                if(UA_String_equal(&current->registeredServer.serverUri,
                                   &request->serverUris[i])) {
                    usable = true;
                    break;
                }
            }
        }

        if(usable)
            setApplicationDescriptionFromRegisteredServer(request, &response->servers[pos++],
                                                          &current->registeredServer);
    }

    /* Set the final size */
    if(pos > 0) {
        response->serversSize = pos;
    } else {
        UA_free(response->servers);
        response->servers = NULL;
    }
#endif

    if(request->endpointUrl.length > 0 && response->servers != NULL) {
        for(size_t i = 0; i < response->serversSize; i++) {
            UA_Array_delete(response->servers[i].discoveryUrls,
                            response->servers[i].discoveryUrlsSize,
                            &UA_TYPES[UA_TYPES_STRING]);
            response->servers[i].discoveryUrls = NULL;
            response->servers[i].discoveryUrlsSize = 0;
            response->responseHeader.serviceResult |=
                UA_Array_appendCopy((void**)&response->servers[i].discoveryUrls,
                                    &response->servers[i].discoveryUrlsSize,
                                    &request->endpointUrl, &UA_TYPES[UA_TYPES_STRING]);
        }
    }
}

#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
/* All filter criteria must be fulfilled in the list entry. The comparison is
 * case insensitive. Returns true if the entry matches the filter. */
static UA_Boolean
entryMatchesCapabilityFilter(size_t serverCapabilityFilterSize,
                             UA_String *serverCapabilityFilter,
                             serverOnNetwork *current) {
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
    serverOnNetwork *current;
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
#endif

static const UA_String UA_SECURITY_POLICY_BASIC256SHA256_URI =
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

UA_SecurityPolicy *
getDefaultEncryptedSecurityPolicy(UA_Server *server) {
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &server->config.securityPolicies[i];
        if(UA_String_equal(&UA_SECURITY_POLICY_BASIC256SHA256_URI, &sp->policyUri))
            return sp;
    }
    for(size_t i = server->config.securityPoliciesSize; i > 0; i--) {
        UA_SecurityPolicy *sp = &server->config.securityPolicies[i-1];
        if(!UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &sp->policyUri))
            return sp;
    }
    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_CLIENT,
                   "Could not find a SecurityPolicy with encryption for the "
                   "UserTokenPolicy. Using an unencrypted policy.");
    return server->config.securityPoliciesSize > 0 ?
        &server->config.securityPolicies[0]: NULL;
}

const char *securityModeStrs[4] = {"-invalid", "-none", "-sign", "-sign+encrypt"};

static UA_String
securityPolicyUriPostfix(const UA_String uri) {
    for(size_t i = 0; i < uri.length; i++) {
        if(uri.data[i] != '#')
            continue;
        UA_String postfix = {uri.length - i, &uri.data[i]};
        return postfix;
    }
    return uri;
}

static UA_StatusCode
updateEndpointUserIdentityToken(UA_Server *server, UA_EndpointDescription *ed) {
    /* Don't change the UserIdentityTokens if there are manually configured
     * entries */
    if(ed->userIdentityTokensSize > 0)
        return UA_STATUSCODE_GOOD;

    /* Copy the UserTokenPolicies from the AccessControl plugin, but only the matching ones to the securityPolicyUri.
     * TODO: Different instances of the AccessControl plugin per Endpoint */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < server->config.accessControl.userTokenPoliciesSize; i++) {
        UA_UserTokenPolicy *utp = &server->config.accessControl.userTokenPolicies[i];
        if(UA_String_equal(&ed->securityPolicyUri, &utp->securityPolicyUri)) {
             res = UA_Array_appendCopy((void**)&ed->userIdentityTokens,
                                &ed->userIdentityTokensSize,
                                utp,
                                &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        }
    }

    for(size_t i = 0; i < ed->userIdentityTokensSize; i++) {
        /* Use the securityPolicy of the SecureChannel. But not if the
         * SecureChannel is unencrypted and there is a non-anonymous token. */
        UA_UserTokenPolicy *utp = &ed->userIdentityTokens[i];
        UA_String_clear(&utp->securityPolicyUri);
        if((!server->config.allowNonePolicyPassword || ed->userIdentityTokens[i].tokenType != UA_USERTOKENTYPE_USERNAME) &&
           UA_String_equal(&ed->securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI) &&
           utp->tokenType != UA_USERTOKENTYPE_ANONYMOUS) {
            UA_SecurityPolicy *encSP = getDefaultEncryptedSecurityPolicy(server);
            if(encSP)
                res |= UA_String_copy(&encSP->policyUri, &utp->securityPolicyUri);
        }

        /* Append the SecurityMode and SecurityPolicy postfix to the PolicyId to
         * make it unique */
        UA_String postfix;
        if(utp->securityPolicyUri.length > 0)
            postfix = securityPolicyUriPostfix(utp->securityPolicyUri);
        else
            postfix = securityPolicyUriPostfix(ed->securityPolicyUri);
        size_t newLen = utp->policyId.length + postfix.length +
            strlen(securityModeStrs[ed->securityMode]);
        UA_Byte *newString = (UA_Byte*)UA_realloc(utp->policyId.data, newLen);
        if(!newString)
            continue;
        size_t pos = utp->policyId.length;
        memcpy(&newString[pos], securityModeStrs[ed->securityMode],
               strlen(securityModeStrs[ed->securityMode]));
        pos += strlen(securityModeStrs[ed->securityMode]);
        memcpy(&newString[pos], postfix.data, postfix.length);
        utp->policyId.data = newString;
        utp->policyId.length = newLen;
    }

    return res;
}

/* Also reused to create the EndpointDescription array in the CreateSessionResponse */
UA_StatusCode
setCurrentEndPointsArray(UA_Server *server, const UA_String endpointUrl,
                         UA_String *profileUris, size_t profileUrisSize,
                         UA_EndpointDescription **arr, size_t *arrSize) {
    /* Clone the endpoint for each discoveryURL? */
    size_t clone_times = 1;
    if(endpointUrl.length == 0)
        clone_times = server->config.applicationDescription.discoveryUrlsSize;

    /* Allocate the array */
    *arr = (UA_EndpointDescription*)
        UA_Array_new(server->config.endpointsSize * clone_times,
                     &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!*arr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    size_t pos = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t j = 0; j < server->config.endpointsSize; ++j) {
        /* Test if the supported binary profile shall be returned */
        UA_Boolean usable = (profileUrisSize == 0);
        if(!usable) {
            for(size_t i = 0; i < profileUrisSize; ++i) {
                if(!UA_String_equal(&profileUris[i], &server->config.endpoints[j].transportProfileUri))
                    continue;
                usable = true;
                break;
            }
        }
        if(!usable)
            continue;

        /* Copy into the results */
        for(size_t i = 0; i < clone_times; ++i) {
            /* Copy the endpoint with a current ApplicationDescription */
            UA_EndpointDescription *ed = &(*arr)[pos];
            retval |= UA_EndpointDescription_copy(&server->config.endpoints[j], ed);
            UA_ApplicationDescription_clear(&ed->server);
            retval |= UA_ApplicationDescription_copy(&server->config.applicationDescription,
                                                     &ed->server);

            /* Return the certificate for the SecurityPolicy. If the
             * SecureChannel is unencrypted, select the default encrypted
             * SecurityPolicy. */
            UA_SecurityPolicy *sp = getSecurityPolicyByUri(server, &ed->securityPolicyUri);
            if(!sp || UA_String_equal(&UA_SECURITY_POLICY_NONE_URI, &sp->policyUri))
                sp = getDefaultEncryptedSecurityPolicy(server);
            if(sp) {
                UA_ByteString_clear(&ed->serverCertificate);
                retval |= UA_String_copy(&sp->localCertificate, &ed->serverCertificate);
            }

            /* Set the User Identity Token list fromt the AccessControl plugin */
            retval |= updateEndpointUserIdentityToken(server, ed);

            /* Set the EndpointURL */
            UA_String_clear(&ed->endpointUrl);
            if(endpointUrl.length == 0) {
                retval |= UA_String_copy(&server->config.applicationDescription.
                                         discoveryUrls[i], &ed->endpointUrl);
            } else {
                /* Mirror back the requested EndpointUrl and also add it to the
                 * array of discovery urls */
                retval |= UA_String_copy(&endpointUrl, &ed->endpointUrl);
                retval |= UA_Array_appendCopy((void**)&ed->server.discoveryUrls,
                                              &ed->server.discoveryUrlsSize,
                                              &endpointUrl, &UA_TYPES[UA_TYPES_STRING]);
            }
            if(retval != UA_STATUSCODE_GOOD)
                goto error;

            pos++;
        }
    }

    *arrSize = pos;
    return UA_STATUSCODE_GOOD;

 error:
    UA_Array_delete(*arr, server->config.endpointsSize * clone_times,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    *arr = NULL;
    return retval;
}

void
Service_GetEndpoints(UA_Server *server, UA_Session *session,
                     const UA_GetEndpointsRequest *request,
                     UA_GetEndpointsResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* If the client expects to see a specific endpointurl, mirror it back. If
     * not, clone the endpoints with the discovery url of all networklayers. */
    if(request->endpointUrl.length > 0) {
        UA_LOG_DEBUG_SESSION(server->config.logging, session,
                             "Processing GetEndpointsRequest with endpointUrl "
                             UA_PRINTF_STRING_FORMAT, UA_PRINTF_STRING_DATA(request->endpointUrl));
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logging, session,
                             "Processing GetEndpointsRequest with an empty endpointUrl");
    }

    response->responseHeader.serviceResult =
        setCurrentEndPointsArray(server, request->endpointUrl,
                                 request->profileUris, request->profileUrisSize,
                                 &response->endpoints, &response->endpointsSize);

    /* Check if the ServerUrl is already present in the DiscoveryUrl array.
     * Add if not already there. */
    UA_SecureChannel *channel = session->channel;
    for(size_t i = 0; i < server->config.applicationDescription.discoveryUrlsSize; i++) {
        if(UA_String_equal(&channel->endpointUrl,
                           &server->config.applicationDescription.discoveryUrls[i])) {
            return;
        }
    }
    if(server->config.applicationDescription.discoveryUrls == NULL){
        server->config.applicationDescription.discoveryUrls = (UA_String*)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
        server->config.applicationDescription.discoveryUrlsSize = 0;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = UA_Array_appendCopy((void**)&server->config.applicationDescription.discoveryUrls,
                        &server->config.applicationDescription.discoveryUrlsSize,
                        &request->endpointUrl, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Error adding the ServerUrl to theDiscoverUrl list.");
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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return;

    if(server->config.applicationDescription.applicationType != UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        responseHeader->serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
        return;
    }

    /* Find the server from the request in the registered list */
    registeredServer *rs = NULL;
    LIST_FOREACH(rs, &dm->registeredServers, pointers) {
        if(UA_String_equal(&rs->registeredServer.serverUri, &requestServer->serverUri))
            break;
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
            UA_LOG_ERROR_SESSION(server->config.logging, session,
                                 "Cannot allocate memory for semaphore path. Out of memory.");
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        memcpy(filePath, requestServer->semaphoreFilePath.data,
               requestServer->semaphoreFilePath.length );
        filePath[requestServer->semaphoreFilePath.length] = '\0';
        if(!UA_fileExists( filePath )) {
            responseHeader->serviceResult = UA_STATUSCODE_BADSEMPAHOREFILEMISSING;
            UA_free(filePath);
            return;
        }
        UA_free(filePath);
#else
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_CLIENT,
                       "Ignoring semaphore file path. open62541 not compiled "
                       "with UA_ENABLE_DISCOVERY_SEMAPHORE=ON");
#endif
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled) {
        for(size_t i = 0; i < requestServer->discoveryUrlsSize; i++) {
            /* create TXT if is online and first index, delete TXT if is offline
             * and last index */
            UA_Boolean updateTxt = (requestServer->isOnline && i==0) ||
                (!requestServer->isOnline && i==requestServer->discoveryUrlsSize);
            UA_Discovery_updateMdnsForDiscoveryUrl(dm, mdnsServerName, mdnsConfig,
                                                   &requestServer->discoveryUrls[i],
                                                   requestServer->isOnline, updateTxt);
        }
    }
#endif

    if(!requestServer->isOnline) {
        // server is shutting down. Remove it from the registered servers list
        if(!rs) {
            // server not found, show warning
            UA_LOG_WARNING_SESSION(server->config.logging, session,
                                   "Could not unregister server %.*s. Not registered.",
                                   (int)requestServer->serverUri.length,
                                   requestServer->serverUri.data);
            responseHeader->serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
            return;
        }

        if(dm->registerServerCallback) {
            UA_UNLOCK(&server->serviceMutex);
            dm->registerServerCallback(requestServer,
                                       dm->registerServerCallbackData);
            UA_LOCK(&server->serviceMutex);
        }

        // server found, remove from list
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_clear(&rs->registeredServer);
        UA_free(rs);
        dm->registeredServersSize--;
        responseHeader->serviceResult = UA_STATUSCODE_GOOD;
        return;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!rs) {
        // server not yet registered, register it by adding it to the list
        UA_LOG_DEBUG_SESSION(server->config.logging, session,
                             "Registering new server: %.*s",
                             (int)requestServer->serverUri.length,
                             requestServer->serverUri.data);

        rs = (registeredServer*)UA_malloc(sizeof(registeredServer));
        if(!rs) {
            responseHeader->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        LIST_INSERT_HEAD(&dm->registeredServers, rs, pointers);
        dm->registeredServersSize++;
    } else {
        UA_RegisteredServer_clear(&rs->registeredServer);
    }

    // Always call the callback, if it is set. Previously we only called it if
    // it was a new register call. It may be the case that this endpoint
    // registered before, then crashed, restarts and registeres again. In that
    // case the entry is not deleted and the callback would not be called.
    if(dm->registerServerCallback) {
        UA_UNLOCK(&server->serviceMutex);
        dm->registerServerCallback(requestServer,
                                   dm->registerServerCallbackData);
        UA_LOCK(&server->serviceMutex);
    }

    // copy the data from the request into the list
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_RegisteredServer_copy(requestServer, &rs->registeredServer);
    rs->lastSeen = nowMonotonic;
    responseHeader->serviceResult = retval;
}

void Service_RegisterServer(UA_Server *server, UA_Session *session,
                            const UA_RegisterServerRequest *request,
                            UA_RegisterServerResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing RegisterServerRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    process_RegisterServer(server, session, &request->requestHeader, &request->server, 0,
                           NULL, &response->responseHeader, 0, NULL, 0, NULL);
}

void Service_RegisterServer2(UA_Server *server, UA_Session *session,
                            const UA_RegisterServer2Request *request,
                             UA_RegisterServer2Response *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing RegisterServer2Request");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    process_RegisterServer(server, session, &request->requestHeader, &request->server,
                           request->discoveryConfigurationSize, request->discoveryConfiguration,
                           &response->responseHeader, &response->configurationResultsSize,
                           &response->configurationResults, &response->diagnosticInfosSize,
                           response->diagnosticInfos);
}

#endif /* UA_ENABLE_DISCOVERY */
