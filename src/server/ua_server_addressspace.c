#include "ua_server.h"
#include "ua_server_internal.h"
#include "ua_namespace_0.h"

const UA_TypeVTable * UA_Node_getTypeVT(const UA_Node *node) {
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        return &UA_TYPES[UA_OBJECTNODE];
    case UA_NODECLASS_VARIABLE:
        return &UA_TYPES[UA_VARIABLENODE];
    case UA_NODECLASS_METHOD:
        return &UA_TYPES[UA_METHODNODE];
    case UA_NODECLASS_OBJECTTYPE:
        return &UA_TYPES[UA_OBJECTTYPENODE];
    case UA_NODECLASS_VARIABLETYPE:
        return &UA_TYPES[UA_VARIABLETYPENODE];
    case UA_NODECLASS_REFERENCETYPE:
        return &UA_TYPES[UA_REFERENCETYPENODE];
    case UA_NODECLASS_DATATYPE:
        return &UA_TYPES[UA_DATATYPENODE];
    case UA_NODECLASS_VIEW:
        return &UA_TYPES[UA_VIEWNODE];
    default: break;
    }

    return &UA_TYPES[UA_INVALIDTYPE];
}

void UA_Server_addScalarVariableNode(UA_Server *server, UA_QualifiedName *browseName, void *value,
                                     const UA_TypeVTable *vt, const UA_ExpandedNodeId *parentNodeId,
                                     const UA_NodeId *referenceTypeId) {
    UA_VariableNode *tmpNode = UA_VariableNode_new();
    UA_QualifiedName_copy(browseName, &tmpNode->browseName);
    UA_String_copy(&browseName->name, &tmpNode->displayName.text);
    /* UA_LocalizedText_copycstring("integer value", &tmpNode->description); */
    tmpNode->nodeClass = UA_NODECLASS_VARIABLE;
    tmpNode->valueRank = -1;
    tmpNode->value.vt = vt;
    tmpNode->value.storage.data.dataPtr = value;
    tmpNode->value.storageType = UA_VARIANT_DATA;
    tmpNode->value.storage.data.arrayLength = 1;
    UA_Server_addNode(server, (const UA_Node**)&tmpNode, parentNodeId, referenceTypeId);
}

/* Adds a one-way reference to the local nodestore */
UA_StatusCode addOneWayReferenceWithSession (UA_Server *server, UA_Session *session,
                                             const UA_AddReferencesItem *item) {
    // use the servers nodestore
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &item->sourceNodeId);
    // todo differentiate between error codes
    if(!node)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_TypeVTable *nodeVT = UA_Node_getTypeVT(node);
    UA_Node *newNode = nodeVT->new();
    nodeVT->copy(node, newNode);

    UA_Int32 count = node->referencesSize;
    if(count < 0)
        count = 0;
    UA_ReferenceNode *old_refs = newNode->references;
    UA_ReferenceNode *new_refs = UA_alloc(sizeof(UA_ReferenceNode)*(count+1));
    if(!new_refs) {
        nodeVT->delete(newNode);
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    // insert the new reference
    UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode)*count);
    UA_ReferenceNode_init(&new_refs[count]);
    UA_StatusCode retval = UA_NodeId_copy(&item->referenceTypeId, &new_refs[count].referenceTypeId);
    new_refs[count].isInverse = !item->isForward;
    retval |= UA_ExpandedNodeId_copy(&item->targetNodeId, &new_refs[count].targetId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(new_refs, ++count, &UA_TYPES[UA_REFERENCENODE]);
        newNode->references = UA_NULL;
        newNode->referencesSize = 0;
        nodeVT->delete(newNode);
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_free(old_refs);
    newNode->references = new_refs;
    newNode->referencesSize = ++count;
    retval = UA_NodeStore_replace(server->nodestore, (const UA_Node **)&newNode, UA_FALSE);
    if(retval)
        nodeVT->delete(newNode);
    UA_NodeStore_release(node);

    return retval;
}

UA_StatusCode UA_Server_addReference(UA_Server *server, const UA_AddReferencesItem *item) {
    return UA_Server_addReferenceWithSession(server, &adminSession, item);
}

UA_StatusCode UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session,
                                                const UA_AddReferencesItem *item) {
    // todo: we don't support references to other servers (expandednodeid) for now
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    
    // Is this for an external nodestore?
    UA_ExternalNodeStore *ensFirst = UA_NULL;
    UA_ExternalNodeStore *ensSecond = UA_NULL;
    for(UA_Int32 j = 0;j<server->externalNamespacesSize && (!ensFirst || !ensSecond);j++) {
        if(item->sourceNodeId.namespaceIndex == server->externalNamespaces[j].index)
            ensFirst = &server->externalNamespaces[j].externalNodeStore;
        if(item->targetNodeId.nodeId.namespaceIndex == server->externalNamespaces[j].index)
            ensSecond = &server->externalNamespaces[j].externalNodeStore;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(ensFirst) {
        // todo: use external nodestore
    } else
        retval = addOneWayReferenceWithSession (server, session, item);

    if(retval) return retval;

    UA_AddReferencesItem secondItem;
    secondItem = *item;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.isForward = !item->isForward;
    if(ensSecond) {
        // todo: use external nodestore
    } else
        retval = addOneWayReferenceWithSession (server, session, &secondItem);
    // todo: remove reference if the second direction failed

    return retval;
} 

UA_AddNodesResult UA_Server_addNode(UA_Server *server, const UA_Node **node,
                                    const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    return UA_Server_addNodeWithSession(server, &adminSession, node, parentNodeId, referenceTypeId);
}

UA_AddNodesResult UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, const UA_Node **node,
                                               const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);

    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId->nodeId);
    if(!parent) {
        result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        return result;
    }

    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode *)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        result.statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret;
    }

    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        result.statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret2;
    }

    if(referenceType->isAbstract == UA_TRUE) {
        result.statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        goto ret2;
    }

    // todo: test if the referencetype is hierarchical
    if(UA_NodeId_isNull(&(*node)->nodeId)) {
        if(UA_NodeStore_insert(server->nodestore, node, UA_TRUE) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result.addedNodeId = (*node)->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&(*node)->nodeId, &result.addedNodeId) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, UA_TRUE) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
            UA_NodeId_deleteMembers(&result.addedNodeId);
            goto ret2;
        }
    }
    
    // reference back to the parent
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = (*node)->nodeId;
    item.referenceTypeId = referenceType->nodeId;
    item.isForward = UA_FALSE;
    item.targetNodeId.nodeId = parent->nodeId;
    UA_Server_addReference(server, &item);

    // todo: error handling. remove new node from nodestore

    UA_NodeStore_release(*node);
    *node = UA_NULL;
    
 ret2:
    UA_NodeStore_release((UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);

    return result;
}
