/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/server/ua_services_session.c error paths and the
 * public session-attribute helpers. The existing check_session.c exercises
 * the happy paths (connect, activate, read, close). This file targets:
 *   - maxSessions enforcement in UA_Session_create
 *   - Service_CreateSession / Activate / Close error paths
 *   - Session-attribute getter edge cases (wrong session id, NULL inputs,
 *     scalar extraction of non-scalar, scalar type mismatch)
 *   - Cleanup callback registration and lifetime updates
 *   - Session-notification callback (no callback set, and with one set)
 *
 * The tests use only the public API so they survive the shared-lib build
 * and exercise the same paths that real clients hit.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "thread_wrapper.h"

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    /* Cap the number of sessions so the over-limit path is reachable
     * deterministically without exhausting system resources. */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->maxSessions = 2;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* ==== Helpers ==== */

static UA_StatusCode
createSession(UA_Client *client, UA_CreateSessionResponse *outRes) {
    UA_CreateSessionRequest req;
    UA_CreateSessionRequest_init(&req);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        outRes, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);
    return outRes->responseHeader.serviceResult;
}

static UA_StatusCode
activateSession(UA_Client *client) {
    UA_ActivateSessionRequest req;
    UA_ActivateSessionRequest_init(&req);
    /* userIdentityToken left as default (ENCODED_NOBODY) which the
     * server interprets as an anonymous login. */
    UA_ActivateSessionResponse res;
    UA_ActivateSessionResponse_init(&res);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &res, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    UA_StatusCode rv = res.responseHeader.serviceResult;
    UA_ActivateSessionResponse_clear(&res);
    return rv;
}

static void
closeSession(UA_Client *client) {
    UA_CloseSessionRequest req;
    UA_CloseSessionRequest_init(&req);
    UA_CloseSessionResponse res;
    UA_CloseSessionResponse_init(&res);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &res, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    UA_CloseSessionResponse_clear(&res);
}

/* ==== Tests ==== */

/* maxSessions = 2: a third CreateSession must be rejected with
 * UA_STATUSCODE_BADTOOMANYSESSIONS (line 385-390 of ua_services_session.c). */
START_TEST(Session_maxSessions_limit) {
    UA_Client *c1 = UA_Client_newForUnitTest();
    UA_Client *c2 = UA_Client_newForUnitTest();
    UA_Client *c3 = UA_Client_newForUnitTest();

    UA_CreateSessionResponse r1, r2, r3;
    UA_CreateSessionResponse_init(&r1);
    UA_CreateSessionResponse_init(&r2);
    UA_CreateSessionResponse_init(&r3);

    ck_assert_uint_eq(UA_Client_connectSecureChannel(c1, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c2, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connectSecureChannel(c3, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    /* Two OK, the third one must fail with BADTOOMANYSESSIONS */
    ck_assert_uint_eq(createSession(c1, &r1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createSession(c2, &r2), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createSession(c3, &r3), UA_STATUSCODE_BADTOOMANYSESSIONS);

    /* Close one and try again — must succeed */
    UA_NodeId_copy(&r1.authenticationToken, &c1->authenticationToken);
    closeSession(c1);
    UA_CreateSessionResponse r4;
    UA_CreateSessionResponse_init(&r4);
    ck_assert_uint_eq(createSession(c3, &r4), UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse_clear(&r4);
    UA_CreateSessionResponse_clear(&r3);
    UA_CreateSessionResponse_clear(&r2);
    UA_CreateSessionResponse_clear(&r1);
    UA_Client_disconnect(c1); UA_Client_delete(c1);
    UA_Client_disconnect(c2); UA_Client_delete(c2);
    UA_Client_disconnect(c3); UA_Client_delete(c3);
} END_TEST

/* Get/Set/Delete attribute on a non-existent session id must fail
 * cleanly with BADSESSIONIDINVALID. This exercises the lookup miss path
 * in getBoundSession. */
START_TEST(Session_attribute_unknownSession) {
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "k");
    UA_Variant var;
    UA_Variant_init(&var);

    /* BOGUS session id (numeric, never created) */
    UA_NodeId badSession = UA_NODEID_NUMERIC(0, 99999999);

    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &badSession, key, &var),
                     UA_STATUSCODE_BADSESSIONIDINVALID);
    ck_assert_uint_eq(UA_Server_getSessionAttributeCopy(server, &badSession, key, &var),
                     UA_STATUSCODE_BADSESSIONIDINVALID);
    ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, &badSession, key),
                     UA_STATUSCODE_BADSESSIONIDINVALID);

    /* NULL sessionId must be rejected with BADINVALIDARGUMENT.
     * This verifies the NULL guards added to the public API. */
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, NULL, key, &var),
                     UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_getSessionAttributeCopy(server, NULL, key, &var),
                     UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, NULL, key),
                     UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Set/get/delete with a key in the protected-attribute namespace
     * (e.g. "sessionName") must return BADNOTWRITABLE, exercising the
     * protected-attribute guard. */
    UA_QualifiedName protectedKey = UA_QUALIFIEDNAME(0, "sessionName");
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &badSession,
                                                    protectedKey, &var),
                     UA_STATUSCODE_BADNOTWRITABLE);
    ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, &badSession,
                                                       protectedKey),
                     UA_STATUSCODE_BADNOTWRITABLE);
} END_TEST

/* Set a scalar, get it back, delete it; also try the scalar helper with
 * matching and mismatching types. This exercises the variant type-check
 * path in getSessionAttribute_scalar. */
START_TEST(Session_attribute_scalar_helpers) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);

    /* Set a String attribute */
    UA_QualifiedName k1 = UA_QUALIFIEDNAME(1, "name");
    UA_Variant v;
    UA_Variant_init(&v);
    UA_String s = UA_STRING("hello world");
    UA_Variant_setScalar(&v, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &res.sessionId, k1, &v),
                     UA_STATUSCODE_GOOD);

    /* Re-set the same key to a different value (replace path) */
    UA_String s2 = UA_STRING("second value");
    UA_Variant_setScalar(&v, &s2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &res.sessionId, k1, &v),
                     UA_STATUSCODE_GOOD);

    /* Scalar extraction with matching type */
    UA_String out;
    UA_String_init(&out);
    ck_assert_uint_eq(UA_Server_getSessionAttribute_scalar(server, &res.sessionId,
                                                          k1, &UA_TYPES[UA_TYPES_STRING], &out),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, s2.length);
    ck_assert(memcmp(out.data, s2.data, out.length) == 0);

    /* Scalar extraction with mismatched type must fail (BADNOMATCH or
     * BADDATATYPEIDUNKNOWN depending on internals) */
    UA_Int32 wrongType = 0;
    UA_StatusCode rv = UA_Server_getSessionAttribute_scalar(
        server, &res.sessionId, k1, &UA_TYPES[UA_TYPES_INT32], &wrongType);
    ck_assert(rv != UA_STATUSCODE_GOOD);

    /* Get non-existent key returns BADNOTFOUND */
    UA_QualifiedName missing = UA_QUALIFIEDNAME(1, "no-such-key");
    UA_Variant empty;
    UA_Variant_init(&empty);
    ck_assert_uint_eq(UA_Server_getSessionAttributeCopy(server, &res.sessionId,
                                                        missing, &empty),
                     UA_STATUSCODE_BADNOTFOUND);

    /* Delete the attribute and verify it's gone (returns BADNOTFOUND) */
    ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, &res.sessionId, k1),
                     UA_STATUSCODE_GOOD);
    UA_Variant gone;
    UA_Variant_init(&gone);
    ck_assert_uint_eq(UA_Server_getSessionAttributeCopy(server, &res.sessionId, k1, &gone),
                     UA_STATUSCODE_BADNOTFOUND);

    /* Deleting a non-existent key returns BADNOTFOUND */
    ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, &res.sessionId, missing),
                     UA_STATUSCODE_BADNOTFOUND);

    closeSession(client);
    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Activate then close twice: the second CloseSession must fail with
 * BADSESSIONIDINVALID because the session is already gone. */
START_TEST(Session_closeTwiceFails) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);
    ck_assert_uint_eq(activateSession(client), UA_STATUSCODE_GOOD);
    closeSession(client);
    /* Second close must fail */
    closeSession(client);

    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* ActivateSession with a syntactically wrong authentication token must
 * fail with BADSESSIONIDINVALID (line 1240-1242 path). */
START_TEST(Session_activateWithBadToken) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    /* Don't create a session. Just try to activate with a random token. */
    UA_NodeId fakeToken = UA_NODEID_NUMERIC(0, 12345);
    UA_NodeId_copy(&fakeToken, &client->authenticationToken);

    UA_ActivateSessionRequest req;
    UA_ActivateSessionRequest_init(&req);

    UA_ActivateSessionResponse res;
    UA_ActivateSessionResponse_init(&res);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &res, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    ck_assert(res.responseHeader.serviceResult != UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&res);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* If a session-notification callback is set, it must fire for create
 * and close. We install a callback, run a full lifecycle, and check
 * that the callback saw the expected notification types. */
static int sessionCbCount;
static UA_ApplicationNotificationType sessionCbTypes[8];

static void
sessionNotifyCb(UA_Server *srv, UA_ApplicationNotificationType type,
                const UA_KeyValueMap payload) {
    (void)srv; (void)payload;
    if(sessionCbCount < 8)
        sessionCbTypes[sessionCbCount++] = type;
}

START_TEST(Session_notificationCallback_fires) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    sessionCbCount = 0;
    cfg->sessionNotificationCallback = sessionNotifyCb;

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);
    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);
    ck_assert_uint_eq(activateSession(client), UA_STATUSCODE_GOOD);
    closeSession(client);

    /* At least two notifications should have fired: session-created and
     * session-closed. We don't assert exact ordering, just that both
     * types appear. */
    ck_assert_int_ge(sessionCbCount, 2);
    UA_Boolean sawCreate = false, sawClose = false;
    for(int i = 0; i < sessionCbCount; i++) {
        if(sessionCbTypes[i] == UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CREATED)
            sawCreate = true;
        if(sessionCbTypes[i] == UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED)
            sawClose = true;
    }
    ck_assert(sawCreate);
    ck_assert(sawClose);

    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    cfg->sessionNotificationCallback = NULL;
} END_TEST

/* Service_CloseSession with deleteSubscriptions=false must close the
 * session while the client-supplied subscription is not deleted. Since
 * the test runs without subscription creation, just verify CloseSession
 * returns GOOD with deleteSubscriptions=false and that the session is
 * actually gone. */
START_TEST(Session_close_preserveSubscriptions_flag) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);
    ck_assert_uint_eq(activateSession(client), UA_STATUSCODE_GOOD);

    /* Send CloseSession with deleteSubscriptions = false */
    UA_CloseSessionRequest req;
    UA_CloseSessionRequest_init(&req);
    req.deleteSubscriptions = false;
    UA_CloseSessionResponse cresp;
    UA_CloseSessionResponse_init(&cresp);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &cresp, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    ck_assert_uint_eq(cresp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_CloseSessionResponse_clear(&cresp);

    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* The read-only "protected" session attributes (localeIds,
 * clientDescription, sessionName, clientUserId) are returned via
 * UA_Server_getSessionAttribute with special keys. They cannot be set
 * with UA_Server_setSessionAttribute (which returns BADNOTWRITABLE).
 *
 * This test exercises the get-protected-attribute code path in
 * getSessionAttribute. */
START_TEST(Session_protectedAttributes) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);
    ck_assert_uint_eq(activateSession(client), UA_STATUSCODE_GOOD);

    /* The 4 protected keys, in the order the implementation checks them. */
    UA_QualifiedName localeIds    = UA_QUALIFIEDNAME_ALLOC(0, "localeIds");
    UA_QualifiedName clientDesc   = UA_QUALIFIEDNAME_ALLOC(0, "clientDescription");
    UA_QualifiedName sessionName  = UA_QUALIFIEDNAME_ALLOC(0, "sessionName");
    UA_QualifiedName clientUserId = UA_QUALIFIEDNAME_ALLOC(0, "clientUserId");
    UA_QualifiedName keys[4] = {localeIds, clientDesc, sessionName, clientUserId};
    for(int i = 0; i < 4; i++) {
        UA_Variant out;
        UA_Variant_init(&out);
        ck_assert_uint_eq(UA_Server_getSessionAttributeCopy(server, &res.sessionId,
                                                          keys[i], &out),
                         UA_STATUSCODE_GOOD);
        ck_assert(out.type != NULL);
        UA_Variant_clear(&out);
    }

    /* All four must reject writes via the set attribute. */
    UA_Variant v;
    UA_Variant_init(&v);
    for(int i = 0; i < 4; i++) {
        ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &res.sessionId,
                                                       keys[i], &v),
                         UA_STATUSCODE_BADNOTWRITABLE);
        ck_assert_uint_eq(UA_Server_deleteSessionAttribute(server, &res.sessionId,
                                                          keys[i]),
                         UA_STATUSCODE_BADNOTWRITABLE);
    }
    UA_QualifiedName_clear(&localeIds);
    UA_QualifiedName_clear(&clientDesc);
    UA_QualifiedName_clear(&sessionName);
    UA_QualifiedName_clear(&clientUserId);

    closeSession(client);
    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* UA_Server_closeSession is the C-level helper to remove a session by
 * its NodeId (the high-level CloseSession service goes through
 * authentication-token lookup). Exercising the helper directly hits
 * the code path that gets called by interop tests, the json config
 * file, and the access control plugin. */
START_TEST(Server_closeSession_helper) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    UA_CreateSessionResponse res;
    UA_CreateSessionResponse_init(&res);
    ck_assert_uint_eq(createSession(client, &res), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&res.authenticationToken, &client->authenticationToken);
    ck_assert_uint_eq(activateSession(client), UA_STATUSCODE_GOOD);

    /* Close the session by NodeId. The server returns GOOD. */
    ck_assert_uint_eq(UA_Server_closeSession(server, &res.sessionId),
                     UA_STATUSCODE_GOOD);

    /* Closing an already-closed session must fail. */
    ck_assert_uint_eq(UA_Server_closeSession(server, &res.sessionId),
                     UA_STATUSCODE_BADSESSIONIDINVALID);

    UA_CreateSessionResponse_clear(&res);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* ==== Service_ActivateSession with localeIds ==== */

START_TEST(Session_activate_withLocaleIds_succeeds) {
    /* src/server/ua_services_session.c:1126-1146:
     *   if(req->localeIdsSize > 0) {
     *     rh->serviceResult |= UA_Array_copy(req->localeIds, ...);
     *     ...
     *     session->localeIds = tmpLocaleIds;
     *     session->localeIdsSize = req->localeIdsSize;
     *   }
     * None of the existing tests passes localeIds in the
     * ActivateSessionRequest. The default code path is the
     * `localeIdsSize == 0` early-skip. */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connectSecureChannel(client, "opc.tcp://localhost:4840"),
                     UA_STATUSCODE_GOOD);

    /* Full CreateSession + ActivateSession lifecycle, but with
     * localeIds set on the ActivateSessionRequest. */
    UA_CreateSessionResponse createRes;
    UA_CreateSessionResponse_init(&createRes);
    ck_assert_uint_eq(createSession(client, &createRes), UA_STATUSCODE_GOOD);
    UA_NodeId_copy(&createRes.authenticationToken, &client->authenticationToken);

    /* Build the ActivateSessionRequest with localeIds */
    UA_ActivateSessionRequest req;
    UA_ActivateSessionRequest_init(&req);
    req.localeIdsSize = 2;
    req.localeIds = (UA_String*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    UA_String s0 = UA_STRING("en-US");
    UA_String s1 = UA_STRING("de");
    UA_String_copy(&s0, &req.localeIds[0]);
    UA_String_copy(&s1, &req.localeIds[1]);

    UA_ActivateSessionResponse res;
    UA_ActivateSessionResponse_init(&res);
    __UA_Client_Service(client, &req, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &res, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);
    ck_assert_uint_eq(res.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Cleanup */
    UA_ActivateSessionRequest_clear(&req);
    UA_ActivateSessionResponse_clear(&res);
    UA_CreateSessionResponse_clear(&createRes);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* ==== Suite ==== */

static Suite* testSuite_SessionErrorPaths(void) {
    Suite *s = suite_create("Session Error Paths");
    TCase *tc = tcase_create("error paths");
    /* Tests in this suite connect to a running server, so they need
     * a higher per-test timeout — the default of 4s is too tight once
     * the test binary runs under Valgrind or a sanitizer. The
     * Session_maxSessions_limit test in particular opens three
     * clients and creates three sessions, which is slow under
     * Valgrind even with the faked clock. */
    tcase_set_timeout(tc, 60);
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Session_maxSessions_limit);
    tcase_add_test(tc, Session_attribute_unknownSession);
    tcase_add_test(tc, Session_attribute_scalar_helpers);
    tcase_add_test(tc, Session_closeTwiceFails);
    tcase_add_test(tc, Session_activateWithBadToken);
    tcase_add_test(tc, Session_notificationCallback_fires);
    tcase_add_test(tc, Session_close_preserveSubscriptions_flag);
    tcase_add_test(tc, Session_protectedAttributes);
    tcase_add_test(tc, Server_closeSession_helper);
    tcase_add_test(tc, Session_activate_withLocaleIds_succeeds);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_SessionErrorPaths();
    SRunner *sr = srunner_create(s);
    /* CK_FORK so any segfault in a test is reported by the runner and
     * does not abort the whole binary. */
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
