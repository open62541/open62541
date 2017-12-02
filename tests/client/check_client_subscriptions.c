/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"

#include "check.h"
#include "testing_clock.h"
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

static void monitoredItemHandler(UA_UInt32 monId, UA_DataValue *value, void *context) {
    notificationReceived = true;
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

#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite* testSuite_Client(void) {
    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client, Client_subscription);
#endif /* UA_ENABLE_SUBSCRIPTIONS */

    TCase *tc_client2 = tcase_create("Client Subscription + Method Call of GetMonitoredItmes");
    tcase_add_checked_fixture(tc_client2, setup, teardown);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client2, Client_methodcall);
#endif /* UA_ENABLE_SUBSCRIPTIONS */

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
