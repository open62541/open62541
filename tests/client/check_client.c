/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

static void
addVariable(size_t size) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32* array = (UA_Int32*)UA_malloc(size * sizeof(UA_Int32));
    for(size_t i = 0; i < size; i++)
        array[i] = (UA_Int32)i;
    UA_Variant_setArray(&attr.value, array, size, &UA_TYPES[UA_TYPES_INT32]);

    char name[] = "my.variable";
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    UA_free(array);
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

#define VARLENGTH 16366

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Instatiate a new AccessControl plugin that knows username/pw */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize-1];
    UA_AccessControl_default(config, true, &sp->policyUri,
                             usernamePasswordsSize, usernamePasswords);
    config->allowNonePolicyPassword = true;

    UA_Server_run_startup(server);
    addVariable(VARLENGTH);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(ClientConfig_Copy){
    UA_ClientConfig dstConfig;
    memset(&dstConfig, 0, sizeof(UA_ClientConfig));
    UA_ClientConfig srcConfig;
    memset(&srcConfig, 0, sizeof(UA_ClientConfig));
    UA_ClientConfig_setDefault(&srcConfig);
    srcConfig.eventLoop->dateTime_now = UA_DateTime_now_fake;
    srcConfig.eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;

    UA_StatusCode retval = UA_ClientConfig_copy(&srcConfig, &dstConfig);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client *dstConfigClient = UA_Client_newWithConfig(&dstConfig);
    retval = UA_Client_connect(dstConfigClient, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(dstConfigClient);
    UA_Client_delete(dstConfigClient);
    UA_ApplicationDescription_clear(&srcConfig.clientDescription);
}
END_TEST

START_TEST(Client_connect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connect_username) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_endpoints) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);

    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_endpoints_empty) {
    /* Issue a getEndpoints call with empty endpointUrl.
     * Using UA_Client_getEndpoints automatically passes the client->endpointUrl as requested endpointUrl.
     * The spec says:
     * The Server should return a suitable default URL if it does not recognize the HostName in the URL.
     *
     * See https://github.com/open62541/open62541/issues/775
     */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    UA_GetEndpointsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    ck_assert(response.endpointsSize > 0);

    UA_GetEndpointsResponse_clear(&response);
    UA_GetEndpointsRequest_clear(&request);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_read) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(val.type == &UA_TYPES[UA_TYPES_INT32]);
    UA_Int32 *var = (UA_Int32*)val.data;
    for(size_t i = 0; i < VARLENGTH; i++)
        ck_assert_uint_eq((size_t)var[i], i);

    UA_Variant_clear(&val);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_renewSecureChannel) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Forward the time */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_fakeSleep((UA_UInt32)((UA_Double)cc->secureChannelLifeTime * 0.8));

    /* Make the client renew the channel */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Now read */
    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
START_TEST(Client_renewSecureChannelWithActiveSubscription) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->secureChannelLifeTime = 60000;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    /* Force the server to send keep alive responses every second to trigg
     * the client to send new publish requests. Requests from the client
     * will make the server to change to the new SecurityToken after renewal.
     */
    request.requestedPublishingInterval = 1000;
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    UA_CreateSubscriptionResponse_clear(&response);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    for(int i = 0; i < 15; ++i) {
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, true);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST
#endif

START_TEST(Client_reconnect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    // FIX THIS FOR THE AZURE BUILD

#if 0
    printf("Restart server to test reconnect\n");
    fflush(stdout);
    teardown();
    setup();

    printf("Read attribute with closed connection\n");
    fflush(stdout);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

    printf("Reconnect client \n");
    fflush(stdout);
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, UA_StatusCode_name(retval));

    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    ck_assert_int_eq(ss, UA_SESSIONSTATE_ACTIVATED);

    printf("Read attribute again\n");
    fflush(stdout);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    printf("Finish up\n");
    fflush(stdout);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_delete_without_connect) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_activateSessionClose) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionTimeout) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);

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

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);

    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionLocaleIds) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);

    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId*)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("fr");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);

    UA_QualifiedName key = {0, UA_STRING_STATIC("localeIds")};
    UA_Variant locales;
    UA_Server_getSessionAttribute(server, &server->sessions.lh_first->session.sessionId,
                                  key, &locales);

    ck_assert_uint_eq(locales.arrayLength, 2);
    UA_String *localeIds = (UA_String*)locales.data;
    ck_assert(UA_String_equal(&localeIds[0], &config->sessionLocaleIds[0]));
    ck_assert(UA_String_equal(&localeIds[1], &config->sessionLocaleIds[1]));

    UA_Client_delete(client);

    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_closes_on_server_error) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);
    UA_SecureChannel *channel = server->sessions.lh_first->session.channel;

    // send error message from server to client
    UA_TcpErrorMessage errMsg = {.error = UA_STATUSCODE_BADSECURITYCHECKSFAILED,
                                 .reason = UA_STRING_NULL};
    UA_SecureChannel_sendError(channel, &errMsg);

    // client should disconnect and close TCP connections, although err was received
    // note: if it fails to do so the tests might hang here
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, ClientConfig_Copy);
    tcase_add_test(tc_client, Client_connect);
    tcase_add_test(tc_client, Client_connect_username);
    tcase_add_test(tc_client, Client_delete_without_connect);
    tcase_add_test(tc_client, Client_endpoints);
    tcase_add_test(tc_client, Client_endpoints_empty);
    tcase_add_test(tc_client, Client_read);
    tcase_add_test(tc_client, Client_closes_on_server_error);
    suite_add_tcase(s,tc_client);
    TCase *tc_client_reconnect = tcase_create("Client Reconnect");
    tcase_add_checked_fixture(tc_client_reconnect, setup, teardown);
    tcase_add_test(tc_client_reconnect, Client_renewSecureChannel);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client_reconnect, Client_renewSecureChannelWithActiveSubscription);
#endif
    tcase_add_test(tc_client_reconnect, Client_reconnect);
    tcase_add_test(tc_client_reconnect, Client_activateSessionClose);
    tcase_add_test(tc_client_reconnect, Client_activateSessionTimeout);
    tcase_add_test(tc_client_reconnect, Client_activateSessionLocaleIds);
    suite_add_tcase(s,tc_client_reconnect);
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
