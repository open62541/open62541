#include "adhoc_nodestore.h"
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/server.h>
#include <open62541/types.h>

#ifndef container_of
#define container_of(ptr, type, member) \
    (type *)((uintptr_t)ptr - offsetof(type,member))
#endif

/*****************/
/* Backend Nodes */
/*****************/

typedef struct {
    UA_UInt32 nodeId;              /* Numeric NodeId */
    UA_UInt16 namespaceIndex;      /* Namespace index */
    char *name;                    /* Node name */
    char *browseName;              /* Browse name */
    UA_NodeClass nodeClass;        /* UA NodeClass (Object, Variable, Method, etc.) */
    UA_UInt32 parentNodeId;        /* Parent node ID (0 for root-level nodes) */
    UA_UInt16 parentNamespaceIndex; /* Parent namespace index */
    UA_UInt32 referenceTypeId;     /* Reference type to parent (e.g., HasComponent, Organizes) */
    char *guid;                    /* GUID as string */
    char *value;                   /* String representation of value (for variables) */
} MockBackendNode;

/* Static mock backend data - represents 20 hierarchical nodes */
static const MockBackendNode mockBackendNodes[] = {
    /* Root folder under Objects */
    {1000, 1, "PilzDeviceRoot", "PilzDeviceRoot", UA_NODECLASS_OBJECT, 
     85, 0, 35, "550e8400-e29b-41d4-a716-446655440000", NULL},
    
    /* Device Information folder */
    {1001, 1, "DeviceInfo", "DeviceInfo", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440001", NULL},
    
    /* Device variables under DeviceInfo */
    {1002, 1, "DeviceName", "DeviceName", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440002", "Pilz Safety Controller"},
    
    {1003, 1, "SerialNumber", "SerialNumber", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440003", "PSC-12345-67890"},
    
    {1004, 1, "FirmwareVersion", "FirmwareVersion", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440004", "v2.1.5"},
    
    {1005, 1, "ManufacturerName", "ManufacturerName", UA_NODECLASS_VARIABLE, 
     1001, 1, 47, "550e8400-e29b-41d4-a716-446655440005", "Pilz GmbH & Co. KG"},
    
    /* Safety System folder */
    {1010, 1, "SafetySystem", "SafetySystem", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440010", NULL},
    
    /* Safety variables */
    {1011, 1, "SafetyState", "SafetyState", UA_NODECLASS_VARIABLE, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440011", "SAFE"},
    
    {1012, 1, "EmergencyStopActive", "EmergencyStopActive", UA_NODECLASS_VARIABLE, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440012", "false"},
    
    {1013, 1, "SafetyOutputs", "SafetyOutputs", UA_NODECLASS_OBJECT, 
     1010, 1, 47, "550e8400-e29b-41d4-a716-446655440013", NULL},
    
    /* Individual safety outputs */
    {1014, 1, "Output1", "Output1", UA_NODECLASS_VARIABLE, 
     1013, 1, 47, "550e8400-e29b-41d4-a716-446655440014", "true"},
    
    {1015, 1, "Output2", "Output2", UA_NODECLASS_VARIABLE, 
     1013, 1, 47, "550e8400-e29b-41d4-a716-446655440015", "false"},
    
    /* Process Data folder */
    {1020, 1, "ProcessData", "ProcessData", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440020", NULL},
    
    /* Process variables */
    {1021, 1, "Temperature", "Temperature", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440021", "23.5"},
    
    {1022, 1, "Pressure", "Pressure", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440022", "1.2"},
    
    {1023, 1, "FlowRate", "FlowRate", UA_NODECLASS_VARIABLE, 
     1020, 1, 47, "550e8400-e29b-41d4-a716-446655440023", "15.7"},
    
    /* Diagnostics folder */
    {1030, 1, "Diagnostics", "Diagnostics", UA_NODECLASS_OBJECT, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440030", NULL},
    
    /* Diagnostic variables */
    {1031, 1, "SystemHealth", "SystemHealth", UA_NODECLASS_VARIABLE, 
     1030, 1, 47, "550e8400-e29b-41d4-a716-446655440031", "OK"},
    
    {1032, 1, "ErrorCount", "ErrorCount", UA_NODECLASS_VARIABLE, 
     1030, 1, 47, "550e8400-e29b-41d4-a716-446655440032", "0"},
    
    /* Control methods */
    {1040, 1, "ResetSystem", "ResetSystem", UA_NODECLASS_METHOD, 
     1000, 1, 47, "550e8400-e29b-41d4-a716-446655440040", NULL}
};

#define MOCK_BACKEND_NODE_COUNT (sizeof(mockBackendNodes) / sizeof(MockBackendNode))

/* Helper function to find a mock backend node by NodeId */
static const MockBackendNode* findMockBackendNode(const UA_NodeId *nodeId) {
    if(nodeId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return NULL;
    UA_UInt32 id = nodeId->identifier.numeric;
    UA_UInt16 namespaceIndex = nodeId->namespaceIndex;
    for(size_t i = 0; i < MOCK_BACKEND_NODE_COUNT; i++) {
        if(mockBackendNodes[i].nodeId == id && 
           mockBackendNodes[i].namespaceIndex == namespaceIndex) {
            return &mockBackendNodes[i];
        }
    }
    return NULL;
}

static void
setAsyncReadResult(UA_Server *server, void *data) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND, "read result");
    UA_DataValue *value= (UA_DataValue*)data;
    UA_Server_setAsyncReadResult(server, value);
}

static UA_StatusCode
delayedValueRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                 UA_DataValue *value) {
    /* Get the mockup node */
    const MockBackendNode *mockNode = findMockBackendNode(nodeId);
    if(!mockNode)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set the value immediately in the output */
    UA_String v = UA_STRING(mockNode->value);
    UA_Variant_setScalarCopy(&value->value, &v, &UA_TYPES[UA_TYPES_STRING]);
    value->hasValue = true;

    /* Return the output with a five second delay */
    UA_DateTime callTime = UA_DateTime_nowMonotonic() + (2 * UA_DATETIME_SEC);
    UA_Server_addTimedCallback(server, setAsyncReadResult, value, callTime, NULL);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
delayedValueWrite(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                  const UA_NumericRange *range, const UA_DataValue *value) {
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

/**********************/
/* Fallback Nodestore */
/**********************/

typedef struct {
    UA_Nodestore ns;
    UA_Nodestore *fallback;
} AdHocNodestore;

/* Create a UA_Node from mock backend data - simplified version */
static UA_Node *
createNodeFromMockData(const MockBackendNode* mockNode) {
    UA_Node *node = (UA_Node*)UA_calloc(1, sizeof(UA_Node));
    if(!node)
        return NULL;

    node->head.nodeClass = mockNode->nodeClass;
    
    /* Set NodeId */
    node->head.nodeId.namespaceIndex = mockNode->namespaceIndex;
    node->head.nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    node->head.nodeId.identifier.numeric = mockNode->nodeId;
    
    /* Set BrowseName */
    node->head.browseName.namespaceIndex = mockNode->namespaceIndex;
    node->head.browseName.name = UA_STRING_ALLOC(mockNode->browseName);
    
    /* Set WriteMask */
    node->head.writeMask = 0;
    
    /* Set node-specific attributes */
    switch(node->head.nodeClass) {
        case UA_NODECLASS_OBJECT: {
            UA_ObjectNode *objNode = &node->objectNode;
            objNode->eventNotifier = 0;
            break;
        }
        case UA_NODECLASS_VARIABLE: {
            UA_VariableNode *varNode = &node->variableNode;
            varNode->accessLevel = UA_ACCESSLEVELMASK_READ;
            varNode->minimumSamplingInterval = 0.0;
            varNode->historizing = false;
            
            /* Set DataType to String */
            varNode->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_STRING);
            varNode->valueRank = UA_VALUERANK_SCALAR;

            /* Set the value to the async callback */
            varNode->valueSourceType = UA_VALUESOURCETYPE_CALLBACK;
            varNode->valueSource.callback.read = delayedValueRead;
            varNode->valueSource.callback.write = delayedValueWrite;
            break;
        }
        case UA_NODECLASS_METHOD: {
            UA_MethodNode *methodNode = &node->methodNode;
            methodNode->executable = true;
            break;
        }
        default:
            UA_assert(false);
            break;
    }

    return node;
}

/***********************/
/* Interface functions */
/***********************/

/* Always create new nodes from the fallback. The backend nodes are already there. */
static UA_Node *
AdHocNodestore_newNode(UA_Nodestore *orig_ns, UA_NodeClass nodeClass) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->newNode(ns->fallback, nodeClass);
}

static void
AdHocNodestore_deleteNode(UA_Nodestore *orig_ns, UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!findMockBackendNode(&node->head.nodeId))
        ns->fallback->deleteNode(ns->fallback, node);
}

static const UA_Node *
AdHocNodestore_getNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid,
                       UA_UInt32 attributeMask, UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    const MockBackendNode *mbd = findMockBackendNode(nodeid);
    if(!mbd)
        return ns->fallback->getNode(ns->fallback, nodeid, attributeMask,
                                     references, referenceDirections);
    return createNodeFromMockData(mbd);
}

static const UA_Node *
AdHocNodestore_getNodeFromPtr(UA_Nodestore *ns, UA_NodePointer ptr,
                              UA_UInt32 attributeMask,
                              UA_ReferenceTypeSet references,
                              UA_BrowseDirection referenceDirections) {
    if(!UA_NodePointer_isLocal(ptr))
        return NULL;
    UA_NodeId id = UA_NodePointer_toNodeId(ptr);
    return AdHocNodestore_getNode(ns, &id, attributeMask, references, referenceDirections);
}

static void
AdHocNodestore_releaseNode(UA_Nodestore *orig_ns, const UA_Node *node) {
    if(!node)
        return;

    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!findMockBackendNode(&node->head.nodeId)) {
        ns->fallback->releaseNode(ns->fallback, node);
        return;
    }

    UA_Node *mutNode = (UA_Node*)(uintptr_t)node;

    UA_Node_clear(mutNode);
    UA_free(mutNode);
}

static UA_StatusCode
AdHocNodestore_getNodeCopy(UA_Nodestore *orig_ns, const UA_NodeId *nodeId,
                           UA_Node **outNode) {
    if(findMockBackendNode(nodeId))
        return UA_STATUSCODE_BADNOTWRITABLE;
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->getNodeCopy(ns->fallback, nodeId, outNode);
}

static UA_StatusCode
AdHocNodestore_removeNode(UA_Nodestore *orig_ns, const UA_NodeId *nodeid) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!findMockBackendNode(nodeid))
        return UA_STATUSCODE_GOOD;
    return ns->fallback->removeNode(ns->fallback, nodeid);
}

/*
 * If this function fails in any way, the node parameter is deleted here,
 * so the caller function does not need to take care of it anymore
 */
static UA_StatusCode
AdHocNodestore_insertNode(UA_Nodestore *orig_ns, UA_Node *node, UA_NodeId *addedNodeId) {
    /* The node was originally created from the fallback nodestore */
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!findMockBackendNode(&node->head.nodeId))
        return ns->fallback->insertNode(ns->fallback, node, addedNodeId);
    UA_Node_clear(node);
    UA_free(node);
    return UA_STATUSCODE_BADALREADYEXISTS;
}

static UA_StatusCode
AdHocNodestore_replaceNode(UA_Nodestore *orig_ns, UA_Node *node) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    if(!findMockBackendNode(&node->head.nodeId))
        return ns->fallback->replaceNode(ns->fallback, node);
    UA_Node_clear(node);
    UA_free(node);
    return UA_STATUSCODE_BADINTERNALERROR;
}

static const UA_NodeId *
AdHocNodestore_getReferenceTypeId(UA_Nodestore *orig_ns, UA_Byte refTypeIndex) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    return ns->fallback->getReferenceTypeId(ns->fallback, refTypeIndex);
}

static void
AdHocNodestore_iterate(UA_Nodestore *orig_ns, UA_NodestoreVisitor visitor,
                       void *visitorContext) {
    AdHocNodestore *ns = (AdHocNodestore*)orig_ns;
    ns->fallback->iterate(ns->fallback, visitor, visitorContext);
}

static void
AdHocNodestore_free(UA_Nodestore *ns) {
    AdHocNodestore *ans = (AdHocNodestore*)ns;
    ans->fallback->free(ans->fallback);
    UA_free(ans);
}

UA_Nodestore *
UA_Nodestore_PilzAdHoc(void) {
    /* Allocate and initialize the nodemap */
    AdHocNodestore *ns = (AdHocNodestore*)UA_malloc(sizeof(AdHocNodestore));
    if(!ns)
        return NULL;

    ns->fallback = UA_Nodestore_ZipTree();
    if(!ns->fallback) {
        UA_free(ns);
        return NULL;
    }

    /* Populate the nodestore */
    ns->ns.free = AdHocNodestore_free;
    ns->ns.newNode = AdHocNodestore_newNode;
    ns->ns.deleteNode = AdHocNodestore_deleteNode;
    ns->ns.getNode = AdHocNodestore_getNode;
    ns->ns.getNodeFromPtr = AdHocNodestore_getNodeFromPtr;
    ns->ns.releaseNode = AdHocNodestore_releaseNode;
    ns->ns.getNodeCopy = AdHocNodestore_getNodeCopy;
    ns->ns.insertNode = AdHocNodestore_insertNode;
    ns->ns.replaceNode = AdHocNodestore_replaceNode;
    ns->ns.removeNode = AdHocNodestore_removeNode;
    ns->ns.getReferenceTypeId = AdHocNodestore_getReferenceTypeId;
    ns->ns.iterate = AdHocNodestore_iterate;

    /* All nodes are stored in RAM. Changes are made in-situ. GetEditNode is
     * identical to GetNode -- but the Node pointer is non-const. */
    ns->ns.getEditNode =
        (UA_Node * (*)(UA_Nodestore *ns, const UA_NodeId *nodeId,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNode;
    ns->ns.getEditNodeFromPtr =
        (UA_Node * (*)(UA_Nodestore *ns, UA_NodePointer ptr,
                       UA_UInt32 attributeMask,
                       UA_ReferenceTypeSet references,
                       UA_BrowseDirection referenceDirections))AdHocNodestore_getNodeFromPtr;

    return &ns->ns;
}
