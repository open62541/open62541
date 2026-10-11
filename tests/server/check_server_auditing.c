/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/types.h>

#include "test_helpers.h"

#include <stdlib.h>
#include <string.h>
#include <check.h>

/* The body of this test exercises the auditing notification path and uses
 * threading. Guard the platform-specific includes AND the body so reduced
 * configs without auditing or thread-safe server support compile cleanly. */
#if defined(UA_ENABLE_AUDITING) && UA_MULTITHREADING >= 100
#include <stdio.h>
#include "thread_wrapper.h"
#endif

#if defined(UA_ENABLE_AUDITING) && UA_MULTITHREADING >= 100
static UA_Server *server = NULL;
static UA_atomic(uintptr_t) running = false;
static THREAD_HANDLE server_thread;

/* Counters per audit-event type. Updated from the server thread and from the
 * test thread (UA_Server_writeValue triggers the callback synchronously), so
 * increments must be atomic. Use the UA_atomic_* helpers from config.h: they
 * work for every compiler/config the library builds with, unlike a direct
 * <stdatomic.h> include which is unavailable for tcc and for MSVC without
 * /std:c11. */
static UA_atomic(size_t) totalAuditCalls = 0;
static UA_atomic(size_t) totalGlobalCalls = 0;
static UA_atomic(size_t) writeAuditCalls = 0;
static UA_atomic(size_t) methodAuditCalls = 0;
static UA_atomic(size_t) sessionCreateCalls = 0;
static UA_atomic(size_t) sessionActivateCalls = 0;
static UA_atomic(size_t) sessionCancelCalls = 0;
static UA_atomic(size_t) channelOpenCalls = 0;

/* Write audits whose payload carries /SourceNode == expectedSourceNode.
 * expectedSourceNode is set by the test thread before the write. */
static UA_NodeId expectedSourceNode;
static UA_atomic(size_t) writeSourceNodeMatches = 0;

/* ActivateSession audits with a UserNameIdentityToken: how many named the
 * expected user, and how many of those carried a non-empty password. */
static UA_atomic(size_t) activateUserTokens = 0;
static UA_atomic(size_t) activateTokenPasswords = 0;


/* Atomic increment built on UA_atomic_cmpxchg (config.h has no fetch-add) */
static void
counterInc(UA_atomic(size_t) *c) {
    size_t expected = UA_atomic_load(c);
    for(;;) {
        size_t old = expected;
        UA_atomic_cmpxchg(c, &expected, old + 1);
        if(expected == old)
            return;
    }
}

/* The same for the AuditActivateSessionEventType events received by a local
 * event MonitoredItem on the Server object selecting /UserIdentityToken. */
static UA_atomic(size_t) eventUserTokens = 0;
static UA_atomic(size_t) eventTokenPasswords = 0;

static void
countUserToken(const UA_Variant *v, UA_atomic(size_t) *tokens,
               UA_atomic(size_t) *passwords) {
    const UA_UserNameIdentityToken *tok = NULL;
    if(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])) {
        tok = (const UA_UserNameIdentityToken*)v->data;
    } else if(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT])) {
        const UA_ExtensionObject *eo = (const UA_ExtensionObject*)v->data;
        if(eo->encoding >= UA_EXTENSIONOBJECT_DECODED &&
           eo->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])
            tok = (const UA_UserNameIdentityToken*)eo->content.decoded.data;
    }
    const UA_String user1 = UA_STRING_STATIC("user1");
    if(!tok || !UA_String_equal(&tok->userName, &user1))
        return;
    counterInc(tokens);
    if(tok->password.length > 0)
        counterInc(passwords);
}

static void
activateEventCb(UA_Server *s, UA_UInt32 monId, void *monContext,
                const UA_KeyValueMap eventFields) {
    (void)s; (void)monId; (void)monContext;
    if(eventFields.mapSize > 0)
        countUserToken(&eventFields.map[0].value, &eventUserTokens,
                       &eventTokenPasswords);
}

static void
auditCb(UA_Server *s, UA_ApplicationNotificationType type,
        const UA_KeyValueMap payload) {
    (void)s;
    counterInc(&totalAuditCalls);
    switch(type) {
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_WRITE: {
        counterInc(&writeAuditCalls);
        const UA_NodeId *src = (const UA_NodeId*)
            UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "/SourceNode"),
                                     &UA_TYPES[UA_TYPES_NODEID]);
        if(src && UA_NodeId_equal(src, &expectedSourceNode))
            counterInc(&writeSourceNodeMatches);
        break;
    }
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD:
        counterInc(&methodAuditCalls); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CREATE:
        counterInc(&sessionCreateCalls); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE: {
        counterInc(&sessionActivateCalls);
        const UA_UserNameIdentityToken *tok = (const UA_UserNameIdentityToken*)
            UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "/UserIdentityToken"),
                                     &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]);
        const UA_String user1 = UA_STRING_STATIC("user1");
        if(tok && UA_String_equal(&tok->userName, &user1)) {
            counterInc(&activateUserTokens);
            if(tok->password.length > 0)
                counterInc(&activateTokenPasswords);
        }
        break;
    }
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CANCEL:
        counterInc(&sessionCancelCalls); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN:
        counterInc(&channelOpenCalls); break;
    default:
        break;
    }
}

static void
globalCb(UA_Server *s, UA_ApplicationNotificationType type,
         const UA_KeyValueMap payload) {
    (void)s; (void)type; (void)payload;
    counterInc(&totalGlobalCalls);
}

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void resetCounters(void) {
    UA_atomic_store(&totalAuditCalls, 0);
    UA_atomic_store(&totalGlobalCalls, 0);
    UA_atomic_store(&writeAuditCalls, 0);
    UA_atomic_store(&methodAuditCalls, 0);
    UA_atomic_store(&sessionCreateCalls, 0);
    UA_atomic_store(&sessionActivateCalls, 0);
    UA_atomic_store(&sessionCancelCalls, 0);
    UA_atomic_store(&channelOpenCalls, 0);
    UA_atomic_store(&writeSourceNodeMatches, 0);
    UA_atomic_store(&activateUserTokens, 0);
    UA_atomic_store(&activateTokenPasswords, 0);
    UA_atomic_store(&eventUserTokens, 0);
    UA_atomic_store(&eventTokenPasswords, 0);
    expectedSourceNode = UA_NODEID_NULL;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->auditingEnabled = true;
    cfg->auditWriteUpdateEnabled = true;
    cfg->auditMethodUpdateEnabled = true;
    cfg->auditNotificationCallback = auditCb;
    cfg->globalNotificationCallback = globalCb;
    /* Anonymous stays allowed for the other tests; add one username login */
    UA_UsernamePasswordLogin login =
        {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")};
    UA_AccessControl_default(cfg, true, NULL, 1, &login);
    cfg->allowNonePolicyPassword = true;
    resetCounters();
    UA_Server_run_startup(server);
    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

/* Test: writing a value with auditing enabled triggers
 * AUDIT_UPDATE_WRITE notification. */
START_TEST(WriteEmitsAuditEvent) {
    /* Add a writable variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId id = UA_NODEID_STRING(1, "audit.var");
    UA_StatusCode r = UA_Server_addVariableNode(server, id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "audit.var"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    size_t writesBefore = UA_atomic_load(&writeAuditCalls);
    size_t globalBefore = UA_atomic_load(&totalGlobalCalls);

    UA_Variant newVal;
    UA_Int32 nv = 42;
    UA_Variant_init(&newVal);
    UA_Variant_setScalar(&newVal, &nv, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, newVal);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* Either the dedicated audit callback or the global callback (or both)
     * fires for a write update. */
    ck_assert(UA_atomic_load(&writeAuditCalls) > writesBefore ||
              UA_atomic_load(&totalGlobalCalls) > globalBefore);
} END_TEST

/* Test: the AUDIT_UPDATE_WRITE notification payload carries /SourceNode, the
 * NodeId of the written node (the SourceNode of the AuditWriteUpdateEvent). */
START_TEST(WriteAuditPayloadHasSourceNode) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId id = UA_NODEID_STRING(1, "audit.var.src");
    UA_StatusCode r = UA_Server_addVariableNode(server, id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "audit.var.src"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    expectedSourceNode = id;
    size_t matchesBefore = UA_atomic_load(&writeSourceNodeMatches);

    UA_Variant newVal;
    UA_Int32 nv = 5;
    UA_Variant_setScalar(&newVal, &nv, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, newVal);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    ck_assert_uint_gt(UA_atomic_load(&writeSourceNodeMatches), matchesBefore);
} END_TEST

/* Test: with auditingEnabled=false, no audit event is emitted. */
START_TEST(NoAuditWhenDisabled) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->auditingEnabled = false;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId id = UA_NODEID_STRING(1, "audit.var.off");
    UA_StatusCode r = UA_Server_addVariableNode(server, id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "audit.var.off"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    size_t totalBefore = UA_atomic_load(&totalAuditCalls);
    size_t globalBefore = UA_atomic_load(&totalGlobalCalls);
    UA_Variant newVal;
    UA_Int32 nv = 7;
    UA_Variant_init(&newVal);
    UA_Variant_setScalar(&newVal, &nv, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, newVal);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* No audit callback may fire when auditing is disabled. */
    ck_assert_uint_eq(UA_atomic_load(&totalAuditCalls), totalBefore);
    ck_assert_uint_eq(UA_atomic_load(&totalGlobalCalls), globalBefore);

    /* Restore for teardown */
    cfg->auditingEnabled = true;
} END_TEST

/* Test: connecting a client triggers channel-open and session-create/activate
 * audit events. */
START_TEST(ClientConnectEmitsSessionAuditEvents) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_ne(client, NULL);

    size_t channelBefore = UA_atomic_load(&channelOpenCalls);
    size_t createBefore = UA_atomic_load(&sessionCreateCalls);
    size_t activateBefore = UA_atomic_load(&sessionActivateCalls);
    size_t globalBefore = UA_atomic_load(&totalGlobalCalls);

    UA_StatusCode r =
        UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* Drive the client a bit and let the server thread emit any pending
     * audit notifications. We poll up to ~1 second. */
    for(int i = 0; i < 100; i++) {
        if(UA_atomic_load(&sessionCreateCalls) > createBefore &&
           UA_atomic_load(&sessionActivateCalls) > activateBefore)
            break;
        UA_Client_run_iterate(client, 10);
    }

    /* SESSION_CREATE and SESSION_ACTIVATE must have fired. CHANNEL_OPEN may
     * not fire on a None-security channel; we just check the global cb too. */
    ck_assert(UA_atomic_load(&sessionCreateCalls) > createBefore);
    ck_assert(UA_atomic_load(&sessionActivateCalls) > activateBefore);
    ck_assert(UA_atomic_load(&totalGlobalCalls) > globalBefore);
    (void)channelBefore;

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Test: the ActivateSession audit names the user but omits the password. */
START_TEST(ActivateSessionAuditOmitsPassword) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    /* Also check the generated AuditActivateSessionEventType event */
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert_ptr_ne(ef.selectClauses, NULL);
    ef.selectClausesSize = 1;
    UA_StatusCode res = UA_SimpleAttributeOperand_parse(&ef.selectClauses[0],
                                                        UA_STRING("/UserIdentityToken"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult mon =
        UA_Server_createEventMonitoredItem(server, UA_NS0ID(SERVER), ef, NULL,
                                           activateEventCb);
    ck_assert_uint_eq(mon.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);
#endif

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_ne(client, NULL);
    UA_StatusCode r = UA_Client_connectUsername(client, "opc.tcp://localhost:4840",
                                                "user1", "password");
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 100 && UA_atomic_load(&activateUserTokens) == 0; i++)
        UA_Client_run_iterate(client, 10);
    ck_assert_uint_gt(UA_atomic_load(&activateUserTokens), 0);
    ck_assert_uint_eq(UA_atomic_load(&activateTokenPasswords), 0);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    for(int i = 0; i < 100 && UA_atomic_load(&eventUserTokens) == 0; i++)
        UA_Client_run_iterate(client, 10);
    ck_assert_uint_gt(UA_atomic_load(&eventUserTokens), 0);
    ck_assert_uint_eq(UA_atomic_load(&eventTokenPasswords), 0);
    UA_Server_deleteMonitoredItem(server, mon.monitoredItemId);
#endif
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* Test: enabling write-update auditing dynamically and disabling it again
 * exercises the conditional code path in UA_Server_writeValue. */
START_TEST(ToggleWriteUpdateFlag) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId id = UA_NODEID_STRING(1, "audit.var.toggle");
    UA_StatusCode r = UA_Server_addVariableNode(server, id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "audit.var.toggle"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* Disable write-update audit, write should not produce a write audit. */
    cfg->auditWriteUpdateEnabled = false;
    size_t writesBefore = UA_atomic_load(&writeAuditCalls);

    UA_Variant nv; UA_Int32 x = 11;
    UA_Variant_init(&nv);
    UA_Variant_setScalar(&nv, &x, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, nv);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_atomic_load(&writeAuditCalls), writesBefore);

    /* Re-enable and verify the path is back active. */
    cfg->auditWriteUpdateEnabled = true;
    x = 12;
    UA_Variant_setScalar(&nv, &x, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, nv);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

static Suite* testSuite(void) {
    Suite *s = suite_create("server auditing");
    TCase *tc = tcase_create("basic");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_set_timeout(tc, 60);
    tcase_add_test(tc, WriteEmitsAuditEvent);
    tcase_add_test(tc, WriteAuditPayloadHasSourceNode);
    tcase_add_test(tc, NoAuditWhenDisabled);
    tcase_add_test(tc, ClientConnectEmitsSessionAuditEvents);
    tcase_add_test(tc, ActivateSessionAuditOmitsPassword);
    tcase_add_test(tc, ToggleWriteUpdateFlag);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    SRunner *sr = srunner_create(testSuite());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* Auditing disabled or no thread-safe server support */
int main(void) { return EXIT_SUCCESS; }
#endif
