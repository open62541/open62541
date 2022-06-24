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

#include <stdio.h>
#include <stdlib.h>

#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;
static UA_Boolean noNewSubscription; /* Don't create a subscription when the
                                        session activates */

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    noNewSubscription = false;
    running = true;
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    if (running) {
        running = false;
        THREAD_JOIN(server_thread);
    }
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
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
createSubscriptionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                           void *r) {
    UA_CreateSubscriptionResponse_copy((const UA_CreateSubscriptionResponse *)r,
                                       (UA_CreateSubscriptionResponse *)userdata);
}

static void
modifySubscriptionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                           void *r) {
    UA_ModifySubscriptionResponse_copy((const UA_ModifySubscriptionResponse *)r,
                                       (UA_ModifySubscriptionResponse *)userdata);
}

static void
createDataChangesCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                          void *r) {
    UA_CreateMonitoredItemsResponse_copy((const UA_CreateMonitoredItemsResponse *)r,
                                         (UA_CreateMonitoredItemsResponse *)userdata);
}

static void
deleteMonitoredItemsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                             void *r) {
    UA_DeleteMonitoredItemsResponse_copy((const UA_DeleteMonitoredItemsResponse *)r,
                                         (UA_DeleteMonitoredItemsResponse *)userdata);
}

static void
deleteSubscriptionsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            void *r) {
    UA_DeleteSubscriptionsResponse_copy((const UA_DeleteSubscriptionsResponse *)r,
                                        (UA_DeleteSubscriptionsResponse *)userdata);
}

START_TEST(Client_subscription) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_async) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    UA_UInt32 requestId = 0;
    UA_CreateSubscriptionResponse response;
    retval = UA_Client_Subscriptions_create_async(client, request, NULL, NULL, NULL,
                                                  createSubscriptionCallback, &response,
                                                  &requestId);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);

    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
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

    retval = UA_Client_Subscriptions_modify_async(
        client, modifySubscriptionRequest, modifySubscriptionCallback,
        &modifySubscriptionResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    modifySubscriptionResponse.responseHeader.serviceResult = 1;
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(modifySubscriptionResponse.responseHeader.serviceResult,
                     UA_STATUSCODE_GOOD);

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
    retval = UA_Client_MonitoredItems_createDataChanges_async(
        client, monRequest, &contexts, &notifications, &deleteCallbacks,
        createDataChangesCallback, &monResponse, &requestId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_run_iterate(server, true);
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(monResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(monResponse.resultsSize, 1);
    ck_assert_uint_eq(monResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.results[0].monitoredItemId;

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    UA_Server_run_iterate(server, true);
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
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

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

    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(subDeleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(subDeleteResponse.resultsSize, 1);
    ck_assert_uint_eq(subDeleteResponse.results[0], UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionResponse_clear(&response);
    UA_ModifySubscriptionResponse_clear(&modifySubscriptionResponse);
    UA_CreateMonitoredItemsResponse_clear(&monResponse);
    UA_DeleteMonitoredItemsResponse_clear(&monDeleteResponse);
    UA_DeleteSubscriptionsResponse_clear(&subDeleteResponse);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_createDataChanges) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    countNotificationReceived = 0;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

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

/* An unchanged value shall not be published after a ModifyMonitoredItem */
START_TEST(Client_subscription_modifyMonitoredItem) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    /* Receive the initial value */
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    /* No further update */
    notificationReceived = false;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
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

    UA_ModifyMonitoredItemsResponse modifyResponse =
        UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    /* Sleep longer than the publishing interval */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    /* Don't receive an immediate update */
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
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

    modifyResponse = UA_Client_MonitoredItems_modify(client, modifyRequest);
    ck_assert_uint_eq(modifyResponse.resultsSize, 1);
    ck_assert_uint_eq(modifyResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResponse);

    /* Sleep longer than the publishing interval */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_realSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

    /* Sleep long enough to trigger the next sampling. */
    UA_fakeSleep((UA_UInt32)(publishingInterval * 0.6));
    UA_realSleep((UA_UInt32)(publishingInterval * 0.6));

    /* Sleep long enough to trigger the publish callback */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* No change here, as the value subscribed value has no source timestamp. */
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(countNotificationReceived, 0);

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

START_TEST(Client_subscription_createDataChanges_async) {
    UA_UInt32 reqId = 0;
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    retval = UA_Client_MonitoredItems_createDataChanges_async(client, createRequest,
                                                              contexts, callbacks, deleteCallbacks,
                                                              createDataChangesCallback,
                                                              &createResponse, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
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
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 3;

    UA_DeleteMonitoredItemsResponse deleteResponse;
    retval = UA_Client_MonitoredItems_delete_async(
        client, deleteRequest, deleteMonitoredItemsCallback, &deleteResponse, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_realSleep(500);  // need to wait until response is at the client
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 3);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.results[1], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(deleteResponse.results[2], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);

    // Async subscription deletion is tested in Client_subscription_async
    // simplify test case using synchronous here
    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_keepAlive) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
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

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.monitoredItemId;

    /* Ensure that the subscription is late */
    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));

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

    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));

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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

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
    running = false;
    THREAD_JOIN(server_thread);

    /* Ensure that the subscription is late */
    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));
    UA_Server_run_iterate(server, true);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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

    /* Simulate BADCONNECTIONCLOSE */
    UA_Client_recvTesting_result = UA_STATUSCODE_BADCONNECTIONCLOSED;

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
    running = false;
    THREAD_JOIN(server_thread);

    /* Send Publish requests */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Still receiving on the MonitoredItem */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);
    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_statusChange) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    /* Manually set the StatusChange */
    UA_Subscription *sub =
        UA_Server_getSubscriptionById(server, response.subscriptionId);
    sub->statusChange = 1234; /* some statuscode */

    /* Send publish requests and receive them on the server side */
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, true);

    /* Server sends a StatusChange notification */
    UA_fakeSleep((UA_UInt32)response.revisedPublishingInterval + 1);
    UA_Server_run_iterate(server, true);

    /* Client receives the StatusChange */
    UA_Client_run_iterate(client, 1);

    /* The same status has been received */
    ck_assert_uint_eq(statusChange, 1234);

    UA_Server_run_shutdown(server);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
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

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_DateTime origTime = UA_DateTime_nowMonotonic();
    do {
        myInteger++;
        retval = UA_Client_writeValueAttribute_async(client, myIntegerNodeId, &attr.value, NULL, NULL, NULL);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Client_run_iterate(client, 1);
        UA_comboSleep(2);
    } while(UA_DateTime_nowMonotonic() - origTime < 1 * UA_DATETIME_SEC);

    printf("done\n");

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_timeout) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    UA_Server_run_shutdown(server);

    do {
        UA_Client_run_iterate(client, 1);
    } while(client->connectStatus == UA_STATUSCODE_GOOD);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_detach) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    /* Let the subscription run its course */
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_without_notification) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

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
    running = false;
    THREAD_JOIN(server_thread);

    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    notificationReceived = false;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    notificationReceived = false;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);

    /* Get the server back up */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

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
        UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                                NULL, NULL, NULL);
        ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
        UA_UInt32 subId = response.subscriptionId;
        ck_assert_uint_ne(subId, 0);

        /* Add a MonitoredItem */
        UA_NodeId currentTime =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        UA_MonitoredItemCreateRequest monRequest =
            UA_MonitoredItemCreateRequest_default(currentTime);

        UA_MonitoredItemCreateResult monResponse =
            UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                      UA_TIMESTAMPSTORETURN_BOTH,
                                                      monRequest, NULL, dataChangeHandler, NULL);
        ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
        UA_UInt32 monId = monResponse.monitoredItemId;

        ck_assert_uint_ne(monId, 0);
    }
}

static UA_Boolean inactivityCallbackCalled = false;

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    inactivityCallbackCalled = true;
}

START_TEST(Client_subscription_async_sub) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Set stateCallback */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
    inactivityCallbackCalled = false;

    /* Activate background publish request */
    cc->outStandingPublishRequests = 10;

    ck_assert_uint_eq(chanState, UA_SECURECHANNELSTATE_FRESH);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    UA_Client_run_iterate(client, 1);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    countNotificationReceived = 0;

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 4);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 5);

    ck_assert_uint_lt(client->config.outStandingPublishRequests, 10);

    notificationReceived = false;
    /* Simulate network cable unplugged (no response from server) */
    UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* Simulate network cable unplugged (no response from server) */
    ck_assert_uint_eq(inactivityCallbackCalled, false);
    UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
    UA_Client_run_iterate(client, (UA_UInt16)cc->timeout);
    ck_assert_uint_eq(inactivityCallbackCalled, true);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* Get the server back up */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_reconnect) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

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

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    UA_Client_run_iterate(client, 1);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    countNotificationReceived = 0;

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
    UA_Client_disconnectSecureChannel(client);

    /* Reconnect to the old session and see if the old subscription still works */
    noNewSubscription = true;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    UA_Client_run_iterate(client, 1);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    /* Get the server back up */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_transfer) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedLifetimeCount = 5;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                              NULL, statusChangeHandler, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    /* Create a second client */
    UA_Client *client2 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client2));

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
    UA_Client_run_iterate(client, 1);
    UA_Client_run_iterate(client2, 1);

    UA_Client_run_iterate(client, 1);
    UA_Client_run_iterate(client2, 1);

    /* Delete */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_Client_disconnect(client2);
    UA_Client_delete(client2);
}
END_TEST

#ifdef UA_ENABLE_METHODCALLS
START_TEST(Client_methodcall) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

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

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client Subscription");

    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_subscription);
    tcase_add_test(tc_client, Client_subscription_async);
    tcase_add_test(tc_client, Client_subscription_statusChange);
    tcase_add_test(tc_client, Client_subscription_timeout);
    tcase_add_test(tc_client, Client_subscription_detach);
    tcase_add_test(tc_client, Client_subscription_connectionClose);
    tcase_add_test(tc_client, Client_subscription_createDataChanges);
    tcase_add_test(tc_client, Client_subscription_modifyMonitoredItem);
    tcase_add_test(tc_client, Client_subscription_createDataChanges_async);
    tcase_add_test(tc_client, Client_subscription_keepAlive);
    tcase_add_test(tc_client, Client_subscription_priority);
    tcase_add_test(tc_client, Client_subscription_without_notification);
    tcase_add_test(tc_client, Client_subscription_async_sub);
    tcase_add_test(tc_client, Client_subscription_reconnect);
    tcase_add_test(tc_client, Client_subscription_transfer);
    tcase_add_test(tc_client, Client_subscription_writeBurst);
    suite_add_tcase(s,tc_client);

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
