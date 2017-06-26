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
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *leafNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize) {
    if(UA_NodeId_equal(nodeToFind, leafNode))
        return true;

    const UA_Node *node = UA_NodeStore_get(ns, leafNode);
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
                            referenceTypeIds, referenceTypeIdsSize))
                return true;
        }
    }
    return false;
}

void getNodeType(UA_Server *server, const UA_Node *node, UA_NodeId *typeId) {
    UA_NodeId_init(typeId);
    UA_NodeId parentRef;
    UA_Boolean inverse;

    /* The reference to the parent is different for variable and variabletype */
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_OBJECT) {
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        inverse = false;
    } else if(node->nodeClass == UA_NODECLASS_VARIABLETYPE ||
              node->nodeClass == UA_NODECLASS_OBJECTTYPE ||
              node->nodeClass == UA_NODECLASS_REFERENCETYPE ||
              node->nodeClass == UA_NODECLASS_DATATYPE) {
        parentRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        inverse = true;
    } else {
        return;
    }

    /* Stop at the first matching candidate */
    for(size_t i = 0; i < node->referencesSize; ++i) {
        if(node->references[i].isInverse != inverse)
            continue;
        if(!UA_NodeId_equal(&node->references[i].referenceTypeId, &parentRef))
            continue;
        if(node->references[i].targetIdsSize == 0)
            return;
        UA_NodeId_copy(&node->references[i].targetIds[0].nodeId, typeId);
        return;
    }
}

const UA_VariableTypeNode *
getVariableNodeType(UA_Server *server, const UA_VariableNode *node) {
    UA_NodeId vtId;
    getNodeType(server, (const UA_Node*)node, &vtId);

    const UA_Node *vt = UA_NodeStore_get(server->nodestore, &vtId);
    if(!vt || vt->nodeClass != UA_NODECLASS_VARIABLETYPE) {
        vt = NULL;
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "No VariableType for the node found");
    }
    return (const UA_VariableTypeNode*)vt;
}

const UA_ObjectTypeNode *
getObjectNodeType(UA_Server *server, const UA_ObjectNode *node) {
    UA_NodeId otId;
    getNodeType(server, (const UA_Node*)node, &otId);

    const UA_Node *ot = UA_NodeStore_get(server->nodestore, &otId);
    if(!ot || ot->nodeClass != UA_NODECLASS_OBJECTTYPE) {
        ot = NULL;
        UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "No ObjectType for the node found");
    }
    return (const UA_ObjectTypeNode*)ot;
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

/* For mulithreading: make a copy of the node, edit and replace.
 * For singletrheading: edit the original */
UA_StatusCode
UA_Server_editNode(UA_Server *server, UA_Session *session,
                   const UA_NodeId *nodeId, UA_EditNodeCallback callback,
                   const void *data) {
#ifndef UA_ENABLE_MULTITHREADING
    const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    UA_Node *editNode = (UA_Node*)(uintptr_t)node; // dirty cast
    return callback(server, session, editNode, data);
#else
    UA_StatusCode retval;
    do {
        UA_RCU_LOCK();
        UA_Node *copy = UA_NodeStore_getCopy(server->nodestore, nodeId);
        UA_RCU_UNLOCK();
        if(!copy)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        retval = callback(server, session, copy, data);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_NodeStore_deleteNode(copy);
            return retval;
        }
        UA_RCU_LOCK();
        retval = UA_NodeStore_replace(server->nodestore, copy);
        UA_RCU_UNLOCK();
    } while(retval != UA_STATUSCODE_GOOD);
    return UA_STATUSCODE_GOOD;
#endif
}
