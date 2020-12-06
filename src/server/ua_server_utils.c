/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"

const UA_DataType *
UA_Server_findDataType(UA_Server *server, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, server->config.customDataTypes);
}

/********************************/
/* Information Model Operations */
/********************************/

const UA_Node *
getNodeType(UA_Server *server, const UA_NodeHead *head) {
    /* The reference to the parent is different for variable and variabletype */
    UA_Byte parentRefIndex;
    UA_Boolean inverse;
    UA_NodeClass typeNodeClass;
    switch(head->nodeClass) {
    case UA_NODECLASS_OBJECT:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASTYPEDEFINITION;
        inverse = false;
        typeNodeClass = UA_NODECLASS_OBJECTTYPE;
        break;
    case UA_NODECLASS_VARIABLE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASTYPEDEFINITION;
        inverse = false;
        typeNodeClass = UA_NODECLASS_VARIABLETYPE;
        break;
    case UA_NODECLASS_OBJECTTYPE:
    case UA_NODECLASS_VARIABLETYPE:
    case UA_NODECLASS_REFERENCETYPE:
    case UA_NODECLASS_DATATYPE:
        parentRefIndex = UA_REFERENCETYPEINDEX_HASSUBTYPE;
        inverse = true;
        typeNodeClass = head->nodeClass;
        break;
    default:
        return NULL;
    }

    /* Return the first matching candidate */
    for(size_t i = 0; i < head->referencesSize; ++i) {
        if(head->references[i].isInverse != inverse)
            continue;
        if(head->references[i].referenceTypeIndex != parentRefIndex)
            continue;
        UA_assert(!TAILQ_EMPTY(&head->references[i].queueHead));
        const UA_NodeId *targetId = &TAILQ_FIRST(&head->references[i].queueHead)->targetId.nodeId;
        const UA_Node *type = UA_NODESTORE_GET(server, targetId);
        if(!type)
            continue;
        if(type->head.nodeClass == typeNodeClass)
            return type;
        UA_NODESTORE_RELEASE(server, type);
    }

    return NULL;
}

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_NodeHead *head) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        if(head->references[i].isInverse == false &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASSUBTYPE)
            return true;
        if(head->references[i].isInverse == true &&
           head->references[i].referenceTypeIndex == UA_REFERENCETYPEINDEX_HASTYPEDEFINITION)
            return true;
    }
    return false;
}

UA_StatusCode
getParentTypeAndInterfaceHierarchy(UA_Server *server, const UA_NodeId *typeNode,
                                   UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    UA_ReferenceTypeSet reftypes_subtype =
        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE);
    UA_ExpandedNodeId *subTypes = NULL;
    size_t subTypesSize = 0;
    UA_StatusCode retval = browseRecursive(server, 1, typeNode,
                                           UA_BROWSEDIRECTION_INVERSE,
                                           &reftypes_subtype, UA_NODECLASS_UNSPECIFIED,
                                           false, &subTypesSize, &subTypes);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_assert(subTypesSize < 1000);

    UA_ReferenceTypeSet reftypes_interface =
        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);
    UA_ExpandedNodeId *interfaces = NULL;
    size_t interfacesSize = 0;
    retval = browseRecursive(server, 1, typeNode, UA_BROWSEDIRECTION_FORWARD,
                             &reftypes_interface, UA_NODECLASS_UNSPECIFIED,
                             false, &interfacesSize, &interfaces);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(subTypes, subTypesSize, &UA_TYPES[UA_TYPES_NODEID]);
        return retval;
    }

    UA_assert(interfacesSize < 1000);

    UA_NodeId *hierarchy = (UA_NodeId*)
        UA_malloc(sizeof(UA_NodeId) * (1 + subTypesSize + interfacesSize));
    if(!hierarchy) {
        UA_Array_delete(subTypes, subTypesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        UA_Array_delete(interfaces, interfacesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    retval = UA_NodeId_copy(typeNode, hierarchy);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(hierarchy);
        UA_Array_delete(subTypes, subTypesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        UA_Array_delete(interfaces, interfacesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < subTypesSize; i++) {
        hierarchy[i+1] = subTypes[i].nodeId;
        UA_NodeId_init(&subTypes[i].nodeId);
    }
    for(size_t i = 0; i < interfacesSize; i++) {
        hierarchy[i+1+subTypesSize] = interfaces[i].nodeId;
        UA_NodeId_init(&interfaces[i].nodeId);
    }

    *typeHierarchy = hierarchy;
    *typeHierarchySize = subTypesSize + interfacesSize + 1;

    UA_assert(*typeHierarchySize < 1000);

    UA_Array_delete(subTypes, subTypesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    UA_Array_delete(interfaces, interfacesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
getInterfaceHierarchy(UA_Server *server, const UA_NodeId *objectNode,
                                   UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    UA_ReferenceTypeSet reftypes_interface =
        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASINTERFACE);
    UA_ExpandedNodeId *interfaces = NULL;
    size_t interfacesSize = 0;
    UA_StatusCode retval = browseRecursive(server, 1, objectNode, UA_BROWSEDIRECTION_FORWARD,
                                           &reftypes_interface, UA_NODECLASS_UNSPECIFIED,
                                           false, &interfacesSize, &interfaces);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    UA_assert(interfacesSize < 1000);

    if (interfacesSize == 0) {
        *typeHierarchySize = 0;
        return UA_STATUSCODE_GOOD;
    }

    UA_NodeId *hierarchy = (UA_NodeId*)
        UA_malloc(sizeof(UA_NodeId) * (interfacesSize));
    if(!hierarchy) {
        UA_Array_delete(interfaces, interfacesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < interfacesSize; i++) {
        hierarchy[i] = interfaces[i].nodeId;
        UA_NodeId_init(&interfaces[i].nodeId);
    }

    *typeHierarchy = hierarchy;
    *typeHierarchySize = interfacesSize;

    UA_assert(*typeHierarchySize < 1000);
    UA_Array_delete(interfaces, interfacesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return UA_STATUSCODE_GOOD;
}

/* For mulithreading: make a copy of the node, edit and replace.
 * For singlethreading: edit the original */
UA_StatusCode
UA_Server_editNode(UA_Server *server, UA_Session *session,
                   const UA_NodeId *nodeId, UA_EditNodeCallback callback,
                   void *data) {
#ifndef UA_ENABLE_IMMUTABLE_NODES
    /* Get the node and process it in-situ */
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode retval = callback(server, session, (UA_Node*)(uintptr_t)node, data);
    UA_NODESTORE_RELEASE(server, node);
    return retval;
#else
    UA_StatusCode retval;
    do {
        /* Get an editable copy of the node */
        UA_Node *node;
        retval = UA_NODESTORE_GETCOPY(server, nodeId, &node);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Run the operation on the copy */
        retval = callback(server, session, node, data);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NODESTORE_DELETE(server, node);
            return retval;
        }

        /* Replace the node */
        retval = UA_NODESTORE_REPLACE(server, node);
    } while(retval != UA_STATUSCODE_GOOD);
    return retval;
#endif
}

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   const void *context, const size_t *requestOperations,
                                   const UA_DataType *requestOperationsType,
                                   size_t *responseOperations,
                                   const UA_DataType *responseOperationsType) {
    size_t ops = *requestOperations;
    if(ops == 0)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* No padding after size_t */
    void **respPos = (void**)((uintptr_t)responseOperations + sizeof(size_t));
    *respPos = UA_Array_new(ops, responseOperationsType);
    if(!(*respPos))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    *responseOperations = ops;
    uintptr_t respOp = (uintptr_t)*respPos;
    /* No padding after size_t */
    uintptr_t reqOp = *(uintptr_t*)((uintptr_t)requestOperations + sizeof(size_t));
    for(size_t i = 0; i < ops; i++) {
        operationCallback(server, session, context, (void*)reqOp, (void*)respOp);
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

/* A few global NodeId definitions */
const UA_NodeId subtypeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASSUBTYPE}};
const UA_NodeId hierarchicalReferences = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HIERARCHICALREFERENCES}};

/*********************************/
/* Default attribute definitions */
/*********************************/

const UA_ObjectAttributes UA_ObjectAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    0                       /* eventNotifier */
};

const UA_VariableAttributes UA_VariableAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    UA_ACCESSLEVELMASK_READ, 0,  /* accessLevel (userAccessLevel) */
    0.0,                         /* minimumSamplingInterval */
    false                        /* historizing */
};

const UA_MethodAttributes UA_MethodAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    true, true              /* executable (userExecutable) */
};

const UA_ObjectTypeAttributes UA_ObjectTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_VariableTypeAttributes UA_VariableTypeAttributes_default = {
    0,                           /* specifiedAttributes */
    {{0, NULL}, {0, NULL}},      /* displayName */
    {{0, NULL}, {0, NULL}},      /* description */
    0, 0,                        /* writeMask (userWriteMask) */
    {NULL, UA_VARIANT_DATA,
     0, NULL, 0, NULL},          /* value */
    {0, UA_NODEIDTYPE_NUMERIC,
     {UA_NS0ID_BASEDATATYPE}},   /* dataType */
    UA_VALUERANK_ANY,            /* valueRank */
    0, NULL,                     /* arrayDimensions */
    false                        /* isAbstract */
};

const UA_ReferenceTypeAttributes UA_ReferenceTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* isAbstract */
    false,                  /* symmetric */
    {{0, NULL}, {0, NULL}}  /* inverseName */
};

const UA_DataTypeAttributes UA_DataTypeAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false                   /* isAbstract */
};

const UA_ViewAttributes UA_ViewAttributes_default = {
    0,                      /* specifiedAttributes */
    {{0, NULL}, {0, NULL}}, /* displayName */
    {{0, NULL}, {0, NULL}}, /* description */
    0, 0,                   /* writeMask (userWriteMask) */
    false,                  /* containsNoLoops */
    0                       /* eventNotifier */
};

