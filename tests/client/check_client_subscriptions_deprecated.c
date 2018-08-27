/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "client/ua_client_internal.h"
#include "ua_client_highlevel.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"

#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"


UA_Server *server;
UA_ServerConfig *config;
UA_Boolean *running;
UA_ServerNetworkLayer nl;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(*running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    *running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS

UA_Boolean notificationReceived;
UA_UInt32 countNotificationReceived = 0;

static void monitoredItemHandler(UA_Client *client, UA_UInt32 monId, UA_DataValue *value, void *context) {
    notificationReceived = true;
    countNotificationReceived++;
}

START_TEST(Client_subscription) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_UInt32 monId;
    retval = UA_Client_Subscriptions_addMonitoredItem(client, subId, UA_NODEID_NUMERIC(0, 2259),
                                                      UA_ATTRIBUTEID_VALUE, monitoredItemHandler,
                                                      NULL, &monId, 250);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    notificationReceived = false;
    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    retval = UA_Client_Subscriptions_removeMonitoredItem(client, subId, monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_remove(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_addMonitoredItems) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest items[3];
    UA_MonitoredItemHandlingFunction hfs[3];
    void *hfContexts[3];
    UA_StatusCode itemResults[3];
    UA_UInt32 newMonitoredItemIds[3];

    /* monitor the server state */
    UA_MonitoredItemCreateRequest_init(&items[0]);
    items[0].itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2259);
    items[0].itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    items[0].monitoringMode = UA_MONITORINGMODE_REPORTING;
    items[0].requestedParameters.samplingInterval = 250;
    items[0].requestedParameters.discardOldest = true;
    items[0].requestedParameters.queueSize = 1;
    hfs[0] = (UA_MonitoredItemHandlingFunction)(uintptr_t)monitoredItemHandler;
    hfContexts[0] = NULL;

    /* monitor current time */
    UA_MonitoredItemCreateRequest_init(&items[1]);
    items[1].itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    items[1].itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    items[1].monitoringMode = UA_MONITORINGMODE_REPORTING;
    items[1].requestedParameters.samplingInterval = 250;
    items[1].requestedParameters.discardOldest = true;
    items[1].requestedParameters.queueSize = 1;
    hfs[1] = (UA_MonitoredItemHandlingFunction)(uintptr_t)monitoredItemHandler;
    hfContexts[1] = NULL;

    /* monitor invalid node */
    UA_MonitoredItemCreateRequest_init(&items[2]);
    items[2].itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 99999999);
    items[2].itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    items[2].monitoringMode = UA_MONITORINGMODE_REPORTING;
    items[2].requestedParameters.samplingInterval = 250;
    items[2].requestedParameters.discardOldest = true;
    items[2].requestedParameters.queueSize = 1;
    hfs[2] = (UA_MonitoredItemHandlingFunction)(uintptr_t)monitoredItemHandler;
    hfContexts[2] = NULL;

    retval = UA_Client_Subscriptions_addMonitoredItems(client, subId, items, 3,
                                                      hfs, hfContexts, itemResults,
                                                      newMonitoredItemIds);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(itemResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(itemResults[1], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(itemResults[2], UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    notificationReceived = false;
    countNotificationReceived = 0;
    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    notificationReceived = false;
    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    retval = UA_Client_Subscriptions_removeMonitoredItem(client, subId, newMonitoredItemIds[0]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_removeMonitoredItem(client, subId, newMonitoredItemIds[1]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_remove(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_keepAlive) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest items[1];
    UA_MonitoredItemHandlingFunction hfs[1];
    void *hfContexts[1];
    UA_StatusCode itemResults[1];
    UA_UInt32 newMonitoredItemIds[1];

    /* monitor the server state */
    UA_MonitoredItemCreateRequest_init(&items[0]);
    items[0].itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2259);
    items[0].itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    items[0].monitoringMode = UA_MONITORINGMODE_REPORTING;
    items[0].requestedParameters.samplingInterval = 250;
    items[0].requestedParameters.discardOldest = true;
    items[0].requestedParameters.queueSize = 1;
    hfs[0] = (UA_MonitoredItemHandlingFunction)(uintptr_t)monitoredItemHandler;
    hfContexts[0] = NULL;

    retval = UA_Client_Subscriptions_addMonitoredItems(client, subId, items, 1,
                                                      hfs, hfContexts, itemResults,
                                                      newMonitoredItemIds);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(itemResults[0], UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    UA_PublishRequest request;
    UA_PublishRequest_init(&request);
    request.subscriptionAcknowledgementsSize = 0;

    UA_PublishResponse response;
    UA_PublishResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.notificationMessage.notificationDataSize, 1);
    UA_PublishResponse_deleteMembers(&response);
    UA_PublishRequest_deleteMembers(&request);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    /* by default maxKeepAlive is set to 1 we must receive a response without notification message */
    UA_PublishRequest_init(&request);
    request.subscriptionAcknowledgementsSize = 0;

    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                        &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.notificationMessage.notificationDataSize, 0);
    UA_PublishResponse_deleteMembers(&response);
    UA_PublishRequest_deleteMembers(&request);

    retval = UA_Client_Subscriptions_removeMonitoredItem(client, subId, newMonitoredItemIds[0]);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_Subscriptions_remove(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_subscription_connectionClose) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_UInt32 monId;
    retval = UA_Client_Subscriptions_addMonitoredItem(client, subId, UA_NODEID_NUMERIC(0, 2259),
                                                      UA_ATTRIBUTEID_VALUE, monitoredItemHandler,
                                                      NULL, &monId, 250);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep((UA_UInt32)UA_SubscriptionSettings_default.requestedPublishingInterval + 1);

    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;

    /* Simulate BADCONNECTIONCLOSE */
    UA_Client_recvTesting_result = UA_STATUSCODE_BADCONNECTIONCLOSED;

    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSERVERNOTCONNECTED);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

#endif /* UA_ENABLE_SUBSCRIPTIONS */

#ifdef UA_ENABLE_METHODCALLS

START_TEST(Client_methodcall) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_default, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_UInt32 monId;
    retval = UA_Client_Subscriptions_addMonitoredItem(client, subId, UA_NODEID_NUMERIC(0, 2259),
                                                      UA_ATTRIBUTEID_VALUE, NULL, NULL, &monId, 250);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

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

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

#endif /* UA_ENABLE_METHODCALLS */

static Suite* testSuite_Client(void) {
    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client, Client_subscription);
    tcase_add_test(tc_client, Client_subscription_connectionClose);
    tcase_add_test(tc_client, Client_subscription_addMonitoredItems);
    tcase_add_test(tc_client, Client_subscription_keepAlive);
#endif /* UA_ENABLE_SUBSCRIPTIONS */

    TCase *tc_client2 = tcase_create("Client Subscription + Method Call of GetMonitoredItmes");
    tcase_add_checked_fixture(tc_client2, setup, teardown);
#ifdef UA_ENABLE_METHODCALLS
    tcase_add_test(tc_client2, Client_methodcall);
#endif /* UA_ENABLE_METHODCALLS */

    Suite *s = suite_create("Client Subscription");
    suite_add_tcase(s,tc_client);
    suite_add_tcase(s,tc_client2);
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
