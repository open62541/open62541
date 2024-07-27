/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
    runServer();
}

static void teardown(void) {
    pauseServer();
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(SecureChannel_timeout_max) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ClientConfig *cconfig = UA_Client_getConfig(client);
    UA_fakeSleep(cconfig->secureChannelLifeTime);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(SecureChannel_renew) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    pauseServer();

    /* Get the channel id */
    UA_UInt32 channelId = client->channel.securityToken.channelId;

    /* Renew the channel for the first time */
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));

    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);

    /* Renew the channel for the second time */
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));

    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);

    /* We are still on the same channel */
    ck_assert_uint_eq(channelId, client->channel.securityToken.channelId);

    runServer();

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Send the next message after the securechannel timed out */
START_TEST(SecureChannel_timeout_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ClientConfig *cconfig = UA_Client_getConfig(client);
    UA_fakeSleep(cconfig->secureChannelLifeTime + 1);
    UA_realSleep(200 + 1); // UA_MAXTIMEOUT+1 wait to be sure UA_Server_run_iterate can be completely executed

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Send an async message and receive the response when the securechannel timed out */
START_TEST(SecureChannel_networkfail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadRequest rq;
    UA_ReadRequest_init(&rq);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    rq.nodesToRead = &rvi;
    rq.nodesToReadSize = 1;

    /* Manually close the TCP connection */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    cm->closeConnection(cm, client->channel.connectionId);
    UA_EventLoop *el = client->config.eventLoop;
    el->run(el, 0);

    /* The connection is re-established automatically */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(SecureChannel_reconnect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnectSecureChannel(client);

    UA_ClientConfig *cconfig = UA_Client_getConfig(client);
    UA_fakeSleep(cconfig->secureChannelLifeTime + 1);
    UA_realSleep(50 + 1);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
}
END_TEST

START_TEST(SecureChannel_cableunplugged) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Manually close the connection. The connection is internally closed at the
     * next iteration of the EventLoop. Hence the next request is sent out. But
     * the connection "actually closes" before receiving the response. */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    uintptr_t connId = client->channel.connectionId;
    cm->closeConnection(cm, connId);

    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

    UA_Client_delete(client);
}
END_TEST

int main(void) {
    TCase *tc_sc = tcase_create("Client SecureChannel");
    tcase_add_checked_fixture(tc_sc, setup, teardown);
    tcase_add_test(tc_sc, SecureChannel_renew);
    tcase_add_test(tc_sc, SecureChannel_timeout_max);
    tcase_add_test(tc_sc, SecureChannel_timeout_fail);
    tcase_add_test(tc_sc, SecureChannel_networkfail);
    tcase_add_test(tc_sc, SecureChannel_reconnect);
    tcase_add_test(tc_sc, SecureChannel_cableunplugged);

    Suite *s = suite_create("Client");
    suite_add_tcase(s, tc_sc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
