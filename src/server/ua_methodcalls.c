#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"
#include "ua_methodcalls.h"
#include "ua_nodes.h"

UA_ArgumentsList *UA_ArgumentsList_new(UA_UInt32 statusSize, UA_UInt32 argumentsSize){
    UA_ArgumentsList *newAList = (UA_ArgumentsList *) UA_malloc(sizeof(UA_ArgumentsList));
    newAList->argumentsSize = argumentsSize;
    newAList->statusSize = statusSize;
    newAList->callResult = UA_STATUSCODE_GOOD;
    if (statusSize > 0) {
        newAList->status = (UA_StatusCode *) UA_malloc(sizeof(UA_StatusCode) * statusSize);
        for (unsigned int i=0; i<statusSize; i++)
            UA_StatusCode_init(&newAList->status[i]);
    }
    if (argumentsSize > 0) {
        newAList->arguments = (UA_Variant *) UA_malloc(sizeof(UA_Variant) * argumentsSize);
        for (unsigned int i=0; i<argumentsSize; i++)
            UA_Variant_init(&newAList->arguments[i]);
    }
    return newAList;
}

void UA_ArgumentsList_deleteMembers(UA_ArgumentsList *value) {
    if (value->statusSize > 0) {
        UA_free(value->status);
    }
    if (value->argumentsSize > 0) {
        for (unsigned int i=0; i<value->argumentsSize; i++)
            UA_Variant_deleteMembers(&value->arguments[i]);
        UA_free(value->arguments);
    }
}

void UA_ArgumentsList_destroy(UA_ArgumentsList *value) {
    UA_ArgumentsList_deleteMembers(value);
    UA_free(value);
}

/* User facing functions */
UA_StatusCode UA_Server_detachMethod_fromNode(UA_Server *server, const UA_NodeId methodNodeId) {
    const UA_Node *methodNode = UA_NodeStore_get(server->nodestore, (const UA_NodeId *) &methodNodeId);
    
    if (server == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    if (methodNode == UA_NULL)
        return UA_STATUSCODE_BADNODEIDINVALID;
    
    if ( methodNode->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    UA_MethodNode *replacement = UA_MethodNode_new();
    if (!replacement) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_MethodNode_copy((const UA_MethodNode *) methodNode, replacement);
    if( retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    replacement->executable = UA_FALSE;
    replacement->attachedMethod = UA_NULL;
    
    retval |= UA_NodeStore_replace(server->nodestore, methodNode, (UA_Node *) replacement, UA_NULL);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_MethodNode_delete(replacement);
        return retval;
    }
    UA_NodeStore_release(methodNode);
    
    // Cannot change a constant node.
    //((UA_MethodNode *) methodNode)->executable = UA_TRUE;
    // FIXME: This should be set by the namespace/server, not this handler.
    //((UA_MethodNode *) methodNode)->userExecutable = UA_TRUE;
    return retval;
}

UA_StatusCode UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId methodNodeId, void *method) {    
    const UA_Node *methodNode = UA_NodeStore_get(server->nodestore, (const UA_NodeId *) &methodNodeId);
    
    if (server == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    if (method == UA_NULL)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    if (methodNode == UA_NULL)
        return UA_STATUSCODE_BADNODEIDINVALID;
    
    if ( methodNode->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    UA_MethodNode *replacement = UA_MethodNode_new();
    if (!replacement) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_MethodNode_copy((const UA_MethodNode *) methodNode, replacement);
    if( retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    replacement->attachedMethod = method;
    replacement->executable = UA_TRUE;
    
    retval |= UA_NodeStore_replace(server->nodestore, methodNode, (UA_Node *) replacement, UA_NULL);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_MethodNode_delete(replacement);
        return retval;
    }
    UA_NodeStore_release(methodNode);
    
    // Cannot change a constant node.
    //((UA_MethodNode *) methodNode)->executable = UA_TRUE;
    // FIXME: This should be set by the namespace/server, not this handler.
    //((UA_MethodNode *) methodNode)->userExecutable = UA_TRUE;
    return retval;
}

UA_StatusCode UA_Server_addMethodNode(UA_Server *server, const UA_QualifiedName browseName, UA_NodeId nodeId,
                        const UA_ExpandedNodeId parentNodeId, const UA_NodeId referenceTypeId, void *method, 
                        UA_Int32 inputArgumentsSize, const UA_Argument *inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument *outputArguments) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    UA_MethodNode *newMethod = UA_MethodNode_new();
    UA_NodeId_copy(&nodeId, &newMethod->nodeId);
    UA_QualifiedName_copy(&browseName, &newMethod->browseName);
    UA_ExpandedNodeId *methodExpandedNodeId = UA_ExpandedNodeId_new();
    UA_NodeId_copy(&newMethod->nodeId, &methodExpandedNodeId->nodeId);
    UA_String_copy(&browseName.name, &newMethod->displayName.text);
    
    newMethod->attachedMethod = method;
    newMethod->executable = UA_TRUE;
    newMethod->userExecutable = UA_TRUE;
    
    UA_AddNodesResult addRes = UA_Server_addNode(server, (UA_Node *) newMethod, parentNodeId, referenceTypeId);
    if (addRes.statusCode != UA_STATUSCODE_GOOD)
        return addRes.statusCode;
    
    // Create InputArguments
    // FIXME: This ID should be random, but ns=1;i=0 doesn't return a new id at the moment
    UA_NodeId argId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, nodeId.identifier.numeric+1);
    UA_VariableNode *inputArgumentsVariableNode  = UA_VariableNode_new();
    retval |= UA_NodeId_copy(&argId, &inputArgumentsVariableNode->nodeId);
    inputArgumentsVariableNode->browseName = UA_QUALIFIEDNAME(0,"InputArguments");
    inputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT("en_US", "InputArguments");
    inputArgumentsVariableNode->valueRank = 1;
    inputArgumentsVariableNode->value.variant.data = UA_malloc(sizeof(UA_Argument) * inputArgumentsSize);
    UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.variant, inputArguments, inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    
    addRes = UA_Server_addNode(server, (UA_Node *) inputArgumentsVariableNode, *methodExpandedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));

    if (addRes.statusCode != UA_STATUSCODE_GOOD)
        // TODO Remove node
        return addRes.statusCode;
    
    // Create OutputArguments
    // FIXME: This ID should be random, but ns=1;i=0 doesn't return a new id at the moment
    argId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, nodeId.identifier.numeric+2);
    UA_VariableNode *outputArgumentsVariableNode  = UA_VariableNode_new();
    retval |= UA_NodeId_copy(&argId, &outputArgumentsVariableNode->nodeId);
    outputArgumentsVariableNode->browseName  = UA_QUALIFIEDNAME(0,"OutputArguments");
    outputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT("en_US", "OutputArguments");
    inputArgumentsVariableNode->valueRank = 1;
    inputArgumentsVariableNode->value.variant.data = UA_malloc(sizeof(UA_Argument) * outputArgumentsSize);
    UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.variant, inputArguments, outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    // Create Arguments Variant
    addRes = UA_Server_addNode(server, (UA_Node *) outputArgumentsVariableNode, *methodExpandedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
    
    if (addRes.statusCode != UA_STATUSCODE_GOOD)
        // TODO Remove node
        return addRes.statusCode;
    
    return retval;
}
