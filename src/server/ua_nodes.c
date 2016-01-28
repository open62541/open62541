#include "ua_nodes.h"
#include "ua_util.h"

void UA_Node_deleteMembersAnyNodeClass(UA_Node *node) {
    /* delete standard content */
    UA_NodeId_deleteMembers(&node->nodeId);
    UA_QualifiedName_deleteMembers(&node->browseName);
    UA_LocalizedText_deleteMembers(&node->displayName);
    UA_LocalizedText_deleteMembers(&node->description);
    UA_Array_delete(node->references, node->referencesSize, &UA_TYPES[UA_TYPES_REFERENCENODE]);
    node->references = NULL;
    node->referencesSize = 0;

    /* delete unique content of the nodeclass */
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
        if(p->valueSource == UA_VALUESOURCE_VARIANT)
            UA_Variant_deleteMembers(&p->value.variant.value);
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
    dst->instanceHandle = src->instanceHandle;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_VariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_VARIANT) {
        UA_StatusCode retval = UA_Variant_copy(&src->value.variant.value, &dst->value.variant.value);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        dst->value.variant.callback = src->value.variant.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    dst->accessLevel = src->accessLevel;
    dst->userAccessLevel = src->accessLevel;
    dst->minimumSamplingInterval = src->minimumSamplingInterval;
    dst->historizing = src->historizing;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_MethodNode_copy(const UA_MethodNode *src, UA_MethodNode *dst) {
    dst->executable = src->executable;
    dst->userExecutable = src->userExecutable;
    dst->methodHandle  = src->methodHandle;
    dst->attachedMethod = src->attachedMethod;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ObjectTypeNode_copy(const UA_ObjectTypeNode *src, UA_ObjectTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    dst->lifecycleManagement = src->lifecycleManagement;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_VariableTypeNode_copy(const UA_VariableTypeNode *src, UA_VariableTypeNode *dst) {
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_VARIANT){
        UA_StatusCode retval = UA_Variant_copy(&src->value.variant.value, &dst->value.variant.value);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        dst->value.variant.callback = src->value.variant.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    dst->isAbstract = src->isAbstract;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReferenceTypeNode_copy(const UA_ReferenceTypeNode *src, UA_ReferenceTypeNode *dst) {
    UA_StatusCode retval = UA_LocalizedText_copy(&src->inverseName, &dst->inverseName);
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
    
    /* copy standard content */
	UA_StatusCode retval = UA_NodeId_copy(&src->nodeId, &dst->nodeId);
	dst->nodeClass = src->nodeClass;
	retval |= UA_QualifiedName_copy(&src->browseName, &dst->browseName);
	retval |= UA_LocalizedText_copy(&src->displayName, &dst->displayName);
	retval |= UA_LocalizedText_copy(&src->description, &dst->description);
	dst->writeMask = src->writeMask;
	dst->userWriteMask = src->userWriteMask;
	if(retval != UA_STATUSCODE_GOOD) {
    	UA_Node_deleteMembersAnyNodeClass(dst);
        return retval;
    }
	retval |= UA_Array_copy(src->references, src->referencesSize, (void**)&dst->references,
                            &UA_TYPES[UA_TYPES_REFERENCENODE]);
	if(retval != UA_STATUSCODE_GOOD) {
    	UA_Node_deleteMembersAnyNodeClass(dst);
        return retval;
    }
    dst->referencesSize = src->referencesSize;

    /* copy unique content of the nodeclass */
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
