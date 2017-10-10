/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"

/**********************/
/* Parse NumericRange */
/**********************/

static size_t
readDimension(UA_Byte *buf, size_t buflen, UA_NumericRangeDimension *dim) {
    size_t progress = UA_readNumber(buf, buflen, &dim->min);
    if(progress == 0)
        return 0;
    if(buflen <= progress + 1 || buf[progress] != ':') {
        dim->max = dim->min;
        return progress;
    }

    ++progress;
    size_t progress2 = UA_readNumber(&buf[progress], buflen - progress, &dim->max);
    if(progress2 == 0)
        return 0;

    /* invalid range */
    if(dim->min >= dim->max)
        return 0;

    return progress + progress2;
}

UA_StatusCode
UA_NumericRange_parseFromString(UA_NumericRange *range, const UA_String *str) {
    size_t idx = 0;
    size_t dimensionsMax = 0;
    UA_NumericRangeDimension *dimensions = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t offset = 0;
    while(true) {
        /* alloc dimensions */
        if(idx >= dimensionsMax) {
            UA_NumericRangeDimension *newds;
            size_t newdssize = sizeof(UA_NumericRangeDimension) * (dimensionsMax + 2);
            newds = (UA_NumericRangeDimension*)UA_realloc(dimensions, newdssize);
            if(!newds) {
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
                break;
            }
            dimensions = newds;
            dimensionsMax = dimensionsMax + 2;
        }

        /* read the dimension */
        size_t progress = readDimension(&str->data[offset], str->length - offset,
                                        &dimensions[idx]);
        if(progress == 0) {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        offset += progress;
        ++idx;

        /* loop into the next dimension */
        if(offset >= str->length)
            break;

        if(str->data[offset] != ',') {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        ++offset;
    }

    if(retval == UA_STATUSCODE_GOOD && idx > 0) {
        range->dimensions = dimensions;
        range->dimensionsSize = idx;
    } else
        UA_free(dimensions);

    return retval;
}

/********************************/
/* Information Model Operations */
/********************************/

UA_Boolean
isNodeInTree(UA_Nodestore *ns, const UA_NodeId *leafNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize) {
    if(UA_NodeId_equal(nodeToFind, leafNode))
        return true;

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
            if(isNodeInTree(ns, &refs->targetIds[j].nodeId, nodeToFind,
                            referenceTypeIds, referenceTypeIdsSize)) {
                ns->releaseNode(ns->context, node);
                return true;
            }
        }
    }

    ns->releaseNode(ns->context, node);
    return false;
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
                   const void *data) {
#ifndef UA_ENABLE_MULTITHREADING
    const UA_Node *node = UA_Nodestore_get(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_StatusCode retval = callback(server, session,
                                    (UA_Node*)(uintptr_t)node, data);
    UA_Nodestore_release(server, node);
    return retval;
#else
    UA_StatusCode retval;
    do {
        UA_Node *node;
        retval = server->config.nodestore.getNodeCopy(server->config.nodestore.context,
                                                      nodeId, &node);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        retval = callback(server, session, node, data);
        if(retval != UA_STATUSCODE_GOOD) {
            server->config.nodestore.deleteNode(server->config.nodestore.context, node);
            return retval;
        }
        retval = server->config.nodestore.replaceNode(server->config.nodestore.context, node);
    } while(retval != UA_STATUSCODE_GOOD);
    return retval;
#endif
}

UA_StatusCode
UA_Server_processServiceOperations(UA_Server *server, UA_Session *session,
                                   UA_ServiceOperation operationCallback,
                                   const size_t *requestOperations,
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
        operationCallback(server, session, (void*)reqOp, (void*)respOp);
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

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

