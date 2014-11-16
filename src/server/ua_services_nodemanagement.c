#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_services_internal.h"
#include "ua_session.h"
#include "ua_util.h"

const UA_VTable_Entry * UA_Node_getVT(const UA_Node *node) {
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

#define COPY_STANDARDATTRIBUTES do {                                    \
    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DISPLAYNAME) {  \
        vnode->displayName = attr.displayName;                          \
        UA_LocalizedText_init(&attr.displayName);                       \
    }                                                                   \
    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DESCRIPTION) {  \
        vnode->description = attr.description;                          \
        UA_LocalizedText_init(&attr.description);                       \
    }                                                                   \
    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_WRITEMASK)      \
        vnode->writeMask = attr.writeMask;                              \
    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_USERWRITEMASK)  \
        vnode->userWriteMask = attr.userWriteMask;                      \
    } while(0)

static UA_StatusCode parseVariableNode(UA_ExtensionObject *attributes, UA_Node **new_node,
                                       const UA_VTable_Entry **vt) {
    if(attributes->typeId.identifier.numeric != 357) // VariableAttributes_Encoding_DefaultBinary,357,Object
        return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;

    UA_VariableAttributes attr;
    UA_UInt32 pos = 0;
    // todo return more informative error codes from decodeBinary
    if(UA_VariableAttributes_decodeBinary(&attributes->body, &pos, &attr) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;

    UA_VariableNode *vnode = UA_VariableNode_new();
    if(!vnode) {
        UA_VariableAttributes_deleteMembers(&attr);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    // now copy all the attributes. This potentially removes them from the decoded attributes.
    COPY_STANDARDATTRIBUTES;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ACCESSLEVEL)
        vnode->accessLevel = attr.accessLevel;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_USERACCESSLEVEL)
        vnode->userAccessLevel = attr.userAccessLevel;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_HISTORIZING)
        vnode->historizing = attr.historizing;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_MINIMUMSAMPLINGINTERVAL)
        vnode->minimumSamplingInterval = attr.minimumSamplingInterval;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUERANK)
        vnode->valueRank = attr.valueRank;

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS) {
        vnode->arrayDimensionsSize = attr.arrayDimensionsSize;
        vnode->arrayDimensions = attr.arrayDimensions;
        attr.arrayDimensionsSize = -1;
        attr.arrayDimensions = UA_NULL;
    }

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DATATYPE ||
       attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_OBJECTTYPEORDATATYPE) {
        vnode->dataType = attr.dataType;
        UA_NodeId_init(&attr.dataType);
    }

    if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_VALUE) {
        vnode->value = attr.value;
        UA_Variant_init(&attr.value);
    }

    UA_VariableAttributes_deleteMembers(&attr);

    *new_node = (UA_Node*)vnode;
    *vt = &UA_TYPES[UA_VARIABLENODE];
    return UA_STATUSCODE_GOOD;
}

/**
   If adding the node succeeds, the pointer will be set to zero. If the nodeid
   of the node is null (ns=0,i=0), a unique new nodeid will be assigned and
   returned in the AddNodesResult.
 */
static void __addNode(UA_Server *server, UA_Session *session, const UA_Node **node,
                      const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                      UA_AddNodesResult *result) {
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId->nodeId);
    if(!parent) {
        result->statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        return;
    }

    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode *)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret;
    }

    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret2;
    }

    if(referenceType->isAbstract == UA_TRUE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        goto ret2;
    }

    // todo: test if the referencetype is hierarchical
    if(UA_NodeId_isNull(&(*node)->nodeId)) {
        if(UA_NodeStore_insert(server->nodestore, node, UA_TRUE) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result->addedNodeId = (*node)->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&(*node)->nodeId, &result->addedNodeId) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, UA_TRUE) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
            UA_NodeId_deleteMembers(&result->addedNodeId);
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

    return;
}

/* Exposed to userland */
UA_AddNodesResult UA_Server_addNode(UA_Server *server, const UA_Node **node,
                                    const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    __addNode(server, &adminSession, node, parentNodeId, referenceTypeId, &result);
    return result;
}

static void __addNodeFromAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                    UA_AddNodesResult *result) {
    // adding nodes to ns0 is not allowed over the wire
    if(item->requestedNewNodeId.nodeId.namespaceIndex == 0) {
        result->statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
        return;
    }

    // parse the node
    UA_Node *node;
    const UA_VTable_Entry *nodeVT = UA_NULL;
    if(item->nodeClass == UA_NODECLASS_VARIABLE)
        result->statusCode = parseVariableNode(&item->nodeAttributes, &node, &nodeVT);
    else // add more node types here..
        result->statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    // add the node
    __addNode(server, session, (const UA_Node **)&node, &item->parentNodeId, &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        nodeVT->delete(node);
}

void Service_AddNodes(UA_Server *server, UA_Session *session, const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    if(request->nodesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToAddSize,
                                        &UA_TYPES[UA_ADDNODESRESULT]);
    if(retval) {
        response->responseHeader.serviceResult = retval;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean isExternal[request->nodesToAddSize];
    memset(isExternal, UA_FALSE, sizeof(UA_Boolean)*request->nodesToAddSize);
    UA_UInt32 indices[request->nodesToAddSize];
    for(UA_Int32 j = 0;j<server->externalNamespacesSize;j++) {
        UA_UInt32 indexSize = 0;
        for(UA_Int32 i = 0;i < request->nodesToAddSize;i++) {
            if(request->nodesToAdd[i].requestedNewNodeId.nodeId.namespaceIndex !=
               server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->addNodes(ens->ensHandle, &request->requestHeader, request->nodesToAdd,
                      indices, indexSize, response->results, response->diagnosticInfos);
    }
    /* ### End External Namespaces */
    
    response->resultsSize = request->nodesToAddSize;
    for(int i = 0;i < request->nodesToAddSize;i++) {
        if(!isExternal[i])
            __addNodeFromAttributes(server, session, &request->nodesToAdd[i], &response->results[i]);
    }
}

static UA_StatusCode __addSingleReference(UA_Server *server, const UA_AddReferencesItem *item) {
    // todo: we don't support references to other servers (expandednodeid) for now
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    
    // Is this for an external nodestore?
    UA_ExternalNodeStore *ens = UA_NULL;
    for(UA_Int32 j = 0;j<server->externalNamespacesSize;j++) {
        if(item->sourceNodeId.namespaceIndex == server->externalNamespaces[j].index) {
            ens = &server->externalNamespaces[j].externalNodeStore;
            break;
        }
    }

    if(ens) {
        // todo: use external nodestore

    } else {
        // use the servers nodestore
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &item->sourceNodeId);
        // todo differentiate between error codes
        if(!node)
            return UA_STATUSCODE_BADINTERNALERROR;

        const UA_VTable_Entry *nodeVT = UA_Node_getVT(node);
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
        UA_NodeStore_replace(server->nodestore, (const UA_Node **)&newNode, UA_FALSE);
        UA_NodeStore_release(node);
    }
    return UA_STATUSCODE_GOOD;
} 

UA_StatusCode UA_Server_addReference(UA_Server *server, const UA_AddReferencesItem *item) {
    // the first direction
    UA_StatusCode retval = __addSingleReference(server, item);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    // detect when the inverse reference shall be added as well
    UA_AddReferencesItem item2;
    UA_AddReferencesItem_init(&item2);
    item2.sourceNodeId = item->targetNodeId.nodeId;
    item2.referenceTypeId = item->referenceTypeId;
    item2.isForward = !item->isForward;
    item2.targetNodeId.nodeId = item->sourceNodeId;
    retval = __addSingleReference(server, &item2);
    // todo: if this fails, remove the first reference

    return retval;
}

void Service_AddReferences(UA_Server *server, UA_Session *session, const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    if(request->referencesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_alloc(sizeof(UA_StatusCode)*request->referencesToAddSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = request->referencesToAddSize;
    for(UA_Int32 i = 0;i < response->resultsSize;i++)
            response->results[i] = UA_Server_addReference(server, &request->referencesToAdd[i]);
}
