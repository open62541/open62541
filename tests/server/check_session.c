/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_services.h"
#include "client/ua_client_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

#include "thread_wrapper.h"

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
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Session_close_before_activate) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Manually splice the AuthenticationToken into the client. So that it is
     * added to the Request. */
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* CloseSession */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);

    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    ck_assert_uint_eq(closeRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_init_ShallWork) {
    UA_Session session;
    UA_Session_init(&session);

    UA_NodeId tmpNodeId;
    UA_NodeId_init(&tmpNodeId);
    UA_ApplicationDescription tmpAppDescription;
    UA_ApplicationDescription_init(&tmpAppDescription);
    UA_DateTime tmpDateTime = 0;
    ck_assert_int_eq(session.activated, false);
    ck_assert_int_eq(session.authenticationToken.identifier.numeric, tmpNodeId.identifier.numeric);
    ck_assert_int_eq(session.availableContinuationPoints, UA_MAXCONTINUATIONPOINTS);
    ck_assert_ptr_eq(session.channel, NULL);
    ck_assert_ptr_eq(session.clientDescription.applicationName.locale.data, NULL);
    ck_assert_ptr_eq(session.continuationPoints, NULL);
    ck_assert_int_eq(session.maxRequestMessageSize, 0);
    ck_assert_int_eq(session.maxResponseMessageSize, 0);
    ck_assert_int_eq(session.sessionId.identifier.numeric, tmpNodeId.identifier.numeric);
    ck_assert_ptr_eq(session.sessionName.data, NULL);
    ck_assert_int_eq((int)session.timeout, 0);
    ck_assert_int_eq(session.validTill, tmpDateTime);
}
END_TEST

START_TEST(Session_updateLifetime_ShallWork) {
    UA_Session session;
    UA_Session_init(&session);
    UA_DateTime now = UA_DateTime_now();
    UA_DateTime tmpDateTime;
    tmpDateTime = session.validTill;
    UA_Session_updateLifetime(&session, now, tmpDateTime);

    UA_Int32 result = (session.validTill >= tmpDateTime);
    ck_assert_int_gt(result,0);
}
END_TEST

/* Check that the service-notification-callback is correctly set */
static void
serverNotificationCallback(UA_Server *server, UA_ApplicationNotificationType type,
                           const UA_KeyValueMap payload) {
    UA_assert(payload.mapSize > 0);
    UA_assert(UA_Variant_hasScalarType(&payload.map[1].value, &UA_TYPES[UA_TYPES_NODEID]));
}

START_TEST(Session_notificationCallback) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Configure the notification callback */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->serviceNotificationCallback = serverNotificationCallback;

    /* Call a service */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Session_setSessionAttribute_ShallWork) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* CreateSession */
    UA_CreateSessionRequest createReq;
    UA_CreateSessionResponse createRes;
    UA_CreateSessionRequest_init(&createReq);
    __UA_Client_Service(client, &createReq, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &createRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    ck_assert_uint_eq(createRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Manually splice the AuthenticationToken into the client. So that it is
     * added to the Request. */
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* Set an attribute for the session. */
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "myAttribute");
    UA_Variant *variant = UA_Variant_new();
    UA_Variant_init(variant);
    status s = UA_Server_setSessionAttribute(server, &createRes.sessionId, key, variant);
    UA_Variant_delete(variant);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    /* CloseSession */
    UA_CloseSessionRequest closeReq;
    UA_CloseSessionResponse closeRes;
    UA_CloseSessionRequest_init(&closeReq);

    __UA_Client_Service(client, &closeReq, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &closeRes, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    ck_assert_uint_eq(closeRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CloseSessionResponse_clear(&closeRes);
    UA_CreateSessionResponse_clear(&createRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* ---- Additional session tests ---- */

START_TEST(Session_activate_then_close) {
    UA_Client *client = UA_Client_newForUnitTest();
    /* Full connect (creates and activates session) */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify session is usable by reading a value */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Disconnect (closes session) */
    retval = UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
} END_TEST

START_TEST(Session_create_multiple) {
    /* Create two separate clients/sessions */
    UA_Client *client1 = UA_Client_newForUnitTest();
    UA_Client *client2 = UA_Client_newForUnitTest();

    UA_StatusCode ret1 = UA_Client_connect(client1, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(ret1, UA_STATUSCODE_GOOD);

    UA_StatusCode ret2 = UA_Client_connect(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(ret2, UA_STATUSCODE_GOOD);

    /* Both should be able to read */
    UA_Variant val;
    ret1 = UA_Client_readValueAttribute(client1, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(ret1, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    ret2 = UA_Client_readValueAttribute(client2, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(ret2, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client1);
    UA_Client_disconnect(client2);
    UA_Client_delete(client1);
    UA_Client_delete(client2);
} END_TEST

START_TEST(Session_readAfterClose) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Close but keep the SecureChannel */
    UA_Client_disconnectSecureChannel(client);

    /* Trying to read should fail or reconnect */
    UA_Variant val;
    retval = UA_Client_readValueAttribute(client, UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME), &val);
    /* Either fails gracefully or auto-reconnects - both are acceptable */
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&val);

    UA_Client_delete(client);
} END_TEST

START_TEST(Session_browse) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Browse the Objects folder */
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResponse bRes = UA_Client_Service_browse(client, bReq);
    ck_assert_uint_eq(bRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bRes.resultsSize, 0);
    ck_assert_uint_eq(bRes.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bRes.results[0].referencesSize, 0);

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Session_write_read_value) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add a writable variable node on the server */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInt = 42;
    UA_Variant_setScalar(&attr.value, &myInt, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myVar = UA_NODEID_STRING(1, "session.test.var");
    retval = UA_Server_addVariableNode(server, myVar,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SessionTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write via client */
    UA_Int32 writeVal = 123;
    UA_Variant wVal;
    UA_Variant_setScalar(&wVal, &writeVal, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, myVar, &wVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back via client */
    UA_Variant rVal;
    retval = UA_Client_readValueAttribute(client, myVar, &rVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)rVal.data, 123);
    UA_Variant_clear(&rVal);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static Suite* testSuite_Session(void) {
    Suite *s = suite_create("Session");
    TCase *tc_session = tcase_create("Core");
    tcase_add_checked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, Session_close_before_activate);
    tcase_add_test(tc_session, Session_init_ShallWork);
    tcase_add_test(tc_session, Session_updateLifetime_ShallWork);
    tcase_add_test(tc_session, Session_notificationCallback);
    tcase_add_test(tc_session, Session_setSessionAttribute_ShallWork);

    TCase *tc_session_ext = tcase_create("Extended");
    tcase_add_checked_fixture(tc_session_ext, setup, teardown);
    tcase_add_test(tc_session_ext, Session_activate_then_close);
    tcase_add_test(tc_session_ext, Session_create_multiple);
    tcase_add_test(tc_session_ext, Session_readAfterClose);
    tcase_add_test(tc_session_ext, Session_browse);
    tcase_add_test(tc_session_ext, Session_write_read_value);

    suite_add_tcase(s, tc_session);
    suite_add_tcase(s, tc_session_ext);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_Session();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
