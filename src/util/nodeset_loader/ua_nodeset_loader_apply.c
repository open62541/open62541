/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Matthias Konnerth
 *    Copyright 2021 (c) Jan Murzyn
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_nodeset_loader_internal.h"

#include "parse_num.h"

static UA_StatusCode addCustomDataType(NodeSet *nodeset, const NL_DataTypeNode *node);
static const UA_DataType *findDataType(NodeSet *nodeset, const UA_NodeId *id);

static UA_NodeId
getParentId(const NL_Node *node, UA_NodeId *parentRefId) {
    if(node->insertionParentRef) {
        if(parentRefId)
            *parentRefId = node->insertionParentRef->refType;
        if(node->insertionParent)
            return node->insertionParent->id;
        return node->insertionParentRef->target;
    }
    return UA_NODEID_NULL;
}

static UA_StatusCode
getArrayDimensions(const char *s, size_t *dimsSize, UA_UInt32 **dims) {
    *dimsSize = 0;
    *dims = NULL;
    if(!s || s[0] == 0)
        return UA_STATUSCODE_GOOD;

    size_t count = 1;
    for(const char *pos = s; *pos; pos++) {
        if(*pos == ',') {
            if(count == SIZE_MAX / sizeof(UA_UInt32))
                return UA_STATUSCODE_BADOUTOFMEMORY;
            count++;
        }
    }

    UA_UInt32 *values = (UA_UInt32 *)UA_malloc(count * sizeof(UA_UInt32));
    if(!values)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    const char *pos = s;
    for(size_t i = 0; i < count; i++) {
        const char *end = pos;
        while(*end && *end != ',')
            end++;
        uint64_t value = 0;
        size_t length = (size_t)(end - pos);
        if(length == 0 || parseUInt64(pos, length, &value) != length || value > UA_UINT32_MAX) {
            UA_free(values);
            return UA_STATUSCODE_BADDECODINGERROR;
        }
        values[i] = (UA_UInt32)value;
        pos = end + (*end == ',');
    }

    *dimsSize = count;
    *dims = values;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
completeArrayDimensions(UA_Int32 valueRank, size_t *dimsSize, UA_UInt32 **dims) {
    if(valueRank <= 0 || *dimsSize > 0)
        return UA_STATUSCODE_GOOD;
    if((size_t)valueRank > SIZE_MAX / sizeof(UA_UInt32))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    *dims = (UA_UInt32 *)UA_calloc((size_t)valueRank, sizeof(UA_UInt32));
    if(!*dims)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *dimsSize = (size_t)valueRank;
    return UA_STATUSCODE_GOOD;
}

static bool
getDataType(UA_Server *server, const UA_NodeId *dataType, const UA_NodeId *inheritedFrom,
            UA_NodeId *result) {
    if(!UA_NodeId_isNull(dataType)) {
        *result = *dataType;
        return false;
    }

    UA_NodeId_init(result);
    if(!UA_NodeId_isNull(inheritedFrom) &&
       UA_Server_readDataType(server, *inheritedFrom, result) == UA_STATUSCODE_GOOD &&
       !UA_NodeId_isNull(result))
        return true;

    UA_NodeId_clear(result);
    *result = UA_TYPES[UA_TYPES_VARIANT].typeId;
    return false;
}

#ifdef UA_ENABLE_XML_ENCODING

static const UA_DataType *
getDecodedValueType(const UA_Variant *value) {
    if(value->type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT] || !value->data ||
       value->data == UA_EMPTY_ARRAY_SENTINEL)
        return value->type;

    const UA_ExtensionObject *eo = (const UA_ExtensionObject *)value->data;
    size_t valuesSize = UA_Variant_isScalar(value) ? 1 : value->arrayLength;
    if(eo[0].encoding != UA_EXTENSIONOBJECT_DECODED &&
       eo[0].encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
        return value->type;
    const UA_DataType *type = eo[0].content.decoded.type;
    for(size_t i = 1; i < valuesSize; i++) {
        if((eo[i].encoding != UA_EXTENSIONOBJECT_DECODED &&
            eo[i].encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
           eo[i].content.decoded.type != type)
            return value->type;
    }
    return type;
}

static void
adjustDecodedValueType(UA_Server *server, UA_Variant *value, const UA_NodeId *dataTypeId) {
    const UA_DataType *targetType = UA_Server_findDataType(server, dataTypeId);
    if(!targetType || targetType->typeKind != UA_DATATYPEKIND_UINT32 ||
       value->type != &UA_TYPES[UA_TYPES_INT32])
        return;

    size_t valuesSize = UA_Variant_isScalar(value) ? 1 : value->arrayLength;
    const UA_Int32 *values = (const UA_Int32 *)value->data;
    for(size_t i = 0; i < valuesSize; i++) {
        if(values[i] < 0)
            return;
    }

    /* The allocation has the same element size and can be converted in situ. */
    for(size_t i = 0; i < valuesSize; i++) {
        UA_UInt32 converted = (UA_UInt32)values[i];
        memcpy((UA_Byte *)value->data + i * sizeof(UA_UInt32), &converted, sizeof(converted));
    }
    value->type = targetType;
}

static bool
isArgumentDescriptorValue(UA_Server *server, const UA_Variant *value, const UA_NodeId *dataTypeId) {
    if(UA_NodeId_equal(dataTypeId, &UA_TYPES[UA_TYPES_VARIANT].typeId))
        return false;
    const UA_DataType *targetType = UA_Server_findDataType(server, dataTypeId);
    const UA_DataType *valueType = getDecodedValueType(value);
    return targetType && valueType == &UA_TYPES[UA_TYPES_ARGUMENT] &&
           targetType->typeKind != UA_DATATYPEKIND_STRUCTURE &&
           targetType->typeKind != UA_DATATYPEKIND_OPTSTRUCT &&
           targetType->typeKind != UA_DATATYPEKIND_UNION;
}

#endif

static UA_StatusCode
handleVariableNode(NodeSet *nodeset, const NL_VariableNode *node, UA_NodeId parentId,
                   UA_NodeId parentReferenceId) {
    UA_Server *server = nodeset->server;
    UA_NodeId typeDefId =
        node->typeDefinitionRef ? node->typeDefinitionRef->target : UA_NODEID_NULL;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = node->displayName;
    UA_NodeId dataType;
    bool dataTypeOwned = getDataType(server, &node->datatype, &typeDefId, &dataType);
    attr.dataType = dataType;
    attr.valueRank = node->valueRank;
    UA_UInt32 *arrDims = NULL;
    UA_StatusCode ret =
        getArrayDimensions(node->arrayDimensions, &attr.arrayDimensionsSize, &arrDims);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        goto cleanup;
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Ignoring malformed ArrayDimensions");
        attr.arrayDimensionsSize = 0;
    }
    attr.arrayDimensions = arrDims;
    attr.accessLevel = node->accessLevel;
    attr.userAccessLevel = node->userAccessLevel;
    attr.description = node->description;
    attr.historizing = node->historizing;
    attr.minimumSamplingInterval = node->minimumSamplingInterval;

    ret = UA_STATUSCODE_GOOD;
    if(node->value.length > 0) {
#ifdef UA_ENABLE_XML_ENCODING
        UA_DecodeXmlOptions opts;
        memset(&opts, 0, sizeof(UA_DecodeXmlOptions));
        opts.unwrapped = true;
        opts.customTypes = UA_Server_getDataTypes(server);
        opts.namespaceMapping = &nodeset->namespaceMapping;
        ret = UA_decodeXml(&node->value, &attr.value, &UA_TYPES[UA_TYPES_VARIANT], &opts);
        if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
            goto cleanup;
        if(ret != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Failed to parse the value of %N", node->id);
        else
            adjustDecodedValueType(server, &attr.value, &attr.dataType);

        if(ret == UA_STATUSCODE_GOOD &&
           isArgumentDescriptorValue(server, &attr.value, &attr.dataType)) {
            UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Ignoring the Argument descriptor used as the initial "
                           "value of %N",
                           node->id);
            UA_Variant_clear(&attr.value);
        } else if(ret == UA_STATUSCODE_GOOD && attr.value.arrayLength > 0 &&
                  attr.arrayDimensionsSize > 0 && attr.valueRank > 1) {
            UA_Array_delete(attr.value.arrayDimensions, attr.value.arrayDimensionsSize,
                            &UA_TYPES[UA_TYPES_UINT32]);
            attr.value.arrayDimensions = NULL;
            attr.value.arrayDimensionsSize = 0;
            ret = UA_Array_copy(attr.arrayDimensions, attr.arrayDimensionsSize,
                                (void **)&attr.value.arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
            if(ret != UA_STATUSCODE_GOOD)
                goto cleanup;
            attr.value.arrayDimensionsSize = attr.arrayDimensionsSize;
        }

        /* Accept a directly encoded value for a one-element array. The scalar
         * allocation already has the same layout as an array with one entry. */
        if(ret == UA_STATUSCODE_GOOD && attr.valueRank == 1 && UA_Variant_isScalar(&attr.value))
            attr.value.arrayLength = 1;
#else
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Ignoring the value of %N because XML "
                       "decoding is disabled",
                       node->id);
#endif
    }

    /* Some legacy NodeSets omit ValueRank for an array and rely on the
     * VariableType's permissive rank. Preserve the XML default for scalars,
     * but accept that established representation for decoded arrays. */
    if(!node->valueRankDefined && attr.value.type && !UA_Variant_isScalar(&attr.value) &&
       !UA_NodeId_isNull(&typeDefId))
        (void)UA_Server_readValueRank(server, typeDefId, &attr.valueRank);

    /* Accept a one-dimensional value that exceeds the declared maximum. */
    if(ret == UA_STATUSCODE_GOOD && attr.valueRank == 1 && attr.arrayDimensionsSize == 1 &&
       arrDims[0] > 0 && attr.value.arrayLength > arrDims[0]) {
        if(attr.value.arrayLength <= UA_UINT32_MAX)
            arrDims[0] = (UA_UInt32)attr.value.arrayLength;
        else
            arrDims[0] = 0;
    }

    /* An omitted ArrayDimensions means that every positive-rank dimension is
     * unspecified. The server requires one entry per dimension. */
    UA_StatusCode dimsRet =
        completeArrayDimensions(attr.valueRank, &attr.arrayDimensionsSize, &arrDims);
    attr.arrayDimensions = arrDims;
    if(dimsRet != UA_STATUSCODE_GOOD) {
        ret = dimsRet;
        goto cleanup;
    }

    /* UA_Server_addNode_begin copies the value. */
    ret = UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLE, node->id, parentId,
                                  parentReferenceId, node->browseName, typeDefId, &attr,
                                  &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], NULL, NULL);

cleanup:
    UA_Variant_clear(&attr.value);
    UA_free(arrDims);
    if(dataTypeOwned)
        UA_NodeId_clear(&dataType);
    return ret;
}

static UA_StatusCode
handleVariableTypeNode(UA_Server *server, const NL_VariableTypeNode *node, UA_NodeId parentId,
                       UA_NodeId parentReferenceId) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = node->displayName;
    UA_NodeId dataType;
    bool dataTypeOwned = getDataType(server, &node->datatype, &parentId, &dataType);
    attr.dataType = dataType;
    attr.description = node->description;
    attr.valueRank = node->valueRank;
    if(!node->valueRankDefined && !UA_NodeId_isNull(&parentId))
        (void)UA_Server_readValueRank(server, parentId, &attr.valueRank);
    attr.isAbstract = node->isAbstract;

    UA_UInt32 *arrayDimensions = NULL;
    UA_StatusCode ret =
        getArrayDimensions(node->arrayDimensions, &attr.arrayDimensionsSize, &arrayDimensions);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        goto cleanup;
    if(ret != UA_STATUSCODE_GOOD)
        attr.arrayDimensionsSize = 0;
    attr.arrayDimensions = arrayDimensions;

    ret = completeArrayDimensions(attr.valueRank, &attr.arrayDimensionsSize, &arrayDimensions);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;
    attr.arrayDimensions = arrayDimensions;

    ret = UA_Server_addNode_begin(server, UA_NODECLASS_VARIABLETYPE, node->id, parentId,
                                  parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                  &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES], NULL, NULL);
cleanup:
    UA_free(arrayDimensions);
    if(dataTypeOwned)
        UA_NodeId_clear(&dataType);
    return ret;
}

typedef enum { NL_DEPENDENCY_READY, NL_DEPENDENCY_WAIT, NL_DEPENDENCY_REJECT } NL_DependencyState;

static bool
nodeExists(const NL_Node *node) {
    return node->addState == NL_NODE_BEGUN || node->addState == NL_NODE_FINISHED;
}

static void
discardBegunNode(NodeSet *nodeset, NL_Node *node) {
    UA_assert(node->addState == NL_NODE_BEGUN);
    UA_StatusCode res = UA_Server_deleteNode(nodeset->server, node->id, true);
    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADNODEIDUNKNOWN)
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Could not discard unfinished node %N (%s)", node->id,
                       UA_StatusCode_name(res));
    node->addState = NL_NODE_REJECTED;
}

static void
discardBegunNodes(NodeSet *nodeset) {
    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        if(node->addState == NL_NODE_BEGUN)
            discardBegunNode(nodeset, node);
    }
}

static UA_StatusCode
finishNode(NodeSet *nodeset, NL_Node *node) {
#ifdef UA_ENABLE_METHODCALLS
    UA_StatusCode res;
    if(node->nodeClass == UA_NODECLASS_METHOD)
        res = UA_Server_addMethodNode_finish(nodeset->server, node->id, NULL, 0, NULL, 0, NULL);
    else
        res = UA_Server_addNode_finish(nodeset->server, node->id);
#else
    UA_StatusCode res = UA_Server_addNode_finish(nodeset->server, node->id);
#endif
    if(res == UA_STATUSCODE_GOOD) {
        node->addState = NL_NODE_FINISHED;
    } else {
        node->addState = NL_NODE_REJECTED;
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Could not finish node %N (%s)", node->id,
                       UA_StatusCode_name(res));
    }
    return res;
}

static UA_StatusCode
beginNode(NodeSet *nodeset, NL_Node *node) {
    UA_NodeId parentReferenceId = UA_NODEID_NULL;
    UA_NodeId parentId = getParentId(node, &parentReferenceId);
    UA_Server *server = nodeset->server;

    UA_StatusCode res = UA_STATUSCODE_BADNODECLASSINVALID;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT: {
        const NL_ObjectNode *object = (const NL_ObjectNode *)node;
        UA_ObjectAttributes attr = UA_ObjectAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.eventNotifier = object->eventNotifier;
        UA_NodeId typeDefinition =
            node->typeDefinitionRef ? node->typeDefinitionRef->target : UA_NODEID_NULL;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, typeDefinition, &attr,
                                      &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);
        break;
    }

    case UA_NODECLASS_METHOD: {
        const NL_MethodNode *method = (const NL_MethodNode *)node;
        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.executable = method->executable;
        attr.userExecutable = method->userExecutable;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                      &UA_TYPES[UA_TYPES_METHODATTRIBUTES], NULL, NULL);
        break;
    }

    case UA_NODECLASS_OBJECTTYPE: {
        const NL_ObjectTypeNode *objectType = (const NL_ObjectTypeNode *)node;
        UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.isAbstract = objectType->isAbstract;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                      &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES], NULL, NULL);
        break;
    }

    case UA_NODECLASS_REFERENCETYPE: {
        const NL_ReferenceTypeNode *referenceType = (const NL_ReferenceTypeNode *)node;
        UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.inverseName = referenceType->inverseName;
        attr.symmetric = referenceType->symmetric;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                      &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES], NULL, NULL);
        break;
    }

    case UA_NODECLASS_VARIABLETYPE:
        res = handleVariableTypeNode(server, (const NL_VariableTypeNode *)node, parentId,
                                     parentReferenceId);
        break;

    case UA_NODECLASS_VARIABLE:
        res =
            handleVariableNode(nodeset, (const NL_VariableNode *)node, parentId, parentReferenceId);
        break;
    case UA_NODECLASS_DATATYPE: {
        const NL_DataTypeNode *dataType = (const NL_DataTypeNode *)node;
        UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.isAbstract = dataType->isAbstract;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                      &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES], NULL, NULL);
        break;
    }

    case UA_NODECLASS_VIEW: {
        const NL_ViewNode *view = (const NL_ViewNode *)node;
        UA_ViewAttributes attr = UA_ViewAttributes_default;
        attr.displayName = node->displayName;
        attr.description = node->description;
        attr.eventNotifier = view->eventNotifier;
        attr.containsNoLoops = view->containsNoLoops;
        res = UA_Server_addNode_begin(server, node->nodeClass, node->id, parentId,
                                      parentReferenceId, node->browseName, UA_NODEID_NULL, &attr,
                                      &UA_TYPES[UA_TYPES_VIEWATTRIBUTES], NULL, NULL);
        break;
    }

    default:
        break;
    }

    if(res == UA_STATUSCODE_GOOD) {
        node->addState = NL_NODE_BEGUN;
        if(node->insertionParentRef)
            node->insertionParentRef->addedWithNode = true;
        if(node->typeDefinitionRef)
            node->typeDefinitionRef->addedWithNode = true;
    } else if(res == UA_STATUSCODE_BADNODEIDEXISTS) {
        node->addState = NL_NODE_FINISHED;
    } else {
        node->addState = NL_NODE_REJECTED;
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Could not add node %N (%s)", node->id,
                       UA_StatusCode_name(res));
    }
    return res;
}

static void
detectExistingNodes(NodeSet *nodeset) {
    UA_Server *server = nodeset->server;
    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
        UA_StatusCode res = UA_Server_readNodeClass(server, node->id, &nodeClass);
        if(res != UA_STATUSCODE_GOOD)
            continue;
        node->addState = NL_NODE_FINISHED;
        if(nodeClass != node->nodeClass)
            UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Existing node %N has a different NodeClass", node->id);
    }
}

static NL_DependencyState
dependencyNodeState(const NL_Node *dependency, bool requireFinished) {
    if(!dependency)
        return NL_DEPENDENCY_READY;
    if(dependency->addState == NL_NODE_REJECTED)
        return NL_DEPENDENCY_REJECT;
    if(requireFinished ? dependency->addState == NL_NODE_FINISHED : nodeExists(dependency))
        return NL_DEPENDENCY_READY;
    return NL_DEPENDENCY_WAIT;
}

static NL_DependencyState
externalNodeReady(UA_Server *server, const UA_NodeId *nodeId, UA_NodeClass expectedClass,
                  UA_StatusCode *status) {
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    *status = UA_Server_readNodeClass(server, *nodeId, &nodeClass);
    if(*status != UA_STATUSCODE_GOOD)
        return NL_DEPENDENCY_REJECT;
    if(expectedClass != UA_NODECLASS_UNSPECIFIED && nodeClass != expectedClass) {
        *status = UA_STATUSCODE_BADNODECLASSINVALID;
        return NL_DEPENDENCY_REJECT;
    }
    return NL_DEPENDENCY_READY;
}

static NL_DependencyState
beginDependenciesReady(NodeSet *nodeset, const NL_Node *node, UA_StatusCode *status) {
    if(node->insertionParent) {
        NL_DependencyState state = dependencyNodeState(node->insertionParent, false);
        if(state != NL_DEPENDENCY_READY) {
            *status = UA_STATUSCODE_BADPARENTNODEIDINVALID;
            return state;
        }
    } else if(node->insertionParentRef) {
        NL_DependencyState state = externalNodeReady(
            nodeset->server, &node->insertionParentRef->target, UA_NODECLASS_UNSPECIFIED, status);
        if(state != NL_DEPENDENCY_READY) {
            *status = UA_STATUSCODE_BADPARENTNODEIDINVALID;
            return state;
        }
    }

    if(node->nodeClass != UA_NODECLASS_OBJECT && node->nodeClass != UA_NODECLASS_VARIABLE)
        return NL_DEPENDENCY_READY;

    NL_Reference *typeDef = node->typeDefinitionRef;
    if(!typeDef)
        return NL_DEPENDENCY_READY;
    UA_NodeClass expected = node->nodeClass == UA_NODECLASS_OBJECT ? UA_NODECLASS_OBJECTTYPE
                                                                   : UA_NODECLASS_VARIABLETYPE;
    if(typeDef->targetPtr) {
        if(typeDef->targetPtr->nodeClass != expected) {
            *status = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            return NL_DEPENDENCY_REJECT;
        }
        NL_DependencyState state = dependencyNodeState(typeDef->targetPtr, false);
        if(state != NL_DEPENDENCY_READY)
            *status = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
        return state;
    }
    NL_DependencyState state =
        externalNodeReady(nodeset->server, &typeDef->target, expected, status);
    if(state != NL_DEPENDENCY_READY)
        *status = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    return state;
}

static UA_StatusCode
beginPhase(NodeSet *nodeset, UA_NodeClass nodeClasses) {
    bool progress;
    do {
        progress = false;
        for(NL_Node *node = nodeset->nodes; node; node = node->next) {
            if(!(nodeClasses & node->nodeClass) || node->addState != NL_NODE_NOT_ADDED)
                continue;

            UA_StatusCode dependencyStatus = UA_STATUSCODE_GOOD;
            NL_DependencyState state = beginDependenciesReady(nodeset, node, &dependencyStatus);
            if(state == NL_DEPENDENCY_WAIT)
                continue;
            if(state == NL_DEPENDENCY_REJECT) {
                node->addState = NL_NODE_REJECTED;
                UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                               "NodeSetLoader: Cannot resolve dependencies of node %N (%s)",
                               node->id, UA_StatusCode_name(dependencyStatus));
                continue;
            }

            UA_StatusCode res = beginNode(nodeset, node);
            if(res == UA_STATUSCODE_BADOUTOFMEMORY)
                return res;
            if(nodeExists(node))
                progress = true;
        }
    } while(progress);

    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        if(!(nodeClasses & node->nodeClass) || node->addState != NL_NODE_NOT_ADDED)
            continue;
        node->addState = NL_NODE_REJECTED;
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Cannot resolve dependencies of node %N", node->id);
    }
    return UA_STATUSCODE_GOOD;
}

static void
addAllReferences(NodeSet *nodeset, NL_Node *node) {
    if(!nodeExists(node))
        return;

    for(NL_Reference *ref = node->refs; ref != NULL; ref = ref->next) {
        /* The begin call already added the insertion-parent and type-definition
         * references. Existing nodes may still be missing either reference. */
        if(ref->addedWithNode)
            continue;
        UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
        target.nodeId = ref->target;
        UA_StatusCode res =
            UA_Server_addReference(nodeset->server, node->id, ref->refType, target, ref->isForward);
        if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                           "NodeSetLoader: Could not add reference from %N "
                           "to %N (%s)",
                           node->id, ref->target, UA_StatusCode_name(res));
    }
}

static const UA_NodeId *
getEncodingId(const NL_DataTypeNode *node, const char *browseName) {
    UA_NodeId encodingRefType = UA_NS0ID(HASENCODING);
    UA_String name = UA_STRING((char *)(uintptr_t)browseName);
    for(NL_Reference *ref = node->refs; ref; ref = ref->next) {
        if(UA_NodeId_equal(&encodingRefType, &ref->refType) && ref->targetPtr &&
           UA_String_equal(&ref->targetPtr->browseName.name, &name))
            return &ref->target;
    }
    return NULL;
}

static const UA_DataType *
findDataType(NodeSet *nodeset, const UA_NodeId *id) {
    const UA_DataType *type = UA_Server_findDataType(nodeset->server, id);
    if(type)
        return type;

    /* Abstract namespace-zero types have no binary representation of their
     * own. Encode their values as Variants, as the nodeset compiler does. */
    if(id->namespaceIndex == 0)
        return &UA_TYPES[UA_TYPES_VARIANT];
    return NULL;
}

static UA_StatusCode
addTypeFromDescription(NodeSet *nodeset, const NL_DataTypeNode *node, UA_NodeId parent,
                       UA_ExtensionObject *description) {
    UA_DataType type;
    memset(&type, 0, sizeof(type));

    UA_StatusCode res =
        UA_DataType_fromDescription(&type, description, UA_Server_getDataTypes(nodeset->server));
    const UA_NodeId *xmlEncodingId = getEncodingId(node, "Default XML");
    if(res == UA_STATUSCODE_GOOD && xmlEncodingId)
        res = UA_NodeId_copy(xmlEncodingId, &type.xmlEncodingId);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Server_addDataType(nodeset->server, parent, &type);
    UA_DataType_clear(&type);
    return res;
}

static UA_StatusCode
addSimpleDataType(NodeSet *nodeset, const NL_DataTypeNode *node, UA_NodeId parent,
                  const UA_DataType *parentType) {
    if(!parentType || parentType->typeKind > UA_DATATYPEKIND_DIAGNOSTICINFO)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_SimpleTypeDescription description;
    UA_SimpleTypeDescription_init(&description);
    description.dataTypeId = node->id;
    description.name = node->browseName;
    description.name.namespaceIndex = 0;
    description.baseDataType = parent;
    description.builtInType = (UA_Byte)(parentType->typeKind + 1);

    UA_ExtensionObject eo;
    UA_ExtensionObject_setValueNoDelete(&eo, &description,
                                        &UA_TYPES[UA_TYPES_SIMPLETYPEDESCRIPTION]);
    return addTypeFromDescription(nodeset, node, parent, &eo);
}

static UA_StatusCode
addEnumDataType(NodeSet *nodeset, const NL_DataTypeNode *node, UA_NodeId parent) {
    size_t fieldsSize = node->definition ? node->definition->fieldsSize : 0;
    if(fieldsSize > UA_DATATYPE_MEMBERS_MAX)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;

    UA_EnumField *fields = (UA_EnumField *)UA_calloc(fieldsSize, sizeof(UA_EnumField));
    if(fieldsSize > 0 && !fields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for(size_t i = 0; i < fieldsSize; i++) {
        const NL_DataTypeDefinitionField *src = &node->definition->fields[i];
        fields[i].name = UA_STRING(src->name);
        fields[i].value = src->value;
    }

    UA_EnumDescription description;
    UA_EnumDescription_init(&description);
    description.dataTypeId = node->id;
    description.name = node->browseName;
    description.name.namespaceIndex = 0;
    description.builtInType = UA_DATATYPEKIND_INT32 + 1;
    description.enumDefinition.fields = fields;
    description.enumDefinition.fieldsSize = fieldsSize;

    UA_ExtensionObject eo;
    UA_ExtensionObject_setValueNoDelete(&eo, &description, &UA_TYPES[UA_TYPES_ENUMDESCRIPTION]);
    UA_StatusCode res = addTypeFromDescription(nodeset, node, parent, &eo);
    UA_free(fields);
    return res;
}

static UA_StatusCode
addStructureDataType(NodeSet *nodeset, const NL_DataTypeNode *node, UA_NodeId parent,
                     const UA_DataType *parentType, bool includeDefinition) {
    size_t inheritedFieldsSize = parentType ? parentType->membersSize : 0;
    size_t ownFieldsSize =
        (includeDefinition && node->definition) ? node->definition->fieldsSize : 0;
    if(inheritedFieldsSize > UA_DATATYPE_MEMBERS_MAX ||
       ownFieldsSize > UA_DATATYPE_MEMBERS_MAX - inheritedFieldsSize)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    size_t fieldsSize = inheritedFieldsSize + ownFieldsSize;

    UA_StructureField *fields =
        (UA_StructureField *)UA_calloc(fieldsSize, sizeof(UA_StructureField));
    if(fieldsSize > 0 && !fields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    bool hasOptionalFields = parentType && parentType->typeKind == UA_DATATYPEKIND_OPTSTRUCT;
    for(size_t i = 0; i < inheritedFieldsSize; i++) {
        const UA_DataTypeMember *src = &parentType->members[i];
#ifdef UA_ENABLE_TYPEDESCRIPTION
        fields[i].name = UA_STRING((char *)(uintptr_t)src->memberName);
#endif
        fields[i].dataType = src->memberType->typeId;
        fields[i].valueRank = src->isArray ? UA_VALUERANK_ONE_DIMENSION : UA_VALUERANK_SCALAR;
        fields[i].isOptional = src->isOptional;
        hasOptionalFields |= src->isOptional;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < ownFieldsSize; i++) {
        const NL_DataTypeDefinitionField *src = &node->definition->fields[i];
        UA_StructureField *dst = &fields[inheritedFieldsSize + i];
        dst->name = UA_STRING(src->name);
        dst->valueRank = src->valueRank >= 0 ? UA_VALUERANK_ONE_DIMENSION : UA_VALUERANK_SCALAR;
        dst->isOptional = src->isOptional;
        hasOptionalFields |= src->isOptional;

        if(UA_NodeId_equal(&src->dataType, &node->id)) {
            dst->dataType = node->id;
            continue;
        }
        const UA_DataType *memberType = findDataType(nodeset, &src->dataType);
        if(!memberType) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
        dst->dataType = memberType->typeId;
    }

    if(res == UA_STATUSCODE_GOOD) {
        UA_StructureDescription description;
        UA_StructureDescription_init(&description);
        description.dataTypeId = node->id;
        description.name = node->browseName;
        description.name.namespaceIndex = 0;
        UA_StructureDefinition *definition = &description.structureDefinition;
        definition->baseDataType = parent;
        definition->fields = fields;
        definition->fieldsSize = fieldsSize;

        bool isUnion = node->definition && node->definition->isUnion;
        if(!node->definition && parentType && parentType->typeKind == UA_DATATYPEKIND_UNION)
            isUnion = true;
        definition->structureType =
            isUnion ? UA_STRUCTURETYPE_UNION
                    : (hasOptionalFields ? UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS
                                         : UA_STRUCTURETYPE_STRUCTURE);

        const UA_NodeId *binaryEncodingId = getEncodingId(node, "Default Binary");
        if(binaryEncodingId)
            definition->defaultEncodingId = *binaryEncodingId;

        UA_ExtensionObject eo;
        UA_ExtensionObject_setValueNoDelete(&eo, &description,
                                            &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
        res = addTypeFromDescription(nodeset, node, parent, &eo);
    }

    UA_free(fields);
    return res;
}

static UA_StatusCode
addCustomDataType(NodeSet *nodeset, const NL_DataTypeNode *node) {
    UA_NodeId parent = getParentId((const NL_Node *)node, NULL);
    const UA_DataType *parentType = findDataType(nodeset, &parent);
    const NL_DataTypeDefinition *definition = node->definition;

    if(definition && definition->isOptionSet && parentType &&
       parentType->typeKind <= UA_DATATYPEKIND_DOUBLE)
        return addSimpleDataType(nodeset, node, parent, parentType);
    if(definition && definition->isOptionSet && parentType &&
       (parentType->typeKind == UA_DATATYPEKIND_STRUCTURE ||
        parentType->typeKind == UA_DATATYPEKIND_OPTSTRUCT)) {
        /* OptionSet fields describe bits, not in-memory structure members. */
        return addStructureDataType(nodeset, node, parent, parentType, false);
    }
    if(definition && (definition->isEnum || definition->isOptionSet))
        return addEnumDataType(nodeset, node, parent);
    if(definition && definition->fieldsSize > 0)
        return addStructureDataType(nodeset, node, parent, parentType, true);
    if(parentType && (parentType->typeKind == UA_DATATYPEKIND_STRUCTURE ||
                      parentType->typeKind == UA_DATATYPEKIND_OPTSTRUCT ||
                      parentType->typeKind == UA_DATATYPEKIND_UNION))
        return addStructureDataType(nodeset, node, parent, parentType, false);
    if(parentType && parentType->typeKind == UA_DATATYPEKIND_ENUM)
        return addEnumDataType(nodeset, node, parent);
    return addSimpleDataType(nodeset, node, parent, parentType);
}

static NL_DependencyState
dataTypeDependencyReady(NodeSet *nodeset, const UA_NodeId *typeId, const NL_DataTypeNode *self) {
    if(UA_NodeId_equal(typeId, &self->id) || findDataType(nodeset, typeId))
        return NL_DEPENDENCY_READY;

    NL_Node *dependency = UA_NodeSet_findNode(nodeset, typeId);
    if(!dependency || dependency->nodeClass != UA_NODECLASS_DATATYPE)
        return NL_DEPENDENCY_REJECT;
    const NL_DataTypeNode *dataType = (const NL_DataTypeNode *)dependency;
    if(dataType->registrationState == NL_DATATYPE_REGISTERED)
        return NL_DEPENDENCY_READY;
    if(dataType->registrationState == NL_DATATYPE_REGISTRATION_REJECTED)
        return NL_DEPENDENCY_REJECT;
    return NL_DEPENDENCY_WAIT;
}

static NL_DependencyState
dataTypeDependenciesReady(NodeSet *nodeset, const NL_DataTypeNode *node) {
    if(!nodeExists((const NL_Node *)node))
        return NL_DEPENDENCY_REJECT;

    UA_NodeId parent = getParentId((const NL_Node *)node, NULL);
    NL_DependencyState state = dataTypeDependencyReady(nodeset, &parent, node);
    if(state != NL_DEPENDENCY_READY)
        return state;

    const NL_DataTypeDefinition *definition = node->definition;
    if(!definition || definition->isEnum || definition->isOptionSet)
        return NL_DEPENDENCY_READY;
    for(size_t i = 0; i < definition->fieldsSize; i++) {
        state = dataTypeDependencyReady(nodeset, &definition->fields[i].dataType, node);
        if(state != NL_DEPENDENCY_READY)
            return state;
    }
    return NL_DEPENDENCY_READY;
}

static UA_StatusCode
registerDataTypes(NodeSet *nodeset) {
    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        if(node->nodeClass != UA_NODECLASS_DATATYPE)
            continue;
        NL_DataTypeNode *dataType = (NL_DataTypeNode *)node;
        if(UA_Server_findDataType(nodeset->server, &node->id))
            dataType->registrationState = NL_DATATYPE_REGISTERED;
    }

    bool progress;
    do {
        progress = false;
        for(NL_Node *node = nodeset->nodes; node; node = node->next) {
            if(node->nodeClass != UA_NODECLASS_DATATYPE)
                continue;
            NL_DataTypeNode *dataType = (NL_DataTypeNode *)node;
            if(dataType->registrationState != NL_DATATYPE_NOT_REGISTERED)
                continue;

            NL_DependencyState state = dataTypeDependenciesReady(nodeset, dataType);
            if(state == NL_DEPENDENCY_WAIT)
                continue;
            if(state == NL_DEPENDENCY_REJECT) {
                dataType->registrationState = NL_DATATYPE_REGISTRATION_REJECTED;
                UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                               "NodeSetLoader: Cannot resolve datatype dependencies of %N",
                               node->id);
                continue;
            }

            UA_StatusCode res = addCustomDataType(nodeset, dataType);
            if(res == UA_STATUSCODE_BADOUTOFMEMORY)
                return res;
            if(res == UA_STATUSCODE_GOOD || res == UA_STATUSCODE_BADNODEIDEXISTS) {
                dataType->registrationState = NL_DATATYPE_REGISTERED;
                progress = true;
            } else {
                dataType->registrationState = NL_DATATYPE_REGISTRATION_REJECTED;
                UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                               "NodeSetLoader: Cannot add datatype description for %N (%s)",
                               node->id, UA_StatusCode_name(res));
            }
        }
    } while(progress);

    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        if(node->nodeClass != UA_NODECLASS_DATATYPE)
            continue;
        NL_DataTypeNode *dataType = (NL_DataTypeNode *)node;
        if(dataType->registrationState != NL_DATATYPE_NOT_REGISTERED)
            continue;
        dataType->registrationState = NL_DATATYPE_REGISTRATION_REJECTED;
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Cannot resolve datatype dependencies of %N", node->id);
    }
    return UA_STATUSCODE_GOOD;
}

static NL_DependencyState
finishDependenciesReady(const NL_Node *node) {
    NL_DependencyState state = dependencyNodeState(node->insertionParent, true);
    if(state != NL_DEPENDENCY_READY)
        return state;

    NL_Reference *typeDef = node->typeDefinitionRef;
    if(typeDef && typeDef->targetPtr)
        return dependencyNodeState(typeDef->targetPtr, true);
    return NL_DEPENDENCY_READY;
}

static UA_StatusCode
finishPhase(NodeSet *nodeset, UA_NodeClass nodeClasses) {
    bool progress;
    do {
        progress = false;
        for(NL_Node *node = nodeset->nodes; node; node = node->next) {
            if(!(nodeClasses & node->nodeClass) || node->addState != NL_NODE_BEGUN)
                continue;
            NL_DependencyState state = finishDependenciesReady(node);
            if(state == NL_DEPENDENCY_WAIT)
                continue;
            if(state == NL_DEPENDENCY_REJECT) {
                discardBegunNode(nodeset, node);
                UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                               "NodeSetLoader: Cannot finish node %N because a dependency failed",
                               node->id);
                continue;
            }

            UA_StatusCode res = finishNode(nodeset, node);
            if(res == UA_STATUSCODE_BADOUTOFMEMORY)
                return res;
            progress = true;
        }
    } while(progress);

    for(NL_Node *node = nodeset->nodes; node; node = node->next) {
        if(!(nodeClasses & node->nodeClass) || node->addState != NL_NODE_BEGUN)
            continue;
        discardBegunNode(nodeset, node);
        UA_LOG_WARNING(nodeset->logger, UA_LOGCATEGORY_SERVER,
                       "NodeSetLoader: Cannot resolve finish dependencies of node %N", node->id);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_NodeSet_apply(NodeSet *nodeset) {
    if(!nodeset)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    detectExistingNodes(nodeset);
    UA_NodeSet_resolveReferences(nodeset);

    /* Complete ReferenceTypes first. Their subtype indices are required to
     * recognize custom hierarchical references as insertion parents. */
    UA_StatusCode res = beginPhase(nodeset, UA_NODECLASS_REFERENCETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = finishPhase(nodeset, UA_NODECLASS_REFERENCETYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = UA_NodeSet_updateParentRefTypes(nodeset);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    UA_NodeSet_resolveReferences(nodeset);

    res = beginPhase(nodeset, UA_NODECLASS_DATATYPE);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = registerDataTypes(nodeset);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_NodeClass typeClasses = (UA_NodeClass)(UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    res = beginPhase(nodeset, typeClasses);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_NodeClass instanceClasses = (UA_NodeClass)(UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE |
                                                  UA_NODECLASS_METHOD | UA_NODECLASS_VIEW);
    res = beginPhase(nodeset, instanceClasses);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    for(NL_Node *node = nodeset->nodes; node; node = node->next)
        addAllReferences(nodeset, node);

    /* All explicit children now exist. Finish types first so their mandatory
     * children are available before instances are completed. */
    typeClasses =
        (UA_NodeClass)(UA_NODECLASS_DATATYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE);
    res = finishPhase(nodeset, typeClasses);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    res = finishPhase(nodeset, instanceClasses);

cleanup:
    if(res != UA_STATUSCODE_GOOD)
        discardBegunNodes(nodeset);
    return res;
}
