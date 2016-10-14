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

static const UA_NodeId nodeIdHasSubType = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASSUBTYPE};
static const UA_NodeId nodeIdHasTypeDefinition = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASTYPEDEFINITION};
static const UA_NodeId nodeIdHasComponent = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASCOMPONENT};
static const UA_NodeId nodeIdHasProperty = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_HASPROPERTY};
static const UA_NodeId nodeIdOrganizes = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_ORGANIZES};
static const UA_NodeId nodeIdFolderType = {
    .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
    .identifier.numeric = UA_NS0ID_FOLDERTYPE};

#ifndef UA_ENABLE_GENERATE_NAMESPACE0
static const UA_NodeId nodeIdNonHierarchicalReferences = {
        .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
        .identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES};
#endif

/**********************/
/* Namespace Handling */
/**********************/

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    const UA_String nameString = (UA_String){.length = strlen(name),
                                             .data = (UA_Byte*)(uintptr_t)name};

    /* Check if the namespace already exists in the server's namespace array */
    for(UA_UInt16 i=0;i<server->namespacesSize;i++) {
        if(UA_String_equal(&nameString, &server->namespaces[i]))
            return i;
    }

    /* Add a new namespace to the namespace array */
    server->namespaces = UA_realloc(server->namespaces,
                                    sizeof(UA_String) * (server->namespacesSize + 1));
    UA_String_copy(&nameString, &server->namespaces[server->namespacesSize]);
    server->namespacesSize++;
    return (UA_UInt16)(server->namespacesSize - 1);
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
    for(UA_UInt32 i = 0; i < server->externalNamespacesSize; i++)
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

    char urlString[256];
    if(url->length >= 256)
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(urlString, url->data, url->length);
    urlString[url->length] = 0;

    size_t size = server->externalNamespacesSize;
    server->externalNamespaces =
        UA_realloc(server->externalNamespaces, sizeof(UA_ExternalNamespace) * (size + 1));
    server->externalNamespaces[size].externalNodeStore = *nodeStore;
    server->externalNamespaces[size].index = (UA_UInt16)server->namespacesSize;
    *assignedNamespaceIndex = (UA_UInt16)server->namespacesSize;
    UA_String_copy(url, &server->externalNamespaces[size].url);
    server->externalNamespacesSize++;
    UA_Server_addNamespace(server, urlString);

    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_EXTERNAL_NAMESPACES*/

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_RCU_LOCK();
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
    if(!parent) {
        UA_RCU_UNLOCK();
        return UA_STATUSCODE_BADNODEIDINVALID;
    }
    for(size_t i = 0; i < parent->referencesSize; i++) {
        UA_ReferenceNode *ref = &parent->references[i];
        retval |= callback(ref->targetId.nodeId, ref->isInverse,
                           ref->referenceTypeId, handle);
    }
    UA_RCU_UNLOCK();
    return retval;
}

static UA_StatusCode
addReferenceInternal(UA_Server *server, const UA_NodeId sourceId, const UA_NodeId refTypeId,
                     const UA_ExpandedNodeId targetId, UA_Boolean isForward) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = sourceId;
    item.referenceTypeId = refTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetId;
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_AddReferences_single(server, &adminSession, &item);
    UA_RCU_UNLOCK();
    return retval;
}

static UA_AddNodesResult
addNodeInternal(UA_Server *server, UA_Node *node, const UA_NodeId parentNodeId,
                const UA_NodeId referenceTypeId, UA_Boolean instantiate) {
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    UA_RCU_LOCK();
    res.statusCode = Service_AddNodes_existing(server, &adminSession, node, &parentNodeId,
                                               &referenceTypeId, &UA_NODEID_NULL,
                                               NULL, &res.addedNodeId,instantiate);
    UA_RCU_UNLOCK();
    return res;
}

static UA_AddNodesResult
addNodeInternalWithType(UA_Server *server, UA_Node *node, const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId, const UA_NodeId typeIdentifier,UA_Boolean instantiate) {
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    UA_RCU_LOCK();
    res.statusCode = Service_AddNodes_existing(server, &adminSession, node, &parentNodeId,
                                               &referenceTypeId, &typeIdentifier,
                                               NULL, &res.addedNodeId, true);
    UA_RCU_UNLOCK();
    return res;
}

// delete any children of an instance without touching the object itself
static void deleteInstanceChildren(UA_Server *server, UA_NodeId *objectNodeId) {
  UA_BrowseDescription bDes;
  UA_BrowseDescription_init(&bDes);
  UA_NodeId_copy(objectNodeId, &bDes.nodeId );
  bDes.browseDirection = UA_BROWSEDIRECTION_FORWARD;
  bDes.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
  bDes.resultMask = UA_BROWSERESULTMASK_ISFORWARD | UA_BROWSERESULTMASK_NODECLASS | UA_BROWSERESULTMASK_REFERENCETYPEINFO;
  UA_BrowseResult bRes;
  UA_BrowseResult_init(&bRes);
  Service_Browse_single(server, &adminSession, NULL, &bDes, 0, &bRes);
  for(size_t i=0; i<bRes.referencesSize; i++) {
    UA_ReferenceDescription *rd = &bRes.references[i];
    if((rd->nodeClass == UA_NODECLASS_OBJECT || rd->nodeClass == UA_NODECLASS_VARIABLE)) 
    {
      Service_DeleteNodes_single(server, &adminSession, &rd->nodeId.nodeId, UA_TRUE) ;
    }
    else if (rd->nodeClass == UA_NODECLASS_METHOD) 
    {
      UA_DeleteReferencesItem dR;
      UA_DeleteReferencesItem_init(&dR);
      dR.sourceNodeId = *objectNodeId;
      dR.isForward = UA_TRUE;
      UA_NodeId_copy(&rd->referenceTypeId, &dR.referenceTypeId);
      UA_NodeId_copy(&rd->nodeId.nodeId, &dR.targetNodeId.nodeId);
      dR.deleteBidirectional = UA_TRUE;
      Service_DeleteReferences_single(server, &adminSession, &dR);
      UA_DeleteReferencesItem_deleteMembers(&dR);
    }
  }
  UA_BrowseResult_deleteMembers(&bRes); 
}

/**********/
/* Server */
/**********/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    // Delete the timed work
    UA_Server_deleteAllRepeatedJobs(server);

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
    registeredServer_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &server->registeredServers, pointers, temp) {
        LIST_REMOVE(current, pointers);
        UA_RegisteredServer_deleteMembers(&current->registeredServer);
        UA_free(current);
    }
#endif

#ifdef UA_ENABLE_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition);
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

static void copyNames(UA_Node *node, char *name) {
    node->browseName = UA_QUALIFIEDNAME_ALLOC(0, name);
    node->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", name);
    node->description = UA_LOCALIZEDTEXT_ALLOC("en_US", name);
}

static void
addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid,
                UA_Boolean isAbstract, UA_UInt32 parent) {
    UA_DataTypeNode *datatype = UA_NodeStore_newDataTypeNode();
    copyNames((UA_Node*)datatype, name);
    datatype->nodeId.identifier.numeric = datatypeid;
    datatype->isAbstract = isAbstract;
    addNodeInternal(server, (UA_Node*)datatype,
                    UA_NODEID_NUMERIC(0, parent), nodeIdHasSubType,true);
}

static void
addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid,
                  UA_UInt32 parent, UA_UInt32 parentreference) {
    UA_ObjectTypeNode *objecttype = UA_NodeStore_newObjectTypeNode();
    copyNames((UA_Node*)objecttype, name);
    objecttype->nodeId.identifier.numeric = objecttypeid;
    addNodeInternal(server, (UA_Node*)objecttype, UA_NODEID_NUMERIC(0, parent),
                    UA_NODEID_NUMERIC(0, parentreference),true);
}

static UA_VariableTypeNode*
createVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                       UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = UA_NodeStore_newVariableTypeNode();
    copyNames((UA_Node*)variabletype, name);
    variabletype->nodeId.identifier.numeric = variabletypeid;
    variabletype->isAbstract = abstract;
    return variabletype;
}

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
static UA_StatusCode
GetMonitoredItems(void *handle, const UA_NodeId objectId, size_t inputSize,
                  const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_UInt32 subscriptionId = *((UA_UInt32*)(input[0].data));
    UA_Session* session = methodCallSession;
    UA_Subscription* subscription = UA_Session_getSubscriptionByID(session, subscriptionId);
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_UInt32 sizeOfOutput = 0;
    UA_MonitoredItem* monitoredItem;
    LIST_FOREACH(monitoredItem, &subscription->MonitoredItems, listEntry) {
        sizeOfOutput++;
    }
    if(sizeOfOutput==0)
        return UA_STATUSCODE_GOOD;

    UA_UInt32* clientHandles = UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32* serverHandles = UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 i = 0;
    LIST_FOREACH(monitoredItem, &subscription->MonitoredItems, listEntry) {
        clientHandles[i] = monitoredItem->clientHandle;
        serverHandles[i] = monitoredItem->itemId;
        i++;
    }
    UA_Variant_setArray(&output[0], clientHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setArray(&output[1], serverHandles, sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_STATUSCODE_GOOD;
}
#endif

UA_Server * UA_Server_new(const UA_ServerConfig config) {
    UA_Server *server = UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    server->config = config;
    server->nodestore = UA_NodeStore_new();
    LIST_INIT(&server->repeatedJobs);

#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    cds_wfcq_init(&server->dispatchQueue_head, &server->dispatchQueue_tail);
    cds_lfs_init(&server->mainLoopJobs);
#else
    SLIST_INIT(&server->delayedCallbacks);
#endif

    /* uncomment for non-reproducible server runs */
    //UA_random_seed(UA_DateTime_now());

    /* ns0 and ns1 */
    server->namespaces = UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Create endpoints w/o endpointurl. It is added from the networklayers at startup */
    server->endpointDescriptions = UA_Array_new(server->config.networkLayersSize,
                                                &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    server->endpointDescriptionsSize = server->config.networkLayersSize;
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_EndpointDescription *endpoint = &server->endpointDescriptions[i];
        endpoint->securityMode = UA_MESSAGESECURITYMODE_NONE;
        endpoint->securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
        endpoint->transportProfileUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

        size_t policies = 0;
        if(server->config.enableAnonymousLogin)
            policies++;
        if(server->config.enableUsernamePasswordLogin)
            policies++;
        endpoint->userIdentityTokensSize = policies;
        endpoint->userIdentityTokens = UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

        size_t currentIndex = 0;
        if(server->config.enableAnonymousLogin) {
            UA_UserTokenPolicy_init(&endpoint->userIdentityTokens[currentIndex]);
            endpoint->userIdentityTokens[currentIndex].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
            endpoint->userIdentityTokens[currentIndex].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
            currentIndex++;
        }
        if(server->config.enableUsernamePasswordLogin) {
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

    UA_Job cleanup = {.type = UA_JOBTYPE_METHODCALL,
                      .job.methodCall = {.method = UA_Server_cleanup, .data = NULL} };
    UA_Server_addRepeatedJob(server, cleanup, 10000, NULL);

#ifdef UA_ENABLE_DISCOVERY
    // Discovery service
    LIST_INIT(&server->registeredServers);
    server->registeredServersSize = 0;
#endif

    server->startTime = UA_DateTime_now();

#ifndef UA_ENABLE_GENERATE_NAMESPACE0

    /*********************************/
    /* Bootstrap reference hierarchy */
    /*********************************/

    UA_ReferenceTypeNode *references = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = true;
    references->symmetric = true;
    references->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "References");

    UA_ReferenceTypeNode *hassubtype = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hassubtype, "HasSubtype");
    hassubtype->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HasSupertype");
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = false;
    hassubtype->symmetric = false;

    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references);
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype);
    UA_RCU_UNLOCK();

    UA_ReferenceTypeNode *hierarchicalreferences = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hierarchicalreferences, "HierarchicalReferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = true;
    hierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = true;
    nonhierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)nonhierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *haschild = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = false;
    haschild->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haschild,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *organizes = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)organizes, "Organizes");
    organizes->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrganizedBy");
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = false;
    organizes->symmetric  = false;
    addNodeInternal(server, (UA_Node*)organizes,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *haseventsource = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haseventsource, "HasEventSource");
    haseventsource->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EventSourceOf");
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = false;
    haseventsource->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseventsource,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasmodellingrule = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodellingrule, "HasModellingRule");
    hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModellingRuleOf");
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = false;
    hasmodellingrule->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodellingrule, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasencoding = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasencoding, "HasEncoding");
    hasencoding->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EncodingOf");
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = false;
    hasencoding->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasencoding, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasdescription = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasdescription, "HasDescription");
    hasdescription->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "DescriptionOf");
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = false;
    hasdescription->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasdescription, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hastypedefinition = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hastypedefinition, "HasTypeDefinition");
    hastypedefinition->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "TypeDefinitionOf");
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = false;
    hastypedefinition->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hastypedefinition, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *generatesevent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)generatesevent, "GeneratesEvent");
    generatesevent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "GeneratedBy");
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = false;
    generatesevent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)generatesevent, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *aggregates = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)aggregates, "Aggregates");
    aggregates->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "AggregatedBy");
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = false;
    aggregates->symmetric  = false;
    addNodeInternal(server, (UA_Node*)aggregates, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType,true);

    /* complete bootstrap of hassubtype */
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), true);

    UA_ReferenceTypeNode *hasproperty = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasproperty, "HasProperty");
    hasproperty->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "PropertyOf");
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = false;
    hasproperty->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasproperty,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hascomponent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hascomponent, "HasComponent");
    hascomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ComponentOf");
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = false;
    hascomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascomponent, UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasnotifier = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasnotifier, "HasNotifier");
    hasnotifier->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "NotifierOf");
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = false;
    hasnotifier->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasnotifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasorderedcomponent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasorderedcomponent, "HasOrderedComponent");
    hasorderedcomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrderedComponentOf");
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = false;
    hasorderedcomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasorderedcomponent, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hasmodelparent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodelparent, "HasModelParent");
    hasmodelparent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModelParentOf");
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = false;
    hasmodelparent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodelparent, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *fromstate = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)fromstate, "FromState");
    fromstate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ToTransition");
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = false;
    fromstate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)fromstate, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *tostate = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)tostate, "ToState");
    tostate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "FromTransition");
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = false;
    tostate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)tostate, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hascause = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hascause, "HasCause");
    hascause->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeCausedBy");
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = false;
    hascause->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascause, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);
    
    UA_ReferenceTypeNode *haseffect = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haseffect, "HasEffect");
    haseffect->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeEffectedBy");
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = false;
    haseffect->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseffect, nodeIdNonHierarchicalReferences, nodeIdHasSubType,true);

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hashistoricalconfiguration, "HasHistoricalConfiguration");
    hashistoricalconfiguration->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HistoricalConfigurationOf");
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = false;
    hashistoricalconfiguration->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hashistoricalconfiguration, UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType,true);

    /**************/
    /* Data Types */
    /**************/

    UA_DataTypeNode *basedatatype = UA_NodeStore_newDataTypeNode();
    copyNames((UA_Node*)basedatatype, "BaseDataType");
    basedatatype->nodeId.identifier.numeric = UA_NS0ID_BASEDATATYPE;
    basedatatype->isAbstract = true;
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)basedatatype);
    UA_RCU_UNLOCK();

    addDataTypeNode(server, "Boolean", UA_NS0ID_BOOLEAN, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Number", UA_NS0ID_NUMBER, true, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Float", UA_NS0ID_FLOAT, false, UA_NS0ID_NUMBER);
    addDataTypeNode(server, "Double", UA_NS0ID_DOUBLE, false, UA_NS0ID_NUMBER);
    addDataTypeNode(server, "Integer", UA_NS0ID_INTEGER, true, UA_NS0ID_NUMBER);
       addDataTypeNode(server, "SByte", UA_NS0ID_SBYTE, false, UA_NS0ID_INTEGER);
       addDataTypeNode(server, "Int16", UA_NS0ID_INT16, false, UA_NS0ID_INTEGER);
       addDataTypeNode(server, "Int32", UA_NS0ID_INT32, false, UA_NS0ID_INTEGER);
       addDataTypeNode(server, "Int64", UA_NS0ID_INT64, false, UA_NS0ID_INTEGER);
       addDataTypeNode(server, "UInteger", UA_NS0ID_UINTEGER, true, UA_NS0ID_INTEGER);
          addDataTypeNode(server, "Byte", UA_NS0ID_BYTE, false, UA_NS0ID_UINTEGER);
          addDataTypeNode(server, "UInt16", UA_NS0ID_UINT16, false, UA_NS0ID_UINTEGER);
          addDataTypeNode(server, "UInt32", UA_NS0ID_UINT32, false, UA_NS0ID_UINTEGER);
          addDataTypeNode(server, "UInt64", UA_NS0ID_UINT64, false, UA_NS0ID_UINTEGER);
    addDataTypeNode(server, "String", UA_NS0ID_STRING, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DateTime", UA_NS0ID_DATETIME, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Guid", UA_NS0ID_GUID, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ByteString", UA_NS0ID_BYTESTRING, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "XmlElement", UA_NS0ID_XMLELEMENT, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "NodeId", UA_NS0ID_NODEID, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ExpandedNodeId", UA_NS0ID_EXPANDEDNODEID, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "StatusCode", UA_NS0ID_STATUSCODE, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "QualifiedName", UA_NS0ID_QUALIFIEDNAME, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "LocalizedText", UA_NS0ID_LOCALIZEDTEXT, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Structure", UA_NS0ID_STRUCTURE, true, UA_NS0ID_BASEDATATYPE);
       addDataTypeNode(server, "ServerStatusDataType", UA_NS0ID_SERVERSTATUSDATATYPE, false, UA_NS0ID_STRUCTURE);
       addDataTypeNode(server, "BuildInfo", UA_NS0ID_BUILDINFO, false, UA_NS0ID_STRUCTURE);
    addDataTypeNode(server, "DataValue", UA_NS0ID_DATAVALUE, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DiagnosticInfo", UA_NS0ID_DIAGNOSTICINFO, false, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Enumeration", UA_NS0ID_ENUMERATION, true, UA_NS0ID_BASEDATATYPE);
       addDataTypeNode(server, "ServerState", UA_NS0ID_SERVERSTATE, false, UA_NS0ID_ENUMERATION);

    /*****************/
    /* VariableTypes */
    /*****************/

    UA_VariableTypeNode *basevartype =
        createVariableTypeNode(server, "BaseVariableType", UA_NS0ID_BASEVARIABLETYPE, true);
    basevartype->valueRank = -2;
    basevartype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)basevartype);
    UA_RCU_UNLOCK();

    UA_VariableTypeNode *basedatavartype =
        createVariableTypeNode(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE, false);
    basedatavartype->valueRank = -2;
    basedatavartype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)basedatavartype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),true);

    UA_VariableTypeNode *propertytype =
        createVariableTypeNode(server, "PropertyType", UA_NS0ID_PROPERTYTYPE, false);
    propertytype->valueRank = -2;
    propertytype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)propertytype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),true);

    UA_VariableTypeNode *buildinfotype =
        createVariableTypeNode(server, "BuildInfoType", UA_NS0ID_BUILDINFOTYPE, false);
    buildinfotype->valueRank = -1;
    buildinfotype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BUILDINFO);
    addNodeInternalWithType(server, (UA_Node*)buildinfotype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableTypeNode *serverstatustype =
        createVariableTypeNode(server, "ServerStatusType", UA_NS0ID_SERVERSTATUSTYPE, false);
    serverstatustype->valueRank = -1;
    serverstatustype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERSTATUSDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)serverstatustype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    /**********************/
    /* Basic Object Types */
    /**********************/

    UA_ObjectTypeNode *baseobjtype = UA_NodeStore_newObjectTypeNode();
    copyNames((UA_Node*)baseobjtype, "BaseObjectType");
    baseobjtype->nodeId.identifier.numeric = UA_NS0ID_BASEOBJECTTYPE;
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)baseobjtype);
    UA_RCU_UNLOCK();

    addObjectTypeNode(server, "FolderType", UA_NS0ID_FOLDERTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerType", UA_NS0ID_SERVERTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerDiagnosticsType", UA_NS0ID_SERVERDIAGNOSTICSTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerCapatilitiesType", UA_NS0ID_SERVERCAPABILITIESTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);

    /******************/
    /* Root and below */
    /******************/

    UA_ObjectNode *root = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root);
    UA_RCU_UNLOCK();
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);

    UA_ObjectNode *objects = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    addNodeInternalWithType(server, (UA_Node*)objects, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);

    UA_ObjectNode *types = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)types, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);

    UA_ObjectNode *referencetypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)referencetypes, "ReferenceTypes");
    referencetypes->nodeId.identifier.numeric = UA_NS0ID_REFERENCETYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)referencetypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES), true);

    UA_ObjectNode *datatypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)datatypes, "DataTypes");
    datatypes->nodeId.identifier.numeric = UA_NS0ID_DATATYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)datatypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), true);

    UA_ObjectNode *variabletypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)variabletypes, "VariableTypes");
    variabletypes->nodeId.identifier.numeric = UA_NS0ID_VARIABLETYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)variabletypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VARIABLETYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE), true);

    UA_ObjectNode *objecttypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)objecttypes, "ObjectTypes");
    objecttypes->nodeId.identifier.numeric = UA_NS0ID_OBJECTTYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)objecttypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTTYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), true);

    UA_ObjectNode *eventtypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)eventtypes, "EventTypes");
    eventtypes->nodeId.identifier.numeric = UA_NS0ID_EVENTTYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)eventtypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);

    UA_ObjectNode *views = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    addNodeInternalWithType(server, (UA_Node*)views, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType,true);

#else
    /* load the generated namespace externally */
    ua_namespaceinit_generated(server);
#endif

    /*********************/
    /* The Server Object */
    /*********************/
    
    /* Create our own server object */ 
    UA_ObjectNode *servernode = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)servernode, "Server");
    servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
    addNodeInternalWithType(server, (UA_Node*)servernode, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            nodeIdOrganizes, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE),true);
    
    // If we are in an UA conformant namespace, the above function just created a full ServerType object.
    // Before readding every variable, delete whatever got instantiated.
    deleteInstanceChildren(server, &servernode->nodeId);
    
    UA_VariableNode *namespaceArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)namespaceArray, "NamespaceArray");
    namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
    namespaceArray->valueSource = UA_VALUESOURCE_DATASOURCE;
    namespaceArray->value.dataSource = (UA_DataSource) {.handle = server, .read = readNamespaces,
                                                        .write = NULL};
    namespaceArray->valueRank = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)namespaceArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *serverArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)serverArray, "ServerArray");
    serverArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERARRAY;
    UA_Variant_setArrayCopy(&serverArray->value.data.value.value,
                            &server->config.applicationDescription.applicationUri, 1,
                            &UA_TYPES[UA_TYPES_STRING]);
    serverArray->value.data.value.hasValue = true;
    serverArray->valueRank = 1;
    serverArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)serverArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_ObjectNode *servercapablities = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)servercapablities, "ServerCapabilities");
    servercapablities->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES;
    addNodeInternalWithType(server, (UA_Node*)servercapablities, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCAPABILITIESTYPE),true);

    UA_VariableNode *localeIdArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)localeIdArray, "LocaleIdArray");
    localeIdArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY;
    UA_String enLocale = UA_STRING("en");
    UA_Variant_setArrayCopy(&localeIdArray->value.data.value.value,
                            &enLocale, 1, &UA_TYPES[UA_TYPES_STRING]);
    localeIdArray->value.data.value.hasValue = true;
    localeIdArray->valueRank = 1;
    localeIdArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)localeIdArray,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *maxBrowseContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxBrowseContinuationPoints, "MaxBrowseContinuationPoints");
    maxBrowseContinuationPoints->nodeId.identifier.numeric =
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxBrowseContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxBrowseContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxBrowseContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    /** ServerProfileArray **/
#define MAX_PROFILEARRAY 16 //a *magic* limit to the number of supported profiles
#define ADDPROFILEARRAY(x) profileArray[profileArraySize++] = UA_STRING_ALLOC(x)
    UA_String profileArray[MAX_PROFILEARRAY];
    UA_UInt16 profileArraySize = 0;
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NanoEmbeddedDevice");

#ifdef UA_ENABLE_SERVICESET_NODEMANAGEMENT
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/NodeManagement");
#endif
#ifdef UA_ENABLE_SERVICESET_METHOD
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/Methods");
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS
    ADDPROFILEARRAY("http://opcfoundation.org/UA-Profile/Server/EmbeddedDataChangeSubscription");
#endif

    UA_VariableNode *serverProfileArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)serverProfileArray, "ServerProfileArray");
    serverProfileArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY;
    UA_Variant_setArray(&serverProfileArray->value.data.value.value,
                        UA_Array_new(profileArraySize, &UA_TYPES[UA_TYPES_STRING]),
                        profileArraySize, &UA_TYPES[UA_TYPES_STRING]);
    for(UA_UInt16 i=0;i<profileArraySize;i++)
        ((UA_String *)serverProfileArray->value.data.value.value.data)[i] = profileArray[i];
    serverProfileArray->value.data.value.hasValue = true;
    serverProfileArray->valueRank = 1;
    serverProfileArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)serverProfileArray,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *softwareCertificates = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)softwareCertificates, "SoftwareCertificates");
    softwareCertificates->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SOFTWARECERTIFICATES;
    softwareCertificates->dataType = UA_TYPES[UA_TYPES_SIGNEDSOFTWARECERTIFICATE].typeId;
    addNodeInternalWithType(server, (UA_Node*)softwareCertificates,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *maxQueryContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxQueryContinuationPoints, "MaxQueryContinuationPoints");
    maxQueryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxQueryContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxQueryContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxQueryContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *maxHistoryContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxHistoryContinuationPoints, "MaxHistoryContinuationPoints");
    maxHistoryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxHistoryContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxHistoryContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxHistoryContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *minSupportedSampleRate = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)minSupportedSampleRate, "MinSupportedSampleRate");
    minSupportedSampleRate->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE;
    UA_Variant_setScalar(&minSupportedSampleRate->value.data.value.value,
                         UA_Double_new(), &UA_TYPES[UA_TYPES_DOUBLE]);
    minSupportedSampleRate->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)minSupportedSampleRate,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_ObjectNode *modellingRules = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)modellingRules, "ModellingRules");
    modellingRules->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES;
    addNodeInternalWithType(server, (UA_Node*)modellingRules,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),true);

    UA_ObjectNode *aggregateFunctions = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)aggregateFunctions, "AggregateFunctions");
    aggregateFunctions->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_AGGREGATEFUNCTIONS;
    addNodeInternalWithType(server, (UA_Node*)aggregateFunctions,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),true);

    UA_ObjectNode *serverdiagnostics = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)serverdiagnostics, "ServerDiagnostics");
    serverdiagnostics->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS;
    addNodeInternalWithType(server, (UA_Node*)serverdiagnostics,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERDIAGNOSTICSTYPE),true);

    UA_VariableNode *enabledFlag = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)enabledFlag, "EnabledFlag");
    enabledFlag->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG;
    UA_Variant_setScalar(&enabledFlag->value.data.value.value, UA_Boolean_new(),
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    enabledFlag->value.data.value.hasValue = true;
    enabledFlag->valueRank = 1;
    enabledFlag->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)enabledFlag,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *serverstatus = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)serverstatus, "ServerStatus");
    serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE;
    serverstatus->value.dataSource = (UA_DataSource) {.handle = server, .read = readStatus,
                                                      .write = NULL};
    addNodeInternalWithType(server, (UA_Node*)serverstatus, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *starttime = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)starttime, "StartTime");
    starttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
    UA_Variant_setScalarCopy(&starttime->value.data.value.value,
                             &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    starttime->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)starttime,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *currenttime = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)currenttime, "CurrentTime");
    currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    currenttime->valueSource = UA_VALUESOURCE_DATASOURCE;
    currenttime->value.dataSource = (UA_DataSource) {.handle = NULL, .read = readCurrentTime,
                                                     .write = NULL};
    addNodeInternalWithType(server, (UA_Node*)currenttime,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *state = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)state, "State");
    state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
    UA_Variant_setScalar(&state->value.data.value.value, UA_ServerState_new(),
                         &UA_TYPES[UA_TYPES_SERVERSTATE]);
    state->value.data.value.hasValue = true;
    state->minimumSamplingInterval = 500.0f;
    addNodeInternalWithType(server, (UA_Node*)state, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *buildinfo = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)buildinfo, "BuildInfo");
    buildinfo->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_Variant_setScalarCopy(&buildinfo->value.data.value.value,
                             &server->config.buildInfo, &UA_TYPES[UA_TYPES_BUILDINFO]);
    buildinfo->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)buildinfo,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BUILDINFOTYPE),true);

    UA_VariableNode *producturi = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)producturi, "ProductUri");
    producturi->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    UA_Variant_setScalarCopy(&producturi->value.data.value.value, &server->config.buildInfo.productUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    producturi->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)producturi,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *manufacturername = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)manufacturername, "ManufacturerName");
    manufacturername->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME);
    UA_Variant_setScalarCopy(&manufacturername->value.data.value.value,
                             &server->config.buildInfo.manufacturerName,
                             &UA_TYPES[UA_TYPES_STRING]);
    manufacturername->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)manufacturername,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *productname = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)productname, "ProductName");
    productname->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME);
    UA_Variant_setScalarCopy(&productname->value.data.value.value, &server->config.buildInfo.productName,
                             &UA_TYPES[UA_TYPES_STRING]);
    productname->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)productname,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *softwareversion = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)softwareversion, "SoftwareVersion");
    softwareversion->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION);
    UA_Variant_setScalarCopy(&softwareversion->value.data.value.value,
                             &server->config.buildInfo.softwareVersion, &UA_TYPES[UA_TYPES_STRING]);
    softwareversion->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)softwareversion,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *buildnumber = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)buildnumber, "BuildNumber");
    buildnumber->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER);
    UA_Variant_setScalarCopy(&buildnumber->value.data.value.value, &server->config.buildInfo.buildNumber,
                             &UA_TYPES[UA_TYPES_STRING]);
    buildnumber->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)buildnumber,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *builddate = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)builddate, "BuildDate");
    builddate->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE);
    UA_Variant_setScalarCopy(&builddate->value.data.value.value, &server->config.buildInfo.buildDate,
                             &UA_TYPES[UA_TYPES_DATETIME]);
    builddate->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)builddate,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *secondstillshutdown = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)secondstillshutdown, "SecondsTillShutdown");
    secondstillshutdown->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN);
    UA_Variant_setScalar(&secondstillshutdown->value.data.value.value, UA_UInt32_new(),
                         &UA_TYPES[UA_TYPES_UINT32]);
    secondstillshutdown->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)secondstillshutdown,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *shutdownreason = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)shutdownreason, "ShutdownReason");
    shutdownreason->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON);
    UA_Variant_setScalar(&shutdownreason->value.data.value.value, UA_LocalizedText_new(),
                         &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    shutdownreason->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)shutdownreason,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),true);

    UA_VariableNode *servicelevel = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)servicelevel, "ServiceLevel");
    servicelevel->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL);
    servicelevel->valueSource = UA_VALUESOURCE_DATASOURCE;
    servicelevel->value.dataSource = (UA_DataSource) {.handle = server, .read = readServiceLevel,
                                                      .write = NULL};
    addNodeInternalWithType(server, (UA_Node*)servicelevel,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_VariableNode *auditing = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)auditing, "Auditing");
    auditing->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING);
    auditing->valueSource = UA_VALUESOURCE_DATASOURCE;
    auditing->value.dataSource = (UA_DataSource) {.handle = server, .read = readAuditing, .write = NULL};
    addNodeInternalWithType(server, (UA_Node*)auditing,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

    UA_ObjectNode *vendorServerInfo = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)vendorServerInfo, "VendorServerInfo");
    vendorServerInfo->nodeId.identifier.numeric = UA_NS0ID_SERVER_VENDORSERVERINFO;
    addNodeInternalWithType(server, (UA_Node*)vendorServerInfo,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),true);
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_VENDORSERVERINFO),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_VENDORSERVERINFOTYPE), true);
    */


    UA_ObjectNode *serverRedundancy = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)serverRedundancy, "ServerRedundancy");
    serverRedundancy->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERREDUNDANCY;
    addNodeInternalWithType(server, (UA_Node*)serverRedundancy,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),true);
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERREDUNDANCYTYPE), true);
    */

    UA_VariableNode *redundancySupport = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)redundancySupport, "RedundancySupport");
    redundancySupport->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT);
    //FIXME: enum is needed for type letting it uninitialized for now
    UA_Variant_setScalar(&redundancySupport->value.data.value.value, UA_Int32_new(),
                         &UA_TYPES[UA_TYPES_INT32]);
    redundancySupport->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)redundancySupport,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),true);

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments.name = UA_STRING("SubscriptionId");
    inputArguments.valueRank = -1; /* scalar argument */

    UA_Argument outputArguments[2];
    UA_Argument_init(&outputArguments[0]);
    outputArguments[0].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outputArguments[0].name = UA_STRING("ServerHandles");
    outputArguments[0].valueRank = 1;

    UA_Argument_init(&outputArguments[1]);
    outputArguments[1].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outputArguments[1].name = UA_STRING("ClientHandles");
    outputArguments[1].valueRank = 1;

    UA_MethodAttributes addmethodattributes;
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("", "GetMonitoredItems");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;

    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(0, "GetMonitoredItems"), addmethodattributes,
        GetMonitoredItems, /* callback of the method node */
        NULL, /* handle passed with the callback */
        1, &inputArguments, 2, outputArguments, NULL);
#endif

    return server;
}

#ifdef UA_ENABLE_DISCOVERY
static UA_StatusCode register_server_with_discovery_server(UA_Server *server, const char* discoveryServerUrl, const UA_Boolean isUnregister, const char* semaphoreFilePath) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, discoveryServerUrl);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    UA_RegisterServerRequest request;
    UA_RegisterServerRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    request.server.isOnline = !isUnregister;

    // copy all the required data from applicationDescription to request
    retval |= UA_String_copy(&server->config.applicationDescription.applicationUri, &request.server.serverUri);
    retval |= UA_String_copy(&server->config.applicationDescription.productUri, &request.server.productUri);

    request.server.serverNamesSize = 1;
    request.server.serverNames = UA_malloc(sizeof(UA_LocalizedText));
    if (!request.server.serverNames) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    retval |= UA_LocalizedText_copy(&server->config.applicationDescription.applicationName, &request.server.serverNames[0]);
    
    request.server.serverType = server->config.applicationDescription.applicationType;
    retval |= UA_String_copy(&server->config.applicationDescription.gatewayServerUri, &request.server.gatewayServerUri);
    // TODO where do we get the discoveryProfileUri for application data?

    request.server.discoveryUrls = UA_malloc(sizeof(UA_String) * server->config.applicationDescription.discoveryUrlsSize);
    if (!request.server.serverNames) {
        UA_RegisteredServer_deleteMembers(&request.server);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    for (size_t i = 0; i<server->config.applicationDescription.discoveryUrlsSize; i++) {
        retval |= UA_String_copy(&server->config.applicationDescription.discoveryUrls[i], &request.server.discoveryUrls[i]);
    }

    /* add the discoveryUrls from the networklayers */
    UA_String *disc = UA_realloc(request.server.discoveryUrls, sizeof(UA_String) *
                                                                           (request.server.discoveryUrlsSize + server->config.networkLayersSize));
    if(!disc) {
        UA_RegisteredServer_deleteMembers(&request.server);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    size_t existing = request.server.discoveryUrlsSize;
    request.server.discoveryUrls = disc;
    request.server.discoveryUrlsSize += server->config.networkLayersSize;

    // TODO: Add nl only if discoveryUrl not already present
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        UA_String_copy(&nl->discoveryUrl, &request.server.discoveryUrls[existing + i]);
    }

    if (semaphoreFilePath) {
        request.server.semaphoreFilePath = UA_String_fromChars(semaphoreFilePath);
    }

    // now send the request
    UA_RegisterServerResponse response;
    UA_RegisterServerResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE]);

    UA_RegisterServerRequest_deleteMembers(&request);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer failed with statuscode 0x%08x", response.responseHeader.serviceResult);
        UA_RegisterServerResponse_deleteMembers(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return response.responseHeader.serviceResult;
    }


    UA_Client_disconnect(client);
    UA_Client_delete(client);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_register_discovery(UA_Server *server, const char* discoveryServerUrl, const char* semaphoreFilePath) {
    return register_server_with_discovery_server(server, discoveryServerUrl, UA_FALSE, semaphoreFilePath);
}

UA_StatusCode UA_Server_unregister_discovery(UA_Server *server, const char* discoveryServerUrl) {
    return register_server_with_discovery_server(server, discoveryServerUrl, UA_TRUE, NULL);
}
#endif
