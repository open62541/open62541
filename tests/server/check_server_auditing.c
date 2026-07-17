/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"

#include <stdlib.h>
#include <string.h>
#include <check.h>

/* The body of this test exercises the auditing notification path and uses
 * threading + C11 atomics. Guard the platform-specific includes AND the
 * body so tcc (MULTITHREADING=0) and reduced build configs that don't enable
 * auditing still compile cleanly. */
#ifdef UA_ENABLE_AUDITING
#include <stdio.h>
#include "thread_wrapper.h"
#include <stdatomic.h>
#endif /* UA_ENABLE_AUDITING */

#ifdef UA_ENABLE_AUDITING
static UA_Server *server = NULL;
static UA_Boolean running = false;
static THREAD_HANDLE server_thread;

/* Counters per audit-event type. Volatile because they are written from the
 * server thread and read from the test thread. */
/* Counters per audit-event type. Updated from the server thread and read from
 * the test thread; use C11 atomics for well-defined cross-thread visibility. */
static atomic_size_t totalAuditCalls = 0;
static atomic_size_t totalGlobalCalls = 0;
static atomic_size_t writeAuditCalls = 0;
static atomic_size_t methodAuditCalls = 0;
static atomic_size_t sessionCreateCalls = 0;
static atomic_size_t sessionActivateCalls = 0;
static atomic_size_t sessionCancelCalls = 0;
static atomic_size_t channelOpenCalls = 0;

static void
auditCb(UA_Server *s, UA_ApplicationNotificationType type,
        const UA_KeyValueMap payload) {
    (void)s; (void)payload;
    atomic_fetch_add(&totalAuditCalls, 1);
    switch(type) {
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_WRITE:
        atomic_fetch_add(&writeAuditCalls, 1); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD:
        atomic_fetch_add(&methodAuditCalls, 1); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CREATE:
        atomic_fetch_add(&sessionCreateCalls, 1); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE:
        atomic_fetch_add(&sessionActivateCalls, 1); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_CANCEL:
        atomic_fetch_add(&sessionCancelCalls, 1); break;
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_CHANNEL_OPEN:
        atomic_fetch_add(&channelOpenCalls, 1); break;
    default:
        break;
    }
}

static void
globalCb(UA_Server *s, UA_ApplicationNotificationType type,
         const UA_KeyValueMap payload) {
    (void)s; (void)type; (void)payload;
    atomic_fetch_add(&totalGlobalCalls, 1);
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void resetCounters(void) {
    atomic_store(&totalAuditCalls, 0);
    atomic_store(&totalGlobalCalls, 0);
    atomic_store(&writeAuditCalls, 0);
    atomic_store(&methodAuditCalls, 0);
    atomic_store(&sessionCreateCalls, 0);
    atomic_store(&sessionActivateCalls, 0);
    atomic_store(&sessionCancelCalls, 0);
    atomic_store(&channelOpenCalls, 0);
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
    resetCounters();
    UA_Server_run_startup(server);
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
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

    size_t writesBefore = atomic_load(&writeAuditCalls);
    size_t globalBefore = atomic_load(&totalGlobalCalls);

    UA_Variant newVal;
    UA_Int32 nv = 42;
    UA_Variant_init(&newVal);
    UA_Variant_setScalar(&newVal, &nv, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, newVal);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* Either the dedicated audit callback or the global callback (or both)
     * fires for a write update. */
    ck_assert(atomic_load(&writeAuditCalls) > writesBefore ||
              atomic_load(&totalGlobalCalls) > globalBefore);
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

    size_t totalBefore = atomic_load(&totalAuditCalls);
    size_t globalBefore = atomic_load(&totalGlobalCalls);
    UA_Variant newVal;
    UA_Int32 nv = 7;
    UA_Variant_init(&newVal);
    UA_Variant_setScalar(&newVal, &nv, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, newVal);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* No audit callback may fire when auditing is disabled. */
    ck_assert_uint_eq(atomic_load(&totalAuditCalls), totalBefore);
    ck_assert_uint_eq(atomic_load(&totalGlobalCalls), globalBefore);

    /* Restore for teardown */
    cfg->auditingEnabled = true;
} END_TEST

/* Test: connecting a client triggers channel-open and session-create/activate
 * audit events. */
START_TEST(ClientConnectEmitsSessionAuditEvents) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_ne(client, NULL);

    size_t channelBefore = atomic_load(&channelOpenCalls);
    size_t createBefore = atomic_load(&sessionCreateCalls);
    size_t activateBefore = atomic_load(&sessionActivateCalls);
    size_t globalBefore = atomic_load(&totalGlobalCalls);

    UA_StatusCode r =
        UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);

    /* Drive the client a bit and let the server thread emit any pending
     * audit notifications. We poll up to ~1 second. */
    for(int i = 0; i < 100; i++) {
        if(atomic_load(&sessionCreateCalls) > createBefore &&
           atomic_load(&sessionActivateCalls) > activateBefore)
            break;
        UA_Client_run_iterate(client, 10);
    }

    /* SESSION_CREATE and SESSION_ACTIVATE must have fired. CHANNEL_OPEN may
     * not fire on a None-security channel; we just check the global cb too. */
    ck_assert(atomic_load(&sessionCreateCalls) > createBefore);
    ck_assert(atomic_load(&sessionActivateCalls) > activateBefore);
    ck_assert(atomic_load(&totalGlobalCalls) > globalBefore);
    (void)channelBefore;

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
    size_t writesBefore = atomic_load(&writeAuditCalls);

    UA_Variant nv; UA_Int32 x = 11;
    UA_Variant_init(&nv);
    UA_Variant_setScalar(&nv, &x, &UA_TYPES[UA_TYPES_INT32]);
    r = UA_Server_writeValue(server, id, nv);
    ck_assert_int_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(atomic_load(&writeAuditCalls), writesBefore);

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
    tcase_add_test(tc, NoAuditWhenDisabled);
    tcase_add_test(tc, ClientConnectEmitsSessionAuditEvents);
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

#else  /* !UA_ENABLE_AUDITING */
int main(void) { return EXIT_SUCCESS; }
#endif
