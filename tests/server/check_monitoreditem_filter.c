/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

UA_Client *client;
UA_UInt32 subId;
UA_NodeId parentNodeId;
UA_NodeId parentReferenceNodeId;
UA_NodeId outNodeId;
UA_NodeId outNodeIdAnalogItem;

UA_Boolean notificationReceived = false;
UA_UInt32 countNotificationReceived = 0;
UA_Double publishingInterval = 500.0;
UA_DataValue lastValue;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void runServer(void) {
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void pauseServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

static void setup(void) {
    UA_DataValue_init(&lastValue);
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
    runServer();

    /* Define the attribute of the double variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double myDouble = 40.0;
    UA_Variant_setScalar(&attr.value, &myDouble, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId doubleNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName doubleName = UA_QUALIFIEDNAME(1, "the answer");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    UA_StatusCode retval =
        UA_Server_addVariableNode(server, doubleNodeId, parentNodeId,
                                  parentReferenceNodeId, doubleName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, &outNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

#ifdef UA_ENABLE_DA
    /* Add an AnalogItem node  */
    UA_NodeId aiNodeId = UA_NODEID_STRING(1, "an AnalogItem");
    UA_QualifiedName aiName = UA_QUALIFIEDNAME(1, "an AnalogItem");
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer AnalogItem");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer AnalogItem");
    retval =
        UA_Server_addVariableNode(server, aiNodeId, parentNodeId,
                                  parentReferenceNodeId, aiName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ANALOGITEMTYPE),
                                  attr, NULL, &outNodeIdAnalogItem);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "EURange");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, outNodeIdAnalogItem, 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);
    UA_Variant v;
    UA_Range range;
    range.high = 120;
    range.low = 10;
    UA_Variant_init(&v);
    UA_Variant_setScalar(&v, &range, &UA_TYPES[UA_TYPES_RANGE]);
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);
#endif /* UA_ENABLE_DA */

    /* Add a boolean node */
    UA_Boolean myBool = false;
    UA_Variant_setScalar(&attr.value, &myBool, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer bool");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer bool");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    retval = UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "the.bool"),
                                       parentNodeId, parentReferenceNodeId,
                                       UA_QUALIFIEDNAME(1, "the bool"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    client = UA_Client_newForUnitTest();
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedMaxKeepAliveCount = 100;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    subId = response.subscriptionId;
    notificationReceived = false;
    countNotificationReceived = 0;
}

static void teardown(void) {
    ck_assert_uint_eq(UA_Client_Subscriptions_deleteSingle(client, subId), UA_STATUSCODE_GOOD);

    /* cleanup */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_NodeId_clear(&parentNodeId);
    UA_NodeId_clear(&parentReferenceNodeId);
    UA_NodeId_clear(&outNodeId);
#ifdef UA_ENABLE_DA
    UA_NodeId_clear(&outNodeIdAnalogItem);
#endif /* UA_ENABLE_DA */
    pauseServer();
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_DataValue_clear(&lastValue);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

static void
dataChangeHandler(UA_Client *thisClient, UA_UInt32 thisSubId, void *subContext,
                  UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    notificationReceived = true;
    ++countNotificationReceived;
    UA_DataValue_clear(&lastValue);
    UA_DataValue_copy(value, &lastValue);
}

static UA_StatusCode
setDouble(UA_Client *thisClient, UA_NodeId node, UA_Double value) {
    UA_Variant variant;
    UA_Variant_setScalar(&variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    return UA_Client_writeValueAttribute(thisClient, node, &variant);
}

static UA_StatusCode
waitForNotification(UA_UInt32 notifications, UA_UInt32 maxTries) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    pauseServer();
    for(UA_UInt32 i = 0; i < maxTries; ++i) {
        UA_fakeSleep((UA_UInt32)publishingInterval + 100);
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        if(retval != UA_STATUSCODE_GOOD)
            break;
        if(countNotificationReceived == notifications)
            break;
    }
    runServer();
    return retval;
}

static UA_Boolean
fuzzyLastValueIsEqualTo(UA_Double value) {
    double offset = 0.001;
    if(lastValue.hasValue &&
       lastValue.value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        double lastDouble = *((UA_Double*)(lastValue.value.data));
        if(lastDouble > value - offset && lastDouble < value + offset)
            return true;
    }
    return false;
}

START_TEST(Server_MonitoredItemsAbsoluteFilterSetLater) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with no filter */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger because no filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(42.0));

    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(44.0));

    // set back to 40.0
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 40.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    /* modify the monitored item with an absolute filter with deadbandvalue = 2.0 */
    UA_MonitoredItemModifyRequest itemModify;
    UA_MonitoredItemModifyRequest_init(&itemModify);

    itemModify.monitoredItemId = newMonitoredItemIds[0];
    itemModify.requestedParameters.samplingInterval = 250;
    itemModify.requestedParameters.discardOldest = true;
    itemModify.requestedParameters.queueSize = 1;
    itemModify.requestedParameters.clientHandle = newMonitoredItemIds[0];
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter.deadbandValue = 2.0;
    itemModify.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    itemModify.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    itemModify.requestedParameters.filter.content.decoded.data = &filter;

    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subId;
    modifyRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    modifyRequest.itemsToModify = &itemModify;
    modifyRequest.itemsToModifySize = 1;

    UA_ModifyMonitoredItemsResponse modifyResponse =
       UA_Client_MonitoredItems_modify(client, modifyRequest);

    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    // This should not yet trigger as the previously sent value is 40
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 39.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger once at 43.0.
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(43.0));

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsAbsoluteFilterSetOnCreateRemoveLater) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with absolute filter */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter.deadbandValue = 2.0;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should not trigger because of filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger once at 43.0.
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(43.0));

    // set back to 40.0
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 40.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    /* modify the monitored item with no filter */
    UA_MonitoredItemModifyRequest itemModify;
    UA_MonitoredItemModifyRequest_init(&itemModify);

    itemModify.monitoredItemId = newMonitoredItemIds[0];
    itemModify.requestedParameters.samplingInterval = 250;
    itemModify.requestedParameters.discardOldest = true;
    itemModify.requestedParameters.queueSize = 1;
    itemModify.requestedParameters.clientHandle = newMonitoredItemIds[0];
    UA_DataChangeFilter unsetfilter;
    UA_DataChangeFilter_init(&unsetfilter);
    unsetfilter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    unsetfilter.deadbandType = UA_DEADBANDTYPE_NONE;
    unsetfilter.deadbandValue = 0.0;
    itemModify.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    itemModify.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    itemModify.requestedParameters.filter.content.decoded.data = &unsetfilter;

    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subId;
    modifyRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    modifyRequest.itemsToModify = &itemModify;
    modifyRequest.itemsToModifySize = 1;

    UA_ModifyMonitoredItemsResponse modifyResponse =
       UA_Client_MonitoredItems_modify(client, modifyRequest);

    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    // This should trigger because now we do not filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(42.0));

    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(44.0));

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsPercentFilterSetLaterMissingEURange) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with no filter */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger because no filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(42.0));

    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(44.0));

    // set back to 40.0
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 40.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    /* modify the monitored item with an percent filter with deadbandvalue = 2.0 */
    UA_MonitoredItemModifyRequest itemModify;
    UA_MonitoredItemModifyRequest_init(&itemModify);

    itemModify.monitoredItemId = newMonitoredItemIds[0];
    itemModify.requestedParameters.samplingInterval = 250;
    itemModify.requestedParameters.discardOldest = true;
    itemModify.requestedParameters.queueSize = 1;
    itemModify.requestedParameters.clientHandle = newMonitoredItemIds[0];
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_PERCENT;
    filter.deadbandValue = 2.0;
    itemModify.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    itemModify.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    itemModify.requestedParameters.filter.content.decoded.data = &filter;

    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subId;
    modifyRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    modifyRequest.itemsToModify = &itemModify;
    modifyRequest.itemsToModifySize = 1;

    UA_ModifyMonitoredItemsResponse modifyResponse =
       UA_Client_MonitoredItems_modify(client, modifyRequest);

    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    /* missing EURange. See https://reference.opcfoundation.org/v104/Core/docs/Part8/6.2/ */
    ck_assert_uint_eq(modifyResponse.results[0].statusCode,
                      UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED);

    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    // This should trigger because setting filter failed
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(42.0));

    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(44.0));

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsNoFilter) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an absolute filter with deadbandvalue = 2.0 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger because no filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived,  2);

    ck_assert(fuzzyLastValueIsEqualTo(42.0));

    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(2, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    ck_assert(fuzzyLastValueIsEqualTo(44.0));

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsSetEmpty) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an absolute filter with deadbandvalue = 2.0 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // Setting the variable empty shold trigger
    notificationReceived = false;
    countNotificationReceived = 0;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_StatusCode res = UA_Client_writeValueAttribute(client, outNodeId, &variant);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived,  1);

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

/* Test if an absolute filter can be added for boolean variables */
START_TEST(Server_MonitoredItemsAbsoluteFilterOnBool) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an absolute filter with deadbandvalue = 2.0 */
    UA_MonitoredItemCreateRequest item =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "the.bool"));
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter.deadbandValue = 0.5;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                  callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode,
                      UA_STATUSCODE_BADFILTERNOTALLOWED);
    UA_CreateMonitoredItemsResponse_clear(&createResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsAbsoluteFilterSetOnCreate) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an absolute filter with deadbandvalue = 2.0 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter.deadbandValue = 2.0;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should not trigger because of filter
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 41.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 42.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    ck_assert(fuzzyLastValueIsEqualTo(40.0));

    // This should trigger once at 43.0.
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeId, 43.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(setDouble(client, outNodeId, 44.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    ck_assert(fuzzyLastValueIsEqualTo(43.0));

    // remove monitored item
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsPercentFilterSetOnCreateMissingEURange) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an percent filter with deadbandvalue = 2.0 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeId);
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_PERCENT;
    filter.deadbandValue = 2.0;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                  callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    /* missing EURange. See https://reference.opcfoundation.org/v104/Core/docs/Part8/6.2/ */
    ck_assert_uint_eq(createResponse.results[0].statusCode,
                      UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ? (must fail)
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    // remove monitored item (must fail)
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

#ifdef UA_ENABLE_DA
START_TEST(Server_MonitoredItemsPercentFilterSetOnCreate) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an percent filter with deadbandvalue = 10.0 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeIdAnalogItem);
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_PERCENT;
    filter.deadbandValue = 10.0;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                  callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ?
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    // This should not trigger because the change is too small and gets filtered
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeIdAnalogItem, 45.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    // This should trigger
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(setDouble(client, outNodeIdAnalogItem, 90.0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(waitForNotification(1, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);
    ck_assert(fuzzyLastValueIsEqualTo(90.0));

    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST

START_TEST(Server_MonitoredItemsPercentFilterSetOnCreateDeadBandValueOutOfRange) {
    UA_DataValue_init(&lastValue);
    /* define a monitored item with an percent filter with deadbandvalue = 101 */
    UA_MonitoredItemCreateRequest item = UA_MonitoredItemCreateRequest_default(outNodeIdAnalogItem);
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    filter.deadbandType = UA_DEADBANDTYPE_PERCENT;
    filter.deadbandValue = 101;
    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    item.requestedParameters.filter.content.decoded.data = &filter;
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    callbacks[0] = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1] = {NULL};
    void *contexts[1];
    contexts[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = &item;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                  callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode,
                      UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    // Do we get initial value ? (must fail)
    notificationReceived = false;
    countNotificationReceived = 0;
    ck_assert_uint_eq(waitForNotification(0, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    // remove monitored item (must fail)
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
}
END_TEST
#endif /* UA_ENABLE_DA */

#endif /*UA_ENABLE_SUBSCRIPTIONS*/

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server monitored item filter");
    TCase *tc_server = tcase_create("Server monitored item filter Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_server, Server_MonitoredItemsNoFilter);
    tcase_add_test(tc_server, Server_MonitoredItemsSetEmpty);
    tcase_add_test(tc_server, Server_MonitoredItemsAbsoluteFilterSetOnCreate);
    tcase_add_test(tc_server, Server_MonitoredItemsAbsoluteFilterOnBool);
    tcase_add_test(tc_server, Server_MonitoredItemsAbsoluteFilterSetLater);
    tcase_add_test(tc_server, Server_MonitoredItemsAbsoluteFilterSetOnCreateRemoveLater);
    tcase_add_test(tc_server, Server_MonitoredItemsPercentFilterSetOnCreateMissingEURange);
    tcase_add_test(tc_server, Server_MonitoredItemsPercentFilterSetLaterMissingEURange);
#ifdef UA_ENABLE_DA
    tcase_add_test(tc_server, Server_MonitoredItemsPercentFilterSetOnCreate);
    tcase_add_test(tc_server, Server_MonitoredItemsPercentFilterSetOnCreateDeadBandValueOutOfRange);
#endif /* UA_ENABLE_DA */
#endif /* UA_ENABLE_SUBSCRIPTIONS */
    suite_add_tcase(s, tc_server);

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
