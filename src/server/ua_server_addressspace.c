#include "ua_server.h"
#include "ua_server_internal.h"

UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, UA_Variant *value, const UA_QualifiedName browseName, 
                          UA_NodeId nodeId, const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId)
{
    UA_VariableNode *node = UA_VariableNode_new();
    node->variable.variant = *value; // copy content
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_String_copy(&browseName.name, &node->displayName.text);
    UA_ExpandedNodeId parentId; // we need an expandednodeid
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    UA_AddNodesResult res =
        UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node, &parentId, &referenceTypeId);
    ADDREFERENCE(res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_NUMERIC(0, value->type->typeId.identifier.numeric));
    if(res.statusCode != UA_STATUSCODE_GOOD) {
        UA_Variant_init(&node->variable.variant);
        UA_VariableNode_delete(node);
    } else
        UA_free(value);
    return res.statusCode;
}

UA_StatusCode
UA_Server_addDataSourceVariableNode(UA_Server *server, UA_DataSource dataSource,
                                    const UA_QualifiedName browseName, UA_NodeId nodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId)
{
    UA_VariableNode *node = UA_VariableNode_new();
    node->variableType = UA_VARIABLENODETYPE_DATASOURCE;
    node->variable.dataSource = dataSource;
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_String_copy(&browseName.name, &node->displayName.text);
    UA_ExpandedNodeId parentId; // dummy exapndednodeid
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    UA_AddNodesResult res =
        UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node, &parentId, &referenceTypeId);
    ADDREFERENCE(res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                 UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));
    if(res.statusCode != UA_STATUSCODE_GOOD)
        UA_VariableNode_delete(node);
    return res.statusCode;
}

/* Adds a one-way reference to the local nodestore */
static UA_StatusCode
addOneWayReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item)
{
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &item->sourceNodeId);
    if(!node)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Node *newNode = UA_NULL;
    void (*deleteNode)(UA_Node*) = UA_NULL;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        newNode = (UA_Node*)UA_ObjectNode_new();
        UA_ObjectNode_copy((const UA_ObjectNode*)node, (UA_ObjectNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ObjectNode_delete;
        break;
    case UA_NODECLASS_VARIABLE:
        newNode = (UA_Node*)UA_VariableNode_new();
        UA_VariableNode_copy((const UA_VariableNode*)node, (UA_VariableNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_VariableNode_delete;
        break;
    case UA_NODECLASS_METHOD:
        newNode = (UA_Node*)UA_MethodNode_new();
        UA_MethodNode_copy((const UA_MethodNode*)node, (UA_MethodNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_MethodNode_delete;
        break;
    case UA_NODECLASS_OBJECTTYPE:
        newNode = (UA_Node*)UA_ObjectTypeNode_new();
        UA_ObjectTypeNode_copy((const UA_ObjectTypeNode*)node, (UA_ObjectTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ObjectTypeNode_delete;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        newNode = (UA_Node*)UA_VariableTypeNode_new();
        UA_VariableTypeNode_copy((const UA_VariableTypeNode*)node, (UA_VariableTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_VariableTypeNode_delete;
        break;
    case UA_NODECLASS_REFERENCETYPE:
        newNode = (UA_Node*)UA_ReferenceTypeNode_new();
        UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode*)node, (UA_ReferenceTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ReferenceTypeNode_delete;
        break;
    case UA_NODECLASS_DATATYPE:
        newNode = (UA_Node*)UA_DataTypeNode_new();
        UA_DataTypeNode_copy((const UA_DataTypeNode*)node, (UA_DataTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_DataTypeNode_delete;
        break;
    case UA_NODECLASS_VIEW:
        newNode = (UA_Node*)UA_ViewNode_new();
        UA_ViewNode_copy((const UA_ViewNode*)node, (UA_ViewNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ViewNode_delete;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Int32 count = node->referencesSize;
    if(count < 0)
        count = 0;
    UA_ReferenceNode *old_refs = newNode->references;
    UA_ReferenceNode *new_refs = UA_malloc(sizeof(UA_ReferenceNode)*(count+1));
    if(!new_refs) {
        deleteNode(newNode);
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
        UA_Array_delete(new_refs, &UA_TYPES[UA_TYPES_REFERENCENODE], ++count);
        newNode->references = UA_NULL;
        newNode->referencesSize = 0;
        deleteNode(newNode);
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_free(old_refs);
    newNode->references = new_refs;
    newNode->referencesSize = ++count;
    retval = UA_NodeStore_replace(server->nodestore, node, newNode, UA_NULL);
    UA_NodeStore_release(node);
    if(retval != UA_STATUSCODE_BADINTERNALERROR)
        return retval;
    
    // error presumably because the node was replaced and an old version was updated just try again
    deleteNode(newNode);
    return addOneWayReferenceWithSession(server, session, item);
}

/* userland version of addReferenceWithSession */
UA_StatusCode
UA_Server_addReference(UA_Server *server, const UA_AddReferencesItem *item)
{
    return UA_Server_addReferenceWithSession(server, &adminSession, item);
}

UA_StatusCode
UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item)
{
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED; // currently no expandednodeids are allowed
    
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
        retval = addOneWayReferenceWithSession(server, session, item);

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

/* userland version of addNodeWithSession */
UA_AddNodesResult
UA_Server_addNode(UA_Server *server, UA_Node *node, const UA_ExpandedNodeId *parentNodeId,
                  const UA_NodeId *referenceTypeId)
{
    return UA_Server_addNodeWithSession(server, &adminSession, node, parentNodeId, referenceTypeId);
}

UA_AddNodesResult
UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, UA_Node *node,
                             const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId)
{
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
    const UA_Node *managed = UA_NULL;
    if(UA_NodeId_isNull(&node->nodeId)) {
        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result.addedNodeId = managed->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&node->nodeId, &result.addedNodeId) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
            UA_NodeId_deleteMembers(&result.addedNodeId);
            goto ret2;
        }
    }
    
    // reference back to the parent
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = managed->nodeId;
    item.referenceTypeId = referenceType->nodeId;
    item.isForward = UA_FALSE;
    item.targetNodeId.nodeId = parent->nodeId;
    UA_Server_addReference(server, &item);

    // todo: error handling. remove new node from nodestore

    UA_NodeStore_release(managed);
    
 ret2:
    UA_NodeStore_release((const UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);

    return result;
}
