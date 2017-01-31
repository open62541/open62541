#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"

UA_Server *server;
UA_Boolean *running;
UA_ServerNetworkLayer nl;
pthread_t server_thread;

static void * serverloop(void *_) {
    while(*running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    UA_ServerConfig config = UA_ServerConfig_standard;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    pthread_create(&server_thread, NULL, serverloop, NULL);
}

static void teardown(void) {
    *running = false;
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
}

UA_Boolean notificationReceived;

static void monitoredItemHandler(UA_UInt32 monId, UA_DataValue *value, void *context) {
    notificationReceived = true;
}

START_TEST(Client_subscription) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_standard, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_UInt32 monId;
    retval = UA_Client_Subscriptions_addMonitoredItem(client, subId, UA_NODEID_NUMERIC(0, 2259),
                                                      UA_ATTRIBUTEID_VALUE, monitoredItemHandler, NULL, &monId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    notificationReceived = false;
    retval = UA_Client_Subscriptions_manuallySendPublishRequest(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_methodcall) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 subId;
    retval = UA_Client_Subscriptions_new(client, UA_SubscriptionSettings_standard, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* monitor the server state */
    UA_UInt32 monId;
    retval = UA_Client_Subscriptions_addMonitoredItem(client, subId, UA_NODEID_NUMERIC(0, 2259),
                                                      UA_ATTRIBUTEID_VALUE, NULL, NULL, &monId);
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

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client Subscription");
    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_subscription);
    suite_add_tcase(s,tc_client);
    TCase *tc_client2 = tcase_create("Client Subscription + Method Call of GetMonitoredItmes");
    tcase_add_checked_fixture(tc_client2, setup, teardown);
    tcase_add_test(tc_client2, Client_methodcall);
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
