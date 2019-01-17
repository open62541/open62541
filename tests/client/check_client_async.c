/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_highlevel_async.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"
#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "client/ua_client_internal.h"

#include "thread_wrapper.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean running;
UA_ServerNetworkLayer nl;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while (running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

static void asyncReadCallback(UA_Client *client, void *userdata,
        UA_UInt32 requestId, const UA_ReadResponse *response) {
    UA_UInt16 *asyncCounter = (UA_UInt16*) userdata;
    if (response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        (*asyncCounter) = 9999;
        UA_fakeSleep(10);
    } else {
        (*asyncCounter)++;
        UA_fakeSleep(10);
    }
}

static void asyncReadValueAtttributeCallback(UA_Client *client, void *userdata,
        UA_UInt32 requestId, UA_Variant *var) {
    UA_UInt16 *asyncCounter = (UA_UInt16*) userdata;
    (*asyncCounter)++;
    UA_fakeSleep(10);
}

START_TEST(Client_highlevel_async_readValue)
    {
        UA_ClientConfig clientConfig = UA_ClientConfig_default;
        clientConfig.outStandingPublishRequests = 0;

        UA_Client *client = UA_Client_new(clientConfig);

        UA_StatusCode retval = UA_Client_connect(client,
                "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_Client_recv = client->connection.recv;
        client->connection.recv = UA_Client_recvTesting;

        UA_UInt16 asyncCounter = 0;

        UA_UInt32 reqId = 0;
        retval = UA_Client_readValueAttribute_async(client,
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
                (UA_ClientAsyncReadValueAttributeCallback) asyncReadValueAtttributeCallback,
                (void*)&asyncCounter, &reqId);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Process async responses during 1s */
        UA_Client_run_iterate(client, 999 + 1);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(asyncCounter, 1);

        /* Simulate network cable unplugged (no response from server) */
        UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
}



START_TEST(Client_read_async)
    {
        UA_Client *client = UA_Client_new(UA_ClientConfig_default);
        UA_StatusCode retval = UA_Client_connect(client,
                "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_UInt16 asyncCounter = 0;

        UA_ReadRequest rr;
        UA_ReadRequest_init(&rr);

        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.attributeId = UA_ATTRIBUTEID_VALUE;
        rvid.nodeId = UA_NODEID_NUMERIC(0,
                UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);

        rr.nodesToRead = &rvid;
        rr.nodesToReadSize = 1;

        /* Send 100 requests */
        for (size_t i = 0; i < 100; i++) {
            retval = __UA_Client_AsyncService(client, &rr,
                    &UA_TYPES[UA_TYPES_READREQUEST],
                    (UA_ClientAsyncServiceCallback) asyncReadCallback,
                    &UA_TYPES[UA_TYPES_READRESPONSE], &asyncCounter, NULL);
            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        }

        /* Process async responses during 1s */
        retval = UA_Client_run_iterate(client, 999);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(asyncCounter, 100);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }END_TEST

START_TEST(Client_read_async_timed)
    {
        UA_ClientConfig clientConfig = UA_ClientConfig_default;
        clientConfig.outStandingPublishRequests = 0;

        UA_Client *client = UA_Client_new(clientConfig);

        UA_StatusCode retval = UA_Client_connect(client,
                "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_Client_recv = client->connection.recv;
        client->connection.recv = UA_Client_recvTesting;

        UA_UInt16 asyncCounter = 0;

        UA_ReadRequest rr;
        UA_ReadRequest_init(&rr);

        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.attributeId = UA_ATTRIBUTEID_VALUE;
        rvid.nodeId = UA_NODEID_NUMERIC(0,
                UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);

        rr.nodesToRead = &rvid;
        rr.nodesToReadSize = 1;

        retval = __UA_Client_AsyncServiceEx(client, &rr,
                &UA_TYPES[UA_TYPES_READREQUEST],
                (UA_ClientAsyncServiceCallback) asyncReadCallback,
                &UA_TYPES[UA_TYPES_READRESPONSE], &asyncCounter, NULL, 999);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Process async responses during 1s */
        retval = UA_Client_run_iterate(client, 999 + 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(asyncCounter, 1);

        /* Simulate network cable unplugged (no response from server) */
        UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        retval = __UA_Client_AsyncServiceEx(client, &rr,
                &UA_TYPES[UA_TYPES_READREQUEST],
                (UA_ClientAsyncServiceCallback) asyncReadCallback,
                &UA_TYPES[UA_TYPES_READRESPONSE], &asyncCounter, NULL, 100);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        /* Process async responses during 1s */
        UA_Client_run_iterate(client, 100 + 1);
        ck_assert_uint_eq(asyncCounter, 9999);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }END_TEST

static UA_Boolean inactivityCallbackTriggered = false;

static void inactivityCallback(UA_Client *client) {
    inactivityCallbackTriggered = true;
}

START_TEST(Client_connectivity_check)
    {
        UA_ClientConfig clientConfig = UA_ClientConfig_default;
        clientConfig.outStandingPublishRequests = 0;
        clientConfig.inactivityCallback = inactivityCallback;
        clientConfig.connectivityCheckInterval = 1000;

        UA_Client *client = UA_Client_new(clientConfig);

        UA_StatusCode retval = UA_Client_connect(client,
                "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_Client_recv = client->connection.recv;
        client->connection.recv = UA_Client_recvTesting;

        inactivityCallbackTriggered = false;

        retval = UA_Client_run_iterate(client, 1000 + 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(inactivityCallbackTriggered, false);

        /* Simulate network cable unplugged (no response from server) */
        UA_Client_recvTesting_result = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        retval = UA_Client_run_iterate(client,
                (UA_UInt16) (1000 + 1 + clientConfig.timeout));
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(inactivityCallbackTriggered, true);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_read_async);
    tcase_add_test(tc_client, Client_read_async_timed);
    tcase_add_test(tc_client, Client_connectivity_check);
    tcase_add_test(tc_client, Client_highlevel_async_readValue);

    suite_add_tcase(s, tc_client);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
