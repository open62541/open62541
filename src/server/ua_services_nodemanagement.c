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
                                       const UA_TypeVTable **vt) {
    if(attributes->typeId.identifier.numeric !=
       UA_NODEIDS[UA_VARIABLEATTRIBUTES].identifier.numeric + UA_ENCODINGOFFSET_BINARY)
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

static void addNodeFromAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                  UA_AddNodesResult *result) {
    // adding nodes to ns0 is not allowed over the wire
    if(item->requestedNewNodeId.nodeId.namespaceIndex == 0) {
        result->statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
        return;
    }

    // parse the node
    UA_Node *node;
    const UA_TypeVTable *nodeVT = UA_NULL;
    if(item->nodeClass == UA_NODECLASS_VARIABLE)
        result->statusCode = parseVariableNode(&item->nodeAttributes, &node, &nodeVT);
    else // add more node types here..
        result->statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;

    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    // add the node
    *result = UA_Server_addNodeWithSession(server, session, (const UA_Node **)&node,
                                           &item->parentNodeId, &item->referenceTypeId);
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
            if(request->nodesToAdd[i].requestedNewNodeId.nodeId.namespaceIndex != server->externalNamespaces[j].index)
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
            addNodeFromAttributes(server, session, &request->nodesToAdd[i], &response->results[i]);
    }
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
        response->results[i] = UA_Server_addReferenceWithSession(server, session,
                                                                 &request->referencesToAdd[i]);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session, const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {

}

void Service_DeleteReferences(UA_Server *server, UA_Session *session,
                              const UA_DeleteReferencesRequest *request,
                              UA_DeleteReferencesResponse *response) {

}
