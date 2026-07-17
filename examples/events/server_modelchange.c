/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

#include <stdlib.h>

static void
changeEventCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                    void *monitoredItemContext,
                    const UA_KeyValueMap eventFields) {
    if(eventFields.mapSize != 1)
        return;

    const UA_Variant *value = &eventFields.map[0].value;
    if(value->type == &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE]) {
        const UA_ModelChangeStructureDataType *changes =
            (const UA_ModelChangeStructureDataType*)value->data;
        for(size_t i = 0; i < value->arrayLength; i++) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                        "ModelChange for %N with verb 0x%02x",
                        changes[i].affected, (unsigned)changes[i].verb);
        }
        return;
    }

    if(value->type == &UA_TYPES[UA_TYPES_SEMANTICCHANGESTRUCTUREDATATYPE]) {
        const UA_SemanticChangeStructureDataType *changes =
            (const UA_SemanticChangeStructureDataType*)value->data;
        for(size_t i = 0; i < value->arrayLength; i++) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                        "SemanticChange for %N", changes[i].affected);
        }
    }
}

static void
dataChangeCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                   void *monitoredItemContext, const UA_NodeId *nodeId,
                   void *nodeContext, UA_UInt32 attributeId,
                   const UA_DataValue *value) {
    if(value->hasStatus &&
       (value->status & UA_STATUSCODE_SEMANTICSCHANGED) != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "DataChange for %N has the SemanticsChanged bit", *nodeId);
    }
}

static UA_StatusCode
addChangeEventMonitoredItem(UA_Server *server) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!filter.selectClauses)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    filter.selectClausesSize = 1;

    UA_StatusCode res = UA_SimpleAttributeOperand_parse(
        &filter.selectClauses[0], UA_STRING("/Changes"));
    if(res == UA_STATUSCODE_GOOD) {
        UA_MonitoredItemCreateResult result =
            UA_Server_createEventMonitoredItem(server, UA_NS0ID(SERVER), filter,
                                               NULL, changeEventCallback);
        res = result.statusCode;
    }
    UA_EventFilter_clear(&filter);
    return res;
}

int
main(void) {
    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;

    UA_StatusCode res = UA_Server_run_startup(server);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* A Variable can own Properties and can itself be monitored for the
     * SemanticsChanged DataChange status bit. */
    const UA_NodeId owner = UA_NODEID_NUMERIC(1, 6000);
    UA_Int32 ownerValue = 42;
    UA_VariableAttributes ownerAttr = UA_VariableAttributes_default;
    ownerAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Machine Value");
    ownerAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Variant_setScalar(&ownerAttr.value, &ownerValue,
                         &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_addVariableNode(
        server, owner, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "MachineValue"), UA_NS0ID(BASEDATAVARIABLETYPE),
        ownerAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;

    const UA_NodeId nodeVersion = UA_NODEID_NUMERIC(1, 6001);
    UA_String initialVersion = UA_STRING("initial");
    UA_VariableAttributes versionAttr = UA_VariableAttributes_default;
    versionAttr.displayName = UA_LOCALIZEDTEXT("en-US", "NodeVersion");
    versionAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    versionAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&versionAttr.value, &initialVersion,
                         &UA_TYPES[UA_TYPES_STRING]);
    res = UA_Server_addVariableNode(
        server, nodeVersion, owner, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(0, "NodeVersion"), UA_NS0ID(PROPERTYTYPE),
        versionAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;

    const UA_NodeId engineeringUnit = UA_NODEID_NUMERIC(1, 6002);
    UA_Int32 unit = 1;
    UA_VariableAttributes propertyAttr = UA_VariableAttributes_default;
    propertyAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Engineering Unit");
    propertyAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    propertyAttr.accessLevel = UA_ACCESSLEVELMASK_READ |
        UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_SEMANTICCHANGE;
    UA_Variant_setScalar(&propertyAttr.value, &unit, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_addVariableNode(
        server, engineeringUnit, owner, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(1, "EngineeringUnit"), UA_NS0ID(PROPERTYTYPE),
        propertyAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;

    const UA_NodeId target = UA_NODEID_NUMERIC(1, 6003);
    UA_ObjectAttributes targetAttr = UA_ObjectAttributes_default;
    targetAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Machine Component");
    res = UA_Server_addObjectNode(
        server, target, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "MachineComponent"), UA_NS0ID(BASEOBJECTTYPE),
        targetAttr, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;

    res = addChangeEventMonitoredItem(server);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;

    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(owner);
    request.requestedParameters.samplingInterval = 0.0;
    UA_MonitoredItemCreateResult mon = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL,
        dataChangeCallback);
    if(mon.statusCode != UA_STATUSCODE_GOOD) {
        res = mon.statusCode;
        goto shutdown;
    }

    /* Consume the initial DataChange notification. */
    UA_Server_run_iterate(server, false);

    /* The reference change updates NodeVersion and emits a ModelChangeEvent. */
    res = UA_Server_addReference(
        server, owner, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 6003), true);
    if(res != UA_STATUSCODE_GOOD)
        goto shutdown;
    UA_Server_run_iterate(server, false);

    UA_Variant currentVersion;
    UA_Variant_init(&currentVersion);
    res = UA_Server_readValue(server, nodeVersion, &currentVersion);
    if(res != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&currentVersion, &UA_TYPES[UA_TYPES_STRING])) {
        if(res == UA_STATUSCODE_GOOD)
            res = UA_STATUSCODE_BADTYPEMISMATCH;
        UA_Variant_clear(&currentVersion);
        goto shutdown;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "NodeVersion is now %S", *(UA_String*)currentVersion.data);
    UA_Variant_clear(&currentVersion);

    /* Writing the marked Property emits a SemanticChangeEvent and marks the
     * owner's next Value notification with SemanticsChanged. */
    unit = 2;
    UA_Variant value;
    UA_Variant_setScalar(&value, &unit, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_writeValue(server, engineeringUnit, value);
    if(res == UA_STATUSCODE_GOOD)
        UA_Server_run_iterate(server, false);

 shutdown:
    UA_Server_run_shutdown(server);
 cleanup:
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Example failed with %s", UA_StatusCode_name(res));
    }
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
