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

    progress++;
    size_t progress2 = UA_readNumber(&buf[progress], buflen - progress, &dim->max);
    if(progress2 == 0)
        return 0;

    /* invalid range */
    if(dim->min >= dim->max)
        return 0;
    
    return progress + progress2;
}

UA_StatusCode
parse_numericrange(const UA_String *str, UA_NumericRange *range) {
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
            newds = UA_realloc(dimensions, newdssize);
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
        idx++;

        /* loop into the next dimension */
        if(offset >= str->length)
            break;

        if(str->data[offset] != ',') {
            retval = UA_STATUSCODE_BADINDEXRANGEINVALID;
            break;
        }
        offset++;
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

/* Returns the type and all subtypes. We start with an array with a single root
 * nodeid. When a relevant reference is found, we add the nodeids to the back of
 * the array and increase the size. Since the hierarchy is not cyclic, we can
 * safely progress in the array to process the newly found referencetype
 * nodeids. */
UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_NodeId *root,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    const UA_Node *node = UA_NodeStore_get(ns, root);
    if(!node)
        return UA_STATUSCODE_BADNOMATCH;
    if(node->nodeClass != UA_NODECLASS_REFERENCETYPE)
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;

    size_t results_size = 20; // probably too big, but saves mallocs
    UA_NodeId *results = UA_malloc(sizeof(UA_NodeId) * results_size);
    if(!results)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_NodeId_copy(root, &results[0]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(results);
        return retval;
    }

    size_t idx = 0; // where are we currently in the array?
    size_t last = 0; // where is the last element in the array?
    const UA_NodeId hasSubtypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    do {
        node = UA_NodeStore_get(ns, &results[idx]);
        if(!node || node->nodeClass != UA_NODECLASS_REFERENCETYPE)
            continue;
        for(size_t i = 0; i < node->referencesSize; i++) {
            if(node->references[i].isInverse == true ||
               !UA_NodeId_equal(&hasSubtypeNodeId, &node->references[i].referenceTypeId))
                continue;

            if(++last >= results_size) { // is the array big enough?
                UA_NodeId *new_results = UA_realloc(results, sizeof(UA_NodeId) * results_size * 2);
                if(!new_results) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                results = new_results;
                results_size *= 2;
            }

            retval = UA_NodeId_copy(&node->references[i].targetId.nodeId, &results[last]);
            if(retval != UA_STATUSCODE_GOOD) {
                last--; // for array_delete
                break;
            }
        }
    } while(++idx <= last && retval == UA_STATUSCODE_GOOD);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(results, last, &UA_TYPES[UA_TYPES_NODEID]);
        return retval;
    }

    *typeHierarchy = results;
    *typeHierarchySize = last + 1;
    return UA_STATUSCODE_GOOD;
}

/* Recursively searches "upwards" in the tree following specific reference types */
UA_StatusCode
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *leafNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize, UA_Boolean *found) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(UA_NodeId_equal(leafNode, nodeToFind)) {
        *found = true;
        return UA_STATUSCODE_GOOD;
    }

    const UA_Node *node = UA_NodeStore_get(ns,leafNode);
    if(!node)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Search upwards in the tree */
    for(size_t i = 0; i < node->referencesSize; i++) {
        if(!node->references[i].isInverse)
            continue;

        /* Recurse only for valid reference types */
        for(size_t j = 0; j < referenceTypeIdsSize; j++) {
            if(!UA_NodeId_equal(&node->references[i].referenceTypeId, &referenceTypeIds[j]))
                continue;
            retval = isNodeInTree(ns, &node->references[i].targetId.nodeId, nodeToFind,
                                  referenceTypeIds, referenceTypeIdsSize, found);
            if(*found || retval != UA_STATUSCODE_GOOD)
                return retval;
            break;
        }
    }

    /* Dead end */
    *found = false;
    return UA_STATUSCODE_GOOD;
}

const UA_Node *
getNodeType(UA_Server *server, const UA_Node *node) {
    /* The reference to the parent is different for variable and variabletype */ 
    UA_NodeId parentRef;
    UA_Boolean inverse;
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_OBJECT) {
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        inverse = false;
    } else if(node->nodeClass == UA_NODECLASS_VARIABLETYPE ||
              /* node->nodeClass == UA_NODECLASS_OBJECTTYPE || // objecttype may have multiple parents */
              node->nodeClass == UA_NODECLASS_REFERENCETYPE ||
              node->nodeClass == UA_NODECLASS_DATATYPE) {
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        inverse = true;
    } else {
        return NULL;
    }

    /* stop at the first matching candidate */
    UA_NodeId *parentId = NULL;
    for(size_t i = 0; i < node->referencesSize; i++) {
        if(node->references[i].isInverse == inverse &&
           UA_NodeId_equal(&node->references[i].referenceTypeId, &parentRef)) {
            parentId = &node->references[i].targetId.nodeId;
            break;
        }
    }

    if(!parentId)
        return NULL;
    return UA_NodeStore_get(server->nodestore, parentId);
}

const UA_VariableTypeNode *
getVariableNodeType(UA_Server *server, const UA_VariableNode *node) {
    const UA_Node *type = getNodeType(server, (const UA_Node*)node);
    if(!type || type->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return NULL;
    return (const UA_VariableTypeNode*)type;
}

const UA_ObjectTypeNode *
getObjectNodeType(UA_Server *server, const UA_ObjectNode *node) {
    const UA_Node *type = getNodeType(server, (const UA_Node*)node);
    if(type->nodeClass != UA_NODECLASS_OBJECTTYPE)
        return NULL;
    return (const UA_ObjectTypeNode*)type;
}

UA_Boolean
UA_Node_hasSubTypeOrInstances(const UA_Node *node) {
    const UA_NodeId hasSubType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    const UA_NodeId hasTypeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    for(size_t i = 0; i < node->referencesSize; i++) {
        if(node->references[i].isInverse == false &&
           UA_NodeId_equal(&node->references[i].referenceTypeId, &hasSubType))
            return true;
        if(node->references[i].isInverse == true &&
           UA_NodeId_equal(&node->references[i].referenceTypeId, &hasTypeDefinition))
            return true;
    }
    return false;
}

/* For mulithreading: make a copy of the node, edit and replace.
 * For singletrheading: edit the original */
UA_StatusCode
UA_Server_editNode(UA_Server *server, UA_Session *session,
                   const UA_NodeId *nodeId, UA_EditNodeCallback callback,
                   const void *data) {
    UA_StatusCode retval;
    do {
#ifndef UA_ENABLE_MULTITHREADING
        const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_Node *editNode = (UA_Node*)(uintptr_t)node; // dirty cast
        retval = callback(server, session, editNode, data);
        return retval;
#else
        UA_Node *copy = UA_NodeStore_getCopy(server->nodestore, nodeId);
        if(!copy)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval = callback(server, session, copy, data);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeStore_deleteNode(copy);
            return retval;
        }
        retval = UA_NodeStore_replace(server->nodestore, copy);
#endif
    } while(retval != UA_STATUSCODE_GOOD);
    return UA_STATUSCODE_GOOD;
}
