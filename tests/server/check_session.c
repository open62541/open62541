/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "client/ua_client_internal.h"

#include <check.h>

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
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
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
    ck_assert_int_eq(session.header.authenticationToken.identifier.numeric, tmpNodeId.identifier.numeric);
    ck_assert_int_eq(session.availableContinuationPoints, UA_MAXCONTINUATIONPOINTS);
    ck_assert_ptr_eq(session.header.channel, NULL);
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
    UA_DateTime tmpDateTime;
    tmpDateTime = session.validTill;
    UA_Session_updateLifetime(&session);

    UA_Int32 result = (session.validTill >= tmpDateTime);
    ck_assert_int_gt(result,0);
}
END_TEST

START_TEST(Session_getSessions) { 
    // first client
    UA_Client* client1 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client1));
    UA_StatusCode retval1 = UA_Client_connect(client1, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval1, UA_STATUSCODE_GOOD);

    // second client
    UA_Client* client2 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client2));
    UA_StatusCode retval2 = UA_Client_connect(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval2, UA_STATUSCODE_GOOD);

    UA_LOCK(&server->serviceMutex);
    const UA_Session* session = UA_Server_GetFirstSession(server);
    ck_assert_ptr_eq(session, &server->sessions.lh_first->session);
    UA_UNLOCK(&server->serviceMutex);

    UA_LOCK(&server->serviceMutex);
    const UA_Session* session2 = UA_Server_GetNextSession(server, session);
    session_list_entry* currentSessionListEntry = server->sessions.lh_first;
    ck_assert_ptr_eq(session2, &(LIST_NEXT(currentSessionListEntry, pointers)->session));
    UA_UNLOCK(&server->serviceMutex);

    // disconnect the first client
    UA_Client_disconnect(client1);
    UA_Client_delete(client1);

    // check if the activeSessionCount get updated 
    ck_assert_int_eq(server->activeSessionCount,1);

    // disconnect the second client
    UA_Client_disconnect(client2);
    UA_Client_delete(client2);

    // check if the activeSessionCount get updated 
    ck_assert_int_eq(server->activeSessionCount,0);

} END_TEST

static Suite* testSuite_Session(void) {
    Suite *s = suite_create("Session");
    TCase *tc_session = tcase_create("Core");
    tcase_add_checked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, Session_close_before_activate);
    tcase_add_test(tc_session, Session_init_ShallWork);
    tcase_add_test(tc_session, Session_updateLifetime_ShallWork);
    tcase_add_test(tc_session, Session_getSessions);
    suite_add_tcase(s,tc_session);
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
