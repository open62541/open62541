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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

/***************/
/* FindServers */
/***************/

static UA_StatusCode
setApplicationDescriptionFromRegisteredServer(size_t localeIdsSize,
                                              UA_String *localeIds,
                                              UA_ApplicationDescription *target,
                                              const UA_RegisteredServer *rs) {
    UA_StatusCode retval =  UA_STATUSCODE_GOOD;
    target->applicationType = rs->serverType;
    retval |= UA_String_copy(&rs->serverUri, &target->applicationUri);
    retval |= UA_String_copy(&rs->productUri, &target->productUri);
    retval |= UA_String_copy(&rs->gatewayServerUri, &target->gatewayServerUri);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* If the client requests a specific locale, select the corresponding server
     * name */
    if(localeIdsSize) {
        UA_Boolean appNameFound = false;
        for(size_t i = 0; i < localeIdsSize && !appNameFound; i++) {
            for(size_t j =0; j < rs->serverNamesSize; j++) {
                if(UA_String_equal(&localeIds[i],
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

        /* Server does not have the requested local, therefore we can select the
         * most suitable one */
        if(!appNameFound && rs->serverNamesSize) {
            retval = UA_LocalizedText_copy(&rs->serverNames[0],
                                           &target->applicationName);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    } else if(rs->serverNamesSize) {
        /* Just take the first name */
        retval = UA_LocalizedText_copy(&rs->serverNames[0],
                                       &target->applicationName);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* TODO: Where do we get the discoveryProfileUri for application data? */

    if(rs->discoveryUrlsSize > 0) {
        target->discoveryUrls = (UA_String *)
            UA_calloc(rs->discoveryUrlsSize, sizeof(UA_String));
        if(!target->discoveryUrls)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        target->discoveryUrlsSize = rs->discoveryUrlsSize;
        for(size_t i = 0; i < rs->discoveryUrlsSize; i++)
            retval |= UA_String_copy(&rs->discoveryUrls[i],
                                     &target->discoveryUrls[i]);
    }

    return retval;
}

#endif /* UA_ENABLE_DISCOVERY */

static UA_StatusCode
process_FindServers(UA_Server *server, UA_Session *session, UA_String endpointUrl,
                    size_t localeIdsSize, UA_String *localeIds,
                    size_t serverUrisSize, UA_String *serverUris,
                    size_t *outServersSize,
                    UA_ApplicationDescription **outServers) {
    UA_ServerConfig *sc = &server->config;
    UA_LOG_DEBUG_SESSION(sc->logging, session, "Processing FindServersRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Return the server itself? */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Boolean foundSelf = false;
    if(serverUrisSize) {
        for(size_t i = 0; i < serverUrisSize; i++) {
            if(UA_String_equal(&serverUris[i],
                               &sc->applicationDescription.applicationUri)) {
                foundSelf = true;
                break;
            }
        }
    } else {
        foundSelf = true;
    }

    /* Direct access to the out-variables */
    size_t serversSize;
    UA_ApplicationDescription *servers;

#ifndef UA_ENABLE_DISCOVERY
    if(!foundSelf)
        return UA_STATUSCODE_GOOD;
    res = UA_Array_copy(&sc->applicationDescription, 1, (void**)outServers,
                        &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    *outServersSize = 1;

    serversSize = 1;
    servers = *outServers;
#else
    /* Allocate enough memory, including memory for the "self" response */
    size_t maxResults = server->registeredServersSize + 1;
    *outServers = (UA_ApplicationDescription*)
        UA_Array_new(maxResults, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    if(!*outServers)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    servers = *outServers;

    size_t pos = 0;
    RegisteredServerRecord *current;

    /* Copy "self" ApplicationDescriptions into the response */
    if(foundSelf) {
        res = UA_ApplicationDescription_copy(&sc->applicationDescription,
                                             &servers[pos]);
        pos++;
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Copy registered ApplicationDescriptions into the response */
    LIST_FOREACH(current, &server->registeredServers, pointers) {
        UA_Boolean usable = (serverUrisSize == 0);
        if(!usable) {
            /* If client only requested a specific set of servers */
            for(size_t i = 0; i < serverUrisSize; i++) {
                if(UA_String_equal(&current->registeredServer.serverUri,
                                   &serverUris[i])) {
                    usable = true;
                    break;
                }
            }
        }

        if(!usable)
            continue;

        res |=
            setApplicationDescriptionFromRegisteredServer(localeIdsSize, localeIds,
                                                          &servers[pos],
                                                          &current->registeredServer);
        pos++;
    }

 cleanup:

    /* Set the final size. This can be less than the initially allocated memory.
     * But if the size is zero, then we need to free. */
    if(res != UA_STATUSCODE_GOOD || pos == 0) {
        UA_Array_delete(*outServers, pos, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        *outServers = NULL;
        *outServersSize = 0;
        return res;
    }

    *outServersSize = pos;
    serversSize = pos;
#endif /* UA_ENABLE_DISCOVERY */

    /* Mirror back the expected EndpointUrl */
    if(endpointUrl.length > 0) {
        for(size_t i = 0; i < serversSize; i++) {
            UA_ApplicationDescription *ad = &servers[i];
            UA_Array_delete(ad->discoveryUrls, ad->discoveryUrlsSize,
                            &UA_TYPES[UA_TYPES_STRING]);
            ad->discoveryUrls = NULL;
            ad->discoveryUrlsSize = 0;
            res = UA_Array_copy(&endpointUrl, 1, (void**)&ad->discoveryUrls,
                                &UA_TYPES[UA_TYPES_STRING]);
            if(res != UA_STATUSCODE_GOOD)
                break;
            ad->discoveryUrlsSize = 1;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_Boolean
Service_FindServers(UA_Server *server, UA_Session *session,
                    const UA_FindServersRequest *request,
                    UA_FindServersResponse *response) {
    response->responseHeader.serviceResult =
        process_FindServers(server, session, request->endpointUrl,
                            request->localeIdsSize, request->localeIds,
                            request->serverUrisSize, request->serverUris,
                            &response->serversSize, &response->servers);
    return true;
}

UA_StatusCode
UA_Server_findServers(UA_Server *server, UA_String endpointUrl,
                      size_t localeIdsSize, UA_LocaleId *localeIds,
                      size_t serverUrisSize, UA_String *serverUris,
                      size_t *outServersSize,
                      UA_ApplicationDescription **outServers) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    lockServer(server);
    res = process_FindServers(server, &server->adminSession, endpointUrl,
                              localeIdsSize, localeIds,
                              serverUrisSize, serverUris,
                              outServersSize, outServers);
    unlockServer(server);
    return res;
}

/************************/
/* FindServersOnNetwork */
/************************/

#if defined(UA_ENABLE_DISCOVERY)

static void
notifyServerOnNetwork(UA_Server *server,
                      const UA_ServerOnNetwork *son,
                      const UA_KeyValueMap params,
                      UA_Boolean added, UA_Boolean updated,
                      UA_Boolean removed) {
    UA_STATIC_THREAD_LOCAL UA_KeyValuePair kvp[6] = {
        {{0, UA_STRING_STATIC("server-on-network")}, {0}},
        {{0, UA_STRING_STATIC("remote-address")}, {0}},
        {{0, UA_STRING_STATIC("ttl")}, {0}},
        {{0, UA_STRING_STATIC("server-added")}, {0}},
        {{0, UA_STRING_STATIC("server-updated")}, {0}},
        {{0, UA_STRING_STATIC("server-removed")}, {0}},
    };

    UA_String remote = UA_STRING_NULL;
    UA_UInt32 ttl = 0;

    const UA_String *params_remote = (const UA_String*)
        UA_KeyValueMap_getScalar(&params,
                                 UA_QUALIFIEDNAME(0, "remote-address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(params_remote)
        remote = *params_remote;

    const UA_UInt32 *ttl_remote = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&params, UA_QUALIFIEDNAME(0, "ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(ttl_remote)
        ttl= *ttl_remote;

    UA_Variant_setScalar(&kvp[0].value, (void*)(uintptr_t)son,
                         &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    UA_Variant_setScalar(&kvp[1].value, &remote, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&kvp[2].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&kvp[3].value, &added, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[4].value, &updated, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[5].value, &removed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {6, kvp};

    notifyApplication(server,
                      UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK,
                      kvm);

    /* Specialized application callback for discovery notifications. Called
     * directly (not via notifyApplication) to avoid recursion: the driver's
     * own notification callback may itself trigger a register/deregister
     * that flows back into notifyServerOnetwork. */
    UA_ServerConfig *config = &server->config;
    if(config->discoveryNotificationCallback)
        config->discoveryNotificationCallback(server,
                                              UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK,
                                              kvm);
}

static void
resetDiscoveryResetIds(UA_Server *server) {
    /* Update the reset time */
    UA_EventLoop *el = server->config.eventLoop;
    server->lastCounterResetTime = el->dateTime_now(el);

    /* Give a new record id to all entries */
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i];
        son->recordId = ++server->serversOnNetworkRecordCounter;
    }
}

/* All filter criteria must be fulfilled in the list entry. The comparison is
 * case insensitive. Returns true if the entry matches the filter. */
static UA_Boolean
entryMatchesCapabilityFilter(size_t serverCapabilityFilterSize,
                             const UA_String *serverCapabilityFilter,
                             UA_ServerOnNetwork *current) {
    /* If the entry has less capabilities defined than the filter, there's no
     * match */
    if(serverCapabilityFilterSize > current->serverCapabilitiesSize)
        return false;
    for(size_t i = 0; i < serverCapabilityFilterSize; i++) {
        UA_Boolean capabilityFound = false;
        for(size_t j = 0; j < current->serverCapabilitiesSize; j++) {
            if(UA_String_equal_ignorecase(&serverCapabilityFilter[i],
                               &current->serverCapabilities[j])) {
                capabilityFound = true;
                break;
            }
        }
        if(!capabilityFound)
            return false;
    }
    return true;
}

static UA_StatusCode
process_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                             UA_UInt32 startingRecordId,
                             UA_UInt32 maxRecordsToReturn,
                             size_t serverCapabilityFilterSize,
                             const UA_String *serverCapabilityFilter,
                             UA_DateTime *outLastCounterResetTime,
                             size_t *outServersSize,
                             UA_ServerOnNetwork **outServers) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(!server->config.serversOnNetworkEnabled)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Set LastCounterResetTime */
    *outLastCounterResetTime = server->lastCounterResetTime;

    /* Skip records under the threshold */
    size_t recordOffset = 0;
    for(; recordOffset < server->serversOnNetworkSize; recordOffset++) {
        if(server->serversOnNetwork[recordOffset].recordId > startingRecordId)
            break;
    }

    /* Compute the max number of records to return */
    size_t recordCount = server->serversOnNetworkSize - recordOffset;
    if(maxRecordsToReturn > 0 && maxRecordsToReturn < recordCount)
        recordCount = maxRecordsToReturn;
    UA_assert(recordCount <= server->serversOnNetworkSize);

    /* Nothing to do */
    if(recordCount == 0)
        return UA_STATUSCODE_GOOD;

    /* Iterate over all records and add to filtered list */
    UA_UInt32 filteredCount = 0;
    UA_STACKARRAY(UA_ServerOnNetwork*, filtered, recordCount);
    for(size_t i = 0; i < recordCount; i++) {
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i + recordOffset];
        if(!entryMatchesCapabilityFilter(serverCapabilityFilterSize,
                                         serverCapabilityFilter, son))
            continue;
        filtered[filteredCount++] = son;
    }

    /* Nothing to do */
    if(filteredCount == 0)
        return UA_STATUSCODE_GOOD;

    /* Allocate the array for the response */
    *outServers = (UA_ServerOnNetwork*)
        UA_calloc(filteredCount, sizeof(UA_ServerOnNetwork));
    if(!*outServers)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *outServersSize = filteredCount;

    /* Copy the server entries */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < filteredCount; i++) {
        res |= UA_ServerOnNetwork_copy(filtered[i], &(*outServers)[i]);
    }

    /* Clean up the array if copying failed */
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(*outServers, *outServersSize,
                        &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
        *outServers = NULL;
        *outServersSize = 0;
    }

    return res;
}

UA_Boolean
Service_FindServersOnNetwork(UA_Server *server, UA_Session *session,
                             const UA_FindServersOnNetworkRequest *request,
                             UA_FindServersOnNetworkResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    response->responseHeader.serviceResult =
        process_FindServersOnNetwork(server, session,
                                     request->startingRecordId,
                                     request->maxRecordsToReturn,
                                     request->serverCapabilityFilterSize,
                                     request->serverCapabilityFilter,
                                     &response->lastCounterResetTime,
                                     &response->serversSize,
                                     &response->servers);
    return true;
}

UA_StatusCode
UA_Server_findServersOnNetwork(UA_Server *server, UA_String endpointUrl,
                               UA_UInt32 startingRecordId,
                               UA_UInt32 maxRecordsToReturn,
                               size_t serverCapabilityFilterSize,
                               const UA_String *serverCapabilityFilter,
                               UA_DateTime *outLastCounterResetTime,
                               size_t *outServersSize,
                               UA_ServerOnNetwork **outServers) {
    lockServer(server);
    UA_StatusCode res =
        process_FindServersOnNetwork(server, &server->adminSession, startingRecordId,
                                     maxRecordsToReturn, serverCapabilityFilterSize,
                                     serverCapabilityFilter, outLastCounterResetTime,
                                     outServersSize, outServers);
    unlockServer(server);
    return res;
}

static UA_ServerOnNetwork *
findServerOnNetworkIndex(UA_Server *server,
                         const UA_String serverName) {
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        /* Check matching record */
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i];
        if(!UA_String_equal(&son->serverName, &serverName))
            continue;
        return son;
    }
    return NULL;
}

UA_StatusCode
UA_Server_registerServerOnNetwork(UA_Server *server,
                                  const UA_ServerOnNetwork *newSon,
                                  const UA_KeyValueMap params) {
    if(!newSon)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(!server->config.serversOnNetworkEnabled)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    lockServer(server);

    /* Search a matching entry */
    UA_StatusCode res;
    UA_ServerOnNetwork *son =
        findServerOnNetworkIndex(server, newSon->serverName);

    /* Update existing */
    if(son) {
        /* Make a copy */
        UA_ServerOnNetwork tmp;
        res = UA_ServerOnNetwork_copy(newSon, &tmp);
        if(res != UA_STATUSCODE_GOOD) {
            unlockServer(server);
            return res;
        }

        /* Replace the previous entry */
        UA_ServerOnNetwork_clear(son);
        *son = tmp;

        /* Notify the application about an updated entry */
        notifyServerOnNetwork(server, son, params, false, true, false);

        /* All new RecordIds in increasing order */
        resetDiscoveryResetIds(server);

        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Append to the list */
    res = UA_Array_appendCopy((void**)&server->serversOnNetwork,
                              &server->serversOnNetworkSize, newSon,
                              &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Increase the record id counter and use it */
    son = &server->serversOnNetwork[server->serversOnNetworkSize-1];
    son->recordId = ++server->serversOnNetworkRecordCounter;

    /* Notify the application about an added entry */
    notifyServerOnNetwork(server, son, params, true, false, false);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_deregisterServerOnNetwork(UA_Server *server,
                                    const UA_String serverName) {
    lockServer(server);

    if(!server->config.serversOnNetworkEnabled)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Find the entry */
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i];
        if(!UA_String_equal(&serverName, &son->serverName))
            continue;

        /* Notify the application about removed entry */
        notifyServerOnNetwork(server, son, UA_KEYVALUEMAP_NULL,
                              false, false, true);

        /* Move the last entry in the current position */
        UA_ServerOnNetwork_clear(son);
        server->serversOnNetworkSize--;
        if(i != server->serversOnNetworkSize) {
            server->serversOnNetwork[i] =
                server->serversOnNetwork[server->serversOnNetworkSize];
        }

        /* Free the array if the last entry was removed */
        if(server->serversOnNetworkSize == 0) {
            UA_free(server->serversOnNetwork);
            server->serversOnNetwork = NULL;
        }

        /* All new RecordIds in increasing order */
        resetDiscoveryResetIds(server);
        break;
    }

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

#endif /* if defined(UA_ENABLE_DISCOVERY) */

/****************/
/* GetEndpoints */
/****************/

static UA_String basic256Sha256Uri =
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

/* Get an encrypted policy or NULL if no encrypted policy is defined */
UA_SecurityPolicy *
getDefaultEncryptedSecurityPolicy(UA_Server *server,
                                  UA_SecurityPolicyType type) {
    UA_SecurityPolicy *best = NULL;
    UA_Byte securityLevel = 0;

    (void)type;
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &server->config.securityPolicies[i];
        if(sp->policyType == UA_SECURITYPOLICYTYPE_NONE)
            continue;
        /* This SecurityPolicy is used to secure a UserIdentityToken on top of a
         * #None SecureChannel (the only situation this function is called for).
         * ECC and RSA-DH policies use ephemeral key agreement that must be
         * bound to a secured SecureChannel - they must never be used to secure
         * an auth token over #None. Only the static-RSA encryption policies
         * (e.g. Basic256Sha256, Aes*_RsaOaep/RsaPss) qualify. */
        if(UA_SecurityPolicy_isEcc(sp) || UA_SecurityPolicy_isEnhancedSecurity(sp))
            continue;
        /* Return early with Basic256Sha256 when available. "Secure enough" and
         * most clients support it.*/
        if(UA_String_equal(&basic256Sha256Uri, &sp->policyUri))
            return sp;
        if(sp->securityLevel >= securityLevel) {
            best = sp;
            securityLevel = sp->securityLevel;
        }
    }
    return best;
}

static const char *securityModeStrs[4] = {"-invalid", "-none", "-sign", "-sign+encrypt"};

UA_String
securityPolicyUriPostfix(const UA_String uri) {
    if(uri.length == 0) return uri;
    for(UA_Byte *b = uri.data + uri.length - 1; b >= uri.data; b--) {
        if(*b != '#')
            continue;
        UA_String postfix = {uri.length - (size_t)(b - uri.data), b};
        return postfix;
    }
    return uri;
}

static UA_StatusCode
updateEndpointUserIdentityToken(UA_Server *server,
                                UA_SecurityPolicyType policyType,
                                UA_EndpointDescription *ed) {
    /* Don't modify the UserIdentityTokens if there are manually configured
     * entries */
    if(ed->userIdentityTokensSize > 0)
        return UA_STATUSCODE_GOOD;

    /* Copy the UserTokenPolicies from the AccessControl plugin, but only the
     * matching ones to the securityPolicyUri.
     * TODO: Different instances of the AccessControl plugin per Endpoint */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_ServerConfig *sc = &server->config;
    for(size_t i = 0; i < sc->accessControl.userTokenPoliciesSize; i++) {
        UA_UserTokenPolicy *utp = &sc->accessControl.userTokenPolicies[i];

        /* Append the UserTokenPolicy from the AccesssControl plugin */
        res = UA_Array_appendCopy((void**)&ed->userIdentityTokens,
                                  &ed->userIdentityTokensSize, utp,
                                  &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
        if(res != UA_STATUSCODE_GOOD)
            return res;

        /* Now we modify the freshly copied last entry and ignore whatever
         * SecurityPolicy was set in sc->accessControl.userTokenPolicies and
         * choose something appropriate. If empty, the SecurityPolicy of the
         * SecureChannel is used. */
        utp = &ed->userIdentityTokens[ed->userIdentityTokensSize - 1];
        UA_String_clear(&utp->securityPolicyUri);

#ifdef UA_ENABLE_ENCRYPTION
        /* Anonymous tokens don't need encryption. All other tokens require
         * encryption with the exception of Username/Password if also the
         * allowNonePolicyPassword option has been set. The same logic is used
         * in selectEndpointAndTokenPolicy (ua_services_session.c). */
        if(utp->tokenType != UA_USERTOKENTYPE_ANONYMOUS &&
           UA_String_equal(&ed->securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI) &&
           (!sc->allowNonePolicyPassword || utp->tokenType != UA_USERTOKENTYPE_USERNAME)) {
            /* Use the SecurityPolicy for the SecureChannel also for the
             * username/password. Otherwise pick the "bëst" SecurityPolicÿ. */
            UA_SecurityPolicy *encSP;
            if(ed->securityMode == UA_MESSAGESECURITYMODE_NONE)
                encSP = getDefaultEncryptedSecurityPolicy(server, policyType);
            else
                encSP = getSecurityPolicyByUri(server, &ed->securityPolicyUri);
            if(!encSP) {
                /* No encrypted SecurityPolicy available */
                UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_CLIENT,
                               "Removing a UserTokenPolicy that would allow the "
                               "password to be transmitted without encryption "
                               "(Can be enabled via config->allowNonePolicyPassword)");
                UA_StatusCode res2 =
                    UA_Array_resize((void **)&ed->userIdentityTokens,
                                    &ed->userIdentityTokensSize,
                                    ed->userIdentityTokensSize - 1,
                                    &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
                (void)res2;
                continue;
            }
            res |= UA_String_copy(&encSP->policyUri, &utp->securityPolicyUri);
        }
#endif

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

/* Also reused to create the EndpointDescription array in the
 * CreateSessionResponse */
UA_StatusCode
setCurrentEndpointsArray(UA_Server *server, const UA_String endpointUrl,
                         UA_String *profileUris, size_t profileUrisSize,
                         UA_EndpointDescription **arr, size_t *arrSize) {
    UA_ServerConfig *sc = &server->config;

    /* Clone the endpoint for each discoveryURL? */
    size_t clone_times = 1;
    if(endpointUrl.length == 0)
        clone_times = sc->applicationDescription.discoveryUrlsSize;

    /* Allocate the array */
    *arr = (UA_EndpointDescription*)
        UA_Array_new(sc->endpointsSize * clone_times,
                     &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!*arr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    size_t pos = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t j = 0; j < sc->endpointsSize; ++j) {
        const UA_EndpointDescription *ep = &sc->endpoints[j];

        /* Test if the supported binary profile shall be returned */
        UA_Boolean usable = (profileUrisSize == 0);
        if(!usable) {
            for(size_t i = 0; i < profileUrisSize; ++i) {
                if(!UA_String_equal(&profileUris[i], &ep->transportProfileUri))
                    continue;
                usable = true;
                break;
            }
        }
        if(!usable)
            continue;

        /* Get the SecurityPolicy */
        UA_SecurityPolicy *sp =
            getSecurityPolicyByUri(server, &ep->securityPolicyUri);
        if(!sp) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "GetEndpoints: Endpoint defines SecurityPolicy "
                           "%S which is not available", ep->securityPolicyUri);
            continue;
        }

        /* Copy into the results */
        for(size_t i = 0; i < clone_times; ++i) {
            /* Copy the endpoint with a current ApplicationDescription */
            UA_EndpointDescription *ed = &(*arr)[pos];
            retval |= UA_EndpointDescription_copy(&sc->endpoints[j], ed);
            UA_ApplicationDescription_clear(&ed->server);
            retval |= UA_ApplicationDescription_copy(&sc->applicationDescription,
                                                     &ed->server);

            /* Set the local certificate configured for the SecurityPolicy */
            UA_ByteString_clear(&ed->serverCertificate);
            retval |= UA_ByteString_copy(&sp->localCertificate,
                                         &ed->serverCertificate);

            /* Set the User Identity Token list from the AccessControl plugin.
             * This also selects an appropriate SecurityPolicy for the
             * AuthenticationToken. */
            retval |= updateEndpointUserIdentityToken(server, sp->policyType, ed);

            /* OPC UA Part 4 §5.4.2:
             *
             * If the endpoint uses None security but a token policy requires
             * encryption, the client needs a certificate to encrypt the token.
             * Set serverCertificate from the first token policy's encryption
             * SecurityPolicy so the client can encrypt the credential. */
            if(ed->serverCertificate.length == 0) {
                for(size_t ti = 0; ti < ed->userIdentityTokensSize; ti++) {
                    UA_UserTokenPolicy *utp = &ed->userIdentityTokens[ti];
                    if(utp->securityPolicyUri.length == 0)
                        continue;
                    UA_SecurityPolicy *encSP =
                        getSecurityPolicyByUri(server, &utp->securityPolicyUri);
                    if(!encSP || encSP->localCertificate.length == 0)
                        continue;
                    retval |= UA_ByteString_copy(&encSP->localCertificate,
                                                 &ed->serverCertificate);
                    break;
                }
            }

            /* Set the EndpointURL */
            UA_String_clear(&ed->endpointUrl);
            if(endpointUrl.length == 0) {
                retval |= UA_String_copy(&sc->applicationDescription.discoveryUrls[i],
                                         &ed->endpointUrl);
            } else {
                /* Mirror back the requested EndpointUrl and also add it to the
                 * array of discovery urls */
                retval |= UA_String_copy(&endpointUrl, &ed->endpointUrl);

                /* Check if the ServerUrl is already present in the DiscoveryUrl
                 * array */
                size_t k = 0;
                for(; k < ed->server.discoveryUrlsSize; k++) {
                    if(UA_String_equal(&ed->endpointUrl, &ed->server.discoveryUrls[k]))
                        break;
                }
                if(k == ed->server.discoveryUrlsSize) {
                    retval |= UA_Array_appendCopy((void **)&ed->server.discoveryUrls,
                                                  &ed->server.discoveryUrlsSize,
                                                  &endpointUrl,
                                                  &UA_TYPES[UA_TYPES_STRING]);
                }
            }
            if(retval != UA_STATUSCODE_GOOD)
                goto error;

            pos++;
        }
    }

    *arrSize = pos;
    return UA_STATUSCODE_GOOD;

 error:
    UA_Array_delete(*arr, sc->endpointsSize * clone_times,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    *arr = NULL;
    return retval;
}

UA_Boolean
Service_GetEndpoints(UA_Server *server, UA_Session *session,
                     const UA_GetEndpointsRequest *request,
                     UA_GetEndpointsResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing GetEndpointsRequest with endpointUrl %S",
                         request->endpointUrl);

    /* If the client expects to see a specific endpointurl, mirror it back. If
     * not, clone the endpoints with the discovery url of all networklayers. */
    response->responseHeader.serviceResult =
        setCurrentEndpointsArray(server, request->endpointUrl,
                                 request->profileUris, request->profileUrisSize,
                                 &response->endpoints, &response->endpointsSize);
    return true;
}

/******************/
/* RegisterServer */
/******************/

#ifdef UA_ENABLE_DISCOVERY

static void
notifyRegisteredServer(UA_Server *server, UA_Session *session,
                       UA_SecureChannel *channel,
                       const UA_RegisteredServer *requestServer,
                       size_t discoveryConfigSize,
                       const UA_ExtensionObject *discoveryConfig,
                       UA_Boolean added, UA_Boolean updated,
                       UA_Boolean removed) {
    UA_STATIC_THREAD_LOCAL UA_KeyValuePair kvp[7] = {
        {{0, UA_STRING_STATIC("registered-server")}, {0}},
        {{0, UA_STRING_STATIC("discovery-configuration")}, {0}},
        {{0, UA_STRING_STATIC("server-added")}, {0}},
        {{0, UA_STRING_STATIC("server-updated")}, {0}},
        {{0, UA_STRING_STATIC("server-removed")}, {0}},
        {{0, UA_STRING_STATIC("securechannel-id")}, {0}},
        {{0, UA_STRING_STATIC("session-id")}, {0}},
    };

    UA_NodeId sessionId = (session) ? session->sessionId : UA_NODEID_NULL;
    UA_UInt32 channelId = (channel) ? channel->securityToken.channelId : 0;

    UA_Variant_setScalar(&kvp[0].value, (void*)(uintptr_t)requestServer,
                         &UA_TYPES[UA_TYPES_REGISTEREDSERVER]);
    UA_Variant_setArray(&kvp[1].value, (void*)(uintptr_t)discoveryConfig,
                        discoveryConfigSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    UA_Variant_setScalar(&kvp[2].value, &added, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[3].value, &updated, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[4].value, &removed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[5].value, &channelId, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&kvp[6].value, &sessionId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_KeyValueMap kvm = {7, kvp};

    notifyApplication(server,
                      UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_REGISTERSERVER,
                      kvm);
}

/* Match the server name and at least one discovery uri */
static RegisteredServerRecord *
findRegisteredServer(UA_Server *server, const UA_String serverUri,
                     size_t discoveryUrlsSize, const UA_String *discoveryUrls) {
    RegisteredServerRecord *rs = NULL;
    LIST_FOREACH(rs, &server->registeredServers, pointers) {
        if(!UA_String_equal(&rs->registeredServer.serverUri, &serverUri))
            continue;

        for(size_t i = 0; i < rs->registeredServer.discoveryUrlsSize; i++) {
            const UA_String *du1 = &rs->registeredServer.discoveryUrls[i];
            for(size_t j = 0; j < discoveryUrlsSize; j++) {
                const UA_String *du2 = &discoveryUrls[j];
                if(UA_String_equal(du1, du2))
                    return rs;
            }
        }
    }
    return NULL;
}

static UA_StatusCode
process_RegisterServer(UA_Server *server, UA_Session *session,
                       const UA_RegisteredServer *requestServer,
                       const size_t requestDiscoveryConfigurationSize,
                       const UA_ExtensionObject *requestDiscoveryConfiguration,
                       size_t *responseConfigurationResultsSize,
                       UA_StatusCode **responseConfigurationResults,
                       UA_Boolean allocateResponseConfigResults) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Only support RegisterServer on discovery servers */
    UA_ServerConfig *sc = &server->config;
    if(sc->applicationDescription.applicationType != UA_APPLICATIONTYPE_DISCOVERYSERVER)
        return UA_STATUSCODE_BADSERVICEUNSUPPORTED;

    /* Check the presence of at least one DiscoveryUrl */
    if(requestServer->discoveryUrlsSize == 0)
        return UA_STATUSCODE_BADDISCOVERYURLMISSING;

    /* If a semaphore file path is defined, check that the file exists */
    if(requestServer->semaphoreFilePath.length) {
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        char filePath[256];
        if(requestServer->semaphoreFilePath.length >= 255)
            return UA_STATUSCODE_BADINTERNALERROR;
        memcpy(filePath, requestServer->semaphoreFilePath.data,
               requestServer->semaphoreFilePath.length );
        filePath[requestServer->semaphoreFilePath.length] = '\0';
        if(!UA_fileExists(filePath))
            return UA_STATUSCODE_BADSEMAPHOREFILEMISSING;
#else
        UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_CLIENT,
                       "Ignoring semaphore file path. open62541 not compiled "
                       "with UA_ENABLE_DISCOVERY_SEMAPHORE=ON");
#endif
    }

    /* Process the requestDiscoveryConfiguration to get the mDNS config and
     * server name */
    const UA_String *mdnsServerName = NULL;
    static const UA_DataType *mdnsConfigType =
        &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION];
    UA_MdnsDiscoveryConfiguration *mdnsConfig = NULL;
    if(requestDiscoveryConfigurationSize > 0) {
        if(allocateResponseConfigResults) {
            *responseConfigurationResults = (UA_StatusCode *)
                UA_Array_new(requestDiscoveryConfigurationSize,
                             &UA_TYPES[UA_TYPES_STATUSCODE]);
            if(!*responseConfigurationResults)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            *responseConfigurationResultsSize = requestDiscoveryConfigurationSize;
        }
        for(size_t i = 0; i < requestDiscoveryConfigurationSize; i++) {
            const UA_ExtensionObject *eo = &requestDiscoveryConfiguration[i];
            if(!UA_ExtensionObject_hasDecodedType(eo, mdnsConfigType)) {
                if(*responseConfigurationResults)
                    (*responseConfigurationResults)[i] = UA_STATUSCODE_BADNOTSUPPORTED;
                continue;
            }
            if(*responseConfigurationResults)
                (*responseConfigurationResults)[i] = UA_STATUSCODE_GOOD;
            mdnsConfig = (UA_MdnsDiscoveryConfiguration *)eo->content.decoded.data;
            if(!mdnsServerName)
                mdnsServerName = &mdnsConfig->mdnsServerName;
        }
    }

    /* Override the mDNS server name with the first "normal" name */
    if(!mdnsServerName && requestServer->serverNamesSize > 0)
        mdnsServerName = &requestServer->serverNames[0].text;
    if(!mdnsServerName)
        return UA_STATUSCODE_BADSERVERNAMEMISSING;

    /* Find the server from the request in the record list */
    RegisteredServerRecord *rs =
        findRegisteredServer(server, requestServer->serverUri,
                             requestServer->discoveryUrlsSize,
                             requestServer->discoveryUrls);

    /* Server is shutting down. Remove it from the registered servers list */
    if(!requestServer->isOnline) {
        if(!rs) {
            UA_LOG_WARNING_SESSION(sc->logging, session,
                                   "Could not unregister unknown server %S",
                                   requestServer->serverUri);
            return UA_STATUSCODE_BADNOTHINGTODO;
        }

        /* Notify the application */
        notifyRegisteredServer(server, session, session->channel, requestServer, 
                               requestDiscoveryConfigurationSize,
                               requestDiscoveryConfiguration,
                               false, false, true);

        /* Remove server record */
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_clear(&rs->registeredServer);
        UA_free(rs);
        server->registeredServersSize--;
        return UA_STATUSCODE_GOOD;
    }

    /* Make a copy of the RegisteredServer structure */
    UA_RegisteredServer tmpServer;
    UA_StatusCode res =
        UA_RegisteredServer_copy(requestServer, &tmpServer);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(!rs) {
        UA_LOG_DEBUG_SESSION(sc->logging, session,
                             "Registering new server: %S",
                             requestServer->serverUri);

        /* Allocate memory for the record */
        rs = (RegisteredServerRecord*)
            UA_calloc(1, sizeof(RegisteredServerRecord));
        if(!rs) {
            UA_RegisteredServer_clear(&tmpServer);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        /* Notify the application */
        notifyRegisteredServer(server, session, session->channel, &tmpServer,
                               requestDiscoveryConfigurationSize,
                               requestDiscoveryConfiguration,
                               true, false, false);

        /* Add new server to the list */
        LIST_INSERT_HEAD(&server->registeredServers, rs, pointers);
        server->registeredServersSize++;
    } else {
        /* Clear the data of the previous record */
        UA_RegisteredServer_clear(&rs->registeredServer);

        /* Notify the application about the update */
        notifyRegisteredServer(server, session, session->channel, &tmpServer,
                               requestDiscoveryConfigurationSize,
                               requestDiscoveryConfiguration,
                               false, true, false);
    }

    /* Prepare / Update the server record */
    UA_EventLoop *el = sc->eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    rs->lastSeen = nowMonotonic;
    rs->registeredServer = tmpServer;

    return UA_STATUSCODE_GOOD;
}

UA_Boolean
Service_RegisterServer(UA_Server *server, UA_Session *session,
                       const UA_RegisterServerRequest *request,
                       UA_RegisterServerResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing RegisterServerRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);
    response->responseHeader.serviceResult =
        process_RegisterServer(server, session, &request->server,
                               0, NULL, 0, NULL, true);
    return true;
}

UA_Boolean
Service_RegisterServer2(UA_Server *server, UA_Session *session,
                        const UA_RegisterServer2Request *request,
                        UA_RegisterServer2Response *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing RegisterServer2Request");
    UA_LOCK_ASSERT(&server->serviceMutex);
    response->responseHeader.serviceResult =
        process_RegisterServer(server, session, &request->server,
                               request->discoveryConfigurationSize,
                               request->discoveryConfiguration,
                               &response->configurationResultsSize,
                               &response->configurationResults, true);
    return true;
}

UA_StatusCode
UA_Server_registerServer(UA_Server *server,
                         const UA_RegisteredServer *registeredServer,
                         const size_t discoveryConfigurationSize,
                         const UA_ExtensionObject *discoveryConfiguration,
                         UA_StatusCode *configurationResults) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    lockServer(server);
    res = process_RegisterServer(server, &server->adminSession,
                                 registeredServer,
                                 discoveryConfigurationSize,
                                 discoveryConfiguration,
                                 NULL, &configurationResults, false);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_deregisterServer(UA_Server *server, const UA_String serverUri,
                           size_t discoveryUrlsSize,
                           const UA_String *discoveryUrls) {
    lockServer(server);

    /* Find the server from the request in the record list */
    RegisteredServerRecord *rs =
        findRegisteredServer(server, serverUri, discoveryUrlsSize, discoveryUrls);
    if(!rs) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Notify the application */
    notifyRegisteredServer(server,  &server->adminSession, NULL,
                           &rs->registeredServer, 0, NULL,
                           false, false, true);

    /* Remove server record */
    LIST_REMOVE(rs, pointers);
    UA_RegisteredServer_clear(&rs->registeredServer);
    UA_free(rs);
    server->registeredServersSize--;

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/* If the semaphore file path is set, then it just checks the existence of the
 * file. When the file is deleted, the registration is removed. If no semaphore
 * file is defined, then the registration will be removed if it is older than
 * the cleanup timeout. */
void
cleanupRegisteredServers(UA_Server *server) {
    /* TimedOut gives the last DateTime at which we must have seen the
     * registered server. Otherwise it is timed out. */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime timedOut = el->dateTime_nowMonotonic(el);
    if(server->config.registeredServerCleanupTimeout)
        timedOut -= server->config.registeredServerCleanupTimeout * UA_DATETIME_SEC;

    RegisteredServerRecord *current, *temp;
    LIST_FOREACH_SAFE(current, &server->registeredServers, pointers, temp) {
        UA_RegisteredServer *rs = &current->registeredServer;

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        if(rs->semaphoreFilePath.length) {
            /* Check the semaphore file */
            char filePath[256];
            UA_assert(rs->semaphoreFilePath.length < 255);
            memcpy(filePath, rs->semaphoreFilePath.data,
                   rs->semaphoreFilePath.length);
            filePath[rs->semaphoreFilePath.length] = '\0';
            if(UA_fileExists(filePath))
                continue; /* Semaphore file still exists */
            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                        "Registration of server with Uri %S is removed because "
                        "the semaphore file '%S' was deleted",
                        rs->serverUri, rs->semaphoreFilePath);
        } else
#endif
        {
            /* Check the timeout */
            if(server->config.registeredServerCleanupTimeout == 0)
                continue; /* No cleanup timeout configured */
            if(current->lastSeen > timedOut)
                continue; /* Server was recently seen */
            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                        "Registration of server with Uri %S has timed out "
                        "and is removed", rs->serverUri);
        }

        /* Notify the application */
        notifyRegisteredServer(server, &server->adminSession, NULL, rs,
                               0, NULL, false, false, true);

        /* Remove the record */
        LIST_REMOVE(current, pointers);
        UA_RegisteredServer_clear(rs);
        UA_free(current);
        server->registeredServersSize--;
    }
}

#endif /* UA_ENABLE_DISCOVERY */
