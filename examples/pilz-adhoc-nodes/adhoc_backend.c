#include "adhoc_nodestore.h"

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

UA_Boolean
isBackendNode(const UA_NodeId *nodeId) {
    return (findMockBackendNode(nodeId) != NULL);
}

/*************************/
/* Async Result Handling */
/*************************/

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

/*******************************/
/* Backend Adhoc Node Creation */
/*******************************/

void
collectParentInfo(UA_Node *node) {

}

void
collectChildsInfo(UA_Node *node) {

}

void
collectNodeAttributes(UA_Node *node) {
    const MockBackendNode *mockNode = findMockBackendNode(&node->head.nodeId);
    if(!mockNode)
        return;

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
}
