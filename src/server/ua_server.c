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

#ifdef UA_ENABLE_DISCOVERY
#include "ua_client.h"
#include "ua_config_standard.h"
#endif

#ifdef UA_ENABLE_GENERATE_NAMESPACE0
#include "ua_namespaceinit_generated.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#if defined(UA_ENABLE_MULTITHREADING) && !defined(NDEBUG)
UA_THREAD_LOCAL bool rcu_locked = false;
#endif

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
UA_THREAD_LOCAL UA_Session* methodCallSession = NULL;
#endif

/**********************/
/* Namespace Handling */
/**********************/

UA_UInt16 addNamespace(UA_Server *server, const UA_String name) {
    /* Check if the namespace already exists in the server's namespace array */
    for(UA_UInt16 i = 0; i < server->namespacesSize; ++i) {
        if(UA_String_equal(&name, &server->namespaces[i]))
            return i;
    }

    /* Make the array bigger */
    UA_String *newNS = (UA_String*)UA_realloc(server->namespaces,
                                  sizeof(UA_String) * (server->namespacesSize + 1));
    if(!newNS)
        return 0;
    server->namespaces = newNS;

    /* Copy the namespace string */
    UA_StatusCode retval = UA_String_copy(&name, &server->namespaces[server->namespacesSize]);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    /* Announce the change (otherwise, the array appears unchanged) */
    ++server->namespacesSize;
    return (UA_UInt16)(server->namespacesSize - 1);
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    UA_String nameString;
    nameString.length = strlen(name);
    nameString.data = (UA_Byte*)(uintptr_t)name;
    return addNamespace(server, nameString);
}

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
static void UA_ExternalNamespace_init(UA_ExternalNamespace *ens) {
    ens->index = 0;
    UA_String_init(&ens->url);
}

static void UA_ExternalNamespace_deleteMembers(UA_ExternalNamespace *ens) {
    UA_String_deleteMembers(&ens->url);
    ens->externalNodeStore.destroy(ens->externalNodeStore.ensHandle);
}

static void UA_Server_deleteExternalNamespaces(UA_Server *server) {
    for(UA_UInt32 i = 0; i < server->externalNamespacesSize; ++i)
        UA_ExternalNamespace_deleteMembers(&server->externalNamespaces[i]);
    if(server->externalNamespacesSize > 0) {
        UA_free(server->externalNamespaces);
        server->externalNamespaces = NULL;
        server->externalNamespacesSize = 0;
    }
}

UA_StatusCode UA_EXPORT
UA_Server_addExternalNamespace(UA_Server *server, const UA_String *url,
                               UA_ExternalNodeStore *nodeStore,
                               UA_UInt16 *assignedNamespaceIndex) {
    if(!nodeStore)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    size_t size = server->externalNamespacesSize;
    server->externalNamespaces =
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

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_RCU_LOCK();
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
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
    UA_RCU_UNLOCK();

    UA_Array_delete(refs, refssize, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    return retval;
}

static void
addReferenceInternal(UA_Server *server, UA_UInt32 sourceId, UA_UInt32 refTypeId,
                     UA_UInt32 targetId, UA_Boolean isForward) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId.identifier.numeric = sourceId;
    item.referenceTypeId.identifier.numeric = refTypeId;
    item.isForward = isForward;
    item.targetNodeId.nodeId.identifier.numeric = targetId;
    UA_RCU_LOCK();
    Service_AddReferences_single(server, &adminSession, &item);
    UA_RCU_UNLOCK();
}

/**********/
/* Server */
/**********/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    // Delete the timed work
    UA_RepeatedJobsList_deleteMembers(&server->repeatedJobs);

    // Delete all internal data
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_RCU_LOCK();
    UA_NodeStore_delete(server->nodestore);
    UA_RCU_UNLOCK();
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Server_deleteExternalNamespaces(server);
#endif
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
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
        UA_Discovery_multicastDestroy(server);

    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &server->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_deleteMembers(&son->serverOnNetwork);
        if(son->pathTmp)
            free(son->pathTmp);
        UA_free(son);
    }

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_PRIME; i++) {
        serverOnNetwork_hash_entry* currHash = server->serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            free(currHash);
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

static UA_StatusCode
readStatus(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    UA_Server *server = (UA_Server*)handle;
    UA_ServerStatusDataType *status = UA_ServerStatusDataType_new();
    status->startTime = server->startTime;
    status->currentTime = UA_DateTime_now();
    status->state = UA_SERVERSTATE_RUNNING;
    status->secondsTillShutdown = 0;
    UA_BuildInfo_copy(&server->config.buildInfo, &status->buildInfo);

    value->value.type = &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE];
    value->value.arrayLength = 0;
    value->value.data = status;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

/** TODO: rework the code duplication in the getter methods **/
static UA_StatusCode
readServiceLevel(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
                 const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BYTE];
    value->value.arrayLength = 0;
    UA_Byte *byte = UA_Byte_new();
    *byte = 255;
    value->value.data = byte;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

/** TODO: rework the code duplication in the getter methods **/
static UA_StatusCode
readAuditing(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 0;
    UA_Boolean *boolean = UA_Boolean_new();
    *boolean = false;
    value->value.data = boolean;
    value->value.arrayDimensionsSize = 0;
    value->value.arrayDimensions = NULL;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readNamespaces(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimestamp,
               const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_Server *server = (UA_Server*)handle;
    UA_StatusCode retval;
    retval = UA_Variant_setArrayCopy(&value->value, server->namespaces,
                                     server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimestamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }
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
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_String *newNamespaces = (UA_String*)data->data;
    size_t newNamespacesSize = data->arrayLength;

    /* Test if we append to the existing namespaces */
    if(newNamespacesSize <= server->namespacesSize)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Test if the existing namespaces are unchanged */
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(!UA_String_equal(&server->namespaces[i], &newNamespaces[i]))
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add namespaces */
    for(size_t i = server->namespacesSize; i < newNamespacesSize; ++i)
        addNamespace(server, newNamespaces[i]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readCurrentTime(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime currentTime = UA_DateTime_now();
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &currentTime,
                                                    &UA_TYPES[UA_TYPES_DATETIME]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    value->hasValue = true;
    if(sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

static void
addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid,
                UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_DataTypeAttributes attr;
    UA_DataTypeAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.isAbstract = isAbstract;
    UA_Server_addDataTypeNode(server, UA_NODEID_NUMERIC(0, datatypeid),
                              UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                              UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}

static void
addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid,
                  UA_Boolean isAbstract, UA_UInt32 parentid) {
    UA_ObjectTypeAttributes attr;
    UA_ObjectTypeAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.isAbstract = isAbstract;
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(0, objecttypeid),
                                UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                UA_QUALIFIEDNAME(0, name), attr, NULL, NULL);
}

static void
addObjectNode(UA_Server *server, char* name, UA_UInt32 objectid,
              UA_UInt32 parentid, UA_UInt32 referenceid, UA_UInt32 typeid) {
    UA_ObjectAttributes object_attr;
    UA_ObjectAttributes_init(&object_attr);
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(0, objectid),
                            UA_NODEID_NUMERIC(0, parentid),
                            UA_NODEID_NUMERIC(0, referenceid),
                            UA_QUALIFIEDNAME(0, name),
                            UA_NODEID_NUMERIC(0, typeid),
                            object_attr, NULL, NULL);

}

static void
addReferenceTypeNode(UA_Server *server, char* name, char *inverseName, UA_UInt32 referencetypeid,
                     UA_Boolean isabstract, UA_Boolean symmetric, UA_UInt32 parentid) {
    UA_ReferenceTypeAttributes reference_attr;
    UA_ReferenceTypeAttributes_init(&reference_attr);
    reference_attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    reference_attr.isAbstract = isabstract;
    reference_attr.symmetric = symmetric;
    if(inverseName)
        reference_attr.inverseName = UA_LOCALIZEDTEXT("en_US", inverseName);
    UA_Server_addReferenceTypeNode(server, UA_NODEID_NUMERIC(0, referencetypeid),
                                   UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                   UA_QUALIFIEDNAME(0, name), reference_attr, NULL, NULL);
}

static void
addVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                    UA_Boolean isAbstract, UA_Int32 valueRank, UA_UInt32 dataType,
                    UA_Variant *value, UA_UInt32 parentid) {
    UA_VariableTypeAttributes attr;
    UA_VariableTypeAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.isAbstract = isAbstract;
    attr.dataType = UA_NODEID_NUMERIC(0, dataType);
    attr.valueRank = valueRank;
    if(value)
        attr.value = *value;
    UA_Server_addVariableTypeNode(server, UA_NODEID_NUMERIC(0, variabletypeid),
                                  UA_NODEID_NUMERIC(0, parentid), UA_NODEID_NULL,
                                  UA_QUALIFIEDNAME(0, name), UA_NODEID_NULL, attr, NULL, NULL);
}

static void
addVariableNode(UA_Server *server, UA_UInt32 nodeid, char* name, UA_Int32 valueRank,
                const UA_NodeId *dataType, UA_Variant *value, UA_UInt32 parentid,
                UA_UInt32 referenceid, UA_UInt32 typeid) {
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", name);
    attr.dataType = *dataType;
    attr.valueRank = valueRank;
    if(value)
        attr.value = *value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(0, nodeid), UA_NODEID_NUMERIC(0, parentid),
                              UA_NODEID_NUMERIC(0, referenceid), UA_QUALIFIEDNAME(0, name),
                              UA_NODEID_NUMERIC(0, typeid), attr, NULL, NULL);
}

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
GetMonitoredItems(void *handle, const UA_NodeId *objectId,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Session* session = methodCallSession;
    UA_Subscription* subscription = UA_Session_getSubscriptionByID(session, subscriptionId);
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        ++sizeOfOutput;
    }
    if(sizeOfOutput==0)
        return UA_STATUSCODE_GOOD;

    UA_UInt32 *clientHandles = (UA_UInt32*)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 *serverHandles = (UA_UInt32*)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->clientHandle;
        serverHandles[i] = monitoredItem->itemId;
        ++i;
    }
    UA_Variant_setArray(&output[0], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_STATUSCODE_GOOD;
}
#endif

UA_Server * UA_Server_new(const UA_ServerConfig config) {
    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    server->config = config;
    server->nodestore = UA_NodeStore_new();

    /* Initialize the handling of repeated jobs */
#ifdef UA_ENABLE_MULTITHREADING
    UA_RepeatedJobsList_init(&server->repeatedJobs,
                             (UA_RepeatedJobsListProcessCallback)UA_Server_dispatchJob,
                             server);
#else
    UA_RepeatedJobsList_init(&server->repeatedJobs,
                             (UA_RepeatedJobsListProcessCallback)UA_Server_processJob,
                             server);
#endif

#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#else
    SLIST_INIT(&server->delayedCallbacks);
#endif

#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    /* ns0 and ns1 */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Create endpoints w/o endpointurl. It is added from the networklayers at startup */
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

    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);

   UA_Job cleanup;
   cleanup.type = UA_JOBTYPE_METHODCALL;
   cleanup.job.methodCall.data = NULL;
   cleanup.job.methodCall.method = UA_Server_cleanup;
   UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

#ifdef UA_ENABLE_DISCOVERY
    // Discovery service
    LIST_INIT(&server->registeredServers);
    server->registeredServersSize = 0;
    server->periodicServerRegisterJob = NULL;
    server->registerServerCallback = NULL;
    server->registerServerCallbackData = NULL;
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    server->mdnsDaemon = NULL;
    server->mdnsSocket = 0;
    server->mdnsMainSrvAdded = UA_FALSE;
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        UA_Discovery_multicastInit(server);
    }

    LIST_INIT(&server->serverOnNetwork);
    server->serverOnNetworkSize = 0;
    server->serverOnNetworkRecordIdCounter = 0;
    server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(server->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*)*SERVER_ON_NETWORK_HASH_PRIME);

    server->serverOnNetworkCallback = NULL;
    server->serverOnNetworkCallbackData = NULL;
# endif
#endif

    server->startTime = UA_DateTime_now();

#ifndef UA_ENABLE_GENERATE_NAMESPACE0
    UA_Server_createNS0(server);
#else
    /* load the generated namespace externally */
    ua_namespaceinit_generated(server);
#endif

    return server;
}

/*************/
/* Discovery */
/*************/

#ifdef UA_ENABLE_DISCOVERY
static UA_StatusCode
register_server_with_discovery_server(UA_Server *server, const char* discoveryServerUrl,
                                      const UA_Boolean isUnregister,
                                      const char* semaphoreFilePath) {
    /* Create the client */
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    const char *url = discoveryServerUrl != NULL ? discoveryServerUrl : "opc.tcp://localhost:4840";
    UA_StatusCode retval = UA_Client_connect(client, url);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Connecting to client failed with statuscode %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return retval;
    }

    /* Prepare the request. Do not cleanup the request after the service call,
     * as the members are stack-allocated or point into the server config. */
    UA_RegisterServer2Request request;
    UA_RegisterServer2Request_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    request.server.isOnline = !isUnregister;
    request.server.serverUri = server->config.applicationDescription.applicationUri;
    request.server.productUri = server->config.applicationDescription.productUri;
    request.server.serverType = server->config.applicationDescription.applicationType;
    request.server.gatewayServerUri = server->config.applicationDescription.gatewayServerUri;

    if(semaphoreFilePath) {
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        request.server.semaphoreFilePath = UA_STRING((char*)(uintptr_t)semaphoreFilePath); /* dirty cast */
#else
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Ignoring semaphore file path. open62541 not compiled "
                       "with UA_ENABLE_DISCOVERY_SEMAPHORE=ON");
#endif
    }

    request.server.serverNames = &server->config.applicationDescription.applicationName;
    request.server.serverNamesSize = 1;

    /* Copy the discovery urls from the server config and the network layers*/
    size_t config_discurls = server->config.applicationDescription.discoveryUrlsSize;
    size_t nl_discurls = server->config.networkLayersSize;
    request.server.discoveryUrls = (UA_String*)UA_alloca(sizeof(UA_String) * (config_discurls + nl_discurls));
    request.server.discoveryUrlsSize = config_discurls + nl_discurls;

    for(size_t i = 0; i < config_discurls; ++i)
        request.server.discoveryUrls[i] = server->config.applicationDescription.discoveryUrls[i];

    /* TODO: Add nl only if discoveryUrl not already present */
    for(size_t i = 0; i < nl_discurls; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        request.server.discoveryUrls[config_discurls + i] = nl->discoveryUrl;
    }

    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_MdnsDiscoveryConfiguration_init(&mdnsConfig);

    request.discoveryConfigurationSize = 0;
    request.discoveryConfigurationSize = 1;
    request.discoveryConfiguration = UA_ExtensionObject_new();
    UA_ExtensionObject_init(&request.discoveryConfiguration[0]);
    request.discoveryConfiguration[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    request.discoveryConfiguration[0].content.decoded.type = &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION];
    request.discoveryConfiguration[0].content.decoded.data = &mdnsConfig;

    mdnsConfig.mdnsServerName = server->config.mdnsServerName;
    mdnsConfig.serverCapabilities = server->config.serverCapabilities;
    mdnsConfig.serverCapabilitiesSize = server->config.serverCapabilitiesSize;

    // First try with RegisterServer2, if that isn't implemented, use RegisterServer
    UA_RegisterServer2Response response;
    UA_RegisterServer2Response_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE]);

    UA_StatusCode serviceResult = response.responseHeader.serviceResult;
    UA_RegisterServer2Response_deleteMembers(&response);
    UA_ExtensionObject_delete(request.discoveryConfiguration);

    if(serviceResult == UA_STATUSCODE_BADNOTIMPLEMENTED ||
       serviceResult == UA_STATUSCODE_BADSERVICEUNSUPPORTED) {
        // try RegisterServer
        UA_RegisterServerResponse response_fallback;
        UA_RegisterServerResponse_init(&response_fallback);

        // copy from RegisterServer2 request
        UA_RegisterServerRequest request_fallback;
        UA_RegisterServerRequest_init(&request_fallback);

        request_fallback.requestHeader = request.requestHeader;
        request_fallback.server = request.server;

        __UA_Client_Service(client, &request_fallback, &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                            &response_fallback, &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE]);

        if(response_fallback.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_CLIENT,
                         "RegisterServer failed with statuscode %s",
                         UA_StatusCode_name(response_fallback.responseHeader.serviceResult));
            serviceResult = response_fallback.responseHeader.serviceResult;
            UA_RegisterServerResponse_deleteMembers(&response_fallback);
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            return serviceResult;
        }
    } else if(serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer2 failed with statuscode %s",
                     UA_StatusCode_name(serviceResult));
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return serviceResult;
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_register_discovery(UA_Server *server, const char* discoveryServerUrl,
                             const char* semaphoreFilePath) {
    return register_server_with_discovery_server(server, discoveryServerUrl, UA_FALSE, semaphoreFilePath);
}

UA_StatusCode
UA_Server_unregister_discovery(UA_Server *server, const char* discoveryServerUrl) {
    return register_server_with_discovery_server(server, discoveryServerUrl, UA_TRUE, NULL);
}

#endif

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
