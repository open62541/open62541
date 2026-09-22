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

#include <check.h>

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

static void setup_channelLimit(void) {
    running = true;
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    /* Cap the number of SecureChannels so the purge path is reachable
     * deterministically without exhausting system resources. */
    UA_Server_getConfig(server)->maxSecureChannels = 2;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void setup_sessionLimit(void) {
    running = true;
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    /* Cap the number of Sessions so the over-limit path is reachable
     * deterministically without exhausting system resources. */
    UA_Server_getConfig(server)->maxSessions = 2;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
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

/* maxSecureChannels = 2: two clients open a channel and CreateSession
 * without ever calling ActivateSession. A third client must still be able
 * to open a channel -- the channel-table purge has to reclaim a channel
 * that only carries an un-activated session instead of treating it as
 * pinned (purgeFirstChannelWithoutSession). */
START_TEST(Session_unactivated_doesNotPinChannel) {
    UA_Client *c1 = UA_Client_new();
    UA_Client *c2 = UA_Client_new();
    UA_Client *c3 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(c1));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c2));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c3));

    ck_assert_uint_eq(UA_Client_connectSecureChannel(c1, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c2, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionRequest req;
    UA_CreateSessionRequest_init(&req);
    UA_CreateSessionResponse r1, r2, r3;
    UA_CreateSessionResponse_init(&r1);
    UA_CreateSessionResponse_init(&r2);
    UA_CreateSessionResponse_init(&r3);

    /* Both channels now carry an un-activated session */
    __UA_Client_Service(c1, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r1, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r1.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    __UA_Client_Service(c2, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r2, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r2.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* maxSecureChannels is reached. A new channel must still be obtainable
     * by purging one of the two above, since neither session was
     * activated. Drive the async connect by hand with a bounded number of
     * iterations: without the fix, the server never accepts the channel
     * and each rejected TCP connection is immediately retried, so the
     * synchronous connect call would otherwise not return within its
     * configured timeout. */
    ck_assert_uint_eq(UA_Client_connectAsync(c3, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    UA_SecureChannelState channelState = UA_SECURECHANNELSTATE_CLOSED;
    for(size_t i = 0; i < 40; i++) {
        UA_Client_run_iterate(c3, 50);
        UA_Client_getState(c3, &channelState, NULL, NULL);
        if(channelState == UA_SECURECHANNELSTATE_OPEN)
            break;
    }
    ck_assert_uint_eq(channelState, UA_SECURECHANNELSTATE_OPEN);
    __UA_Client_Service(c3, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r3, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r3.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse_clear(&r3);
    UA_CreateSessionResponse_clear(&r2);
    UA_CreateSessionResponse_clear(&r1);
    UA_Client_disconnect(c1); UA_Client_delete(c1);
    UA_Client_disconnect(c2); UA_Client_delete(c2);
    UA_Client_disconnect(c3); UA_Client_delete(c3);
} END_TEST

/* maxSessions = 2: with two un-activated Sessions already open, a third
 * CreateSession must still succeed by reclaiming the oldest one instead of
 * being rejected with BADTOOMANYSESSIONS (purgeOldestUnactivatedSession() in
 * ua_services_session.c). The reclaimed Session is gone for good: its
 * authenticationToken no longer resolves to anything. */
START_TEST(Session_maxSessions_limit_purgesOldestUnactivated) {
    UA_Client *c1 = UA_Client_new();
    UA_Client *c2 = UA_Client_new();
    UA_Client *c3 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(c1));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c2));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c3));

    ck_assert_uint_eq(UA_Client_connectSecureChannel(c1, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c2, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c3, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionRequest req;
    UA_CreateSessionRequest_init(&req);
    UA_CreateSessionResponse r1, r2, r3;
    UA_CreateSessionResponse_init(&r1);
    UA_CreateSessionResponse_init(&r2);
    UA_CreateSessionResponse_init(&r3);

    /* Two un-activated Sessions fill the limit */
    __UA_Client_Service(c1, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r1, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r1.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    __UA_Client_Service(c2, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r2, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r2.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* A third Session is still obtainable by reclaiming the oldest (c1's)
     * un-activated Session */
    __UA_Client_Service(c3, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r3, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r3.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* c1's Session was purged: its authenticationToken no longer resolves */
    UA_NodeId_copy(&r1.authenticationToken, &c1->authenticationToken);
    UA_ActivateSessionRequest areq;
    UA_ActivateSessionRequest_init(&areq);
    UA_ActivateSessionResponse ares;
    UA_ActivateSessionResponse_init(&ares);
    __UA_Client_Service(c1, &areq, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &ares, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    ck_assert_uint_eq(ares.responseHeader.serviceResult, UA_STATUSCODE_BADSESSIONIDINVALID);
    UA_ActivateSessionResponse_clear(&ares);

    UA_CreateSessionResponse_clear(&r3);
    UA_CreateSessionResponse_clear(&r2);
    UA_CreateSessionResponse_clear(&r1);
    UA_Client_disconnect(c1); UA_Client_delete(c1);
    UA_Client_disconnect(c2); UA_Client_delete(c2);
    UA_Client_disconnect(c3); UA_Client_delete(c3);
} END_TEST

/* maxSessions = 2: once every existing Session has been activated, none of
 * them can be reclaimed and a third CreateSession must still fail with
 * BADTOOMANYSESSIONS — the limit is not silently disabled by the purge. */
START_TEST(Session_maxSessions_limit_rejectsWhenAllActivated) {
    UA_Client *c1 = UA_Client_new();
    UA_Client *c2 = UA_Client_new();
    UA_Client *c3 = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(c1));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c2));
    UA_ClientConfig_setDefault(UA_Client_getConfig(c3));

    ck_assert_uint_eq(UA_Client_connectSecureChannel(c1, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c2, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c3, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionRequest req;
    UA_CreateSessionRequest_init(&req);
    UA_CreateSessionResponse r1, r2, r3;
    UA_CreateSessionResponse_init(&r1);
    UA_CreateSessionResponse_init(&r2);
    UA_CreateSessionResponse_init(&r3);

    __UA_Client_Service(c1, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r1, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r1.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&r1.authenticationToken, &c1->authenticationToken);

    UA_ActivateSessionRequest areq1;
    UA_ActivateSessionRequest_init(&areq1);
    UA_ActivateSessionResponse ares1;
    UA_ActivateSessionResponse_init(&ares1);
    __UA_Client_Service(c1, &areq1, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &ares1, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    ck_assert_uint_eq(ares1.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&ares1);

    __UA_Client_Service(c2, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r2, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r2.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&r2.authenticationToken, &c2->authenticationToken);

    UA_ActivateSessionRequest areq2;
    UA_ActivateSessionRequest_init(&areq2);
    UA_ActivateSessionResponse ares2;
    UA_ActivateSessionResponse_init(&ares2);
    __UA_Client_Service(c2, &areq2, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &ares2, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    ck_assert_uint_eq(ares2.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&ares2);

    __UA_Client_Service(c3, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &r3, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    ck_assert_uint_eq(r3.responseHeader.serviceResult, UA_STATUSCODE_BADTOOMANYSESSIONS);

    UA_CreateSessionResponse_clear(&r3);
    UA_CreateSessionResponse_clear(&r2);
    UA_CreateSessionResponse_clear(&r1);
    UA_Client_disconnect(c1); UA_Client_delete(c1);
    UA_Client_disconnect(c2); UA_Client_delete(c2);
    UA_Client_disconnect(c3); UA_Client_delete(c3);
} END_TEST

static Suite* testSuite_Session(void) {
    Suite *s = suite_create("Session");
    TCase *tc_session = tcase_create("Core");
    tcase_add_checked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, Session_close_before_activate);
    tcase_add_test(tc_session, Session_init_ShallWork);
    tcase_add_test(tc_session, Session_updateLifetime_ShallWork);
    suite_add_tcase(s,tc_session);

    TCase *tc_channelLimit = tcase_create("channel purge");
    tcase_add_checked_fixture(tc_channelLimit, setup_channelLimit, teardown);
    tcase_add_test(tc_channelLimit, Session_unactivated_doesNotPinChannel);
    suite_add_tcase(s, tc_channelLimit);

    TCase *tc_sessionLimit = tcase_create("session purge");
    tcase_add_checked_fixture(tc_sessionLimit, setup_sessionLimit, teardown);
    tcase_add_test(tc_sessionLimit, Session_maxSessions_limit_purgesOldestUnactivated);
    tcase_add_test(tc_sessionLimit, Session_maxSessions_limit_rejectsWhenAllActivated);
    suite_add_tcase(s, tc_sessionLimit);

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
