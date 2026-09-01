/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
static UA_Boolean serverThreadRunning;
THREAD_HANDLE server_thread;
static UA_Boolean noNewSubscription; /* Don't create a subscription when the
                                        session activates */

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void runServer(void) {
    if(serverThreadRunning)
        return;
    running = true;
    THREAD_CREATE(server_thread, serverloop);
    serverThreadRunning = true;
}

static void pauseServer(void) {
    if(!serverThreadRunning)
        return;
    running = false;
    THREAD_JOIN(server_thread);
    serverThreadRunning = false;
}

static void setup(void) {
    noNewSubscription = false;
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);
    serverThreadRunning = false;
    runServer();
}

static void teardown(void) {
    if(!server)
        return;

    pauseServer();

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

UA_Boolean notificationReceived = false;
UA_UInt32 countNotificationReceived = 0;
UA_Double publishingInterval = 500.0;
UA_StatusCode statusChange = UA_STATUSCODE_GOOD;

static void
statusChangeHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                    UA_StatusChangeNotification *notification) {
    statusChange = notification->status;
}

static void
dataChangeHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                  UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    notificationReceived = true;
    countNotificationReceived++;
}

static void
iterateUntilNotification(UA_Client *client, UA_UInt32 maxIterations) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(UA_UInt32 i = 0; i < maxIterations && !notificationReceived; ++i) {
        UA_Server_run_iterate(server, false);
        /* Wait 1ms per iteration. A zero timeout makes this a busy-poll that
         * can return before the PublishResponse is readable on the socket. */
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_fakeSleep(1);
    }
}

static void
createSubscriptionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                           UA_CreateSubscriptionResponse *r) {
    UA_CreateSubscriptionResponse_copy(r, (UA_CreateSubscriptionResponse *)userdata);
}

static void
modifySubscriptionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                           UA_ModifySubscriptionResponse *r) {
    UA_ModifySubscriptionResponse_copy(r, (UA_ModifySubscriptionResponse *)userdata);
}

static void
createDataChangesCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                          UA_CreateMonitoredItemsResponse *r) {
    UA_CreateMonitoredItemsResponse_copy(r, (UA_CreateMonitoredItemsResponse *)userdata);
}

static void
deleteMonitoredItemsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                             UA_DeleteMonitoredItemsResponse *r) {
    UA_DeleteMonitoredItemsResponse_copy(r, (UA_DeleteMonitoredItemsResponse *)userdata);
}

static void
modifyMonitoredItemsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                             UA_ModifyMonitoredItemsResponse *response) {
    UA_ModifyMonitoredItemsResponse_copy(
        response, (UA_ModifyMonitoredItemsResponse*)userdata);
}

static void
injectNotification(UA_Client *client, UA_Client_Subscription *sub,
                   const UA_DataType *notificationType, void *notification) {
    UA_ExtensionObject notificationData;
    UA_ExtensionObject_init(&notificationData);
    notificationData.encoding = UA_EXTENSIONOBJECT_DECODED;
    notificationData.content.decoded.type = notificationType;
    notificationData.content.decoded.data = notification;

    UA_PublishRequest request;
    UA_PublishRequest_init(&request);
    UA_PublishResponse response;
    UA_PublishResponse_init(&response);
    response.responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    response.subscriptionId = sub->subscriptionId;
    response.notificationMessage.sequenceNumber = sub->sequenceNumber + 1;
    response.notificationMessage.notificationData = &notificationData;
    response.notificationMessage.notificationDataSize = 1;

    lockClient(client);
    client->currentlyOutStandingPublishRequests++;
    __Client_Subscriptions_processPublishResponse(client, &request, &response);
    unlockClient(client);
}

static void
injectDataChangeNotification(UA_Client *client, UA_Client_Subscription *sub,
                             UA_UInt32 clientHandle) {
    UA_MonitoredItemNotification min;
    UA_MonitoredItemNotification_init(&min);
    min.clientHandle = clientHandle;

    UA_DataChangeNotification dcn;
    UA_DataChangeNotification_init(&dcn);
    dcn.monitoredItems = &min;
    dcn.monitoredItemsSize = 1;
    injectNotification(client, sub,
                       &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION], &dcn);
}

static size_t eventFieldsSize;

static void
eventHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
             UA_UInt32 monId, void *monContext,
             const UA_KeyValueMap eventFields) {
    eventFieldsSize = eventFields.mapSize;
}

static void
injectEventNotification(UA_Client *client, UA_Client_Subscription *sub,
                        UA_UInt32 clientHandle, size_t fieldsSize) {
    UA_STACKARRAY(UA_Variant, fields, fieldsSize);
    for(size_t i = 0; i < fieldsSize; i++)
        UA_Variant_init(&fields[i]);

    UA_EventFieldList efl;
    UA_EventFieldList_init(&efl);
    efl.clientHandle = clientHandle;
    efl.eventFields = fields;
    efl.eventFieldsSize = fieldsSize;

    UA_EventNotificationList enl;
    UA_EventNotificationList_init(&enl);
    enl.events = &efl;
    enl.eventsSize = 1;
    injectNotification(client, sub,
                       &UA_TYPES[UA_TYPES_EVENTNOTIFICATIONLIST], &enl);
}

static UA_Client_MonitoredItem *
findTestMonitoredItem(UA_Client_MonitoredItem *mon, UA_UInt32 monitoredItemId) {
    if(!mon || mon->monitoredItemId == monitoredItemId)
        return mon;
    UA_Client_MonitoredItem *found =
        findTestMonitoredItem(ZIP_LEFT(mon, zipfields), monitoredItemId);
    if(found)
        return found;
    return findTestMonitoredItem(ZIP_RIGHT(mon, zipfields), monitoredItemId);
}

static size_t
countTestMonitoredItems(UA_Client_MonitoredItem *mon) {
    if(!mon)
        return 0;
    return 1 + countTestMonitoredItems(ZIP_LEFT(mon, zipfields)) +
        countTestMonitoredItems(ZIP_RIGHT(mon, zipfields));
}

static void
deleteSubscriptionsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            UA_DeleteSubscriptionsResponse *r) {
    UA_DeleteSubscriptionsResponse_copy(r, (UA_DeleteSubscriptionsResponse *)userdata);
}

START_TEST(Client_subscription) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    // a valid UA_Client_Subscriptions_modify
    UA_ModifySubscriptionRequest modifySubscriptionRequest;
    UA_ModifySubscriptionRequest_init(&modifySubscriptionRequest);
    modifySubscriptionRequest.subscriptionId = response.subscriptionId;
    modifySubscriptionRequest.requestedPublishingInterval = response.revisedPublishingInterval;
    modifySubscriptionRequest.requestedLifetimeCount = response.revisedLifetimeCount;
    modifySubscriptionRequest.requestedMaxKeepAliveCount = response.revisedMaxKeepAliveCount;
    UA_ModifySubscriptionResponse modifySubscriptionResponse =
        UA_Client_Subscriptions_modify(client,modifySubscriptionRequest);
    ck_assert_int_eq(modifySubscriptionResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    // an invalid UA_Client_Subscriptions_modify
    modifySubscriptionRequest.subscriptionId = 99999; // invalid
    modifySubscriptionResponse = UA_Client_Subscriptions_modify(client,modifySubscriptionRequest);
    ck_assert_int_eq(modifySubscriptionResponse.responseHeader.serviceResult,
                     UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.monitoredItemId;

    /* manually control the server thread */
    pauseServer();

    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    /* run the server in an independent thread again */
    runServer();

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_async) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();

    /* manually control the server thread */
    pauseServer();

    UA_UInt32 requestId = 0;
    UA_CreateSubscriptionResponse response;
    retval = UA_Client_Subscriptions_create_async(client, request, NULL, NULL, NULL,
                                                  createSubscriptionCallback, &response,
                                                  &requestId);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    response.responseHeader.serviceResult = 1;
    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(response.responseHeader.serviceResult == 1);

    UA_UInt32 subId = response.subscriptionId;

    UA_ModifySubscriptionRequest modifySubscriptionRequest;
    UA_ModifySubscriptionRequest_init(&modifySubscriptionRequest);
    modifySubscriptionRequest.subscriptionId = response.subscriptionId;
    modifySubscriptionRequest.requestedPublishingInterval =
        response.revisedPublishingInterval;
    modifySubscriptionRequest.requestedLifetimeCount = response.revisedLifetimeCount;
    modifySubscriptionRequest.requestedMaxKeepAliveCount =
        response.revisedMaxKeepAliveCount;

    UA_ModifySubscriptionResponse modifySubscriptionResponse;
    retval = UA_Client_Subscriptions_modify_async(client, modifySubscriptionRequest,
                                                  modifySubscriptionCallback,
                                                  &modifySubscriptionResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    modifySubscriptionResponse.responseHeader.serviceResult = 1;
    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(modifySubscriptionResponse.responseHeader.serviceResult == 1);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest singleMonRequest =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    void *contexts = NULL;
    UA_Client_DataChangeNotificationCallback notifications = dataChangeHandler;
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks = NULL;

    UA_CreateMonitoredItemsRequest monRequest;
    UA_CreateMonitoredItemsRequest_init(&monRequest);
    monRequest.subscriptionId = subId;
    monRequest.itemsToCreate = &singleMonRequest;
    monRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse monResponse;
    UA_CreateMonitoredItemsResponse_init(&monResponse);
    retval = UA_Client_MonitoredItems_createDataChanges_async(client, monRequest,
                                                              &contexts, &notifications, &deleteCallbacks,
                                                              createDataChangesCallback, &monResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(monResponse.resultsSize == 0 &&
            monResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(monResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(monResponse.resultsSize, 1);
    ck_assert_uint_eq(monResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.results[0].monitoredItemId;

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    UA_DeleteMonitoredItemsRequest monDeleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&monDeleteRequest);
    monDeleteRequest.subscriptionId = subId;
    monDeleteRequest.monitoredItemIds = &monId;
    monDeleteRequest.monitoredItemIdsSize = 1;
    UA_DeleteMonitoredItemsResponse monDeleteResponse;
    UA_DeleteMonitoredItemsResponse_init(&monDeleteResponse);

    retval = UA_Client_MonitoredItems_delete_async(client, monDeleteRequest,
                                                   deleteMonitoredItemsCallback,
                                                   &monDeleteResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    monDeleteResponse.responseHeader.serviceResult = 1;
    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(monDeleteResponse.responseHeader.serviceResult == 1);

    ck_assert_uint_eq(monDeleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(monDeleteResponse.resultsSize, 1);
    ck_assert_uint_eq(monDeleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_DeleteSubscriptionsRequest subDeleteRequest;
    UA_DeleteSubscriptionsRequest_init(&subDeleteRequest);
    subDeleteRequest.subscriptionIds = &monId;
    subDeleteRequest.subscriptionIdsSize = 1;
    UA_DeleteSubscriptionsResponse subDeleteResponse;
    UA_DeleteSubscriptionsResponse_init(&subDeleteResponse);
    retval = UA_Client_Subscriptions_delete_async(client, subDeleteRequest,
                                                  deleteSubscriptionsCallback,
                                                  &subDeleteResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    subDeleteResponse.responseHeader.serviceResult = 1;
    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(subDeleteResponse.responseHeader.serviceResult == 1);

    ck_assert_uint_eq(subDeleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(subDeleteResponse.resultsSize, 1);
    ck_assert_uint_eq(subDeleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionResponse_clear(&response);
    UA_ModifySubscriptionResponse_clear(&modifySubscriptionResponse);
    UA_CreateMonitoredItemsResponse_clear(&monResponse);
    UA_DeleteMonitoredItemsResponse_clear(&monDeleteResponse);
    UA_DeleteSubscriptionsResponse_clear(&subDeleteResponse);

    /* run the server in an independent thread again */
    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_delete_async_noCallback) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create a subscription synchronously */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* manually control the server thread */
    pauseServer();

    /* Delete the subscription asynchronously without a userland callback.
     * This must not dereference a NULL callback pointer. */
    UA_UInt32 requestId = 0;
    UA_DeleteSubscriptionsRequest subDeleteRequest;
    UA_DeleteSubscriptionsRequest_init(&subDeleteRequest);
    subDeleteRequest.subscriptionIds = &subId;
    subDeleteRequest.subscriptionIdsSize = 1;
    retval = UA_Client_Subscriptions_delete_async(client, subDeleteRequest,
                                                  NULL, NULL, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Run until the async response has been processed and the subscription
     * has been removed from the internal representation. A missing NULL-check
     * on the userland callback would crash here. */
    void *ctx = NULL;
    UA_DateTime maxStop = UA_DateTime_nowMonotonic() + (UA_DateTime)(2 * UA_DATETIME_SEC);
    do {
        UA_Server_run_iterate(server, true);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(UA_Client_Subscriptions_getContext(client, subId, &ctx) ==
            UA_STATUSCODE_GOOD &&
            UA_DateTime_nowMonotonic() < maxStop);

    /* The subscription has been removed from the internal representation */
    retval = UA_Client_Subscriptions_getContext(client, subId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    /* run the server in an independent thread again */
    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_createDataChanges) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    UA_MonitoredItemCreateRequest items[3];
    UA_UInt32 newMonitoredItemIds[3];
    UA_Client_DataChangeNotificationCallback callbacks[3];
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[3];
    void *contexts[3];

    /* monitor the server state */
    items[0] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    callbacks[0] = dataChangeHandler;
    contexts[0] = NULL;
    deleteCallbacks[0] = NULL;

    /* monitor invalid node */
    items[1] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, 999999));
    callbacks[1] = dataChangeHandler;
    contexts[1] = NULL;
    deleteCallbacks[1] = NULL;

    /* monitor current time */
    items[2] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    callbacks[2] = dataChangeHandler;
    contexts[2] = NULL;
    deleteCallbacks[2] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = 3;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 3);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[1].statusCode, UA_STATUSCODE_BADNODEIDUNKNOWN);
    newMonitoredItemIds[1] = createResponse.results[1].monitoredItemId;
    ck_assert_uint_eq(newMonitoredItemIds[1], 0);
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[2] = createResponse.results[2].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    /* manually control the server thread */
    pauseServer();

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    countNotificationReceived = 0;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    /* run the server in an independent thread again */
    runServer();

    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 3;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 3);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.results[1], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(deleteResponse.results[2], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* An interval of -1 links the subscription to the publishing interval of the
 * server */
START_TEST(Client_subscription_createDataChanges_negativeInterval) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* monitor current time */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    monRequest.requestedParameters.samplingInterval = -2.0;

    UA_MonitoredItemCreateResult monResult =
        UA_Client_MonitoredItems_createDataChange(client, subId, UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);

    ck_assert_uint_eq(monResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(monResult.revisedSamplingInterval == response.revisedPublishingInterval);
    UA_MonitoredItemCreateResult_clear(&monResult);
    UA_CreateSubscriptionResponse_clear(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* An unchanged value shall not be published after a ModifyMonitoredItem */
START_TEST(Client_subscription_modifyMonitoredItem) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    UA_MonitoredItemCreateRequest items[1];
    UA_UInt32 newMonitoredItemIds[1];
    UA_Client_DataChangeNotificationCallback callbacks[1];
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[1];
    void *contexts[1];

    /* Monitor the server state. Does not change during the unit test. */
    items[0] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    items[0].requestedParameters.samplingInterval = publishingInterval * 0.2;
    callbacks[0] = dataChangeHandler;
    contexts[0] = NULL;
    deleteCallbacks[0] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse createResponse =
       UA_Client_MonitoredItems_createDataChanges(client, createRequest, contexts,
                                                  callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 1);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    /* manually control the publish cycles */
    pauseServer();

    /* Receive the initial value */
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    iterateUntilNotification(client, 100);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    /* No further update */
    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 1);

    /* Modify the MonitoredItem and change the sampling interval */
    UA_MonitoredItemModifyRequest modify1;
    UA_MonitoredItemModifyRequest_init(&modify1);
    modify1.monitoredItemId = newMonitoredItemIds[0];
    modify1.requestedParameters.samplingInterval = publishingInterval * 1.5;

    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subId;
    modifyRequest.itemsToModify = &modify1;
    modifyRequest.itemsToModifySize = 1;

    runServer();

    UA_ModifyMonitoredItemsResponse modifyResponse =
        UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    pauseServer();

    /* Sleep longer than the publishing interval */
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    /* Modify the MonitoredItem and change the trigger. We want to see an
     * update. But not immediately. */
    UA_DataChangeFilter filter;
    UA_DataChangeFilter_init(&filter);
    filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP;
    modify1.requestedParameters.filter.content.decoded.data = &filter;
    modify1.requestedParameters.filter.content.decoded.type =
        &UA_TYPES[UA_TYPES_DATACHANGEFILTER];
    modify1.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;

    runServer();

    modifyResponse = UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    pauseServer();

    /* Sleep longer than the publishing interval */
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    /* Sleep long enough to trigger the next sampling. */
    UA_fakeSleep((UA_UInt32)(publishingInterval * 0.6));
    UA_Server_run_iterate(server, false);

    /* Sleep long enough to trigger the publish callback */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* No change here, as the value subscribed value has no source timestamp. */
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    runServer();

    /* Delete and clean up */
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_modifyMonitoredItem_doubleBuffer) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Notifications are injected below in a deterministic order. */
    UA_Client_getConfig(client)->outStandingPublishRequests = 0;

    UA_CreateSubscriptionRequest subRequest =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResponse =
        UA_Client_Subscriptions_create(client, subRequest, NULL, NULL, NULL);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(
            client, subResponse.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,
            monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_Client_Subscription *sub = LIST_FIRST(&client->subscriptions);
    ck_assert_ptr_ne(sub, NULL);
    UA_Client_MonitoredItem *mon = ZIP_ROOT(&sub->monitoredItems);
    ck_assert_ptr_ne(mon, NULL);
    UA_UInt32 oldHandle =
        mon->parameters.clientHandle;
    UA_Double oldSamplingInterval = mon->parameters.samplingInterval;

    UA_MonitoredItemModifyRequest item;
    UA_MonitoredItemModifyRequest_init(&item);
    item.monitoredItemId = monResponse.monitoredItemId;
    item.requestedParameters.samplingInterval = 1000.0;
    item.requestedParameters.queueSize = 1;
    item.requestedParameters.discardOldest = true;

    UA_ModifyMonitoredItemsRequest request;
    UA_ModifyMonitoredItemsRequest_init(&request);
    request.subscriptionId = subResponse.subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    request.itemsToModify = &item;
    request.itemsToModifySize = 1;

    /* First exercise response-before-notification. */
    UA_ModifyMonitoredItemsResponse response =
        UA_Client_MonitoredItems_modify(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&response);

    ck_assert(mon->pendingParameters.clientHandle);
    ck_assert(mon->parameters.samplingInterval == oldSamplingInterval);
    ck_assert(mon->pendingParameters.samplingInterval == 1000.0);
    UA_UInt32 newHandle =
        mon->pendingParameters.clientHandle;
    ck_assert_uint_ne(oldHandle, newHandle);

    notificationReceived = false;
    countNotificationReceived = 0;
    injectDataChangeNotification(client, sub, oldHandle);
    ck_assert(notificationReceived);
    ck_assert_uint_eq(countNotificationReceived, 1);
    ck_assert_uint_eq(mon->parameters.clientHandle, oldHandle);
    ck_assert(mon->pendingParameters.clientHandle);

    injectDataChangeNotification(client, sub, newHandle);
    ck_assert_uint_eq(countNotificationReceived, 2);
    ck_assert_uint_eq(mon->parameters.clientHandle, newHandle);
    ck_assert(!mon->pendingParameters.clientHandle);
    ck_assert(mon->parameters.samplingInterval == 1000.0);

    /* Now deliver the new handle before the asynchronous modify response. */
    pauseServer();
    item.requestedParameters.samplingInterval = 2000.0;
    UA_ModifyMonitoredItemsResponse asyncResponse;
    UA_ModifyMonitoredItemsResponse_init(&asyncResponse);
    UA_UInt32 requestId = 0;
    retval = UA_Client_MonitoredItems_modify_async(
        client, request, modifyMonitoredItemsCallback, &asyncResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(mon->pendingParameters.clientHandle);
    UA_UInt32 newestHandle =
        mon->pendingParameters.clientHandle;
    ck_assert_uint_ne(newHandle, newestHandle);

    injectDataChangeNotification(client, sub, newestHandle);
    ck_assert_uint_eq(mon->parameters.clientHandle,
                      newestHandle);
    ck_assert(!mon->pendingParameters.clientHandle);
    ck_assert(mon->parameters.samplingInterval == 2000.0);

    runServer();
    for(size_t i = 0; i < 1000 && asyncResponse.resultsSize == 0; i++)
        UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(asyncResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(asyncResponse.resultsSize, 1);
    ck_assert_uint_eq(asyncResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(mon->parameters.clientHandle,
                      newestHandle);
    UA_ModifyMonitoredItemsResponse_clear(&asyncResponse);

    /* A newer pending generation supersedes the previous one. */
    item.requestedParameters.samplingInterval = 3000.0;
    response = UA_Client_MonitoredItems_modify(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&response);
    UA_UInt32 supersededHandle =
        mon->pendingParameters.clientHandle;

    item.requestedParameters.samplingInterval = 4000.0;
    response = UA_Client_MonitoredItems_modify(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&response);
    UA_UInt32 latestHandle =
        mon->pendingParameters.clientHandle;
    ck_assert_uint_ne(supersededHandle, latestHandle);

    UA_UInt32 notificationCount = countNotificationReceived;
    injectDataChangeNotification(client, sub, supersededHandle);
    ck_assert_uint_eq(countNotificationReceived, notificationCount);
    ck_assert_uint_eq(mon->parameters.clientHandle,
                      newestHandle);
    ck_assert(mon->pendingParameters.clientHandle);

    injectDataChangeNotification(client, sub, latestHandle);
    ck_assert_uint_eq(countNotificationReceived, notificationCount + 1);
    ck_assert_uint_eq(mon->parameters.clientHandle,
                      latestHandle);
    ck_assert(!mon->pendingParameters.clientHandle);

    /* A rejected modification drops only its matching pending slot. */
    request.timestampsToReturn = (UA_TimestampsToReturn)100;
    response = UA_Client_MonitoredItems_modify(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID);
    ck_assert(!mon->pendingParameters.clientHandle);
    ck_assert_uint_eq(mon->parameters.clientHandle,
                      latestHandle);
    UA_ModifyMonitoredItemsResponse_clear(&response);

    retval = UA_Client_Subscriptions_deleteSingle(client,
                                                   subResponse.subscriptionId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_modifyEventFilter_doubleBuffer) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_getConfig(client)->outStandingPublishRequests = 0;

    UA_CreateSubscriptionRequest subRequest =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResponse =
        UA_Client_Subscriptions_create(client, subRequest, NULL, NULL, NULL);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_QualifiedName oldBrowsePath = UA_QUALIFIEDNAME(0, "Message");
    UA_SimpleAttributeOperand oldClause;
    UA_SimpleAttributeOperand_init(&oldClause);
    oldClause.typeDefinitionId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    oldClause.browsePath = &oldBrowsePath;
    oldClause.browsePathSize = 1;
    oldClause.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_EventFilter oldFilter;
    UA_EventFilter_init(&oldFilter);
    oldFilter.selectClauses = &oldClause;
    oldFilter.selectClausesSize = 1;

    UA_MonitoredItemCreateRequest monRequest;
    UA_MonitoredItemCreateRequest_init(&monRequest);
    monRequest.itemToMonitor.nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    monRequest.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    monRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monRequest.requestedParameters.queueSize = 1;
    monRequest.requestedParameters.discardOldest = true;
    UA_ExtensionObject_setValueNoDelete(
        &monRequest.requestedParameters.filter, &oldFilter,
        &UA_TYPES[UA_TYPES_EVENTFILTER]);

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createEvent(
            client, subResponse.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,
            monRequest, NULL, eventHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_Client_Subscription *sub = LIST_FIRST(&client->subscriptions);
    ck_assert_ptr_ne(sub, NULL);
    UA_Client_MonitoredItem *mon = ZIP_ROOT(&sub->monitoredItems);
    ck_assert_ptr_ne(mon, NULL);
    ck_assert_uint_eq(mon->eventFields.mapSize, 0);
    UA_UInt32 oldHandle =
        mon->parameters.clientHandle;

    UA_QualifiedName newBrowsePaths[2] = {
        UA_QUALIFIEDNAME(0, "Severity"),
        UA_QUALIFIEDNAME(0, "SourceName")
    };
    UA_SimpleAttributeOperand newClauses[2];
    for(size_t i = 0; i < 2; i++) {
        UA_SimpleAttributeOperand_init(&newClauses[i]);
        newClauses[i].typeDefinitionId =
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        newClauses[i].browsePath = &newBrowsePaths[i];
        newClauses[i].browsePathSize = 1;
        newClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }

    UA_EventFilter newFilter;
    UA_EventFilter_init(&newFilter);
    newFilter.selectClauses = newClauses;
    newFilter.selectClausesSize = 2;

    UA_MonitoredItemModifyRequest item;
    UA_MonitoredItemModifyRequest_init(&item);
    item.monitoredItemId = monResponse.monitoredItemId;
    item.requestedParameters.queueSize = 1;
    item.requestedParameters.discardOldest = true;
    UA_ExtensionObject_setValueNoDelete(
        &item.requestedParameters.filter, &newFilter,
        &UA_TYPES[UA_TYPES_EVENTFILTER]);

    UA_ModifyMonitoredItemsRequest request;
    UA_ModifyMonitoredItemsRequest_init(&request);
    request.subscriptionId = subResponse.subscriptionId;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    request.itemsToModify = &item;
    request.itemsToModifySize = 1;
    UA_ModifyMonitoredItemsResponse response =
        UA_Client_MonitoredItems_modify(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&response);

    ck_assert(mon->pendingParameters.clientHandle);
    ck_assert_uint_eq(mon->eventFields.mapSize, 0);
    ck_assert_ptr_eq(mon->parameters.filter.content.decoded.type,
                     &UA_TYPES[UA_TYPES_EVENTFILTER]);
    ck_assert_ptr_eq(mon->pendingParameters.filter.content.decoded.type,
                     &UA_TYPES[UA_TYPES_EVENTFILTER]);
    UA_EventFilter *activeFilter = (UA_EventFilter*)
        mon->parameters.filter.content.decoded.data;
    UA_EventFilter *pendingFilter = (UA_EventFilter*)
        mon->pendingParameters.filter.content.decoded.data;
    ck_assert_uint_eq(activeFilter->selectClausesSize, 1);
    ck_assert_uint_eq(pendingFilter->selectClausesSize, 2);
    ck_assert_ptr_ne(pendingFilter, &newFilter);
    UA_UInt32 newHandle =
        mon->pendingParameters.clientHandle;

    eventFieldsSize = 0;
    injectEventNotification(client, sub, oldHandle, 1);
    ck_assert_uint_eq(eventFieldsSize, 1);
    ck_assert_uint_eq(mon->eventFields.mapSize, 1);
    ck_assert_uint_eq(mon->parameters.clientHandle, oldHandle);
    ck_assert(mon->pendingParameters.clientHandle);

    injectEventNotification(client, sub, newHandle, 2);
    ck_assert_uint_eq(eventFieldsSize, 2);
    ck_assert_uint_eq(mon->eventFields.mapSize, 2);
    ck_assert_uint_eq(mon->parameters.clientHandle, newHandle);
    ck_assert(!mon->pendingParameters.clientHandle);
    activeFilter = (UA_EventFilter*)mon->parameters.filter.content.decoded.data;
    ck_assert_uint_eq(activeFilter->selectClausesSize, 2);

    retval = UA_Client_Subscriptions_deleteSingle(client,
                                                   subResponse.subscriptionId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_modifyMonitoredItem_edgeCases) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_getConfig(client)->outStandingPublishRequests = 0;

    UA_CreateSubscriptionRequest subRequest =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResponse =
        UA_Client_Subscriptions_create(client, subRequest, NULL, NULL, NULL);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResponse.subscriptionId;

    UA_MonitoredItemCreateRequest items[3] = {
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE)),
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME)),
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN))
    };
    UA_Client_DataChangeNotificationCallback callbacks[3] = {
        dataChangeHandler, dataChangeHandler, dataChangeHandler
    };
    void *contexts[3] = {NULL, NULL, NULL};
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[3] = {
        NULL, NULL, NULL
    };

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = 3;
    UA_CreateMonitoredItemsResponse createResponse =
        UA_Client_MonitoredItems_createDataChanges(
            client, createRequest, contexts, callbacks, deleteCallbacks);
    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 3);

    UA_UInt32 monIds[3];
    for(size_t i = 0; i < 3; i++) {
        ck_assert_uint_eq(createResponse.results[i].statusCode, UA_STATUSCODE_GOOD);
        monIds[i] = createResponse.results[i].monitoredItemId;
    }
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    UA_Client_Subscription *sub = LIST_FIRST(&client->subscriptions);
    ck_assert_ptr_ne(sub, NULL);
    UA_Client_MonitoredItem *mons[3];
    for(size_t i = 0; i < 3; i++) {
        mons[i] = findTestMonitoredItem(ZIP_ROOT(&sub->monitoredItems), monIds[i]);
        ck_assert_ptr_ne(mons[i], NULL);
    }
    ck_assert_uint_eq(countTestMonitoredItems(ZIP_ROOT(&sub->monitoredItems)), 3);

    /* A mixed response keeps the successful item's pending state and rolls
     * back only the failed item. */
    UA_MonitoredItemModifyRequest modifyItems[2];
    for(size_t i = 0; i < 2; i++) {
        UA_MonitoredItemModifyRequest_init(&modifyItems[i]);
        modifyItems[i].monitoredItemId = monIds[i];
        modifyItems[i].requestedParameters.samplingInterval = 1000.0;
        modifyItems[i].requestedParameters.queueSize = 1;
        modifyItems[i].requestedParameters.discardOldest = true;
    }
    UA_DataChangeFilter unsupportedFilter;
    UA_DataChangeFilter_init(&unsupportedFilter);
    unsupportedFilter.deadbandType = (UA_DeadbandType)100;
    UA_ExtensionObject_setValueNoDelete(
        &modifyItems[1].requestedParameters.filter, &unsupportedFilter,
        &UA_TYPES[UA_TYPES_DATACHANGEFILTER]);

    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subId;
    modifyRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    modifyRequest.itemsToModify = modifyItems;
    modifyRequest.itemsToModifySize = 2;
    UA_ModifyMonitoredItemsResponse modifyResponse =
        UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(modifyResponse.resultsSize, 2);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(modifyResponse.results[1].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);
    ck_assert(mons[0]->pendingParameters.clientHandle);
    ck_assert(!mons[1]->pendingParameters.clientHandle);
    ck_assert(!mons[2]->pendingParameters.clientHandle);

    /* Find and promote a pending handle while several monitored items exist. */
    UA_UInt32 pendingHandle =
        mons[0]->pendingParameters.clientHandle;
    UA_UInt32 otherHandles[2] = {
        mons[1]->parameters.clientHandle,
        mons[2]->parameters.clientHandle
    };
    injectDataChangeNotification(client, sub, pendingHandle);
    ck_assert_uint_eq(mons[0]->parameters.clientHandle,
                      pendingHandle);
    ck_assert_uint_eq(mons[1]->parameters.clientHandle,
                      otherHandles[0]);
    ck_assert_uint_eq(mons[2]->parameters.clientHandle,
                      otherHandles[1]);

    /* Deletion clears both embedded slots while settings are pending. */
    modifyRequest.itemsToModify = &modifyItems[1];
    modifyRequest.itemsToModifySize = 1;
    UA_ExtensionObject_init(&modifyItems[1].requestedParameters.filter);
    modifyResponse = UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);
    ck_assert(mons[1]->pendingParameters.clientHandle);
    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monIds[1]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(findTestMonitoredItem(ZIP_ROOT(&sub->monitoredItems), monIds[1]),
                     NULL);
    ck_assert_uint_eq(countTestMonitoredItems(ZIP_ROOT(&sub->monitoredItems)), 2);

    /* A local asynchronous submission failure rolls back the prepared slot. */
    UA_MonitoredItemModifyRequest asyncItem;
    UA_MonitoredItemModifyRequest_init(&asyncItem);
    asyncItem.monitoredItemId = monIds[2];
    asyncItem.requestedParameters.samplingInterval = 2000.0;
    asyncItem.requestedParameters.queueSize = 1;
    asyncItem.requestedParameters.discardOldest = true;
    modifyRequest.itemsToModify = &asyncItem;
    modifyRequest.itemsToModifySize = 1;
    UA_SecureChannelState channelState = client->channel.state;
    client->channel.state = UA_SECURECHANNELSTATE_CLOSED;
    retval = UA_Client_MonitoredItems_modify_async(
        client, modifyRequest, NULL, NULL, NULL);
    client->channel.state = channelState;
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSERVERNOTCONNECTED);
    ck_assert(!mons[2]->pendingParameters.clientHandle);
    ck_assert_uint_eq(mons[2]->parameters.clientHandle,
                      otherHandles[1]);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_createDataChanges_async) {
    UA_UInt32 reqId = 0;
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Async subscription creation is tested in Client_subscription_async
    // simplify test case using synchronous here
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    UA_MonitoredItemCreateRequest items[3];
    UA_UInt32 newMonitoredItemIds[3];
    UA_Client_DataChangeNotificationCallback callbacks[3];
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[3];
    void *contexts[3];

    /* monitor the server state */
    items[0] = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    callbacks[0] = dataChangeHandler;
    contexts[0] = NULL;
    deleteCallbacks[0] = NULL;

    /* monitor invalid node */
    items[1] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, 999999));
    callbacks[1] = dataChangeHandler;
    contexts[1] = NULL;
    deleteCallbacks[1] = NULL;

    /* monitor current time */
    items[2] = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    callbacks[2] = dataChangeHandler;
    contexts[2] = NULL;
    deleteCallbacks[2] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = 3;
    UA_CreateMonitoredItemsResponse createResponse;

    /* manually control the server thread */
    pauseServer();

    retval = UA_Client_MonitoredItems_createDataChanges_async(client, createRequest,
                                                              contexts, callbacks, deleteCallbacks,
                                                              createDataChangesCallback,
                                                              &createResponse, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    createResponse.responseHeader.serviceResult = 1;
    do {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(createResponse.responseHeader.serviceResult == 1);

    ck_assert_uint_eq(createResponse.resultsSize, 3);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[1].statusCode,
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    newMonitoredItemIds[1] = createResponse.results[1].monitoredItemId;
    ck_assert_uint_eq(newMonitoredItemIds[1], 0);
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[2] = createResponse.results[2].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 3;

    UA_DeleteMonitoredItemsResponse deleteResponse;
    UA_DeleteMonitoredItemsResponse_init(&deleteResponse);
    retval = UA_Client_MonitoredItems_delete_async(client, deleteRequest,
                                                   deleteMonitoredItemsCallback, &deleteResponse, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    pauseServer();
    for(size_t i = 0; i < 1000 &&
        deleteResponse.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
        deleteResponse.resultsSize == 0; ++i) {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 3);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.results[1], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(deleteResponse.results[2], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);

    /* run the server in an independent thread again */
    runServer();

    // Async subscription deletion is tested in Client_subscription_async
    // simplify test case using synchronous here
    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_keepAlive) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* For the keepalive tests disable automatically sending publish requests */
    UA_Client_getConfig(client)->outStandingPublishRequests = 0;

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.monitoredItemId;

    /* Ensure that the subscription is late */
    pauseServer();
    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));
    runServer();

    /* Manually send a publish request */
    UA_PublishRequest pr;
    UA_PublishRequest_init(&pr);
    pr.subscriptionAcknowledgementsSize = 0;
    UA_PublishResponse presponse;
    UA_PublishResponse_init(&presponse);
    __UA_Client_Service(client, &pr, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &presponse, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    ck_assert_uint_eq(presponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(presponse.notificationMessage.notificationDataSize, 1);
    UA_PublishResponse_clear(&presponse);
    UA_PublishRequest_clear(&pr);

    /* Ensure that the subscription is late */
    pauseServer();
    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));
    runServer();

    UA_PublishRequest_init(&pr);
    pr.subscriptionAcknowledgementsSize = 0;
    UA_PublishResponse_init(&presponse);
    __UA_Client_Service(client, &pr, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &presponse, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    ck_assert_uint_eq(presponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(presponse.notificationMessage.notificationDataSize, 0);
    UA_PublishResponse_clear(&presponse);
    UA_PublishRequest_clear(&pr);

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_priority) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* For the test disable automatically sending publish requests */
    UA_Client_getConfig(client)->outStandingPublishRequests = 0;

    // prio = 0
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId1 = response.subscriptionId;

    // prio = 255 (highest)
    request.priority = 255;
    response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId2 = response.subscriptionId;

    // prio = 0
    request.priority = 0;
    response = UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId3 = response.subscriptionId;

    /* manually control the server thread */
    pauseServer();

    /* Ensure that the subscription is late */
    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));
    UA_Server_run_iterate(server, false);

    /* run the server in an independent thread again */
    runServer();

    /* Manually send a publish request */
    UA_PublishRequest pr;
    UA_PublishRequest_init(&pr);
    pr.subscriptionAcknowledgementsSize = 0;
    UA_PublishResponse presponse;
    UA_PublishResponse_init(&presponse);
    __UA_Client_Service(client, &pr, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &presponse, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    ck_assert_uint_eq(presponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(presponse.notificationMessage.notificationDataSize, 0);
    ck_assert_uint_eq(presponse.subscriptionId, subId2); /* the highest prio */
    UA_PublishResponse_clear(&presponse);
    UA_PublishRequest_clear(&pr);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Client_Subscriptions_deleteSingle(client, subId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Client_Subscriptions_deleteSingle(client, subId3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_connectionClose) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Manually close the connection. The connection is internally closed at the
     * next iteration of the EventLoop. Hence the next request is sent out. But
     * the connection "actually closes" before receiving the response. */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    uintptr_t connId = client->channel.connectionId;
    cm->closeConnection(cm, connId);

    notificationReceived = false;

    /* Reconnect a new SecureChannel and recover the Session */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    ck_assert_uint_eq(ss, UA_SESSIONSTATE_CREATED);
    while(ss != UA_SESSIONSTATE_ACTIVATED) {
        UA_Client_run_iterate(client, 1);
        UA_Client_getState(client, NULL, &ss, NULL);
    }

    /* manually control the server thread */
    pauseServer();

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);

    /* Send Publish requests */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Still receiving on the MonitoredItem */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    UA_Server_run_iterate(server, false);
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    /* run the server in an independent thread again */
    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_statusChange) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedLifetimeCount = 5;
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request,
                                       NULL, statusChangeHandler, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_NodeId monTarget = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(monTarget);

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH, monRequest,
                                                  NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    /* Manually control the server thread */
    pauseServer();

    /* Manually set the StatusChange */
    UA_Subscription *sub = getSubscriptionById(server, response.subscriptionId);
    sub->statusChange = 1234; /* some statuscode */

    /* Send publish requests and receive them on the server side */
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);

    /* Server sends a StatusChange notification */
    UA_fakeSleep((UA_UInt32)response.revisedPublishingInterval + 1);
    UA_Server_run_iterate(server, false);

    /* Client receives the StatusChange */
    UA_Client_run_iterate(client, 1);

    /* The same status has been received */
    ck_assert_uint_eq(statusChange, 1234);

    UA_Server_run_shutdown(server);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    runServer();
}
END_TEST

/* Write to the variable that is being monitored at a high rate */
START_TEST(Client_subscription_writeBurst) {
    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                     parentReferenceNodeId, myIntegerName,
                                                     UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client *client = UA_Client_newForUnitTest();
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(myIntegerNodeId);

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    pauseServer();

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_DateTime origTime = UA_DateTime_nowMonotonic();
    do {
        myInteger++;
        retval = UA_Client_writeValueAttribute_async(client, myIntegerNodeId, &attr.value,
                                                     NULL, NULL, NULL);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 1);
        UA_fakeSleep(2);
    } while(UA_DateTime_nowMonotonic() - origTime < 1 * UA_DATETIME_SEC);

    printf("done\n");

    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_timeout) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Still receiving on the MonitoredItem */
    UA_DateTime renewSleep = UA_DateTime_nowMonotonic() - client->nextChannelRenewal;
    UA_fakeSleep((UA_UInt32)(renewSleep / UA_DATETIME_MSEC) + 1);

    /* manually control the server thread */
    pauseServer();

    /* Shut down the server */
    UA_Server_run_shutdown(server);

    /* The client tries to reconnect, but has to fail eventually as the server
     * is down */
    do {
        UA_Client_run_iterate(client, 1);
    } while(client->connectStatus == UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    /* Run the server in an independent thread again for the shutdown after the
     * unit test */
    runServer();
}
END_TEST

START_TEST(Client_subscription_detach) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);


    /* Close the session but detach the subscription */
    UA_CloseSessionRequest closeRequest;
    UA_CloseSessionRequest_init(&closeRequest);
    closeRequest.deleteSubscriptions = false;
    UA_CloseSessionResponse closeResponse;

    __UA_Client_Service(client,
                        &closeRequest, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeResponse, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    /* manually control the server thread */
    pauseServer();

    /* Let the subscription run its course */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    /* run the server in an independent thread again */
    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_without_notification) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    monRequest.requestedParameters.samplingInterval = 99999999.0;

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    UA_UInt32 monId = monResponse.monitoredItemId;
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    /* manually control the server thread */
    pauseServer();

    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    notificationReceived = false;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    notificationReceived = false;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);

    /* Get the server back up */
    runServer();

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_SecureChannelState chanState;
static UA_SessionState sessState;
static UA_Boolean hasMon;

static void
monCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
            UA_CreateMonitoredItemsResponse *r) {
    hasMon = true;
}

static void
createSubscriptionCallback2(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            UA_CreateSubscriptionResponse *r) {
    UA_CreateSubscriptionResponse *rr = (UA_CreateSubscriptionResponse*)r;

    ck_assert_uint_ne(rr->subscriptionId, 0);

    /* Add a MonitoredItem */
    UA_NodeId currentTime =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    UA_CreateMonitoredItemsRequest req;
    UA_CreateMonitoredItemsRequest_init(&req);
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(currentTime);
    req.itemsToCreate = &monRequest;
    req.itemsToCreateSize = 1;
    req.subscriptionId = rr->subscriptionId;
    
    UA_Client_DataChangeNotificationCallback callbacks[1] = {dataChangeHandler};
    UA_StatusCode res =
        UA_Client_MonitoredItems_createDataChanges_async(client, req,
                                                         NULL, callbacks, NULL,
                                                         monCallback, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    chanState = channelState;
    sessState = sessionState;

    if(noNewSubscription)
        return;

    if(sessionState == UA_SESSIONSTATE_ACTIVATED) {
        /* A new session was created. We need to create the subscription. */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        request.requestedMaxKeepAliveCount = 1;
        UA_StatusCode res =
            UA_Client_Subscriptions_create_async(client, request, NULL, NULL, NULL,
                                                 createSubscriptionCallback2, NULL, NULL);

        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }
}

static UA_Boolean inactivityCallbackCalled = false;

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    inactivityCallbackCalled = true;
}

START_TEST(Client_subscription_async_sub) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Set stateCallback */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
    inactivityCallbackCalled = false;

    /* Activate background publish request */
    cc->outStandingPublishRequests = 10;

    ck_assert_uint_eq(chanState, UA_SECURECHANNELSTATE_CLOSED);

    hasMon = false;
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    UA_Client_run_iterate(client, 1);

    /* manually control the server thread */
    pauseServer();

    countNotificationReceived = 0;
    notificationReceived = false;

    while(!hasMon) {
        UA_Client_run_iterate(client, 1);
        UA_Server_run_iterate(server, false);
    }

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);

    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);

    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 4);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 5);

    /* Simulate network cable unplugged (no response from server) */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    cm->closeConnection(cm, client->channel.connectionId);
    UA_fakeSleep((UA_UInt32)cc->timeout * 2);

    ck_assert_uint_lt(client->config.outStandingPublishRequests, 10);
    ck_assert_uint_eq(inactivityCallbackCalled, false);

    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(inactivityCallbackCalled, true);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_CREATED);

    /* Get the server back up */
    runServer();

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_reconnect) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Set stateCallback */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
    inactivityCallbackCalled = false;

    /* Activate background publish request */
    cc->outStandingPublishRequests = 10;

    hasMon = false;
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    UA_Client_run_iterate(client, 1);

    /* manually control the server thread */
    pauseServer();

    while(!hasMon) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 1);
    }

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, false);

    countNotificationReceived = 0;

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    runServer();
    UA_Client_disconnectSecureChannel(client);

    /* Reconnect to the old session and see if the old subscription still works */
    noNewSubscription = true;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* manually control the server thread */
    pauseServer();

    notificationReceived = false;

    while(!notificationReceived) {
        UA_fakeSleep((UA_UInt32)publishingInterval + 1);
        UA_Client_run_iterate(client, 1);
        UA_Server_run_iterate(server, false);
    }

    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    /* Get the server back up */
    runServer();

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_server_disappears) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Set stateCallback */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
    inactivityCallbackCalled = false;

    /* Activate background publish request */
    cc->outStandingPublishRequests = 10;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* Create a Subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Create a MonitoredItem */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Client_run_iterate(client, 1);

    /* Shut down the server */
    teardown();

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Client_run_iterate(client, 1);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_transfer) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedLifetimeCount = 5;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                              NULL, statusChangeHandler, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

    for(size_t i = 0; i < 5; i++) {
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    }

    /* Create a second client */
    UA_Client *client2 = UA_Client_newForUnitTest();
    retval = UA_Client_connect(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Move the subscription to the second client */
    UA_TransferSubscriptionsRequest trequest;
    UA_TransferSubscriptionsRequest_init(&trequest);
    trequest.subscriptionIds = &response.subscriptionId;
    trequest.subscriptionIdsSize = 1;

    UA_TransferSubscriptionsResponse tresponse;
    __UA_Client_Service(client2,
                        &trequest, &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSREQUEST],
                        &tresponse,  &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSRESPONSE]);

    UA_TransferSubscriptionsResponse_clear(&tresponse);

    /* Iterate the clients some more to see what happens */
    for(size_t i = 0; i < 10; i++) {
        UA_Client_run_iterate(client, 1);
        UA_Client_run_iterate(client2, 1);

        UA_fakeSleep(100);
    }

    /* Delete */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_Client_disconnect(client2);
    UA_Client_delete(client2);
}
END_TEST

START_TEST(Client_subscription_getSetContext) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Get context — initially NULL */
    void *ctx = (void*)0xDEAD;
    retval = UA_Client_Subscriptions_getContext(client, subId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, NULL);

    /* Set context */
    int myData = 42;
    retval = UA_Client_Subscriptions_setContext(client, subId, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get context again */
    retval = UA_Client_Subscriptions_getContext(client, subId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, &myData);

    /* Invalid subscription id */
    retval = UA_Client_Subscriptions_getContext(client, 99999, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    retval = UA_Client_Subscriptions_setContext(client, 99999, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    /* NULL args */
    retval = UA_Client_Subscriptions_getContext(client, subId, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Client_Subscriptions_getContext(NULL, subId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_deleteSingle) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Delete it */
    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete again — should fail */
    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_setPublishingMode) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Disable publishing */
    UA_SetPublishingModeRequest spmReq;
    UA_SetPublishingModeRequest_init(&spmReq);
    spmReq.publishingEnabled = false;
    spmReq.subscriptionIds = &subId;
    spmReq.subscriptionIdsSize = 1;
    UA_SetPublishingModeResponse spmResp =
        UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(spmResp.resultsSize, 1);
    ck_assert_uint_eq(spmResp.results[0], UA_STATUSCODE_GOOD);
    UA_SetPublishingModeResponse_clear(&spmResp);

    /* Re-enable publishing */
    spmReq.publishingEnabled = true;
    spmResp = UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetPublishingModeResponse_clear(&spmResp);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static void
dataChangeCallback_ext(UA_Client *c, UA_UInt32 subId, void *subContext,
                       UA_UInt32 monId, void *monContext,
                       UA_DataValue *value) {
    /* no-op: just need a valid callback */
}

START_TEST(Client_monitoredItem_getSetContext) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Create a monitored item */
    UA_MonitoredItemCreateRequest monReq =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    UA_MonitoredItemCreateResult monResult =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, monReq, NULL, dataChangeCallback_ext, NULL);
    ck_assert_uint_eq(monResult.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResult.monitoredItemId;

    /* Get context — initially NULL */
    void *ctx = (void*)0xDEAD;
    retval = UA_Client_MonitoredItem_getContext(client, subId, monId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, NULL);

    /* Set context */
    int myData = 123;
    retval = UA_Client_MonitoredItem_setContext(client, subId, monId, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get context again */
    retval = UA_Client_MonitoredItem_getContext(client, subId, monId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, &myData);

    /* Invalid subscription id */
    retval = UA_Client_MonitoredItem_getContext(client, 99999, monId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    /* Invalid monitored item id */
    retval = UA_Client_MonitoredItem_getContext(client, subId, 99999, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADMONITOREDITEMIDINVALID);

    /* NULL args */
    retval = UA_Client_MonitoredItem_getContext(client, subId, monId, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Client_MonitoredItem_getContext(NULL, subId, monId, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Set context with invalid sub */
    retval = UA_Client_MonitoredItem_setContext(client, 99999, monId, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    retval = UA_Client_MonitoredItem_setContext(client, subId, 99999, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADMONITOREDITEMIDINVALID);

    retval = UA_Client_MonitoredItem_setContext(NULL, subId, monId, &myData);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_setMonitoringMode) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription + monitored item */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    UA_MonitoredItemCreateRequest monReq =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    UA_MonitoredItemCreateResult monResult =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, monReq, NULL, dataChangeCallback_ext, NULL);
    ck_assert_uint_eq(monResult.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResult.monitoredItemId;

    /* Set monitoring mode to Disabled */
    UA_SetMonitoringModeRequest smmReq;
    UA_SetMonitoringModeRequest_init(&smmReq);
    smmReq.subscriptionId = subId;
    smmReq.monitoringMode = UA_MONITORINGMODE_DISABLED;
    smmReq.monitoredItemIds = &monId;
    smmReq.monitoredItemIdsSize = 1;
    UA_SetMonitoringModeResponse smmResp =
        UA_Client_MonitoredItems_setMonitoringMode(client, smmReq);
    ck_assert_uint_eq(smmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetMonitoringModeResponse_clear(&smmResp);

    /* Set back to Reporting */
    smmReq.monitoringMode = UA_MONITORINGMODE_REPORTING;
    smmResp = UA_Client_MonitoredItems_setMonitoringMode(client, smmReq);
    ck_assert_uint_eq(smmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetMonitoringModeResponse_clear(&smmResp);

    UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_setTriggering) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Create trigger and triggering items */
    UA_MonitoredItemCreateRequest monReqs[2];
    monReqs[0] = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    monReqs[1] = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResults[2];
    monResults[0] = UA_Client_MonitoredItems_createDataChange(client, subId,
        UA_TIMESTAMPSTORETURN_BOTH, monReqs[0], NULL, dataChangeCallback_ext, NULL);
    ck_assert_uint_eq(monResults[0].statusCode, UA_STATUSCODE_GOOD);
    monResults[1] = UA_Client_MonitoredItems_createDataChange(client, subId,
        UA_TIMESTAMPSTORETURN_BOTH, monReqs[1], NULL, dataChangeCallback_ext, NULL);
    ck_assert_uint_eq(monResults[1].statusCode, UA_STATUSCODE_GOOD);

    UA_UInt32 triggeringItemId = monResults[0].monitoredItemId;
    UA_UInt32 linkedItemId = monResults[1].monitoredItemId;

    /* SetTriggering: add a link */
    UA_SetTriggeringRequest stReq;
    UA_SetTriggeringRequest_init(&stReq);
    stReq.subscriptionId = subId;
    stReq.triggeringItemId = triggeringItemId;
    stReq.linksToAdd = &linkedItemId;
    stReq.linksToAddSize = 1;
    UA_SetTriggeringResponse stResp =
        UA_Client_MonitoredItems_setTriggering(client, stReq);
    ck_assert_uint_eq(stResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetTriggeringResponse_clear(&stResp);

    /* Remove the link */
    UA_SetTriggeringRequest stReq2;
    UA_SetTriggeringRequest_init(&stReq2);
    stReq2.subscriptionId = subId;
    stReq2.triggeringItemId = triggeringItemId;
    stReq2.linksToRemove = &linkedItemId;
    stReq2.linksToRemoveSize = 1;
    stResp = UA_Client_MonitoredItems_setTriggering(client, stReq2);
    ck_assert_uint_eq(stResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetTriggeringResponse_clear(&stResp);

    UA_Client_MonitoredItems_deleteSingle(client, subId, monResults[0].monitoredItemId);
    UA_Client_MonitoredItems_deleteSingle(client, subId, monResults[1].monitoredItemId);
    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_modifyAsync) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = response.subscriptionId;

    /* Modify via async */
    UA_ModifySubscriptionRequest modReq;
    UA_ModifySubscriptionRequest_init(&modReq);
    modReq.subscriptionId = subId;
    modReq.requestedPublishingInterval = 200.0;
    modReq.requestedMaxKeepAliveCount = 20;
    modReq.requestedLifetimeCount = 600;
    modReq.maxNotificationsPerPublish = 100;

    UA_UInt32 reqId = 0;
    retval = UA_Client_Subscriptions_modify_async(client, modReq,
                                                  NULL, NULL, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(reqId != 0);

    /* Drive iterations to get response */
    for(int i = 0; i < 20; i++) {
        UA_fakeSleep(10);
        UA_Client_run_iterate(client, 1);
    }

    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

#ifdef UA_ENABLE_METHODCALLS
START_TEST(Client_methodcall) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, NULL, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

/* Minimal nodeset does not contain any method we can call here */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_UInt32 monId = monResponse.monitoredItemId;
    UA_UInt32 subId = response.subscriptionId;

    /* call a method to get monitored item id */
    UA_Variant input;
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);
    size_t outputSize;
    UA_Variant *output;
    retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
                            1, &input, &outputSize, &output);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outputSize, 2);

    ck_assert_uint_eq(output[0].arrayLength, 1);

    ck_assert_uint_eq(*((UA_UInt32*)output[0].data), monId);

    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_Variant_clear(&input);

    /* call with invalid subscription id */
    UA_Variant_init(&input);
    subId = 0;
    UA_Variant_setScalarCopy(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);
    retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
                            1, &input, &outputSize, &output);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);
    UA_Variant_clear(&input);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST
#endif /* UA_ENABLE_METHODCALLS */

START_TEST(Client_subscription_modifyAsync_invalidSubscriptionId) {
    /* src/client/ua_client_subscriptions.c:280-283:
     *   sub = findSubscriptionById(client, request.subscriptionId);
     *   if(!sub) { unlockClient; return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID; }
     * The sync UA_Client_Subscriptions_modify is tested for this path;
     * the async variant is not. */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Build a request with a clearly invalid subscriptionId */
    UA_ModifySubscriptionRequest req;
    UA_ModifySubscriptionRequest_init(&req);
    req.subscriptionId = 99999;
    req.requestedPublishingInterval = 500.0;
    req.requestedLifetimeCount = 100;
    req.requestedMaxKeepAliveCount = 10;

    UA_ModifySubscriptionResponse resp;
    UA_UInt32 requestId = 0;
    retval = UA_Client_Subscriptions_modify_async(
        client, req,
        (UA_ClientAsyncModifySubscriptionCallback)NULL,
        &resp, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_renewSecureChannel_returnsGoodCallAgain) {
    /* src/client/ua_client_connect.c:712-715:
     *   if(channel.state != OPEN || renewState == SENT || nextRenewal > now)
     *     return UA_STATUSCODE_GOODCALLAGAIN;
     * Right after a fresh connect, the channel is OPEN and the renewal
     * timer is in the future, so the call returns GOODCALLAGAIN without
     * sending anything. None of the existing tests call the public
     * UA_Client_renewSecureChannel directly. */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* First call: channel is fresh, so GOODCALLAGAIN */
    retval = UA_Client_renewSecureChannel(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOODCALLAGAIN);

    /* Second call: same -- channel is still fresh */
    retval = UA_Client_renewSecureChannel(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOODCALLAGAIN);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client Subscription");

    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_subscription);
    tcase_add_test(tc_client, Client_subscription_async);
    tcase_add_test(tc_client, Client_subscription_delete_async_noCallback);
    tcase_add_test(tc_client, Client_subscription_statusChange);
    tcase_add_test(tc_client, Client_subscription_timeout);
    tcase_add_test(tc_client, Client_subscription_detach);
    tcase_add_test(tc_client, Client_subscription_connectionClose);
    tcase_add_test(tc_client, Client_subscription_createDataChanges);
    tcase_add_test(tc_client, Client_subscription_createDataChanges_negativeInterval);
    tcase_add_test(tc_client, Client_subscription_modifyMonitoredItem);
    tcase_add_test(tc_client, Client_subscription_createDataChanges_async);
    tcase_add_test(tc_client, Client_subscription_keepAlive);
    tcase_add_test(tc_client, Client_subscription_priority);
    tcase_add_test(tc_client, Client_subscription_without_notification);
    tcase_add_test(tc_client, Client_subscription_async_sub);
    tcase_add_test(tc_client, Client_subscription_reconnect);
    tcase_add_test(tc_client, Client_subscription_server_disappears);
    tcase_add_test(tc_client, Client_subscription_transfer);
    tcase_add_test(tc_client, Client_subscription_writeBurst);
    tcase_add_test(tc_client, Client_subscription_getSetContext);
    tcase_add_test(tc_client, Client_subscription_deleteSingle);
    tcase_add_test(tc_client, Client_subscription_setPublishingMode);
    tcase_add_test(tc_client, Client_monitoredItem_getSetContext);
    tcase_add_test(tc_client, Client_subscription_setMonitoringMode);
    tcase_add_test(tc_client, Client_subscription_setTriggering);
    tcase_add_test(tc_client, Client_subscription_modifyAsync);
    tcase_add_test(tc_client, Client_subscription_modifyAsync_invalidSubscriptionId);
    tcase_add_test(tc_client, Client_renewSecureChannel_returnsGoodCallAgain);
    suite_add_tcase(s,tc_client);

    TCase *tc_doubleBuffer = tcase_create("MonitoredItem Double Buffer");
    tcase_add_checked_fixture(tc_doubleBuffer, setup, teardown);
    tcase_add_test(tc_doubleBuffer,
                   Client_subscription_modifyMonitoredItem_doubleBuffer);
    tcase_add_test(tc_doubleBuffer,
                   Client_subscription_modifyEventFilter_doubleBuffer);
    tcase_add_test(tc_doubleBuffer,
                   Client_subscription_modifyMonitoredItem_edgeCases);
    suite_add_tcase(s, tc_doubleBuffer);

#ifdef UA_ENABLE_METHODCALLS
    TCase *tc_client2 = tcase_create("Client Subscription + Method Call of GetMonitoredItmes");
    tcase_add_checked_fixture(tc_client2, setup, teardown);
    tcase_add_test(tc_client2, Client_methodcall);
    suite_add_tcase(s,tc_client2);
#endif

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
