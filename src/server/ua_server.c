#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_securechannel_manager.h"
#include "ua_session_manager.h"
#include "ua_util.h"
#include "ua_services.h"
#include "ua_nodeids.h"

#ifdef UA_ENABLE_GENERATE_NAMESPACE0
#include "ua_namespaceinit_generated.h"
#endif

#if defined(UA_ENABLE_MULTITHREADING) && !defined(NDEBUG)
UA_THREAD_LOCAL bool rcu_locked = false;
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

static const UA_ExpandedNodeId expandedNodeIdBaseDataVariabletype = {
    .nodeId = {.namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
               .identifier.numeric = UA_NS0ID_BASEDATAVARIABLETYPE},
    .namespaceUri = {.length = 0, .data = NULL}, .serverIndex = 0};

#ifndef UA_ENABLE_GENERATE_NAMESPACE0
static const UA_NodeId nodeIdNonHierarchicalReferences = {
        .namespaceIndex = 0, .identifierType = UA_NODEIDTYPE_NUMERIC,
        .identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES};
#endif

/**********************/
/* Namespace Handling */
/**********************/

static UA_UInt16 addNamespaceInternal(UA_Server *server, const UA_String *name) {
    //check if the namespace already exists in the server's namespace array
    for(UA_UInt16 i=0;i<server->namespacesSize;i++) {
        if(UA_String_equal(name, &server->namespaces[i]))
            return i;
    }
    //the namespace URI did not match - add a new namespace to the namsepace array
    server->namespaces = UA_realloc(server->namespaces,
        sizeof(UA_String) * (server->namespacesSize + 1));
    UA_String_copy(name, &(server->namespaces[server->namespacesSize]));
    server->namespacesSize++;
    return (UA_UInt16)(server->namespacesSize - 1);
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    UA_String nameString = UA_STRING_ALLOC(name);
    return addNamespaceInternal(server, &nameString);
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
                               UA_ExternalNodeStore *nodeStore, UA_UInt16 *assignedNamespaceIndex) {
    if(!nodeStore)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    size_t size = server->externalNamespacesSize;
    server->externalNamespaces =
        UA_realloc(server->externalNamespaces, sizeof(UA_ExternalNamespace) * (size + 1));
    server->externalNamespaces[size].externalNodeStore = *nodeStore;
    server->externalNamespaces[size].index = (UA_UInt16)server->namespacesSize;
    *assignedNamespaceIndex = (UA_UInt16)server->namespacesSize;
    UA_String_copy(url, &server->externalNamespaces[size].url);
    server->externalNamespacesSize++;
    addNamespaceInternal(server, url);
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_EXTERNAL_NAMESPACES*/

UA_StatusCode
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId, UA_Boolean deleteReferences) {
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_DeleteNodes_single(server, &adminSession, &nodeId, deleteReferences);
    UA_RCU_UNLOCK();
    return retval;
}

UA_StatusCode
UA_Server_deleteReference(UA_Server *server, const UA_NodeId sourceNodeId, const UA_NodeId referenceTypeId,
                          UA_Boolean isForward, const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional) {
    UA_DeleteReferencesItem item;
    item.sourceNodeId = sourceNodeId;
    item.referenceTypeId = referenceTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetNodeId;
    item.deleteBidirectional = deleteBidirectional;
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_DeleteReferences_single(server, &adminSession, &item);
    UA_RCU_UNLOCK();
    return retval;
}

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

UA_StatusCode
UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                       const UA_NodeId refTypeId, const UA_ExpandedNodeId targetId,
                       UA_Boolean isForward) {
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
                const UA_NodeId referenceTypeId) {
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    UA_RCU_LOCK();
    Service_AddNodes_existing(server, &adminSession, node, &parentNodeId,
                              &referenceTypeId, &res);
    UA_RCU_UNLOCK();
    return res;
}

UA_StatusCode
__UA_Server_addNode(UA_Server *server, const UA_NodeClass nodeClass,
                    const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                    const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                    const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType, 
                    UA_InstantiationCallback *instantiationCallback, UA_NodeId *outNewNodeId) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentNodeId;
    item.referenceTypeId = referenceTypeId;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.typeDefinition.nodeId = typeDefinition;
    item.nodeAttributes = (UA_ExtensionObject){.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE,
                                               .content.decoded = {attributeType, (void*)(uintptr_t)attr}};
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    UA_RCU_LOCK();
    Service_AddNodes_single(server, &adminSession, &item, &result, instantiationCallback);
    UA_RCU_UNLOCK();

    if(outNewNodeId && result.statusCode == UA_STATUSCODE_GOOD)
        *outNewNodeId = result.addedNodeId;
    else
        UA_AddNodesResult_deleteMembers(&result);
    return result.statusCode;
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

#ifdef UA_ENABLE_MULTITHREADING
    pthread_cond_destroy(&server->dispatchQueue_condition);
#endif
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void UA_Server_cleanup(UA_Server *server, void *_) {
    UA_DateTime now = UA_DateTime_now();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, now);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, now);
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
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &currentTime, &UA_TYPES[UA_TYPES_DATETIME]);
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
addDataTypeNode(UA_Server *server, char* name, UA_UInt32 datatypeid, UA_UInt32 parent) {
    UA_DataTypeNode *datatype = UA_NodeStore_newDataTypeNode();
    copyNames((UA_Node*)datatype, name);
    datatype->nodeId.identifier.numeric = datatypeid;
    addNodeInternal(server, (UA_Node*)datatype, UA_NODEID_NUMERIC(0, parent), nodeIdOrganizes);
}

static void
addObjectTypeNode(UA_Server *server, char* name, UA_UInt32 objecttypeid,
                  UA_UInt32 parent, UA_UInt32 parentreference) {
    UA_ObjectTypeNode *objecttype = UA_NodeStore_newObjectTypeNode();
    copyNames((UA_Node*)objecttype, name);
    objecttype->nodeId.identifier.numeric = objecttypeid;
    addNodeInternal(server, (UA_Node*)objecttype, UA_NODEID_NUMERIC(0, parent),
                    UA_NODEID_NUMERIC(0, parentreference));
}

static UA_VariableTypeNode*
createVariableTypeNode(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                       UA_UInt32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = UA_NodeStore_newVariableTypeNode();
    copyNames((UA_Node*)variabletype, name);
    variabletype->nodeId.identifier.numeric = variabletypeid;
    variabletype->isAbstract = abstract;
    variabletype->value.variant.value.type = &UA_TYPES[UA_TYPES_VARIANT];
    return variabletype;
}

static void
addVariableTypeNode_organized(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                              UA_UInt32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = createVariableTypeNode(server, name, variabletypeid, parent, abstract);
    addNodeInternal(server, (UA_Node*)variabletype, UA_NODEID_NUMERIC(0, parent), nodeIdOrganizes);
}

static void
addVariableTypeNode_subtype(UA_Server *server, char* name, UA_UInt32 variabletypeid,
                            UA_UInt32 parent, UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype =
        createVariableTypeNode(server, name, variabletypeid, parent, abstract);
    addNodeInternal(server, (UA_Node*)variabletype, UA_NODEID_NUMERIC(0, parent), nodeIdHasSubType);
}

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
#endif

    /* uncomment for non-reproducible server runs */
    //UA_random_seed(UA_DateTime_now());

    /* ns0 and ns1 */
    server->namespaces = UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

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

    server->startTime = UA_DateTime_now();

    /**************/
    /* References */
    /**************/
#ifndef UA_ENABLE_GENERATE_NAMESPACE0
    /* Bootstrap by manually inserting "references" and "hassubtype" */
    UA_ReferenceTypeNode *references = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = true;
    references->symmetric = true;
    references->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "References");
    /* The reference to root is later inserted */
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)references);
    UA_RCU_UNLOCK();

    UA_ReferenceTypeNode *hassubtype = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hassubtype, "HasSubtype");
    hassubtype->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HasSupertype");
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = false;
    hassubtype->symmetric = false;
    /* The reference to root is later inserted */
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)hassubtype);
    UA_RCU_UNLOCK();

    /* Continue adding reference types with normal "addnode" */
    UA_ReferenceTypeNode *hierarchicalreferences = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hierarchicalreferences, "HierarchicalReferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = true;
    hierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = true;
    nonhierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)nonhierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haschild = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = false;
    haschild->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haschild,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *organizes = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)organizes, "Organizes");
    organizes->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrganizedBy");
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = false;
    organizes->symmetric  = false;
    addNodeInternal(server, (UA_Node*)organizes,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haseventsource = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haseventsource, "HasEventSource");
    haseventsource->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EventSourceOf");
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = false;
    haseventsource->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseventsource,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodellingrule = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodellingrule, "HasModellingRule");
    hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModellingRuleOf");
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = false;
    hasmodellingrule->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodellingrule, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hasencoding = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasencoding, "HasEncoding");
    hasencoding->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EncodingOf");
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = false;
    hasencoding->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasencoding, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hasdescription = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasdescription, "HasDescription");
    hasdescription->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "DescriptionOf");
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = false;
    hasdescription->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasdescription, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hastypedefinition = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hastypedefinition, "HasTypeDefinition");
    hastypedefinition->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "TypeDefinitionOf");
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = false;
    hastypedefinition->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hastypedefinition, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *generatesevent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)generatesevent, "GeneratesEvent");
    generatesevent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "GeneratedBy");
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = false;
    generatesevent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)generatesevent, nodeIdNonHierarchicalReferences,
                    nodeIdHasSubType);

    UA_ReferenceTypeNode *aggregates = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)aggregates, "Aggregates");
    // Todo: Is there an inverse name?
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = false;
    aggregates->symmetric  = false;
    addNodeInternal(server, (UA_Node*)aggregates,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType);

    // complete bootstrap of hassubtype
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), true);

    UA_ReferenceTypeNode *hasproperty = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasproperty, "HasProperty");
    hasproperty->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "PropertyOf");
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = false;
    hasproperty->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasproperty,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hascomponent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hascomponent, "HasComponent");
    hascomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ComponentOf");
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = false;
    hascomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascomponent,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasnotifier = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasnotifier, "HasNotifier");
    hasnotifier->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "NotifierOf");
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = false;
    hasnotifier->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasnotifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE),
                    nodeIdHasSubType);

    UA_ReferenceTypeNode *hasorderedcomponent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasorderedcomponent, "HasOrderedComponent");
    hasorderedcomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrderedComponentOf");
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = false;
    hasorderedcomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasorderedcomponent, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                    nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodelparent = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodelparent, "HasModelParent");
    hasmodelparent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModelParentOf");
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = false;
    hasmodelparent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodelparent, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *fromstate = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)fromstate, "FromState");
    fromstate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ToTransition");
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = false;
    fromstate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)fromstate, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *tostate = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)tostate, "ToState");
    tostate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "FromTransition");
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = false;
    tostate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)tostate, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hascause = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hascause, "HasCause");
    hascause->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeCausedBy");
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = false;
    hascause->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascause, nodeIdNonHierarchicalReferences, nodeIdHasSubType);
    
    UA_ReferenceTypeNode *haseffect = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)haseffect, "HasEffect");
    haseffect->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeEffectedBy");
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = false;
    haseffect->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseffect, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_NodeStore_newReferenceTypeNode();
    copyNames((UA_Node*)hashistoricalconfiguration, "HasHistoricalConfiguration");
    hashistoricalconfiguration->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HistoricalConfigurationOf");
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = false;
    hashistoricalconfiguration->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hashistoricalconfiguration,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    /*****************/
    /* Basic Folders */
    /*****************/

    UA_ObjectNode *root = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_RCU_LOCK();
    UA_NodeStore_insert(server->nodestore, (UA_Node*)root);
    UA_RCU_UNLOCK();

    UA_ObjectNode *objects = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    addNodeInternal(server, (UA_Node*)objects, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                    nodeIdOrganizes);

    UA_ObjectNode *types = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    addNodeInternal(server, (UA_Node*)types, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                    nodeIdOrganizes);

    UA_ObjectNode *views = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    addNodeInternal(server, (UA_Node*)views, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                    nodeIdOrganizes);

    UA_ObjectNode *referencetypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)referencetypes, "ReferenceTypes");
    referencetypes->nodeId.identifier.numeric = UA_NS0ID_REFERENCETYPESFOLDER;
    addNodeInternal(server, (UA_Node*)referencetypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                    nodeIdOrganizes);

    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES), true);

    /**********************/
    /* Basic Object Types */
    /**********************/

    UA_ObjectNode *objecttypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)objecttypes, "ObjectTypes");
    objecttypes->nodeId.identifier.numeric = UA_NS0ID_OBJECTTYPESFOLDER;
    addNodeInternal(server, (UA_Node*)objecttypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                    nodeIdOrganizes);

    addObjectTypeNode(server, "BaseObjectType", UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_OBJECTTYPESFOLDER,
                      UA_NS0ID_ORGANIZES);
    addObjectTypeNode(server, "FolderType", UA_NS0ID_FOLDERTYPE, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTTYPESFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addObjectTypeNode(server, "ServerType", UA_NS0ID_SERVERTYPE, UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerDiagnosticsType", UA_NS0ID_SERVERDIAGNOSTICSTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerCapatilitiesType", UA_NS0ID_SERVERCAPABILITIESTYPE,
                      UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "ServerStatusType", UA_NS0ID_SERVERSTATUSTYPE, UA_NS0ID_BASEOBJECTTYPE,
                      UA_NS0ID_HASSUBTYPE);
    addObjectTypeNode(server, "BuildInfoType", UA_NS0ID_BUILDINFOTYPE, UA_NS0ID_BASEOBJECTTYPE,
                      UA_NS0ID_HASSUBTYPE);

    /**************/
    /* Data Types */
    /**************/

    UA_ObjectNode *datatypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)datatypes, "DataTypes");
    datatypes->nodeId.identifier.numeric = UA_NS0ID_DATATYPESFOLDER;
    addNodeInternal(server, (UA_Node*)datatypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), nodeIdOrganizes);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPESFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);

    addDataTypeNode(server, "BaseDataType", UA_NS0ID_BASEDATATYPE, UA_NS0ID_DATATYPESFOLDER);
    addDataTypeNode(server, "Boolean", UA_NS0ID_BOOLEAN, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Number", UA_NS0ID_NUMBER, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "Float", UA_NS0ID_FLOAT, UA_NS0ID_NUMBER);
        addDataTypeNode(server, "Double", UA_NS0ID_DOUBLE, UA_NS0ID_NUMBER);
        addDataTypeNode(server, "Integer", UA_NS0ID_INTEGER, UA_NS0ID_NUMBER);
            addDataTypeNode(server, "SByte", UA_NS0ID_SBYTE, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int16", UA_NS0ID_INT16, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int32", UA_NS0ID_INT32, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "Int64", UA_NS0ID_INT64, UA_NS0ID_INTEGER);
            addDataTypeNode(server, "UInteger", UA_NS0ID_UINTEGER, UA_NS0ID_INTEGER);
                addDataTypeNode(server, "Byte", UA_NS0ID_BYTE, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt16", UA_NS0ID_UINT16, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt32", UA_NS0ID_UINT32, UA_NS0ID_UINTEGER);
                addDataTypeNode(server, "UInt64", UA_NS0ID_UINT64, UA_NS0ID_UINTEGER);
    addDataTypeNode(server, "String", UA_NS0ID_STRING, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DateTime", UA_NS0ID_DATETIME, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Guid", UA_NS0ID_GUID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ByteString", UA_NS0ID_BYTESTRING, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "XmlElement", UA_NS0ID_XMLELEMENT, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "NodeId", UA_NS0ID_NODEID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "ExpandedNodeId", UA_NS0ID_EXPANDEDNODEID, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "StatusCode", UA_NS0ID_STATUSCODE, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "QualifiedName", UA_NS0ID_QUALIFIEDNAME, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "LocalizedText", UA_NS0ID_LOCALIZEDTEXT, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Structure", UA_NS0ID_STRUCTURE, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "ServerStatusDataType", UA_NS0ID_SERVERSTATUSDATATYPE, UA_NS0ID_STRUCTURE);
        addDataTypeNode(server, "BuildInfo", UA_NS0ID_BUILDINFO, UA_NS0ID_STRUCTURE);
    addDataTypeNode(server, "DataValue", UA_NS0ID_DATAVALUE, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "DiagnosticInfo", UA_NS0ID_DIAGNOSTICINFO, UA_NS0ID_BASEDATATYPE);
    addDataTypeNode(server, "Enumeration", UA_NS0ID_ENUMERATION, UA_NS0ID_BASEDATATYPE);
        addDataTypeNode(server, "ServerState", UA_NS0ID_SERVERSTATE, UA_NS0ID_ENUMERATION);

    UA_ObjectNode *variabletypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)variabletypes, "VariableTypes");
    variabletypes->nodeId.identifier.numeric = UA_NS0ID_VARIABLETYPESFOLDER;
    addNodeInternal(server, (UA_Node*)variabletypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                    nodeIdOrganizes);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VARIABLETYPESFOLDER),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
    addVariableTypeNode_organized(server, "BaseVariableType", UA_NS0ID_BASEVARIABLETYPE,
                                  UA_NS0ID_VARIABLETYPESFOLDER, true);
    addVariableTypeNode_subtype(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE,
                                UA_NS0ID_BASEVARIABLETYPE, false);
    addVariableTypeNode_subtype(server, "PropertyType", UA_NS0ID_PROPERTYTYPE,
                                UA_NS0ID_BASEVARIABLETYPE, false);


    //Event types folder below is needed by CTT
    /***************/
    /* Event Types */
    /***************/

    UA_ObjectNode *eventtypes = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)eventtypes, "EventTypes");
    eventtypes->nodeId.identifier.numeric = UA_NS0ID_EVENTTYPESFOLDER;
    addNodeInternal(server, (UA_Node*)eventtypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), nodeIdOrganizes);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTTYPESFOLDER), nodeIdHasTypeDefinition,
            UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);
#endif

#ifdef UA_ENABLE_GENERATE_NAMESPACE0
    //load the generated namespace
    ua_namespaceinit_generated(server);
#endif

    /*********************/
    /* The Server Object */
    /*********************/

    UA_ObjectNode *servernode = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)servernode, "Server");
    servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
    addNodeInternal(server, (UA_Node*)servernode, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                    nodeIdOrganizes);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE), true);

    UA_VariableNode *namespaceArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)namespaceArray, "NamespaceArray");
    namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
    namespaceArray->valueSource = UA_VALUESOURCE_DATASOURCE;
    namespaceArray->value.dataSource = (UA_DataSource) {.handle = server, .read = readNamespaces,
                                                        .write = NULL};
    namespaceArray->valueRank = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    addNodeInternal(server, (UA_Node*)namespaceArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *serverArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)serverArray, "ServerArray");
    serverArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERARRAY;
    UA_Variant_setArrayCopy(&serverArray->value.variant.value,
                            &server->config.applicationDescription.applicationUri, 1,
                            &UA_TYPES[UA_TYPES_STRING]);
    serverArray->valueRank = 1;
    serverArray->minimumSamplingInterval = 1.0;
    addNodeInternal(server, (UA_Node*)serverArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_ObjectNode *servercapablities = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)servercapablities, "ServerCapabilities");
    servercapablities->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES;
    addNodeInternal(server, (UA_Node*)servercapablities, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                    nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERCAPABILITIESTYPE), true);

    UA_VariableNode *localeIdArray = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)localeIdArray, "LocaleIdArray");
    localeIdArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY;
    localeIdArray->value.variant.value.data = UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    localeIdArray->value.variant.value.arrayLength = 1;
    localeIdArray->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING];
    *(UA_String *)localeIdArray->value.variant.value.data = UA_STRING_ALLOC("en");
    localeIdArray->valueRank = 1;
    localeIdArray->minimumSamplingInterval = 1.0;
    addNodeInternal(server, (UA_Node*)localeIdArray,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *maxBrowseContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxBrowseContinuationPoints, "MaxBrowseContinuationPoints");
    maxBrowseContinuationPoints->nodeId.identifier.numeric =
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS;
    maxBrowseContinuationPoints->value.variant.value.data = UA_UInt16_new();
    *((UA_UInt16*)maxBrowseContinuationPoints->value.variant.value.data) = MAXCONTINUATIONPOINTS;
    maxBrowseContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16];
    addNodeInternal(server, (UA_Node*)maxBrowseContinuationPoints,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

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
    serverProfileArray->value.variant.value.arrayLength = profileArraySize;
    serverProfileArray->value.variant.value.data = UA_Array_new(profileArraySize, &UA_TYPES[UA_TYPES_STRING]);
    serverProfileArray->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING];
    for(UA_UInt16 i=0;i<profileArraySize;i++)
        ((UA_String *)serverProfileArray->value.variant.value.data)[i] = profileArray[i];
    serverProfileArray->valueRank = 1;
    serverProfileArray->minimumSamplingInterval = 1.0;
    addNodeInternal(server, (UA_Node*)serverProfileArray,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *softwareCertificates = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)softwareCertificates, "SoftwareCertificates");
    softwareCertificates->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SOFTWARECERTIFICATES;
    softwareCertificates->value.variant.value.type = &UA_TYPES[UA_TYPES_SIGNEDSOFTWARECERTIFICATE];
    addNodeInternal(server, (UA_Node*)softwareCertificates,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_SOFTWARECERTIFICATES),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *maxQueryContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxQueryContinuationPoints, "MaxQueryContinuationPoints");
    maxQueryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS;
    maxQueryContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16];
    maxQueryContinuationPoints->value.variant.value.data = UA_UInt16_new();
    //FIXME
    *((UA_UInt16*)maxQueryContinuationPoints->value.variant.value.data) = 0;
    addNodeInternal(server, (UA_Node*)maxQueryContinuationPoints,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *maxHistoryContinuationPoints = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)maxHistoryContinuationPoints, "MaxHistoryContinuationPoints");
    maxHistoryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS;
    maxHistoryContinuationPoints->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT16];
    maxHistoryContinuationPoints->value.variant.value.data = UA_UInt16_new();
    //FIXME
    *((UA_UInt16*)maxHistoryContinuationPoints->value.variant.value.data) = 0;
    addNodeInternal(server, (UA_Node*)maxHistoryContinuationPoints,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *minSupportedSampleRate = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)minSupportedSampleRate, "MinSupportedSampleRate");
    minSupportedSampleRate->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE;
    minSupportedSampleRate->value.variant.value.type = &UA_TYPES[UA_TYPES_DOUBLE];
    minSupportedSampleRate->value.variant.value.data = UA_Double_new();
    //FIXME
    *((UA_Double*)minSupportedSampleRate->value.variant.value.data) = 0.0;
    addNodeInternal(server, (UA_Node*)minSupportedSampleRate,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_ObjectNode *modellingRules = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)modellingRules, "ModellingRules");
    modellingRules->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES;
    addNodeInternal(server, (UA_Node*)modellingRules,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);

    UA_ObjectNode *aggregateFunctions = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)aggregateFunctions, "AggregateFunctions");
    aggregateFunctions->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_AGGREGATEFUNCTIONS;
    addNodeInternal(server, (UA_Node*)aggregateFunctions,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_AGGREGATEFUNCTIONS),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);

    UA_ObjectNode *serverdiagnostics = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)serverdiagnostics, "ServerDiagnostics");
    serverdiagnostics->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS;
    addNodeInternal(server, (UA_Node*)serverdiagnostics,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERDIAGNOSTICSTYPE), true);

    UA_VariableNode *enabledFlag = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)enabledFlag, "EnabledFlag");
    enabledFlag->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG;
    enabledFlag->value.variant.value.data = UA_Boolean_new(); //initialized as false
    enabledFlag->value.variant.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    enabledFlag->valueRank = 1;
    enabledFlag->minimumSamplingInterval = 1.0;
    addNodeInternal(server, (UA_Node*)enabledFlag,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS), nodeIdHasProperty);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *serverstatus = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)serverstatus, "ServerStatus");
    serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE;
    serverstatus->value.dataSource = (UA_DataSource) {.handle = server, .read = readStatus, .write = NULL};
    addNodeInternal(server, (UA_Node*)serverstatus, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERSTATUSTYPE), true);

    UA_VariableNode *starttime = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)starttime, "StartTime");
    starttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
    starttime->value.variant.value.storageType = UA_VARIANT_DATA_NODELETE;
    starttime->value.variant.value.data = &server->startTime;
    starttime->value.variant.value.type = &UA_TYPES[UA_TYPES_DATETIME];
    addNodeInternal(server, (UA_Node*)starttime, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                    nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *currenttime = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)currenttime, "CurrentTime");
    currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    currenttime->valueSource = UA_VALUESOURCE_DATASOURCE;
    currenttime->value.dataSource = (UA_DataSource) {.handle = NULL, .read = readCurrentTime,
                                                     .write = NULL};
    addNodeInternal(server, (UA_Node*)currenttime,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *state = UA_NodeStore_newVariableNode();
    UA_ServerState *stateEnum = UA_ServerState_new();
    *stateEnum = UA_SERVERSTATE_RUNNING;
    copyNames((UA_Node*)state, "State");
    state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
    state->value.variant.value.type = &UA_TYPES[UA_TYPES_SERVERSTATE];
    state->value.variant.value.arrayLength = 0;
    state->value.variant.value.data = stateEnum; // points into the other object.
    addNodeInternal(server, (UA_Node*)state, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                    nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *buildinfo = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)buildinfo, "BuildInfo");
    buildinfo->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_Variant_setScalarCopy(&buildinfo->value.variant.value,
                             &server->config.buildInfo,
                             &UA_TYPES[UA_TYPES_BUILDINFO]);
    addNodeInternal(server, (UA_Node*)buildinfo,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BUILDINFOTYPE), true);

    UA_VariableNode *producturi = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)producturi, "ProductUri");
    producturi->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    UA_Variant_setScalarCopy(&producturi->value.variant.value, &server->config.buildInfo.productUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    producturi->value.variant.value.type = &UA_TYPES[UA_TYPES_STRING];
    addNodeInternal(server, (UA_Node*)producturi,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *manufacturername = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)manufacturername, "ManufacturerName");
    manufacturername->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME);
    UA_Variant_setScalarCopy(&manufacturername->value.variant.value,
                             &server->config.buildInfo.manufacturerName,
                             &UA_TYPES[UA_TYPES_STRING]);
    addNodeInternal(server, (UA_Node*)manufacturername,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *productname = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)productname, "ProductName");
    productname->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME);
    UA_Variant_setScalarCopy(&productname->value.variant.value, &server->config.buildInfo.productName,
                             &UA_TYPES[UA_TYPES_STRING]);
    addNodeInternal(server, (UA_Node*)productname,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *softwareversion = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)softwareversion, "SoftwareVersion");
    softwareversion->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION);
    UA_Variant_setScalarCopy(&softwareversion->value.variant.value, &server->config.buildInfo.softwareVersion,
                             &UA_TYPES[UA_TYPES_STRING]);
    addNodeInternal(server, (UA_Node*)softwareversion,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *buildnumber = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)buildnumber, "BuildNumber");
    buildnumber->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER);
    UA_Variant_setScalarCopy(&buildnumber->value.variant.value, &server->config.buildInfo.buildNumber,
                             &UA_TYPES[UA_TYPES_STRING]);
    addNodeInternal(server, (UA_Node*)buildnumber,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *builddate = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)builddate, "BuildDate");
    builddate->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE);
    UA_Variant_setScalarCopy(&builddate->value.variant.value, &server->config.buildInfo.buildDate,
                             &UA_TYPES[UA_TYPES_DATETIME]);
    addNodeInternal(server, (UA_Node*)builddate,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *secondstillshutdown = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)secondstillshutdown, "SecondsTillShutdown");
    secondstillshutdown->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN);
    secondstillshutdown->value.variant.value.data = UA_UInt32_new();
    secondstillshutdown->value.variant.value.type = &UA_TYPES[UA_TYPES_UINT32];
    addNodeInternal(server, (UA_Node*)secondstillshutdown,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *shutdownreason = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)shutdownreason, "ShutdownReason");
    shutdownreason->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON);
    shutdownreason->value.variant.value.data = UA_LocalizedText_new();
    shutdownreason->value.variant.value.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    addNodeInternal(server, (UA_Node*)shutdownreason,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON),
                         nodeIdHasTypeDefinition, expandedNodeIdBaseDataVariabletype, true);

    UA_VariableNode *servicelevel = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)servicelevel, "ServiceLevel");
    servicelevel->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL);
    servicelevel->valueSource = UA_VALUESOURCE_DATASOURCE;
    servicelevel->value.dataSource = (UA_DataSource) {.handle = server, .read = readServiceLevel, .write = NULL};
    addNodeInternal(server, (UA_Node*)servicelevel,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_VariableNode *auditing = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)auditing, "Auditing");
    auditing->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING);
    auditing->valueSource = UA_VALUESOURCE_DATASOURCE;
    auditing->value.dataSource = (UA_DataSource) {.handle = server, .read = readAuditing, .write = NULL};
    addNodeInternal(server, (UA_Node*)auditing,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    UA_ObjectNode *vendorServerInfo = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)vendorServerInfo, "VendorServerInfo");
    vendorServerInfo->nodeId.identifier.numeric = UA_NS0ID_SERVER_VENDORSERVERINFO;
    addNodeInternal(server, (UA_Node*)vendorServerInfo,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty);
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_VENDORSERVERINFO),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_VENDORSERVERINFOTYPE), true);
    */


    UA_ObjectNode *serverRedundancy = UA_NodeStore_newObjectNode();
    copyNames((UA_Node*)serverRedundancy, "ServerRedundancy");
    serverRedundancy->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERREDUNDANCY;
    addNodeInternal(server, (UA_Node*)serverRedundancy,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty);
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERREDUNDANCYTYPE), true);
    */

    UA_VariableNode *redundancySupport = UA_NodeStore_newVariableNode();
    copyNames((UA_Node*)redundancySupport, "RedundancySupport");
    redundancySupport->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT);
    //FIXME: enum is needed for type letting it uninitialized for now
    redundancySupport->value.variant.value.data = UA_Int32_new();
    redundancySupport->value.variant.value.type = &UA_TYPES[UA_TYPES_INT32];
    addNodeInternal(server, (UA_Node*)redundancySupport, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
            nodeIdHasComponent);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), true);

    return server;
}

UA_StatusCode
__UA_Server_write(UA_Server *server, const UA_NodeId *nodeId,
                  const UA_AttributeId attributeId, const UA_DataType *attr_type,
                  const void *value) {
    UA_WriteValue wvalue;
    UA_WriteValue_init(&wvalue);
    wvalue.nodeId = *nodeId;
    wvalue.attributeId = attributeId;
    if(attributeId != UA_ATTRIBUTEID_VALUE)
        /* hacked cast. the target WriteValue is used as const anyway */
        UA_Variant_setScalar(&wvalue.value.value, (void*)(uintptr_t)value, attr_type);
    else {
        if(attr_type != &UA_TYPES[UA_TYPES_VARIANT])
            return UA_STATUSCODE_BADTYPEMISMATCH;
        wvalue.value.value = *(const UA_Variant*)value;
    }
    wvalue.value.hasValue = true;
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wvalue);
    UA_RCU_UNLOCK();
    return retval;
}

static UA_StatusCode
setValueCallback(UA_Server *server, UA_Session *session, UA_VariableNode *node, UA_ValueCallback *callback) {
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->value.variant.callback = *callback;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_setVariableNode_valueCallback(UA_Server *server, const UA_NodeId nodeId,
                                        const UA_ValueCallback callback) {
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setValueCallback, &callback);
    UA_RCU_UNLOCK();
    return retval;
}

static UA_StatusCode
setDataSource(UA_Server *server, UA_Session *session,
              UA_VariableNode* node, UA_DataSource *dataSource) {
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    if(node->valueSource == UA_VALUESOURCE_VARIANT)
        UA_Variant_deleteMembers(&node->value.variant.value);
    node->value.dataSource = *dataSource;
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                                     const UA_DataSource dataSource) {
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setDataSource, &dataSource);
    UA_RCU_UNLOCK();
    return retval;
}

static UA_StatusCode
setObjectTypeLifecycleManagement(UA_Server *server, UA_Session *session, UA_ObjectTypeNode* node,
                                 UA_ObjectLifecycleManagement *olm) {
    if(node->nodeClass != UA_NODECLASS_OBJECTTYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->lifecycleManagement = *olm;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_setObjectTypeNode_lifecycleManagement(UA_Server *server, UA_NodeId nodeId,
                                                UA_ObjectLifecycleManagement olm) {
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setObjectTypeLifecycleManagement, &olm);
    UA_RCU_UNLOCK();
    return retval;
}

#ifdef UA_ENABLE_METHODCALLS

struct addMethodCallback {
    UA_MethodCallback callback;
    void *handle;
};

static UA_StatusCode
editMethodCallback(UA_Server *server, UA_Session* session, UA_Node* node, const void* handle) {
    if(node->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    const struct addMethodCallback *newCallback = handle;
    UA_MethodNode *mnode = (UA_MethodNode*) node;
    mnode->attachedMethod = newCallback->callback;
    mnode->methodHandle   = newCallback->handle;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_setMethodNode_callback(UA_Server *server, const UA_NodeId methodNodeId,
                                 UA_MethodCallback method, void *handle) {
    struct addMethodCallback cb = { method, handle };
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &methodNodeId, editMethodCallback, &cb);
    UA_RCU_UNLOCK();
    return retval;
}

#endif

UA_StatusCode
__UA_Server_read(UA_Server *server, const UA_NodeId *nodeId, const UA_AttributeId attributeId, void *v) {
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = *nodeId;
    item.attributeId = attributeId;
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_RCU_LOCK();
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER,
                        &item, &dv);
    UA_RCU_UNLOCK();
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(dv.hasStatus)
        retval = dv.hasStatus;
    else if(!dv.hasValue)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&dv);
        return retval;
    }
    if(attributeId == UA_ATTRIBUTEID_VALUE ||
       attributeId == UA_ATTRIBUTEID_ARRAYDIMENSIONS)
        memcpy(v, &dv.value, sizeof(UA_Variant));
    else {
        memcpy(v, dv.value.data, dv.value.type->memSize);
        dv.value.data = NULL;
        dv.value.arrayLength = 0;
        UA_Variant_deleteMembers(&dv.value);
    }
    return UA_STATUSCODE_GOOD;
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxrefs, const UA_BrowseDescription *descr) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_RCU_LOCK();
    Service_Browse_single(server, &adminSession, NULL, descr, maxrefs, &result);
    UA_RCU_UNLOCK();
    return result;
}

UA_BrowseResult
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_RCU_LOCK();
    UA_Server_browseNext_single(server, &adminSession, releaseContinuationPoint,
                                continuationPoint, &result);
    UA_RCU_UNLOCK();
    return result;
}

#ifdef UA_ENABLE_METHODCALLS
UA_CallMethodResult UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request) {
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    UA_RCU_LOCK();
    Service_Call_single(server, &adminSession, request, &result);
    UA_RCU_UNLOCK();
    return result;
}
#endif
