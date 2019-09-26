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

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
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

#ifdef UA_ENABLE_SUBSCRIPTIONS

UA_Boolean notificationReceived = false;
UA_UInt32 countNotificationReceived = 0;
UA_Double publishingInterval = 500.0;

static void
dataChangeHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                  UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    notificationReceived = true;
    countNotificationReceived++;
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
    UA_ModifySubscriptionResponse modifySubscriptionResponse = UA_Client_Subscriptions_modify(client,modifySubscriptionRequest);
    ck_assert_int_eq(modifySubscriptionResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    // an invalid UA_Client_Subscriptions_modify
    modifySubscriptionRequest.subscriptionId = 99999; // invalid
    modifySubscriptionResponse = UA_Client_Subscriptions_modify(client,modifySubscriptionRequest);
    ck_assert_int_eq(modifySubscriptionResponse.responseHeader.serviceResult, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    /* monitor the server state */
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    UA_UInt32 monId = monResponse.monitoredItemId;

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;

    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

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

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

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
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 60));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Simulate BADCONNECTIONCLOSE */
    UA_Client_recvTesting_result = UA_STATUSCODE_BADCONNECTIONCLOSED;

    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 60));
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

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

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, false);

    retval = UA_Client_MonitoredItems_deleteSingle(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_ClientState callbackClientState;

static void
stateCallback (UA_Client *client, UA_ClientState clientState){
    callbackClientState = clientState;

    if (clientState == UA_CLIENTSTATE_SESSION){
        /* A new session was created. We need to create the subscription. */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        request.requestedMaxKeepAliveCount = 1;
        UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                                NULL, NULL, NULL);
        ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
        UA_UInt32 subId = response.subscriptionId;
        ck_assert_uint_ne(subId, 0);

        /* Add a MonitoredItem */
        UA_MonitoredItemCreateRequest monRequest =
            UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    
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

    ck_assert_uint_eq(callbackClientState, UA_CLIENTSTATE_DISCONNECTED);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callbackClientState, UA_CLIENTSTATE_SESSION);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);

    countNotificationReceived = 0;

    notificationReceived = false;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 1);

    notificationReceived = false;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    notificationReceived = false;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    notificationReceived = false;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 4);

    notificationReceived = false;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 5);

    ck_assert_uint_lt(client->config.outStandingPublishRequests, 10);

    notificationReceived = false;
    /* Simulate network cable unplugged (no response from server) */
    UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
    UA_Client_run_iterate(client, (UA_UInt16)(publishingInterval + 1));
    ck_assert_uint_eq(notificationReceived, false);
    ck_assert_uint_eq(callbackClientState, UA_CLIENTSTATE_SESSION);

    /* Simulate network cable unplugged (no response from server) */
    ck_assert_uint_eq(inactivityCallbackCalled, false);
    UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
    UA_Client_run_iterate(client, (UA_UInt16)cc->timeout);
    ck_assert_uint_eq(inactivityCallbackCalled, true);
    ck_assert_uint_eq(callbackClientState, UA_CLIENTSTATE_SESSION);

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
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), 1, &input, &outputSize, &output);

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
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), 1, &input, &outputSize, &output);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID);

    UA_Variant_deleteMembers(&input);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST
#endif /* UA_ENABLE_METHODCALLS */

#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client Subscription");

#ifdef UA_ENABLE_SUBSCRIPTIONS
    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_subscription);
    tcase_add_test(tc_client, Client_subscription_connectionClose);
    tcase_add_test(tc_client, Client_subscription_createDataChanges);
    tcase_add_test(tc_client, Client_subscription_keepAlive);
    tcase_add_test(tc_client, Client_subscription_without_notification);
    tcase_add_test(tc_client, Client_subscription_async_sub);
    suite_add_tcase(s,tc_client);
#endif /* UA_ENABLE_SUBSCRIPTIONS */

#if defined(UA_ENABLE_SUBSCRIPTIONS) && defined(UA_ENABLE_METHODCALLS)
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
