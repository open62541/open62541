#include "ua_nodes.h"
#include "ua_util.h"

/* UA_Node */
static void UA_Node_deleteMembers(UA_Node *p) {
	UA_NodeId_deleteMembers(&p->nodeId);
	UA_QualifiedName_deleteMembers(&p->browseName);
	UA_LocalizedText_deleteMembers(&p->displayName);
	UA_LocalizedText_deleteMembers(&p->description);
	UA_Array_delete(p->references, p->referencesSize, &UA_TYPES[UA_TYPES_REFERENCENODE]);
}

static UA_StatusCode UA_Node_copy(const UA_Node *src, UA_Node *dst) {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	retval |= UA_NodeId_copy(&src->nodeId, &dst->nodeId);
	dst->nodeClass = src->nodeClass;
	retval |= UA_QualifiedName_copy(&src->browseName, &dst->browseName);
	retval |= UA_LocalizedText_copy(&src->displayName, &dst->displayName);
	retval |= UA_LocalizedText_copy(&src->description, &dst->description);
	dst->writeMask = src->writeMask;
	dst->userWriteMask = src->userWriteMask;
	if(retval) {
    	UA_Node_deleteMembers(dst);
        return retval;
    }
	retval |= UA_Array_copy(src->references, src->referencesSize, (void**)&dst->references,
                            &UA_TYPES[UA_TYPES_REFERENCENODE]);
	if(retval == UA_STATUSCODE_GOOD)
        dst->referencesSize = src->referencesSize;
	return retval;
}

void UA_Node_deleteAnyNodeClass(UA_Node *node) {
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        UA_ObjectNode_delete((UA_ObjectNode*)node);
        break;
    case UA_NODECLASS_VARIABLE:
        UA_VariableNode_delete((UA_VariableNode*)node);
        break;
    case UA_NODECLASS_METHOD:
        UA_MethodNode_delete((UA_MethodNode*)node);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        UA_ObjectTypeNode_delete((UA_ObjectTypeNode*)node);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        UA_VariableTypeNode_delete((UA_VariableTypeNode*)node);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode*)node);
        break;
    case UA_NODECLASS_DATATYPE:
        UA_DataTypeNode_delete((UA_DataTypeNode*)node);
        break;
    case UA_NODECLASS_VIEW:
        UA_ViewNode_delete((UA_ViewNode*)node);
        break;
    default:
        break;
    }
}

typedef UA_Node *(*UA_NewNodeFunction)(void);
typedef UA_StatusCode (*UA_CopyNodeFunction)(const UA_Node *src, UA_Node *dst);
typedef void (*UA_DeleteNodeFunction)(UA_Node *p);

UA_Node * UA_Node_copyAnyNodeClass(const UA_Node *node) {
    UA_NewNodeFunction newNode;
    UA_CopyNodeFunction copyNode;
    UA_DeleteNodeFunction deleteNode;

    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        newNode = (UA_NewNodeFunction)UA_ObjectNode_new;
        copyNode = (UA_CopyNodeFunction)UA_ObjectNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_ObjectNode_delete;
        break;
    case UA_NODECLASS_VARIABLE:
        newNode = (UA_NewNodeFunction)UA_VariableNode_new;
        copyNode = (UA_CopyNodeFunction)UA_VariableNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_VariableNode_delete;
        break;
    case UA_NODECLASS_METHOD:
        newNode = (UA_NewNodeFunction)UA_MethodNode_new;
        copyNode = (UA_CopyNodeFunction)UA_MethodNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_MethodNode_delete;
        break;
    case UA_NODECLASS_OBJECTTYPE:
        newNode = (UA_NewNodeFunction)UA_ObjectTypeNode_new;
        copyNode = (UA_CopyNodeFunction)UA_ObjectTypeNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_ObjectTypeNode_delete;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        newNode = (UA_NewNodeFunction)UA_VariableTypeNode_new;
        copyNode = (UA_CopyNodeFunction)UA_VariableTypeNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_VariableTypeNode_delete;
        break;
    case UA_NODECLASS_REFERENCETYPE:
        newNode = (UA_NewNodeFunction)UA_ReferenceTypeNode_new;
        copyNode = (UA_CopyNodeFunction)UA_ReferenceTypeNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_ReferenceTypeNode_delete;
        break;
    case UA_NODECLASS_DATATYPE:
        newNode = (UA_NewNodeFunction)UA_DataTypeNode_new;
        copyNode = (UA_CopyNodeFunction)UA_DataTypeNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_DataTypeNode_delete;
        break;
    case UA_NODECLASS_VIEW:
        newNode = (UA_NewNodeFunction)UA_ViewNode_new;
        copyNode = (UA_CopyNodeFunction)UA_ViewNode_copy;
        deleteNode = (UA_DeleteNodeFunction)UA_ViewNode_delete;
        break;
    default:
        return NULL;
        break;
    }

    UA_Node *copy = newNode();
    if(!copy)
        return NULL;
    if(copyNode(node, copy) != UA_STATUSCODE_GOOD) {
        deleteNode(copy);
        return NULL;
    }
    return copy;
}

/* UA_ObjectNode */
void UA_ObjectNode_init(UA_ObjectNode *p) {
    memset(p, 0, sizeof(UA_ObjectNode));
    p->nodeClass = UA_NODECLASS_OBJECT;
}

UA_ObjectNode * UA_ObjectNode_new(void) {
    UA_ObjectNode *p = (UA_ObjectNode*)UA_malloc(sizeof(UA_ObjectNode));
    if(p)
        UA_ObjectNode_init(p);
    return p;
}

void UA_ObjectNode_deleteMembers(UA_ObjectNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
}

void UA_ObjectNode_delete(UA_ObjectNode *p) {
    UA_ObjectNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_ObjectNode_copy(const UA_ObjectNode *src, UA_ObjectNode *dst) {
    dst->eventNotifier = src->eventNotifier;
    dst->instanceHandle = src->instanceHandle;
	return UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
}

/* UA_ObjectTypeNode */
void UA_ObjectTypeNode_init(UA_ObjectTypeNode *p) {
    memset(p, 0, sizeof(UA_ObjectTypeNode));
    p->nodeClass = UA_NODECLASS_OBJECTTYPE;
}

UA_ObjectTypeNode * UA_ObjectTypeNode_new(void) {
    UA_ObjectTypeNode *p = (UA_ObjectTypeNode*)UA_malloc(sizeof(UA_ObjectTypeNode));
    if(p)
        UA_ObjectTypeNode_init(p);
    return p;
}

void UA_ObjectTypeNode_deleteMembers(UA_ObjectTypeNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
}

void UA_ObjectTypeNode_delete(UA_ObjectTypeNode *p) {
    UA_ObjectTypeNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_ObjectTypeNode_copy(const UA_ObjectTypeNode *src, UA_ObjectTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    dst->lifecycleManagement = src->lifecycleManagement;
	return UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
}

/* UA_VariableNode */
void UA_VariableNode_init(UA_VariableNode *p) {
    memset(p, 0, sizeof(UA_VariableNode));
    p->nodeClass = UA_NODECLASS_VARIABLE;
    p->valueRank = -2; // scalar or array of any dimension
    p->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    p->userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
}

UA_VariableNode * UA_VariableNode_new(void) {
    UA_VariableNode *p = (UA_VariableNode*)UA_malloc(sizeof(UA_VariableNode));
    if(p)
        UA_VariableNode_init(p);
    return p;
}

void UA_VariableNode_deleteMembers(UA_VariableNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
    if(p->valueSource == UA_VALUESOURCE_VARIANT)
        UA_Variant_deleteMembers(&p->value.variant.value);
}

void UA_VariableNode_delete(UA_VariableNode *p) {
    UA_VariableNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_VariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
	UA_StatusCode retval = UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_VARIANT) {
        retval = UA_Variant_copy(&src->value.variant.value, &dst->value.variant.value);
        dst->value.variant.callback = src->value.variant.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    if(retval) {
        UA_VariableNode_deleteMembers(dst);
        return retval;
    }
    dst->accessLevel = src->accessLevel;
    dst->userAccessLevel = src->accessLevel;
    dst->minimumSamplingInterval = src->minimumSamplingInterval;
    dst->historizing = src->historizing;
    return UA_STATUSCODE_GOOD;
}

/* UA_VariableTypeNode */
void UA_VariableTypeNode_init(UA_VariableTypeNode *p) {
    memset(p, 0, sizeof(UA_VariableTypeNode));
    p->nodeClass = UA_NODECLASS_VARIABLETYPE;
    p->valueRank = -2; // scalar or array of any dimension
}

UA_VariableTypeNode * UA_VariableTypeNode_new(void) {
    UA_VariableTypeNode *p = (UA_VariableTypeNode*)UA_malloc(sizeof(UA_VariableTypeNode));
    if(p)
        UA_VariableTypeNode_init(p);
    return p;
}

void UA_VariableTypeNode_deleteMembers(UA_VariableTypeNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
    if(p->valueSource == UA_VALUESOURCE_VARIANT)
        UA_Variant_deleteMembers(&p->value.variant.value);
}

void UA_VariableTypeNode_delete(UA_VariableTypeNode *p) {
    UA_VariableTypeNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_VariableTypeNode_copy(const UA_VariableTypeNode *src, UA_VariableTypeNode *dst) {
	UA_StatusCode retval = UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_VARIANT){
        UA_Variant_copy(&src->value.variant.value, &dst->value.variant.value);
        dst->value.variant.callback = src->value.variant.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    if(retval) {
        UA_VariableTypeNode_deleteMembers(dst);
        return retval;
    }
    dst->isAbstract = src->isAbstract;
    return UA_STATUSCODE_GOOD;
}

/* UA_ReferenceTypeNode */
void UA_ReferenceTypeNode_init(UA_ReferenceTypeNode *p) {
    memset(p, 0, sizeof(UA_ReferenceTypeNode));
    p->nodeClass = UA_NODECLASS_REFERENCETYPE;
}

UA_ReferenceTypeNode * UA_ReferenceTypeNode_new(void) {
    UA_ReferenceTypeNode *p = (UA_ReferenceTypeNode*)UA_malloc(sizeof(UA_ReferenceTypeNode));
    if(p)
        UA_ReferenceTypeNode_init(p);
    return p;
}

void UA_ReferenceTypeNode_deleteMembers(UA_ReferenceTypeNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
    UA_LocalizedText_deleteMembers(&p->inverseName);
}

void UA_ReferenceTypeNode_delete(UA_ReferenceTypeNode *p) {
    UA_ReferenceTypeNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_ReferenceTypeNode_copy(const UA_ReferenceTypeNode *src, UA_ReferenceTypeNode *dst) {
    UA_StatusCode retval = UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
    if(retval)
        return retval;
    retval = UA_LocalizedText_copy(&src->inverseName, &dst->inverseName);
    if(retval) {
        UA_ReferenceTypeNode_deleteMembers(dst);
        return retval;
    }
    dst->isAbstract = src->isAbstract;
    dst->symmetric = src->symmetric;
    return UA_STATUSCODE_GOOD;
}

/* UA_MethodNode */
void UA_MethodNode_init(UA_MethodNode *p) {
    memset(p, 0, sizeof(UA_MethodNode));
    p->nodeClass = UA_NODECLASS_METHOD;
}

UA_MethodNode * UA_MethodNode_new(void) {
    UA_MethodNode *p = (UA_MethodNode*)UA_malloc(sizeof(UA_MethodNode));
    if(p)
        UA_MethodNode_init(p);
    return p;
}

void UA_MethodNode_deleteMembers(UA_MethodNode *p) {
#ifdef ENABLE_METHODCALLS
    p->attachedMethod = NULL;
#endif
    UA_Node_deleteMembers((UA_Node*)p);
}

void UA_MethodNode_delete(UA_MethodNode *p) {
    UA_MethodNode_deleteMembers(p);
#ifdef ENABLE_METHODCALLS
    p->methodHandle   = NULL;
    p->attachedMethod = NULL;
#endif
    UA_free(p);
}

UA_StatusCode UA_MethodNode_copy(const UA_MethodNode *src, UA_MethodNode *dst) {
    UA_StatusCode retval = UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    dst->executable = src->executable;
    dst->userExecutable = src->userExecutable;
#ifdef ENABLE_METHODCALLS
    dst->methodHandle  = src->methodHandle;
    dst->attachedMethod = src->attachedMethod;
#endif
    return retval;
}

/* UA_ViewNode */
void UA_ViewNode_init(UA_ViewNode *p) {
    memset(p, 0, sizeof(UA_ViewNode));
    p->nodeClass = UA_NODECLASS_VIEW;
}

UA_ViewNode * UA_ViewNode_new(void) {
    UA_ViewNode *p = UA_malloc(sizeof(UA_ViewNode));
    if(p)
        UA_ViewNode_init(p);
    return p;
}

void UA_ViewNode_deleteMembers(UA_ViewNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
}

void UA_ViewNode_delete(UA_ViewNode *p) {
    UA_ViewNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_ViewNode_copy(const UA_ViewNode *src, UA_ViewNode *dst) {
    dst->containsNoLoops = src->containsNoLoops;
    dst->eventNotifier = src->eventNotifier;
	return UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
}

/* UA_DataTypeNode */
void UA_DataTypeNode_init(UA_DataTypeNode *p) {
    memset(p, 0, sizeof(UA_DataTypeNode));
    p->nodeClass = UA_NODECLASS_DATATYPE;
}

UA_DataTypeNode * UA_DataTypeNode_new(void) {
    UA_DataTypeNode *p = UA_malloc(sizeof(UA_DataTypeNode));
    if(p)
        UA_DataTypeNode_init(p);
    return p;
}

void UA_DataTypeNode_deleteMembers(UA_DataTypeNode *p) {
    UA_Node_deleteMembers((UA_Node*)p);
}

void UA_DataTypeNode_delete(UA_DataTypeNode *p) {
    UA_DataTypeNode_deleteMembers(p);
    UA_free(p);
}

UA_StatusCode UA_DataTypeNode_copy(const UA_DataTypeNode *src, UA_DataTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
	return UA_Node_copy((const UA_Node*)src, (UA_Node*)dst);
}
