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
    if(dim->min > dim->max)
        return 0;
    
    return progress + progress2;
}

#ifndef UA_BUILD_UNIT_TESTS
static
#endif
UA_StatusCode parse_numericrange(const UA_String *str, UA_NumericRange *range) {
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

enum type_equivalence {
    TYPE_EQUIVALENCE_NONE,
    TYPE_EQUIVALENCE_ENUM,
    TYPE_EQUIVALENCE_OPAQUE
};

static enum type_equivalence typeEquivalence(const UA_DataType *t) {
    if(t->membersSize != 1 || !t->members[0].namespaceZero)
        return TYPE_EQUIVALENCE_NONE;
    if(t->members[0].memberTypeIndex == UA_TYPES_INT32)
        return TYPE_EQUIVALENCE_ENUM;
    if(t->members[0].memberTypeIndex == UA_TYPES_BYTE && t->members[0].isArray)
        return TYPE_EQUIVALENCE_OPAQUE;
    return TYPE_EQUIVALENCE_NONE;
}

static const UA_DataType *
findDataType(const UA_NodeId *typeId) {
    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        if(UA_TYPES[i].typeId.identifier.numeric == typeId->identifier.numeric)
            return &UA_TYPES[i];
    }
    return NULL;
}

/* Tests whether the value matches a variable definition given by
 * - datatype
 * - valueranke
 * - array dimensions.
 * Sometimes it can be necessary to transform the content of the value, e.g.
 * byte array to bytestring or uint32 to some enum. If successful the equivalent
 * variant contains the possibly corrected definition (NODELETE, pointing to the
 * members of the original value, so do not delete) */
UA_StatusCode
UA_Variant_matchVariableDefinition(UA_Server *server, const UA_NodeId *variableDataTypeId,
                                   UA_Int32 variableValueRank, size_t variableArrayDimensionsSize,
                                   const UA_UInt32 *variableArrayDimensions, const UA_Variant *value,
                                   const UA_NumericRange *range, UA_Variant *equivalent) {
    /* Prepare the output variant */
    if(equivalent) {
        *equivalent = *value;
        equivalent->storageType = UA_VARIANT_DATA_NODELETE;
    }

    /* No content is only allowed for BaseDataType */
    const UA_NodeId *valueDataTypeId;
    UA_NodeId basedatatype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
    if(value->type)
        valueDataTypeId = &value->type->typeId;
    else
        valueDataTypeId = &basedatatype;
    
    /* See if the types match. The nodeid on the wire may be != the nodeid in
     * the node for opaque types, enums and bytestrings. value contains the
     * correct type definition after the following paragraph */
    if(!UA_NodeId_equal(valueDataTypeId, variableDataTypeId)) {
        /* is this a subtype? */
        UA_NodeId subtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        UA_Boolean found = false;
        UA_StatusCode retval = isNodeInTree(server->nodestore, valueDataTypeId,
                                            variableDataTypeId, &subtypeId, 1, &found);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        if(found)
            goto check_array;

        const UA_DataType *variableDataType = findDataType(variableDataTypeId);
        const UA_DataType *valueDataType = findDataType(valueDataTypeId);
        if(!equivalent || !variableDataType || !valueDataType)
            return UA_STATUSCODE_BADTYPEMISMATCH;

        /* a string is written to a byte array. the valuerank and array
           dimensions are checked later */
        if(variableDataType == &UA_TYPES[UA_TYPES_BYTE] &&
           valueDataType == &UA_TYPES[UA_TYPES_BYTESTRING] &&
           !range && !UA_Variant_isScalar(value)) {
            UA_ByteString *str = (UA_ByteString*)value->data;
            equivalent->type = &UA_TYPES[UA_TYPES_BYTE];
            equivalent->arrayLength = str->length;
            equivalent->data = str->data;
            goto check_array;
        }

        /* An enum was sent as an int32, or an opaque type as a bytestring. This
         * is detected with the typeIndex indicating the "true" datatype. */
        enum type_equivalence te1 = typeEquivalence(variableDataType);
        enum type_equivalence te2 = typeEquivalence(valueDataType);
        if(te1 != TYPE_EQUIVALENCE_NONE && te1 == te2) {
            equivalent->type = variableDataType;
            goto check_array;
        }

        /* No more possible equivalencies */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Work only on the possibly transformed variant */
    if(equivalent)
        value = equivalent;

 check_array:
    /* Check if the valuerank allows for the value dimension */
    switch(variableValueRank) {
    case -3: /* the value can be a scalar or a one dimensional array */
        if(value->arrayDimensionsSize > 1)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        break;
    case -2: /* the value can be a scalar or an array with any number of dimensions */
        break;
    case -1: /* the value is a scalar */
        if(!UA_Variant_isScalar(value))
            return UA_STATUSCODE_BADTYPEMISMATCH;
    case 0: /* the value is an array with one or more dimensions */
        if(UA_Variant_isScalar(value))
            return UA_STATUSCODE_BADTYPEMISMATCH;
        break;
    default: /* >= 1: the value is an array with the specified number of dimensions */
        if(value->arrayDimensionsSize != (size_t)variableValueRank)
            return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Ranges are checked in detail during writing into the variant */
    if(range)
        return UA_STATUSCODE_GOOD;

    /* See if the array dimensions match */
    if(variableArrayDimensions) {
        if(value->arrayDimensionsSize != variableArrayDimensionsSize)
            return UA_STATUSCODE_BADTYPEMISMATCH;
        /* dimension size zero in the node definition: this value dimension can be any size */
        for(size_t i = 0; i < variableArrayDimensionsSize; i++) {
            if(variableArrayDimensions[i] != value->arrayDimensions[i] &&
               variableArrayDimensions[i] != 0)
                return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }
    return UA_STATUSCODE_GOOD;
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
