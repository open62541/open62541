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

#define UA_MAX_TREE_RECURSE 50 /* How deep up/down the tree do we recurse at most? */

/********************************/
/* Information Model Operations */
/********************************/

/* Keeps track of already visited nodes to detect circular references */
struct ref_history {
    struct ref_history *parent; /* the previous element */
    const UA_NodeId *id; /* the id of the node at this depth */
    UA_UInt16 depth;
};

static UA_Boolean
isNodeInTreeNoCircular(UA_Nodestore *ns, const UA_NodeId *leafNode, const UA_NodeId *nodeToFind,
                       struct ref_history *visitedRefs, const UA_NodeId *referenceTypeIds,
                       size_t referenceTypeIdsSize) {
    if(UA_NodeId_equal(nodeToFind, leafNode))
        return true;

    if(visitedRefs->depth >= UA_MAX_TREE_RECURSE)
        return false;

    const UA_Node *node = ns->getNode(ns->context, leafNode);
    if(!node)
        return false;

    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        /* Search upwards in the tree */
        if(!refs->isInverse)
            continue;

        /* Consider only the indicated reference types */
        UA_Boolean match = false;
        for(size_t j = 0; j < referenceTypeIdsSize; ++j) {
            if(UA_NodeId_equal(&refs->referenceTypeId, &referenceTypeIds[j])) {
                match = true;
                break;
            }
        }
        if(!match)
            continue;

        /* Match the targets or recurse */
        for(size_t j = 0; j < refs->targetIdsSize; ++j) {
            /* Check if we already have seen the referenced node and skip to
             * avoid endless recursion. Do this only at every 5th depth to save
             * effort. Circular dependencies are rare and forbidden for most
             * reference types. */
            if(visitedRefs->depth % 5 == 4) {
                struct ref_history *last = visitedRefs;
                UA_Boolean skip = UA_FALSE;
                while(!skip && last) {
                    if(UA_NodeId_equal(last->id, &refs->targetIds[j].nodeId))
                        skip = UA_TRUE;
                    last = last->parent;
                }
                if(skip)
                    continue;
            }

            /* Stack-allocate the visitedRefs structure for the next depth */
            struct ref_history nextVisitedRefs = {visitedRefs, &refs->targetIds[j].nodeId,
                                                  (UA_UInt16)(visitedRefs->depth+1)};

            /* Recurse */
            UA_Boolean foundRecursive =
                isNodeInTreeNoCircular(ns, &refs->targetIds[j].nodeId, nodeToFind, &nextVisitedRefs,
                                       referenceTypeIds, referenceTypeIdsSize);
            if(foundRecursive) {
                ns->releaseNode(ns->context, node);
                return true;
            }
        }
    }

    ns->releaseNode(ns->context, node);
    return false;
}

UA_Boolean
isNodeInTree(UA_Nodestore *ns, const UA_NodeId *leafNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize) {
    struct ref_history visitedRefs = {NULL, leafNode, 0};
    return isNodeInTreeNoCircular(ns, leafNode, nodeToFind, &visitedRefs, referenceTypeIds, referenceTypeIdsSize);
}

const UA_Node *
getNodeType(UA_Server *server, const UA_Node *node) {
    /* The reference to the parent is different for variable and variabletype */
    UA_NodeId parentRef;
    UA_Boolean inverse;
    UA_NodeClass typeNodeClass;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        inverse = false;
        typeNodeClass = UA_NODECLASS_OBJECTTYPE;
        break;
    case UA_NODECLASS_VARIABLE:
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        inverse = false;
        typeNodeClass = UA_NODECLASS_VARIABLETYPE;
        break;
    case UA_NODECLASS_OBJECTTYPE:
    case UA_NODECLASS_VARIABLETYPE:
    case UA_NODECLASS_REFERENCETYPE:
    case UA_NODECLASS_DATATYPE:
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        inverse = true;
        typeNodeClass = node->nodeClass;
        break;
    default:
        return NULL;
    }

    /* Return the first matching candidate */
    for(size_t i = 0; i < node->referencesSize; ++i) {
        if(node->references[i].isInverse != inverse)
            continue;
        if(!UA_NodeId_equal(&node->references[i].referenceTypeId, &parentRef))
            continue;
        UA_assert(node->references[i].targetIdsSize > 0);
        const UA_NodeId *targetId = &node->references[i].targetIds[0].nodeId;
        const UA_Node *type = UA_Nodestore_get(server, targetId);
        if(!type)
            continue;
        if(type->nodeClass == typeNodeClass)
            return type;
        UA_Nodestore_release(server, type);
    }

    return NULL;
}

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_Node *node) {
    const UA_NodeId hasSubType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    const UA_NodeId hasTypeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    for(size_t i = 0; i < node->referencesSize; ++i) {
        if(node->references[i].isInverse == false &&
           UA_NodeId_equal(&node->references[i].referenceTypeId, &hasSubType))
            return true;
        if(node->references[i].isInverse == true &&
           UA_NodeId_equal(&node->references[i].referenceTypeId, &hasTypeDefinition))
            return true;
    }
    return false;
}

static const UA_NodeId hasSubtypeNodeId =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASSUBTYPE}};

static UA_StatusCode
getTypeHierarchyFromNode(UA_NodeId **results_ptr, size_t *results_count,
                         size_t *results_size, const UA_Node *node) {
    UA_NodeId *results = *results_ptr;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        /* Is the reference kind relevant? */
        UA_NodeReferenceKind *refs = &node->references[i];
        if(!refs->isInverse)
            continue;
        if(!UA_NodeId_equal(&hasSubtypeNodeId, &refs->referenceTypeId))
            continue;

        /* Append all targets of the reference kind .. if not a duplicate */
        for(size_t j = 0; j < refs->targetIdsSize; ++j) {
            /* Is the target a duplicate? (multi-inheritance) */
            UA_NodeId *targetId = &refs->targetIds[j].nodeId;
            UA_Boolean duplicate = false;
            for(size_t k = 0; k < *results_count; ++k) {
                if(UA_NodeId_equal(targetId, &results[k])) {
                    duplicate = true;
                    break;
                }
            }
            if(duplicate)
                continue;

            /* Increase array length if necessary */
            if(*results_count >= *results_size) {
                size_t new_size = sizeof(UA_NodeId) * (*results_size) * 2;
                UA_NodeId *new_results = (UA_NodeId*)UA_realloc(results, new_size);
                if(!new_results) {
                    UA_Array_delete(results, *results_count, &UA_TYPES[UA_TYPES_NODEID]);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                results = new_results;
                *results_ptr = results;
                *results_size *= 2;
            }

            /* Copy new nodeid to the end of the list */
            UA_StatusCode retval = UA_NodeId_copy(targetId, &results[*results_count]);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_Array_delete(results, *results_count, &UA_TYPES[UA_TYPES_NODEID]);
                return retval;
            }
            *results_count += 1;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
getTypeHierarchy(UA_Nodestore *ns, const UA_NodeId *leafType,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    /* Allocate the results array. Probably too big, but saves mallocs. */
    size_t results_size = 20;
    UA_NodeId *results = (UA_NodeId*)UA_malloc(sizeof(UA_NodeId) * results_size);
    if(!results)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* The leaf is the first element */
    size_t results_count = 1;
    UA_StatusCode retval = UA_NodeId_copy(leafType, &results[0]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(results);
        return retval;
    }

    /* Loop over the array members .. and add new elements to the end */
    for(size_t idx = 0; idx < results_count; ++idx) {
        /* Get the node */
        const UA_Node *node = ns->getNode(ns->context, &results[idx]);

        /* Invalid node, remove from the array */
        if(!node) {
            for(size_t i = idx; i < results_count-1; ++i)
                results[i] = results[i+1];
            results_count--;
            continue;
        }

        /* Add references from the current node to the end of the array */
        retval = getTypeHierarchyFromNode(&results, &results_count,
                                          &results_size, node);

        /* Release the node */
        ns->releaseNode(ns->context, node);

        if(retval != UA_STATUSCODE_GOOD) {
            UA_Array_delete(results, results_count, &UA_TYPES[UA_TYPES_NODEID]);
            return retval;
        }
    }

    /* Zero results. The leaf node was not found */
    if(results_count == 0) {
        UA_free(results);
        results = NULL;
    }

    *typeHierarchy = results;
    *typeHierarchySize = results_count;
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
    const UA_Node *node = UA_Nodestore_get(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode retval = callback(server, session, (UA_Node*)(uintptr_t)node, data);
    UA_Nodestore_release(server, node);
    return retval;
#else
    UA_StatusCode retval;
    do {
        /* Get an editable copy of the node */
        UA_Node *node;
        retval = server->config.nodestore.
            getNodeCopy(server->config.nodestore.context, nodeId, &node);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Run the operation on the copy */
        retval = callback(server, session, node, data);
        if(retval != UA_STATUSCODE_GOOD) {
            server->config.nodestore.deleteNode(server->config.nodestore.context, node);
            return retval;
        }

        /* Replace the node */
        retval = server->config.nodestore.replaceNode(server->config.nodestore.context, node);
    } while(retval != UA_STATUSCODE_GOOD);
    return retval;
#endif
}

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   void *context, const size_t *requestOperations,
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
    -2,                          /* valueRank */
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
    -2,                          /* valueRank */
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

