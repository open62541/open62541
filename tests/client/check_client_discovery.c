/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "thread_wrapper.h"
#include "testing_clock.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;


THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_connect_badEndpointUrl) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Use the internal API to force a bad DiscoveryUrl */
    UA_String_clear(&client->config.endpointUrl);
    UA_String_clear(&client->discoveryUrl);
    client->config.endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    client->discoveryUrl = UA_STRING_ALLOC("abc://xxx:4840");

    /* Open a Session when possible */
    client->config.noSession = false;

    lockClient(client);
    connectSync(client);
    unlockClient(client);
    ck_assert_uint_eq(client->connectStatus, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static void
removeEndpoints(void) {
    lockServer(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    for(size_t i = 0; i < config->endpointsSize; i++)
        UA_EndpointDescription_clear(&config->endpoints[i]);
    UA_free(config->endpoints);
    config->endpoints = NULL;
    config->endpointsSize = 0;
    unlockServer(server);
}

static void
findServers(UA_Client *client) {
    size_t serverCount = 0;
    UA_ApplicationDescription *servers = NULL;
    UA_StatusCode retval = UA_Client_findServers(client,
                                "opc.tcp://localhost:4840",
                                0, NULL, 0, NULL,
                                &serverCount, &servers);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(serverCount > 0);
    UA_Array_delete(servers, serverCount,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
}

static void
checkConnected(UA_Client *client, UA_Boolean withSession) {
    /* Bound asynchronous connection attempts even when the fake clock stops. */
    for(size_t i = 0; i < 1000 && !isFullyConnected(client); i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10), UA_STATUSCODE_GOOD);

    ck_assert(isFullyConnected(client));
    ck_assert_uint_eq(client->channel.state, UA_SECURECHANNELSTATE_OPEN);
    ck_assert(client->endpoint.endpointUrl.length > 0);
    ck_assert_uint_eq(client->sessionState, withSession ?
                      UA_SESSIONSTATE_ACTIVATED : UA_SESSIONSTATE_CLOSED);
    if(withSession) {
        UA_Variant value;
        UA_Variant_init(&value);
        UA_StatusCode retval = UA_Client_readValueAttribute(client,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &value);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant_clear(&value);
    }
}

START_TEST(Client_findServers_withoutEndpoints) {
    /* Discovery must work even when no session endpoint is offered. */
    removeEndpoints();
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    findServers(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_getEndpoints_withoutEndpoints) {
    removeEndpoints();
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    size_t endpointsSize = 0;
    UA_EndpointDescription *endpoints = NULL;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointsSize, &endpoints);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(endpointsSize, 0);
    UA_Array_delete(endpoints, endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connect_afterDiscovery) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    findServers(client);

    /* Every public connect entry point must resume endpoint selection. */
    UA_StatusCode retval;
    switch(_i) {
    case 0:
        retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        break;
    case 1:
        retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
        break;
    case 2:
        retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
        break;
    default:
        retval = UA_Client_connectSecureChannelAsync(client, "opc.tcp://localhost:4840");
        break;
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    checkConnected(client, _i < 2);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_reverseConnect_afterDiscovery) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    findServers(client);

    /* Reverse connect uses the configured Session preference. */
    config->noSession = false;
    UA_String listenHost = UA_STRING("127.0.0.1");
    UA_StatusCode retval =
        UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt64 handle = 0;
    retval = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                         NULL, NULL, &handle);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    checkConnected(client, true);
    ck_assert_uint_eq(UA_Server_removeReverseConnect(server, handle), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
}
END_TEST

static void
discoveryTimeout(void *application, void *data) {
    *(UA_Boolean*)data = true;
    UA_fakeSleep(1000);
}

START_TEST(Client_findServers_timeoutRecovery) {
    /* Keep the listening socket, but do not answer the handshake. */
    running = false;
    THREAD_JOIN(server_thread);

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    config->timeout = 100;
    UA_Boolean fired = false;
    UA_StatusCode retval = config->eventLoop->addTimedCallback(config->eventLoop,
        discoveryTimeout, NULL, &fired, UA_DateTime_nowMonotonic(), NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t serverCount = 0;
    UA_ApplicationDescription *servers = NULL;
    retval = UA_Client_findServers(client, "opc.tcp://localhost:4840",
                                  0, NULL, 0, NULL, &serverCount, &servers);
    UA_Array_delete(servers, serverCount, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
    ck_assert(fired);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTIMEOUT);
    ck_assert_uint_eq(client->channel.state, UA_SECURECHANNELSTATE_CLOSED);

    /* Recover directly, without an explicit disconnect or a new client. */
    config->timeout = 5000;
    if(_i == 0) {
        findServers(client);
    } else {
        retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        checkConnected(client, true);
    }
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Discovery");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_connect_badEndpointUrl);
    tcase_add_test(tc_client, Client_findServers_withoutEndpoints);
    tcase_add_test(tc_client, Client_getEndpoints_withoutEndpoints);
    tcase_add_loop_test(tc_client, Client_connect_afterDiscovery, 0, 4);
    tcase_add_test(tc_client, Client_reverseConnect_afterDiscovery);
    tcase_add_loop_test(tc_client, Client_findServers_timeoutRecovery, 0, 2);
    suite_add_tcase(s,tc_client);
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
