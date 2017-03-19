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
    0,UA_NODEIDTYPE_NUMERIC,{UA_NS0ID_HASSUBTYPE}};
static const UA_NodeId nodeIdHasComponent = {
    0,UA_NODEIDTYPE_NUMERIC,{UA_NS0ID_HASCOMPONENT}};
static const UA_NodeId nodeIdHasProperty = {
    0,UA_NODEIDTYPE_NUMERIC,{UA_NS0ID_HASPROPERTY}};
static const UA_NodeId nodeIdOrganizes = {
    0,UA_NODEIDTYPE_NUMERIC,{UA_NS0ID_ORGANIZES}};

#ifndef UA_ENABLE_GENERATE_NAMESPACE0
static const UA_NodeId nodeIdNonHierarchicalReferences = {
    0,UA_NODEIDTYPE_NUMERIC,{UA_NS0ID_NONHIERARCHICALREFERENCES}};
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

    char urlString[256];
    if(url->length >= 256)
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(urlString, url->data, url->length);
    urlString[url->length] = 0;

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
    res.statusCode = Service_AddNodes_existing(server, &adminSession, node, &parentNodeId,
                                               &referenceTypeId, &UA_NODEID_NULL,
                                               NULL, &res.addedNodeId);
    UA_RCU_UNLOCK();
    return res;
}

static UA_AddNodesResult
addNodeInternalWithType(UA_Server *server, UA_Node *node, const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId, const UA_NodeId typeIdentifier) {
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    UA_RCU_LOCK();
    res.statusCode = Service_AddNodes_existing(server, &adminSession, node, &parentNodeId,
                                               &referenceTypeId, &typeIdentifier,
                                               NULL, &res.addedNodeId);
    UA_RCU_UNLOCK();
    return res;
}

// delete any children of an instance without touching the object itself
static void
deleteInstanceChildren(UA_Server *server, UA_NodeId *objectNodeId) {
    UA_BrowseDescription bDes;
    UA_BrowseDescription_init(&bDes);
    UA_NodeId_copy(objectNodeId, &bDes.nodeId );
    bDes.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bDes.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    bDes.resultMask = UA_BROWSERESULTMASK_ISFORWARD | UA_BROWSERESULTMASK_NODECLASS |
        UA_BROWSERESULTMASK_REFERENCETYPEINFO;
    UA_BrowseResult bRes;
    UA_BrowseResult_init(&bRes);
    UA_RCU_LOCK();
    Service_Browse_single(server, &adminSession, NULL, &bDes, 0, &bRes);
    UA_RCU_unLOCK();
    for(size_t i=0; i<bRes.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &bRes.references[i];
        if((rd->nodeClass == UA_NODECLASS_OBJECT || rd->nodeClass == UA_NODECLASS_VARIABLE)) {
            Service_DeleteNodes_single(server, &adminSession, &rd->nodeId.nodeId, UA_TRUE) ;
        } else if (rd->nodeClass == UA_NODECLASS_METHOD) {
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
    //Copy namespace array in UA_String array
    UA_String* namespacesArray = (UA_String*)UA_malloc(sizeof(UA_String) *server->namespacesSize);
    if(!namespacesArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < server->namespacesSize; ++i){
        namespacesArray[i] = server->namespaces[i].uri;
    }
    UA_Variant_setArrayCopy(&value->value, namespacesArray,
            server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);
    UA_free(namespacesArray);
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

static void copyNames(UA_Node *node, const char *name) {
    node->browseName = UA_QUALIFIEDNAME_ALLOC(0, name);
    node->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", name);
    node->description = UA_LOCALIZEDTEXT_ALLOC("en_US", name);
}

static void
addDataTypeNode(UA_Server *server, const char* name, UA_UInt32 datatypeid,
                UA_Boolean isAbstract, UA_UInt32 parent) {
    UA_DataTypeNode *datatype = UA_Nodestore_newDataTypeNode();
    copyNames((UA_Node*)datatype, name);
    datatype->nodeId.identifier.numeric = datatypeid;
    datatype->isAbstract = isAbstract;
    addNodeInternal(server, (UA_Node*)datatype,
                    UA_NODEID_NUMERIC(0, parent), nodeIdHasSubType);
}

static void
addObjectTypeNode(UA_Server *server, const char* name, UA_UInt32 objecttypeid,
                  UA_UInt32 parent, UA_UInt32 parentreference) {
    UA_ObjectTypeNode *objecttype =UA_Nodestore_newObjectTypeNode();
    copyNames((UA_Node*)objecttype, name);
    objecttype->nodeId.identifier.numeric = objecttypeid;
    addNodeInternal(server, (UA_Node*)objecttype, UA_NODEID_NUMERIC(0, parent),
                    UA_NODEID_NUMERIC(0, parentreference));
}

static UA_VariableTypeNode*
createVariableTypeNode(UA_Server *server, const char* name, UA_UInt32 variabletypeid,
                       UA_Boolean abstract) {
    UA_VariableTypeNode *variabletype = UA_Nodestore_newVariableTypeNode();
    copyNames((UA_Node*)variabletype, name);
    variabletype->nodeId.identifier.numeric = variabletypeid;
    variabletype->isAbstract = abstract;
    return variabletype;
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

    UA_UInt32* clientHandles = (UA_UInt32 *)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32* serverHandles = (UA_UInt32 *)UA_Array_new(sizeOfOutput, &UA_TYPES[UA_TYPES_UINT32]);
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
    //Initialize a default nodeStoreInterface for namespaces
    server->nodestore_std = (UA_NodestoreInterface*)UA_malloc(sizeof(UA_NodestoreInterface));
    *server->nodestore_std = UA_Nodestore_standard();
    /* Namespace0 and Namespace1 initialization*/
    //TODO move to UA_ServerConfig_standard as namespace array of size2
    UA_Namespace *ns0 = UA_Namespace_newFromChar("http://opcfoundation.org/UA/");
    ns0->dataTypes = UA_TYPES;
    ns0->dataTypesSize = UA_TYPES_COUNT;
    UA_Server_addNamespace_full(server, ns0);
    UA_Namespace_deleteMembers(ns0);
    UA_free(ns0);

    UA_Namespace *ns1 = UA_Namespace_new(&config.applicationDescription.applicationUri);
    UA_Server_addNamespace_full(server, ns1);
    UA_Namespace_deleteMembers(ns1);
    UA_free(ns1);
    /* Custom configuration of Namespaces at beginning*/
    for(size_t i = 0 ; i < config.namespacesSize ; ++i){

        UA_Server_addNamespace_full(server, &config.namespaces[i]);
    }

    /* Create endpoints w/o endpointurl. It is added from the networklayers at startup */
    server->endpointDescriptions = (UA_EndpointDescription *)UA_Array_new(server->config.networkLayersSize,
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
        endpoint->userIdentityTokens = (UA_UserTokenPolicy *)UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

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
    if (server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        UA_Discovery_multicastInit(server);
    }

    LIST_INIT(&server->serverOnNetwork);
    server->serverOnNetworkSize = 0;
    server->serverOnNetworkRecordIdCounter = 0;
    server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(server->serverOnNetworkHash,0,sizeof(struct serverOnNetwork_hash_entry*)*SERVER_ON_NETWORK_HASH_PRIME);

    server->serverOnNetworkCallback = NULL;
    server->serverOnNetworkCallbackData = NULL;
# endif
#endif

    server->startTime = UA_DateTime_now();

#ifdef UA_ENABLE_LOAD_NAMESPACE0
#ifndef UA_ENABLE_GENERATE_NAMESPACE0

    /*********************************/
    /* Bootstrap reference hierarchy */
    /*********************************/

    UA_ReferenceTypeNode *references = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)references, "References");
    references->nodeId.identifier.numeric = UA_NS0ID_REFERENCES;
    references->isAbstract = true;
    references->symmetric = true;
    references->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "References");

    UA_ReferenceTypeNode *hassubtype = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hassubtype, "HasSubtype");
    hassubtype->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HasSupertype");
    hassubtype->nodeId.identifier.numeric = UA_NS0ID_HASSUBTYPE;
    hassubtype->isAbstract = false;
    hassubtype->symmetric = false;

    UA_RCU_LOCK();
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)references, NULL);
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)hassubtype, NULL);
    UA_RCU_UNLOCK();

    UA_ReferenceTypeNode *hierarchicalreferences = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hierarchicalreferences, "HierarchicalReferences");
    hierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_HIERARCHICALREFERENCES;
    hierarchicalreferences->isAbstract = true;
    hierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *nonhierarchicalreferences = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)nonhierarchicalreferences, "NonHierarchicalReferences");
    nonhierarchicalreferences->nodeId.identifier.numeric = UA_NS0ID_NONHIERARCHICALREFERENCES;
    nonhierarchicalreferences->isAbstract = true;
    nonhierarchicalreferences->symmetric  = false;
    addNodeInternal(server, (UA_Node*)nonhierarchicalreferences,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haschild = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)haschild, "HasChild");
    haschild->nodeId.identifier.numeric = UA_NS0ID_HASCHILD;
    haschild->isAbstract = false;
    haschild->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haschild,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *organizes = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)organizes, "Organizes");
    organizes->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrganizedBy");
    organizes->nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    organizes->isAbstract = false;
    organizes->symmetric  = false;
    addNodeInternal(server, (UA_Node*)organizes,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *haseventsource = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)haseventsource, "HasEventSource");
    haseventsource->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EventSourceOf");
    haseventsource->nodeId.identifier.numeric = UA_NS0ID_HASEVENTSOURCE;
    haseventsource->isAbstract = false;
    haseventsource->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseventsource,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodellingrule = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodellingrule, "HasModellingRule");
    hasmodellingrule->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModellingRuleOf");
    hasmodellingrule->nodeId.identifier.numeric = UA_NS0ID_HASMODELLINGRULE;
    hasmodellingrule->isAbstract = false;
    hasmodellingrule->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodellingrule, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hasencoding = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasencoding, "HasEncoding");
    hasencoding->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "EncodingOf");
    hasencoding->nodeId.identifier.numeric = UA_NS0ID_HASENCODING;
    hasencoding->isAbstract = false;
    hasencoding->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasencoding, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hasdescription = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasdescription, "HasDescription");
    hasdescription->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "DescriptionOf");
    hasdescription->nodeId.identifier.numeric = UA_NS0ID_HASDESCRIPTION;
    hasdescription->isAbstract = false;
    hasdescription->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasdescription, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hastypedefinition = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hastypedefinition, "HasTypeDefinition");
    hastypedefinition->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "TypeDefinitionOf");
    hastypedefinition->nodeId.identifier.numeric = UA_NS0ID_HASTYPEDEFINITION;
    hastypedefinition->isAbstract = false;
    hastypedefinition->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hastypedefinition, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *generatesevent = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)generatesevent, "GeneratesEvent");
    generatesevent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "GeneratedBy");
    generatesevent->nodeId.identifier.numeric = UA_NS0ID_GENERATESEVENT;
    generatesevent->isAbstract = false;
    generatesevent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)generatesevent, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *aggregates = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)aggregates, "Aggregates");
    aggregates->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "AggregatedBy");
    aggregates->nodeId.identifier.numeric = UA_NS0ID_AGGREGATES;
    aggregates->isAbstract = false;
    aggregates->symmetric  = false;
    addNodeInternal(server, (UA_Node*)aggregates, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType);

    /* complete bootstrap of hassubtype */
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCHILD), nodeIdHasSubType,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), true);

    UA_ReferenceTypeNode *hasproperty = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasproperty, "HasProperty");
    hasproperty->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "PropertyOf");
    hasproperty->nodeId.identifier.numeric = UA_NS0ID_HASPROPERTY;
    hasproperty->isAbstract = false;
    hasproperty->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasproperty,
                    UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hascomponent = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hascomponent, "HasComponent");
    hascomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ComponentOf");
    hascomponent->nodeId.identifier.numeric = UA_NS0ID_HASCOMPONENT;
    hascomponent->isAbstract = false;
    hascomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascomponent, UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasnotifier = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasnotifier, "HasNotifier");
    hasnotifier->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "NotifierOf");
    hasnotifier->nodeId.identifier.numeric = UA_NS0ID_HASNOTIFIER;
    hasnotifier->isAbstract = false;
    hasnotifier->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasnotifier, UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasorderedcomponent = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasorderedcomponent, "HasOrderedComponent");
    hasorderedcomponent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OrderedComponentOf");
    hasorderedcomponent->nodeId.identifier.numeric = UA_NS0ID_HASORDEREDCOMPONENT;
    hasorderedcomponent->isAbstract = false;
    hasorderedcomponent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasorderedcomponent, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), nodeIdHasSubType);

    UA_ReferenceTypeNode *hasmodelparent = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hasmodelparent, "HasModelParent");
    hasmodelparent->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ModelParentOf");
    hasmodelparent->nodeId.identifier.numeric = UA_NS0ID_HASMODELPARENT;
    hasmodelparent->isAbstract = false;
    hasmodelparent->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hasmodelparent, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *fromstate = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)fromstate, "FromState");
    fromstate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "ToTransition");
    fromstate->nodeId.identifier.numeric = UA_NS0ID_FROMSTATE;
    fromstate->isAbstract = false;
    fromstate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)fromstate, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *tostate = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)tostate, "ToState");
    tostate->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "FromTransition");
    tostate->nodeId.identifier.numeric = UA_NS0ID_TOSTATE;
    tostate->isAbstract = false;
    tostate->symmetric  = false;
    addNodeInternal(server, (UA_Node*)tostate, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hascause = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hascause, "HasCause");
    hascause->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeCausedBy");
    hascause->nodeId.identifier.numeric = UA_NS0ID_HASCAUSE;
    hascause->isAbstract = false;
    hascause->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hascause, nodeIdNonHierarchicalReferences, nodeIdHasSubType);
    
    UA_ReferenceTypeNode *haseffect = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)haseffect, "HasEffect");
    haseffect->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "MayBeEffectedBy");
    haseffect->nodeId.identifier.numeric = UA_NS0ID_HASEFFECT;
    haseffect->isAbstract = false;
    haseffect->symmetric  = false;
    addNodeInternal(server, (UA_Node*)haseffect, nodeIdNonHierarchicalReferences, nodeIdHasSubType);

    UA_ReferenceTypeNode *hashistoricalconfiguration = UA_Nodestore_newReferenceTypeNode();
    copyNames((UA_Node*)hashistoricalconfiguration, "HasHistoricalConfiguration");
    hashistoricalconfiguration->inverseName = UA_LOCALIZEDTEXT_ALLOC("en_US", "HistoricalConfigurationOf");
    hashistoricalconfiguration->nodeId.identifier.numeric = UA_NS0ID_HASHISTORICALCONFIGURATION;
    hashistoricalconfiguration->isAbstract = false;
    hashistoricalconfiguration->symmetric  = false;
    addNodeInternal(server, (UA_Node*)hashistoricalconfiguration, UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES), nodeIdHasSubType);

    /**************/
    /* Data Types */
    /**************/

    UA_DataTypeNode *basedatatype = UA_Nodestore_newDataTypeNode();
    copyNames((UA_Node*)basedatatype, "BaseDataType");
    basedatatype->nodeId.identifier.numeric = UA_NS0ID_BASEDATATYPE;
    basedatatype->isAbstract = true;
    UA_RCU_LOCK();
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)basedatatype, NULL);
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
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)basevartype, NULL);
    UA_RCU_UNLOCK();

    UA_VariableTypeNode *basedatavartype =
        createVariableTypeNode(server, "BaseDataVariableType", UA_NS0ID_BASEDATAVARIABLETYPE, false);
    basedatavartype->valueRank = -2;
    basedatavartype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)basedatavartype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE));

    UA_VariableTypeNode *propertytype =
        createVariableTypeNode(server, "PropertyType", UA_NS0ID_PROPERTYTYPE, false);
    propertytype->valueRank = -2;
    propertytype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)propertytype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE));

    UA_VariableTypeNode *buildinfotype =
        createVariableTypeNode(server, "BuildInfoType", UA_NS0ID_BUILDINFOTYPE, false);
    buildinfotype->valueRank = -1;
    buildinfotype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BUILDINFO);
    addNodeInternalWithType(server, (UA_Node*)buildinfotype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableTypeNode *serverstatustype =
        createVariableTypeNode(server, "ServerStatusType", UA_NS0ID_SERVERSTATUSTYPE, false);
    serverstatustype->valueRank = -1;
    serverstatustype->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERSTATUSDATATYPE);
    addNodeInternalWithType(server, (UA_Node*)serverstatustype,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                            nodeIdHasSubType, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    /**********************/
    /* Basic Object Types */
    /**********************/

    UA_ObjectTypeNode *baseobjtype =UA_Nodestore_newObjectTypeNode();
    copyNames((UA_Node*)baseobjtype, "BaseObjectType");
    baseobjtype->nodeId.identifier.numeric = UA_NS0ID_BASEOBJECTTYPE;
    UA_RCU_LOCK();
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)baseobjtype, NULL);
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
    const UA_NodeId nodeIdFolderType = UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE);
    const UA_NodeId nodeIdHasTypeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    UA_ObjectNode *root = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)root, "Root");
    root->nodeId.identifier.numeric = UA_NS0ID_ROOTFOLDER;
    UA_RCU_LOCK();
    UA_NodestoreSwitch_insertNode(server, (UA_Node*)root,NULL);
    UA_RCU_UNLOCK();
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), nodeIdHasTypeDefinition,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), true);

    UA_ObjectNode *objects = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)objects, "Objects");
    objects->nodeId.identifier.numeric = UA_NS0ID_OBJECTSFOLDER;
    addNodeInternalWithType(server, (UA_Node*)objects, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);

    UA_ObjectNode *types = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)types, "Types");
    types->nodeId.identifier.numeric = UA_NS0ID_TYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)types, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);

    UA_ObjectNode *referencetypes = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)referencetypes, "ReferenceTypes");
    referencetypes->nodeId.identifier.numeric = UA_NS0ID_REFERENCETYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)referencetypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCETYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_REFERENCES), true);

    UA_ObjectNode *datatypes = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)datatypes, "DataTypes");
    datatypes->nodeId.identifier.numeric = UA_NS0ID_DATATYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)datatypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_DATATYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), true);

    UA_ObjectNode *variabletypes = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)variabletypes, "VariableTypes");
    variabletypes->nodeId.identifier.numeric = UA_NS0ID_VARIABLETYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)variabletypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VARIABLETYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE), true);

    UA_ObjectNode *objecttypes = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)objecttypes, "ObjectTypes");
    objecttypes->nodeId.identifier.numeric = UA_NS0ID_OBJECTTYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)objecttypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTTYPESFOLDER), nodeIdOrganizes,
                         UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), true);

    UA_ObjectNode *eventtypes = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)eventtypes, "EventTypes");
    eventtypes->nodeId.identifier.numeric = UA_NS0ID_EVENTTYPESFOLDER;
    addNodeInternalWithType(server, (UA_Node*)eventtypes, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);

    UA_ObjectNode *views = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)views, "Views");
    views->nodeId.identifier.numeric = UA_NS0ID_VIEWSFOLDER;
    addNodeInternalWithType(server, (UA_Node*)views, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                            nodeIdOrganizes, nodeIdFolderType);

#else
    /* load the generated namespace externally */
    ua_namespaceinit_generated(server);
#endif
#endif //UA_ENABLE_LOAD_NAMESPACE0

    /*********************/
    /* The Server Object */
    /*********************/
    
    /* Create our own server object */ 
    UA_ObjectNode *servernode = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)servernode, "Server");
    servernode->nodeId.identifier.numeric = UA_NS0ID_SERVER;
    addNodeInternalWithType(server, (UA_Node*)servernode, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            nodeIdOrganizes, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE));
    
    // If we are in an UA conformant namespace, the above function just created a full ServerType object.
    // Before readding every variable, delete whatever got instantiated.
    // here we can't reuse servernode->nodeId because it may be deleted in addNodeInternalWithType if the node could not be added
    UA_NodeId serverNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    deleteInstanceChildren(server, &serverNodeId);
    
    UA_VariableNode *namespaceArray = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)namespaceArray, "NamespaceArray");
    namespaceArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_NAMESPACEARRAY;
    namespaceArray->valueSource = UA_VALUESOURCE_DATASOURCE;
    namespaceArray->value.dataSource.handle = server;
    namespaceArray->value.dataSource.read = readNamespaces;
    namespaceArray->value.dataSource.write = writeNamespaces;
    namespaceArray->dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    namespaceArray->valueRank = 1;
    namespaceArray->minimumSamplingInterval = 1.0;
    namespaceArray->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    addNodeInternalWithType(server, (UA_Node*)namespaceArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *serverArray = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)serverArray, "ServerArray");
    serverArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERARRAY;
    UA_Variant_setArrayCopy(&serverArray->value.data.value.value,
                            &server->config.applicationDescription.applicationUri, 1,
                            &UA_TYPES[UA_TYPES_STRING]);
    serverArray->value.data.value.hasValue = true;
    serverArray->valueRank = 1;
    serverArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)serverArray, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_ObjectNode *servercapablities = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)servercapablities, "ServerCapabilities");
    servercapablities->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES;
    addNodeInternalWithType(server, (UA_Node*)servercapablities, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCAPABILITIESTYPE));
    UA_NodeId ServerCapabilitiesNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES);
    deleteInstanceChildren(server, &ServerCapabilitiesNodeId);
    
    UA_VariableNode *localeIdArray = UA_Nodestore_newVariableNode();
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
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *maxBrowseContinuationPoints = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)maxBrowseContinuationPoints, "MaxBrowseContinuationPoints");
    maxBrowseContinuationPoints->nodeId.identifier.numeric =
        UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxBrowseContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxBrowseContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxBrowseContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

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

    UA_VariableNode *serverProfileArray = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)serverProfileArray, "ServerProfileArray");
    serverProfileArray->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY;
    UA_Variant_setArray(&serverProfileArray->value.data.value.value,
                        UA_Array_new(profileArraySize, &UA_TYPES[UA_TYPES_STRING]),
                        profileArraySize, &UA_TYPES[UA_TYPES_STRING]);
    for(UA_UInt16 i=0;i<profileArraySize;++i)
        ((UA_String *)serverProfileArray->value.data.value.value.data)[i] = profileArray[i];
    serverProfileArray->value.data.value.hasValue = true;
    serverProfileArray->valueRank = 1;
    serverProfileArray->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)serverProfileArray,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *softwareCertificates = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)softwareCertificates, "SoftwareCertificates");
    softwareCertificates->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_SOFTWARECERTIFICATES;
    softwareCertificates->dataType = UA_TYPES[UA_TYPES_SIGNEDSOFTWARECERTIFICATE].typeId;
    addNodeInternalWithType(server, (UA_Node*)softwareCertificates,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *maxQueryContinuationPoints = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)maxQueryContinuationPoints, "MaxQueryContinuationPoints");
    maxQueryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxQueryContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxQueryContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxQueryContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *maxHistoryContinuationPoints = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)maxHistoryContinuationPoints, "MaxHistoryContinuationPoints");
    maxHistoryContinuationPoints->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS;
    UA_Variant_setScalar(&maxHistoryContinuationPoints->value.data.value.value,
                         UA_UInt16_new(), &UA_TYPES[UA_TYPES_UINT16]);
    maxHistoryContinuationPoints->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)maxHistoryContinuationPoints,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *minSupportedSampleRate = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)minSupportedSampleRate, "MinSupportedSampleRate");
    minSupportedSampleRate->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE;
    UA_Variant_setScalar(&minSupportedSampleRate->value.data.value.value,
                         UA_Double_new(), &UA_TYPES[UA_TYPES_DOUBLE]);
    minSupportedSampleRate->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)minSupportedSampleRate,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_ObjectNode *modellingRules = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)modellingRules, "ModellingRules");
    modellingRules->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_MODELLINGRULES;
    addNodeInternalWithType(server, (UA_Node*)modellingRules,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *aggregateFunctions = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)aggregateFunctions, "AggregateFunctions");
    aggregateFunctions->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERCAPABILITIES_AGGREGATEFUNCTIONS;
    addNodeInternalWithType(server, (UA_Node*)aggregateFunctions,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE));

    UA_ObjectNode *serverdiagnostics = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)serverdiagnostics, "ServerDiagnostics");
    serverdiagnostics->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS;
    addNodeInternalWithType(server, (UA_Node*)serverdiagnostics,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERDIAGNOSTICSTYPE));
    UA_NodeId ServerDiagnosticsNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS);
    deleteInstanceChildren(server, &ServerDiagnosticsNodeId);
    
    UA_VariableNode *enabledFlag = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)enabledFlag, "EnabledFlag");
    enabledFlag->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG;
    UA_Variant_setScalar(&enabledFlag->value.data.value.value, UA_Boolean_new(),
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    enabledFlag->value.data.value.hasValue = true;
    enabledFlag->valueRank = 1;
    enabledFlag->minimumSamplingInterval = 1.0;
    addNodeInternalWithType(server, (UA_Node*)enabledFlag,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *serverstatus = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)serverstatus, "ServerStatus");
    serverstatus->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    serverstatus->valueSource = UA_VALUESOURCE_DATASOURCE;
    serverstatus->value.dataSource.handle = server;
    serverstatus->value.dataSource.read = readStatus;
    serverstatus->value.dataSource.write = NULL;
    addNodeInternalWithType(server, (UA_Node*)serverstatus, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *starttime = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)starttime, "StartTime");
    starttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
    UA_Variant_setScalarCopy(&starttime->value.data.value.value,
                             &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    starttime->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)starttime,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *currenttime = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)currenttime, "CurrentTime");
    currenttime->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    currenttime->valueSource = UA_VALUESOURCE_DATASOURCE;
    currenttime->value.dataSource.handle = NULL;
    currenttime->value.dataSource.read = readCurrentTime;
    currenttime->value.dataSource.write = NULL;
    addNodeInternalWithType(server, (UA_Node*)currenttime,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *state = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)state, "State");
    state->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERSTATUS_STATE;
    UA_Variant_setScalar(&state->value.data.value.value, UA_ServerState_new(),
                         &UA_TYPES[UA_TYPES_SERVERSTATE]);
    state->value.data.value.hasValue = true;
    state->minimumSamplingInterval = 500.0f;
    addNodeInternalWithType(server, (UA_Node*)state, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *buildinfo = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)buildinfo, "BuildInfo");
    buildinfo->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_Variant_setScalarCopy(&buildinfo->value.data.value.value,
                             &server->config.buildInfo, &UA_TYPES[UA_TYPES_BUILDINFO]);
    buildinfo->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)buildinfo,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BUILDINFOTYPE));

    UA_VariableNode *producturi = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)producturi, "ProductUri");
    producturi->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    UA_Variant_setScalarCopy(&producturi->value.data.value.value, &server->config.buildInfo.productUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    producturi->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)producturi,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *manufacturername = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)manufacturername, "ManufacturerName");
    manufacturername->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME);
    UA_Variant_setScalarCopy(&manufacturername->value.data.value.value,
                             &server->config.buildInfo.manufacturerName,
                             &UA_TYPES[UA_TYPES_STRING]);
    manufacturername->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)manufacturername,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *productname = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)productname, "ProductName");
    productname->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME);
    UA_Variant_setScalarCopy(&productname->value.data.value.value, &server->config.buildInfo.productName,
                             &UA_TYPES[UA_TYPES_STRING]);
    productname->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)productname,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *softwareversion = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)softwareversion, "SoftwareVersion");
    softwareversion->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION);
    UA_Variant_setScalarCopy(&softwareversion->value.data.value.value,
                             &server->config.buildInfo.softwareVersion, &UA_TYPES[UA_TYPES_STRING]);
    softwareversion->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)softwareversion,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *buildnumber = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)buildnumber, "BuildNumber");
    buildnumber->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER);
    UA_Variant_setScalarCopy(&buildnumber->value.data.value.value, &server->config.buildInfo.buildNumber,
                             &UA_TYPES[UA_TYPES_STRING]);
    buildnumber->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)buildnumber,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *builddate = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)builddate, "BuildDate");
    builddate->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE);
    UA_Variant_setScalarCopy(&builddate->value.data.value.value, &server->config.buildInfo.buildDate,
                             &UA_TYPES[UA_TYPES_DATETIME]);
    builddate->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)builddate,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *secondstillshutdown = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)secondstillshutdown, "SecondsTillShutdown");
    secondstillshutdown->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN);
    UA_Variant_setScalar(&secondstillshutdown->value.data.value.value, UA_UInt32_new(),
                         &UA_TYPES[UA_TYPES_UINT32]);
    secondstillshutdown->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)secondstillshutdown,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *shutdownreason = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)shutdownreason, "ShutdownReason");
    shutdownreason->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON);
    UA_Variant_setScalar(&shutdownreason->value.data.value.value, UA_LocalizedText_new(),
                         &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    shutdownreason->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)shutdownreason,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS),
                            nodeIdHasComponent, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));

    UA_VariableNode *servicelevel = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)servicelevel, "ServiceLevel");
    servicelevel->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL);
    servicelevel->valueSource = UA_VALUESOURCE_DATASOURCE;
    servicelevel->value.dataSource.handle = server;
    servicelevel->value.dataSource.read = readServiceLevel;
    servicelevel->value.dataSource.write = NULL;
    addNodeInternalWithType(server, (UA_Node*)servicelevel,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_VariableNode *auditing = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)auditing, "Auditing");
    auditing->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING);
    auditing->valueSource = UA_VALUESOURCE_DATASOURCE;
    auditing->value.dataSource.handle = server;
    auditing->value.dataSource.read = readAuditing;
    auditing->value.dataSource.write = NULL;
    addNodeInternalWithType(server, (UA_Node*)auditing,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasComponent,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

    UA_ObjectNode *vendorServerInfo = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)vendorServerInfo, "VendorServerInfo");
    vendorServerInfo->nodeId.identifier.numeric = UA_NS0ID_SERVER_VENDORSERVERINFO;
    addNodeInternalWithType(server, (UA_Node*)vendorServerInfo,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE));
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_VENDORSERVERINFO),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_VENDORSERVERINFOTYPE), true);
    */


    UA_ObjectNode *serverRedundancy = UA_Nodestore_newObjectNode();
    copyNames((UA_Node*)serverRedundancy, "ServerRedundancy");
    serverRedundancy->nodeId.identifier.numeric = UA_NS0ID_SERVER_SERVERREDUNDANCY;
    addNodeInternalWithType(server, (UA_Node*)serverRedundancy,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), nodeIdHasProperty,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE));
    /*
    addReferenceInternal(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
                         nodeIdHasTypeDefinition, UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_SERVERREDUNDANCYTYPE), true);
    */

    UA_VariableNode *redundancySupport = UA_Nodestore_newVariableNode();
    copyNames((UA_Node*)redundancySupport, "RedundancySupport");
    redundancySupport->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_REDUNDANCYSUPPORT);
    redundancySupport->valueRank = -1;
    redundancySupport->dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    //FIXME: enum is needed for type letting it uninitialized for now
    UA_Variant_setScalar(&redundancySupport->value.data.value.value, UA_Int32_new(),
                         &UA_TYPES[UA_TYPES_INT32]);
    redundancySupport->value.data.value.hasValue = true;
    addNodeInternalWithType(server, (UA_Node*)redundancySupport,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY),
                            nodeIdHasProperty, UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE));

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
