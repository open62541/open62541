/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include <check.h>
#include <stdlib.h>

#include "testing_clock.h"
#include "testing_networklayers.h"

#ifdef UA_ENABLE_STATUSCODE_DESCRIPTIONS
    #define ASSERT_STATUSCODE(a,b) ck_assert_str_eq(UA_StatusCode_name(a),UA_StatusCode_name(b));
#else
    #define ASSERT_STATUSCODE(a,b) ck_assert_uint_eq((a),(b));
#endif

UA_Server *server;
size_t callbackCount = 0;
UA_StatusCode expectedDataValueStatus;

UA_NodeId parentNodeId;
UA_NodeId parentReferenceNodeId;
UA_NodeId outNodeId;

static void setup(void) {
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    //attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId uint32NodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName uint32Name = UA_QUALIFIEDNAME(1, "the answer");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    ASSERT_STATUSCODE(UA_Server_addVariableNode(server,
                                                uint32NodeId,
                                                parentNodeId,
                                                parentReferenceNodeId,
                                                uint32Name,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                attr,
                                                NULL,
                                                &outNodeId), UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    /* cleanup */
    UA_NodeId_clear(&parentNodeId);
    UA_NodeId_clear(&parentReferenceNodeId);
    UA_NodeId_clear(&outNodeId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
dataChangeNotificationCallback(UA_Server *thisServer,
                               UA_UInt32 monitoredItemId,
                               void *monitoredItemContext,
                               const UA_NodeId *nodeId,
                               void *nodeContext,
                               UA_UInt32 attributeId,
                               const UA_DataValue *value)
{
    static UA_UInt32 lastValue = 100;
    UA_UInt32 currentValue = *((UA_UInt32*)value->value.data);
    ck_assert_uint_ne(lastValue, currentValue);
    lastValue = currentValue;
    callbackCount++;
}

START_TEST(Server_LocalMonitoredItem) {
    callbackCount = 0;

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 1);

    UA_UInt32 count = 0;
    UA_Variant val;
    UA_Variant_setScalar(&val, &count, &UA_TYPES[UA_TYPES_UINT32]);

    for(size_t i = 0; i < 10; i++) {
        count++;
        UA_Server_writeValue(server, outNodeId, val);
        UA_fakeSleep(100);
        UA_Server_run_iterate(server, 1);
    }
    ck_assert_uint_eq(callbackCount, 11);
}
END_TEST

static UA_UInt32 staticUInt32 = 1337;

static UA_StatusCode
readDataSource(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext,
               UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
               UA_DataValue *value) {
    UA_Variant_setScalar(&value->value, &staticUInt32, &UA_TYPES[UA_TYPES_UINT32]);
    value->value.storageType = UA_VARIANT_DATA_NODELETE;
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

/* Use a datasource with a static memory location */
START_TEST(Server_LocalMonitoredItem_dataSource) {
    callbackCount = 0;

    UA_DataSource ds = {readDataSource, NULL};
    UA_Server_setVariableNode_dataSource(server, outNodeId, ds);

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 1);

    for(size_t i = 0; i < 10; i++) {
        staticUInt32++;
        UA_fakeSleep(100);
        UA_Server_run_iterate(server, 1);
    }
    ck_assert_uint_eq(callbackCount, 11);
}
END_TEST

/* Custom datatype with a String NodeId */
typedef struct {
    UA_Float p;
} Point;

static UA_DataTypeMember members[1] = {
    {
        UA_TYPENAME("p")           /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT], /* .memberType */
        0,                         /* .padding */
        false,                     /* .isArray */
        false                      /* .isOptional*/
    }
};

static UA_DataType PointType = {
    UA_TYPENAME("Point")             /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {0}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {0}}, /* .binaryEncodingId, the numeric
                                         identifier used on the wire (the
                                         namespaceindex is from .typeId) */
    sizeof(Point),                   /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,       /* .typeKind */
    true,                            /* .pointerFree */
    false,                           /* .overlayable (depends on endianness and
                                         the absence of padding) */
    1,                               /* .membersSize */
    members
};

UA_DataTypeArray customDataTypes = {NULL, 1, &PointType, UA_FALSE};

START_TEST(Server_LocalMonitoredItem_CustomType) {
    callbackCount = 0;

    PointType.binaryEncodingId = UA_NODEID_STRING(1, "pointbinary");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    customDataTypes.next = config->customDataTypes;
    config->customDataTypes = &customDataTypes;

    UA_MonitoredItemCreateRequest monitorRequest =
            UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult result =
            UA_Server_createDataChangeMonitoredItem(server,
                                                    UA_TIMESTAMPSTORETURN_BOTH,
                                                    monitorRequest,
                                                    NULL,
                                                    &dataChangeNotificationCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 1);

    /* Use a value that requires the ExtensionObject to encode the NodeId of the
     * data type */
    Point p = {0.0};
    UA_Variant val;
    UA_ExtensionObject arr[100];
    UA_Variant_setArray(&val, arr, 100, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    for(size_t i = 0; i < 100; i++) {
        UA_ExtensionObject_setValueNoDelete(&arr[i], &p, &PointType);
    }
    UA_StatusCode retval = UA_Server_writeValue(server, outNodeId, val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep(100);
    UA_Server_run_iterate(server, 1);
    ck_assert_uint_eq(callbackCount, 2);

    p.p = 1.0;
    UA_fakeSleep(100);
    UA_Server_run_iterate(server, 1);
    ck_assert_uint_eq(callbackCount, 2);
}
END_TEST

static void setupIndexRange(void) {
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    /* Define the attribute of the uint32 array variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32Array[3] = {40, 41, 42};
    UA_Variant_setArray(&attr.value, &myUint32Array, 3, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","UInt32 Array");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","UInt32 Array");
    //attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId uint32ArrayNodeId = UA_NODEID_STRING(1, "UInt32Array");
    UA_QualifiedName uint32ArrayName = UA_QUALIFIEDNAME(1, "UInt32 Array");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    ASSERT_STATUSCODE(UA_Server_addVariableNode(server,
                                                uint32ArrayNodeId,
                                                parentNodeId,
                                                parentReferenceNodeId,
                                                uint32ArrayName,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                attr,
                                                NULL,
                                                &outNodeId), UA_STATUSCODE_GOOD);
}

static void
dataChangeNotificationValidateStatusCallback(UA_Server *thisServer, UA_UInt32 monitoredItemId,
                                             void *monitoredItemContext, const UA_NodeId *nodeId,
                                             void *nodeContext, UA_UInt32 attributeId,
                                             const UA_DataValue *value) {
    ASSERT_STATUSCODE(value->status, expectedDataValueStatus);
    callbackCount++;
}

START_TEST(Server_LocalMonitoredItemIndexRange) {
    callbackCount = 0;
    expectedDataValueStatus = UA_STATUSCODE_GOOD;

    UA_MonitoredItemCreateRequest monitorRequest =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monitorRequest.itemToMonitor.indexRange = UA_STRING("0:2");
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, monitorRequest, NULL,
        &dataChangeNotificationValidateStatusCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 1);
}
END_TEST

START_TEST(Server_LocalMonitoredItemIndexRangeOutOfBounds) {
    callbackCount = 0;
    expectedDataValueStatus = UA_STATUSCODE_BADINDEXRANGENODATA;

    UA_MonitoredItemCreateRequest monitorRequest =
        UA_MonitoredItemCreateRequest_default(outNodeId);
    monitorRequest.requestedParameters.samplingInterval = (double)100;
    monitorRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monitorRequest.itemToMonitor.indexRange = UA_STRING("3:5");
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_BOTH, monitorRequest, NULL,
        &dataChangeNotificationValidateStatusCallback);

    ASSERT_STATUSCODE(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackCount, 1);
}
END_TEST

static UA_UInt32 eventCount = 0;

static void setupEvent(void) {
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_StatusCode retval = UA_Server_run_startup(server);
    ASSERT_STATUSCODE(retval, UA_STATUSCODE_GOOD);
    eventCount = 0;
}

static void eventCB (UA_Server *server, UA_UInt32 monId, void *monContext,
        size_t nEventFields, const UA_Variant *eventFields)
{
    eventCount++;
    size_t*eventFiledsSize = (size_t *) monContext;
    *eventFiledsSize = nEventFields;
}

START_TEST(Server_LocalMonitoredItemEvent)
{
    /* Create monitored event */
    UA_MonitoredItemCreateRequest req;
    UA_MonitoredItemCreateRequest_init(&req);
    req.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    req.monitoringMode = UA_MONITORINGMODE_REPORTING;
    req.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    req.requestedParameters.samplingInterval = 250;
    req.requestedParameters.discardOldest = true;
    req.requestedParameters.queueSize = 1;

    UA_QualifiedName eventIdQN = UA_QUALIFIEDNAME(0, "EventId");
    UA_SimpleAttributeOperand select[1];
    UA_SimpleAttributeOperand_init(&select[0]);
    select[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    select[0].attributeId = UA_ATTRIBUTEID_VALUE;
    select[0].browsePathSize = 1;
    select[0].browsePath = &eventIdQN;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClausesSize = 1;
    filter.selectClauses = select;

    req.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    req.requestedParameters.filter.content.decoded.data = &filter;
    req.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;

    size_t eventFieldsSize = 0;

    UA_MonitoredItemCreateResult res = UA_Server_createEventMonitoredItem(
        server,
        UA_TIMESTAMPSTORETURN_NEITHER,
        req,
        &eventFieldsSize,
        eventCB
    );
    ASSERT_STATUSCODE(res.statusCode, UA_STATUSCODE_GOOD);

    UA_NodeId eventId;
    UA_StatusCode status = UA_Server_createEvent(
        server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
        &eventId
    );
    ASSERT_STATUSCODE(status, UA_STATUSCODE_GOOD);

    status = UA_Server_triggerEvent(
        server,
        eventId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        NULL,
        true
    );
    ASSERT_STATUSCODE(status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventCount,1);
    ck_assert_uint_eq(eventFieldsSize, 1);
}

END_TEST

static Suite * testSuite_Client(void) {
    Suite *s = suite_create("Local Monitored Item");
    TCase *tc_server = tcase_create("Local Monitored Item Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_LocalMonitoredItem);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_dataSource);
    tcase_add_test(tc_server, Server_LocalMonitoredItem_CustomType);
    suite_add_tcase(s, tc_server);

    TCase *tc_server_indexrange = tcase_create("Local Monitored Item Index Range");
    tcase_add_checked_fixture(tc_server_indexrange, setupIndexRange, teardown);
    tcase_add_test(tc_server_indexrange, Server_LocalMonitoredItemIndexRange);
    tcase_add_test(tc_server_indexrange, Server_LocalMonitoredItemIndexRangeOutOfBounds);
    suite_add_tcase(s, tc_server_indexrange);

    TCase *tc_server_event = tcase_create("Local Monitored Item Event");
    tcase_add_checked_fixture(tc_server_event, setupEvent, teardown);
    tcase_add_test(tc_server_event, Server_LocalMonitoredItemEvent);
    suite_add_tcase(s, tc_server_event);

    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
