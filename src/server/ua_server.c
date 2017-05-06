/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"
#include "ua_nodestore_standard.h"

#ifdef UA_ENABLE_GENERATE_NAMESPACE0
#include "ua_namespaceinit_generated.h"
#endif

#if defined(UA_ENABLE_MULTITHREADING) && !defined(NDEBUG)
UA_THREAD_LOCAL bool rcu_locked = false;
#endif

/**********************/
/* Namespace Handling */
/**********************/

static UA_StatusCode
replaceNamespaceArray_server(UA_Server * server,
        UA_String * newNsUris, size_t newNsSize){

    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
            "Changing the servers namespace array with new length: %i.", newNsSize);
    /* Check if new namespace uris are unique */
    for(size_t i = 0 ; i < newNsSize-1 ; ++i){
        for(size_t j = i+1 ; j < newNsSize ; ++j){
            if(UA_String_equal(&newNsUris[i], &newNsUris[j])){
                return UA_STATUSCODE_BADINVALIDARGUMENT;
            }
        }
    }

    /* Announce changing process */
    //TODO set lock flag
    size_t oldNsSize = server->namespacesSize;
    server->namespacesSize = 0;

    /* Alloc new NS Array  */
    UA_Namespace * newNsArray = (UA_Namespace*)UA_malloc(newNsSize * sizeof(UA_Namespace));
    if(!newNsArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    /* Alloc new index mapping array. Old ns index --> new ns index */
    size_t* oldNsIdxToNewNsIdx = (size_t*)UA_malloc(oldNsSize * sizeof(size_t));
    if(!oldNsIdxToNewNsIdx){
        UA_free(newNsArray);
        server->namespacesSize = oldNsSize;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    //Fill oldNsIdxToNewNsIdx with default values
    for(size_t i = 0 ; i < oldNsSize ; ++i){
        oldNsIdxToNewNsIdx[i] = (size_t)UA_NAMESPACE_UNDEFINED;
    }

    /* Search for old ns and copy it. If not found add a new namespace with default values. */
    //TODO forbid change of namespace 0?
    for(size_t newIdx = 0 ; newIdx < newNsSize ; ++newIdx){
        UA_Boolean nsExists = UA_FALSE;
        for(size_t oldIdx = 0 ; oldIdx < oldNsSize ; ++oldIdx){
            if(UA_String_equal(&newNsUris[newIdx], &server->namespaces[oldIdx].uri)){
                nsExists = UA_TRUE;
                newNsArray[newIdx] = server->namespaces[oldIdx];
                oldNsIdxToNewNsIdx[oldIdx] = newIdx; //Mark as already copied
                break;
            }
        }
        if(nsExists == UA_FALSE){
            UA_Namespace_init(&newNsArray[newIdx], &newNsUris[newIdx]);
        }
    }

    /* Update the namespace indices in data types, new namespaces and nodestores. Set default nodestores */
    UA_Namespace_updateNodestores(newNsArray,newNsSize,
                                  oldNsIdxToNewNsIdx, oldNsSize);
    for(size_t newIdx = 0 ; newIdx < newNsSize ; ++newIdx){
        UA_Namespace_updateDataTypes(&newNsArray[newIdx], NULL, (UA_UInt16)newIdx);
        newNsArray[newIdx].index = (UA_UInt16)newIdx;
        if(!newNsArray[newIdx].nodestore){
            newNsArray[newIdx].nodestore = server->nodestore_std;
            newNsArray[newIdx].nodestore->linkNamespace(newNsArray[newIdx].nodestore->handle, (UA_UInt16)newIdx);
        }
    }

    /* Delete old unused namespaces */
    for(size_t i = 0; i<oldNsSize; ++i){
        if(oldNsIdxToNewNsIdx[i] == (size_t)UA_NAMESPACE_UNDEFINED)
            UA_Namespace_deleteMembers(&server->namespaces[i]);
    }

    /* Cleanup, copy new namespace array to server and make visible */
    UA_free(oldNsIdxToNewNsIdx);
    UA_free(server->namespaces);
    server->namespaces = newNsArray;
    server->namespacesSize = newNsSize;
    //TODO make multithreading save and do at last step --> add real namespace array size as parameter or lock namespacearray?

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeNamespaces(void *handle, const UA_NodeId nodeid, const UA_Variant *data,
                const UA_NumericRange *range) {
    UA_Server *server = (UA_Server*)handle;

    /* Check the data type */
    if(data->type != &UA_TYPES[UA_TYPES_STRING])
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check that the variant is not empty */
    if(!data->data)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* TODO: Writing with a range is not implemented */
    if(range)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Reorder and replace the namespaces with all consequences */
    UA_StatusCode retval = replaceNamespaceArray_server(server,
            (UA_String *)data->data, (size_t)data->arrayLength);

    if(retval == UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_GOODEDITED; //Don't return good, as namespace array could be moved
    return retval;
}

static void
changeNamespace_server(UA_Server * server, UA_Namespace* newNs,  size_t newNsIdx){
    //change Nodestore
    UA_Namespace_changeNodestore(&server->namespaces[newNsIdx], newNs, server->nodestore_std,
            (UA_UInt16) newNsIdx);

    //Change and update DataTypes
    UA_Namespace_updateDataTypes(&server->namespaces[newNsIdx], newNs, (UA_UInt16)newNsIdx);

    //Update indices in namespaces
    newNs->index = (UA_UInt16)newNsIdx;
    server->namespaces[newNsIdx].index = (UA_UInt16)newNsIdx;
}

UA_StatusCode
UA_Server_addNamespace_full(UA_Server *server, UA_Namespace* namespacePtr){
    /* Check if the namespace already exists in the server's namespace array */
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(UA_String_equal(&namespacePtr->uri, &server->namespaces[i].uri)){
            changeNamespace_server(server, namespacePtr, i);
            return UA_STATUSCODE_GOOD;
        }
    }
    /* Namespace doesn't exist alloc space in namespaces array */
    UA_Namespace *newNsArray = (UA_Namespace*)UA_realloc(server->namespaces,
            sizeof(UA_Namespace) * (server->namespacesSize + 1));
    if(!newNsArray)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    server->namespaces = newNsArray;

    /* Fill new namespace with values */
    UA_Namespace_init(&server->namespaces[server->namespacesSize], &namespacePtr->uri);
    changeNamespace_server(server, namespacePtr, server->namespacesSize);

    /* Announce the change (otherwise, the array appears unchanged) */
    ++server->namespacesSize;
    return UA_STATUSCODE_GOOD;
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* namespaceUri){
    UA_Namespace * ns = UA_Namespace_newFromChar(namespaceUri);
    UA_Server_addNamespace_full(server, ns);
    UA_UInt16 retIndex = ns->index;
    UA_Namespace_deleteMembers(ns);
    UA_free(ns);
    return retIndex;
}

UA_StatusCode UA_Server_deleteNamespace_full(UA_Server *server, UA_Namespace * namespacePtr){
    UA_String * newNsUris = (UA_String*)UA_malloc((server->namespacesSize-1) * sizeof(UA_String));
    if(!newNsUris)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Check if the namespace already exists in the server's namespace array */
    size_t j = 0;
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(!UA_String_equal(&namespacePtr->uri, &server->namespaces[i].uri)){
            if(j == server->namespacesSize){
                UA_free(newNsUris);
                return UA_STATUSCODE_BADNOTFOUND;
            }
            newNsUris[j++] = server->namespaces[i].uri;
        }
    }
    return replaceNamespaceArray_server(server, newNsUris, j);
}

UA_StatusCode UA_Server_deleteNamespace(UA_Server *server, const char* namespaceUri){
    UA_Namespace * ns = UA_Namespace_newFromChar(namespaceUri);
    UA_StatusCode retVal = UA_Server_deleteNamespace_full(server, ns);
    UA_Namespace_deleteMembers(ns);
    UA_free(ns);
    return retVal;
}


#ifdef UA_ENABLE_EXTERNAL_NAMESPACES

static void
UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
    ens->index = 0;
    UA_String_init(&ens->url);
}

static void
UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
    UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.destroy(ens->externalNodeStore.ensHandle);
}

static void
UA_Server_deleteExternalNamespaces(UA_Server *server) {
    for(UA_UInt32 i = 0; i < server->externalNamespacesSize; ++i)
        UA_ExternalNamespace_deleteMembers(&server->externalNamespaces[i]);
    if(server->externalNamespacesSize > 0) {
        UA_free(server->externalNamespaces);
        server->externalNamespaces = NULL;
        server->externalNamespacesSize = 0;
    }
}

UA_StatusCode
UA_Server_addExternalNamespace(UA_Server *server, const UA_String *url,
                               UA_ExternalNodeStore *nodeStore,
                               UA_UInt16 *assignedNamespaceIndex) {
    if(!nodeStore)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    size_t size = server->externalNamespacesSize;
    server->externalNamespaces = (UA_ExternalNamespace*)
        UA_realloc(server->externalNamespaces, sizeof(UA_ExternalNamespace) * (size + 1));
    server->externalNamespaces[size].externalNodeStore = *nodeStore;
    server->externalNamespaces[size].index = (UA_UInt16)server->namespacesSize;
    *assignedNamespaceIndex = (UA_UInt16)server->namespacesSize;
    UA_String_copy(url, &server->externalNamespaces[size].url);
    ++server->externalNamespacesSize;
    UA_Server_addNamespace(server, urlString);
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_EXTERNAL_NAMESPACES*/




/**********************/
/* Utility Functions  */
/**********************/

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_RCU_LOCK();
    const UA_Node *parent = UA_NodestoreSwitch_getNode(server, &parentNodeId);
    if(!parent) {
        UA_RCU_UNLOCK();
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* TODO: We need to do an ugly copy of the references array since users may
     * delete references from within the callback. In single-threaded mode this
     * changes the same node we point at here. In multi-threaded mode, this
     * creates a new copy as nodes are truly immutable. */
    UA_ReferenceNode *refs = NULL;
    size_t refssize = parent->referencesSize;
    UA_StatusCode retval = UA_Array_copy(parent->references, parent->referencesSize,
                                         (void**)&refs, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_RCU_UNLOCK();
        return retval;
    }

    for(size_t i = parent->referencesSize; i > 0; --i) {
        UA_ReferenceNode *ref = &refs[i-1];
        retval |= callback(ref->targetId.nodeId, ref->isInverse,
                           ref->referenceTypeId, handle);
    }
    UA_NodestoreSwitch_releaseNode(server, parent);
    UA_RCU_UNLOCK();

    UA_Array_delete(refs, refssize, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    return retval;
}

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    // Delete the timed work
    UA_RepeatedJobsList_deleteMembers(&server->repeatedJobs);

    // Delete all internal data
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_RCU_LOCK();
    //delete all namespaces and nodestores
    for(size_t i = 0; i<server->namespacesSize; ++i){
        UA_Namespace_deleteMembers(&server->namespaces[i]);
    }
    UA_free(server->namespaces);
    //Delete the standard nodestore
    UA_Nodestore_standard_delete(server->nodestore_std);
    UA_RCU_UNLOCK();
    UA_free(server->nodestore_std);
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Server_deleteExternalNamespaces(server);
#endif
    UA_Array_delete(server->endpointDescriptions, server->endpointDescriptionsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

#ifdef UA_ENABLE_DISCOVERY
    registeredServer_list_entry *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &server->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_deleteMembers(&rs->registeredServer);
        UA_free(rs);
    }
    if(server->periodicServerRegisterJob)
        UA_free(server->periodicServerRegisterJob);

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        destroyMulticastDiscoveryServer(server);

    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &server->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_deleteMembers(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_PRIME; i++) {
        serverOnNetwork_hash_entry* currHash = server->serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }
# endif

#endif

#ifdef UA_ENABLE_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition);
    pthread_mutex_destroy(&server->dispatchQueue_mutex);
#endif
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void UA_Server_cleanup(UA_Server *server, void *_) {
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, nowMonotonic);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, nowMonotonic);
#ifdef UA_ENABLE_DISCOVERY
    UA_Discovery_cleanupTimedOut(server, nowMonotonic);
#endif
}

/* Create endpoints w/o endpointurl. It is added from the networklayers at startup */
static void
addEndpointDefinitions(UA_Server *server) {
    server->endpointDescriptions =
        (UA_EndpointDescription*)UA_Array_new(server->config.networkLayersSize,
                                              &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    server->endpointDescriptionsSize = server->config.networkLayersSize;

    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_EndpointDescription *endpoint = &server->endpointDescriptions[i];
        endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
        endpoint->securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
        endpoint->transportProfileUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

        size_t policies = 0;
        if(server->config.accessControl.enableAnonymousLogin)
            ++policies;
        if(server->config.accessControl.enableUsernamePasswordLogin)
            ++policies;
        endpoint->userIdentityTokensSize = policies;
        endpoint->userIdentityTokens =
            (UA_UserTokenPolicy*)UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

        size_t currentIndex = 0;
        if(server->config.accessControl.enableAnonymousLogin) {
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
            ++currentIndex;
        }
        if(server->config.accessControl.enableUsernamePasswordLogin) {
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_USERNAME;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(USERNAME_POLICY);
        }

        /* The standard says "the HostName specified in the Server Certificate is the
           same as the HostName contained in the endpointUrl provided in the
           EndpointDescription */
        UA_String_copy(&server->config.serverCertificate, &endpoint->serverCertificate);
        UA_ApplicationDescription_copy(&server->config.applicationDescription, &endpoint->server);

        /* copy the discovery url only once the networlayer has been started */
        // UA_String_copy(&server->config.networkLayers[i].discoveryUrl, &endpoint->endpointUrl);
    }
}

UA_Server *
UA_Server_new(const UA_ServerConfig config) {
    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    server->config = config;
    server->startTime = UA_DateTime_now();
    server->nodestore = UA_NodeStore_new();

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    /* Initialize the handling of repeated jobs */
#ifdef UA_ENABLE_MULTITHREADING
    UA_RepeatedJobsList_init(&server->repeatedJobs,
                             (UA_RepeatedJobsListProcessCallback)UA_Server_dispatchJob, server);
#else
    UA_RepeatedJobsList_init(&server->repeatedJobs,
                             (UA_RepeatedJobsListProcessCallback)UA_Server_processJob, server);
#endif

    /* Initialized the linked list for delayed callbacks */
#ifndef UA_ENABLE_MULTITHREADING
    SLIST_INIT(&server->delayedCallbacks);
#endif

    /* Initialized the dispatch queue for worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#endif

    /* Create Namespaces 0 and 1 */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Create Endpoint Definitions */
    addEndpointDefinitions(server);

    /* Initialized SecureChannel and Session managers */
    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);

    /* Add a regular job for cleanup and maintenance */
    UA_Job cleanup;
    cleanup.type = UA_JOBTYPE_METHODCALL;
    cleanup.job.methodCall.data = NULL;
    cleanup.job.methodCall.method = UA_Server_cleanup;
    UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

    /* Initialized discovery database */
#ifdef UA_ENABLE_DISCOVERY
    LIST_INIT(&server->registeredServers);
    server->registeredServersSize = 0;
    server->periodicServerRegisterJob = NULL;
    server->registerServerCallback = NULL;
    server->registerServerCallbackData = NULL;
#endif

    /* Initialize multicast discovery */
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
    server->mdnsDaemon = NULL;
    server->mdnsSocket = 0;
    server->mdnsMainSrvAdded = UA_FALSE;
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        initMulticastDiscoveryServer(server);

    LIST_INIT(&server->serverOnNetwork);
    server->serverOnNetworkSize = 0;
    server->serverOnNetworkRecordIdCounter = 0;
    server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(server->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*) * SERVER_ON_NETWORK_HASH_PRIME);

    server->serverOnNetworkCallback = NULL;
    server->serverOnNetworkCallbackData = NULL;
#endif

    /* Initialize Namespace 0 */
#ifdef UA_ENABLE_LOAD_NAMESPACE0
#ifndef UA_ENABLE_GENERATE_NAMESPACE0
    UA_Server_createNS0(server);
#else
    ua_namespaceinit_generated(server);
#endif
#endif //UA_ENABLE_LOAD_NAMESPACE0

    return server;
}

/*****************/
/* Repeated Jobs */
/*****************/

UA_StatusCode
UA_Server_addRepeatedJob(UA_Server *server, UA_Job job,
                         UA_UInt32 interval, UA_Guid *jobId) {
    return UA_RepeatedJobsList_addRepeatedJob(&server->repeatedJobs, job, interval, jobId);
}

UA_StatusCode
UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId) {
    return UA_RepeatedJobsList_removeRepeatedJob(&server->repeatedJobs, jobId);
}
