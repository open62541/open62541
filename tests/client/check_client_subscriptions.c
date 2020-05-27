/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
UA_ServerNetworkLayer nl;
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
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

UA_Boolean notificationReceived = false;
UA_UInt32 countNotificationReceived = 0;
UA_Double publishingInterval = 500.0;

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

    UA_CreateSubscriptionResponse_deleteMembers(&response);
    UA_ModifySubscriptionResponse_deleteMembers(&modifySubscriptionResponse);
    UA_CreateMonitoredItemsResponse_deleteMembers(&monResponse);
    UA_DeleteMonitoredItemsResponse_deleteMembers(&monDeleteResponse);
    UA_DeleteSubscriptionsResponse_deleteMembers(&subDeleteResponse);

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
    UA_CreateMonitoredItemsResponse_deleteMembers(&createResponse);

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

    UA_DeleteMonitoredItemsResponse_deleteMembers(&deleteResponse);

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
    UA_CreateMonitoredItemsResponse_deleteMembers(&createResponse);

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
    UA_realSleep(50);  // need to wait until response is at the client
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 3);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.results[1], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(deleteResponse.results[2], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_deleteMembers(&deleteResponse);

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
    UA_PublishResponse_deleteMembers(&presponse);
    UA_PublishRequest_deleteMembers(&pr);

    UA_fakeSleep((UA_UInt32)(publishingInterval + 1));

    UA_PublishRequest_init(&pr);
    pr.subscriptionAcknowledgementsSize = 0;
    UA_PublishResponse_init(&presponse);
    __UA_Client_Service(client, &pr, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &presponse, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    ck_assert_uint_eq(presponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(presponse.notificationMessage.notificationDataSize, 0);
    UA_PublishResponse_deleteMembers(&presponse);
    UA_PublishRequest_deleteMembers(&pr);

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
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

    ck_assert_uint_eq(chanState, UA_SECURECHANNELSTATE_CLOSED);

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
    UA_Variant_deleteMembers(&input);

    /* call with invalid subscription id */
    UA_Variant_init(&input);
    subId = 0;
    UA_Variant_setScalarCopy(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);
    retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
                            1, &input, &outputSize, &output);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);
    UA_Variant_deleteMembers(&input);
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
    tcase_add_test(tc_client, Client_subscription_timeout);
    tcase_add_test(tc_client, Client_subscription_connectionClose);
    tcase_add_test(tc_client, Client_subscription_createDataChanges);
    tcase_add_test(tc_client, Client_subscription_createDataChanges_async);
    tcase_add_test(tc_client, Client_subscription_keepAlive);
    tcase_add_test(tc_client, Client_subscription_without_notification);
    tcase_add_test(tc_client, Client_subscription_async_sub);
    tcase_add_test(tc_client, Client_subscription_reconnect);
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
