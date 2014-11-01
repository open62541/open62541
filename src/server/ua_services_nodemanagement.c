#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_services_internal.h"
#include "ua_session.h"
#include "ua_util.h"

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

UA_Int32 AddReference(UA_NodeStore *nodestore, UA_Node *node, UA_ReferenceNode *reference);

/**
   If adding the node succeeds, the pointer will be set to zero. If the nodeid
   of the node is null (ns=0,i=0), a unique new nodeid will be assigned and
   returned in the AddNodesResult.
 */
UA_AddNodesResult AddNode(UA_Server *server, UA_Session *session, UA_Node **node,
                          const UA_ExpandedNodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    
    const UA_Node *parent;
    if(UA_NodeStore_get(server->nodestore, &parentNodeId->nodeId, &parent) != UA_STATUSCODE_GOOD) {
        result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        return result;
    }

    const UA_ReferenceTypeNode *referenceType;
    if(UA_NodeStore_get(server->nodestore, referenceTypeId, (const UA_Node**)&referenceType) != UA_STATUSCODE_GOOD) {
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

    // todo: test if the referenetype is hierarchical

    if(UA_NodeId_isNull(&(*node)->nodeId)) {
        if(UA_NodeStore_insert(server->nodestore, node,
                               UA_NODESTORE_INSERT_UNIQUE | UA_NODESTORE_INSERT_GETMANAGED) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result.addedNodeId = (*node)->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&(*node)->nodeId, &result.addedNodeId) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, UA_NODESTORE_INSERT_GETMANAGED) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
            UA_NodeId_deleteMembers(&result.addedNodeId);
            goto ret2;
        }
    }
    
    UA_ReferenceNode ref;
    UA_ReferenceNode_init(&ref);
    ref.referenceTypeId = referenceType->nodeId; // is numeric
    ref.isInverse = UA_TRUE; // todo: check if they are all not inverse..
    ref.targetId.nodeId = parent->nodeId;
    AddReference(server->nodestore, *node, &ref);

    // todo: error handling. remove new node from nodestore

    UA_NodeStore_release(*node);
    *node = UA_NULL;
    
 ret2:
    UA_NodeStore_release((UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);

    return result;
}

static void addNodeFromAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                  UA_AddNodesResult *result) {
    if(item->requestedNewNodeId.nodeId.namespaceIndex == 0) {
        // adding nodes to ns0 is not allowed over the wire
        result->statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
        return;
    }

    UA_Node *newNode;
    const UA_VTable_Entry *newNodeVT = UA_NULL;
    if(item->nodeClass == UA_NODECLASS_VARIABLE)
        result->statusCode = parseVariableNode(&item->nodeAttributes, &newNode, &newNodeVT);
    else // add more node types here..
        result->statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    *result = AddNode(server, session, &newNode, &item->parentNodeId, &item->referenceTypeId);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        newNodeVT->delete(newNode);
}

void Service_AddNodes(UA_Server *server, UA_Session *session,
                      const UA_AddNodesRequest *request, UA_AddNodesResponse *response) {
    if(request->nodesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    UA_StatusCode retval = UA_Array_new((void**)&response->results, request->nodesToAddSize, &UA_TYPES[UA_ADDNODESRESULT]);
    if(retval) {
        response->responseHeader.serviceResult = retval;
        return;
    }
    
    response->resultsSize = request->nodesToAddSize;
    for(int i = 0;i < request->nodesToAddSize;i++)
        addNodeFromAttributes(server, session, &request->nodesToAdd[i], &response->results[i]);
}

static UA_Int32 AddSingleReference(UA_Node *node, UA_ReferenceNode *reference) {
    // TODO: Check if reference already exists
    UA_Int32 count = node->referencesSize;
    UA_ReferenceNode *old_refs = node->references;
    UA_ReferenceNode *new_refs;

    if(count < 0) count = 0;

    if(!(new_refs = UA_alloc(sizeof(UA_ReferenceNode)*(count+1))))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode)*count);
    if(UA_ReferenceNode_copy(reference, &new_refs[count]) != UA_STATUSCODE_GOOD) {
        UA_free(new_refs);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    node->references     = new_refs;
    node->referencesSize = count+1;
    UA_free(old_refs);
    return UA_STATUSCODE_GOOD;
}

UA_Int32 AddReference(UA_NodeStore *nodestore, UA_Node *node, UA_ReferenceNode *reference) {
    UA_Int32 retval = AddSingleReference(node, reference);
    UA_Node *targetnode;
    UA_ReferenceNode inversereference;
    if(retval != UA_STATUSCODE_GOOD || nodestore == UA_NULL)
        return retval;

    // Do a copy every time?
    if(UA_NodeStore_get(nodestore, &reference->targetId.nodeId, (const UA_Node **)&targetnode) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;

    inversereference.referenceTypeId       = reference->referenceTypeId;
    inversereference.isInverse             = !reference->isInverse;
    inversereference.targetId.nodeId       = node->nodeId;
    inversereference.targetId.namespaceUri = UA_STRING_NULL;
    inversereference.targetId.serverIndex  = 0;
    retval = AddSingleReference(targetnode, &inversereference);
    UA_NodeStore_release(targetnode);

    return retval;
}

void Service_AddReferences(UA_Server *server, UA_Session *session,
                           const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}
