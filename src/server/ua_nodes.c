/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_nodes.h"

void UA_Node_deleteReferences(UA_Node *node) {
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        for(size_t j = 0; j < refs->targetIdsSize; ++j)
            UA_ExpandedNodeId_deleteMembers(&refs->targetIds[j]);
        UA_free(refs->targetIds);
        UA_NodeId_deleteMembers(&refs->referenceTypeId);
    }
    if(node->references)
        UA_free(node->references);
    node->references = NULL;
    node->referencesSize = 0;
}

void UA_Node_deleteMembersAnyNodeClass(UA_Node *node) {
    /* Delete standard content */
    UA_NodeId_deleteMembers(&node->nodeId);
    UA_QualifiedName_deleteMembers(&node->browseName);
    UA_LocalizedText_deleteMembers(&node->displayName);
    UA_LocalizedText_deleteMembers(&node->description);

    /* Delete references */
    UA_Node_deleteReferences(node);

    /* Delete unique content of the nodeclass */
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        break;
    case UA_NODECLASS_METHOD:
        break;
    case UA_NODECLASS_OBJECTTYPE:
        break;
    case UA_NODECLASS_VARIABLE:
    case UA_NODECLASS_VARIABLETYPE: {
        UA_VariableNode *p = (UA_VariableNode*)node;
        UA_NodeId_deleteMembers(&p->dataType);
        UA_Array_delete(p->arrayDimensions, p->arrayDimensionsSize,
                        &UA_TYPES[UA_TYPES_INT32]);
        p->arrayDimensions = NULL;
        p->arrayDimensionsSize = 0;
        if(p->valueSource == UA_VALUESOURCE_DATA)
            UA_DataValue_deleteMembers(&p->value.data.value);
        break;
    }
    case UA_NODECLASS_REFERENCETYPE: {
        UA_ReferenceTypeNode *p = (UA_ReferenceTypeNode*)node;
        UA_LocalizedText_deleteMembers(&p->inverseName);
        break;
    }
    case UA_NODECLASS_DATATYPE:
        break;
    case UA_NODECLASS_VIEW:
        break;
    default:
        break;
    }
}

static UA_StatusCode
UA_ObjectNode_copy(const UA_ObjectNode *src, UA_ObjectNode *dst) {
    dst->eventNotifier = src->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_CommonVariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
    UA_StatusCode retval = UA_Array_copy(src->arrayDimensions,
                                         src->arrayDimensionsSize,
                                         (void**)&dst->arrayDimensions,
                                         &UA_TYPES[UA_TYPES_INT32]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    dst->arrayDimensionsSize = src->arrayDimensionsSize;
    retval = UA_NodeId_copy(&src->dataType, &dst->dataType);
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_DATA) {
        retval |= UA_DataValue_copy(&src->value.data.value,
                                    &dst->value.data.value);
        dst->value.data.callback = src->value.data.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    return retval;
}

static UA_StatusCode
UA_VariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
    UA_StatusCode retval = UA_CommonVariableNode_copy(src, dst);
    dst->accessLevel = src->accessLevel;
    dst->minimumSamplingInterval = src->minimumSamplingInterval;
    dst->historizing = src->historizing;
    return retval;
}

static UA_StatusCode
UA_VariableTypeNode_copy(const UA_VariableTypeNode *src,
                         UA_VariableTypeNode *dst) {
    UA_StatusCode retval = UA_CommonVariableNode_copy((const UA_VariableNode*)src,
                                                      (UA_VariableNode*)dst);
    dst->isAbstract = src->isAbstract;
    return retval;
}

static UA_StatusCode
UA_MethodNode_copy(const UA_MethodNode *src, UA_MethodNode *dst) {
    dst->executable = src->executable;
    dst->method = src->method;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ObjectTypeNode_copy(const UA_ObjectTypeNode *src, UA_ObjectTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    dst->lifecycle = src->lifecycle;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReferenceTypeNode_copy(const UA_ReferenceTypeNode *src,
                          UA_ReferenceTypeNode *dst) {
    UA_StatusCode retval = UA_LocalizedText_copy(&src->inverseName,
                                                 &dst->inverseName);
    dst->isAbstract = src->isAbstract;
    dst->symmetric = src->symmetric;
    return retval;
}

static UA_StatusCode
UA_DataTypeNode_copy(const UA_DataTypeNode *src, UA_DataTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ViewNode_copy(const UA_ViewNode *src, UA_ViewNode *dst) {
    dst->containsNoLoops = src->containsNoLoops;
    dst->eventNotifier = src->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Node_copyAnyNodeClass(const UA_Node *src, UA_Node *dst) {
    if(src->nodeClass != dst->nodeClass)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    /* Copy standard content */
    UA_StatusCode retval = UA_NodeId_copy(&src->nodeId, &dst->nodeId);
    dst->nodeClass = src->nodeClass;
    retval |= UA_QualifiedName_copy(&src->browseName, &dst->browseName);
    retval |= UA_LocalizedText_copy(&src->displayName, &dst->displayName);
    retval |= UA_LocalizedText_copy(&src->description, &dst->description);
    dst->writeMask = src->writeMask;
    dst->context = src->context;
    dst->lifecycleState = src->lifecycleState;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Node_deleteMembersAnyNodeClass(dst);
        return retval;
    }

    /* Copy the references */
    dst->references = NULL;
    if(src->referencesSize > 0) {
        dst->references =
            (UA_NodeReferenceKind*)UA_calloc(src->referencesSize,
                                             sizeof(UA_NodeReferenceKind));
        if(!dst->references) {
            UA_Node_deleteMembersAnyNodeClass(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->referencesSize = src->referencesSize;

        for(size_t i = 0; i < src->referencesSize; ++i) {
            UA_NodeReferenceKind *srefs = &src->references[i];
            UA_NodeReferenceKind *drefs = &dst->references[i];
            drefs->isInverse = srefs->isInverse;
            retval = UA_NodeId_copy(&srefs->referenceTypeId, &drefs->referenceTypeId);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            retval = UA_Array_copy(srefs->targetIds, srefs->targetIdsSize,
                                    (void**)&drefs->targetIds,
                                    &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            drefs->targetIdsSize = srefs->targetIdsSize;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Node_deleteMembersAnyNodeClass(dst);
            return retval;
        }
    }

    /* Copy unique content of the nodeclass */
    switch(src->nodeClass) {
    case UA_NODECLASS_OBJECT:
        retval = UA_ObjectNode_copy((const UA_ObjectNode*)src, (UA_ObjectNode*)dst);
        break;
    case UA_NODECLASS_VARIABLE:
        retval = UA_VariableNode_copy((const UA_VariableNode*)src, (UA_VariableNode*)dst);
        break;
    case UA_NODECLASS_METHOD:
        retval = UA_MethodNode_copy((const UA_MethodNode*)src, (UA_MethodNode*)dst);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        retval = UA_ObjectTypeNode_copy((const UA_ObjectTypeNode*)src, (UA_ObjectTypeNode*)dst);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        retval = UA_VariableTypeNode_copy((const UA_VariableTypeNode*)src, (UA_VariableTypeNode*)dst);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        retval = UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode*)src, (UA_ReferenceTypeNode*)dst);
        break;
    case UA_NODECLASS_DATATYPE:
        retval = UA_DataTypeNode_copy((const UA_DataTypeNode*)src, (UA_DataTypeNode*)dst);
        break;
    case UA_NODECLASS_VIEW:
        retval = UA_ViewNode_copy((const UA_ViewNode*)src, (UA_ViewNode*)dst);
        break;
    default:
        break;
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Node_deleteMembersAnyNodeClass(dst);

    return retval;
}
