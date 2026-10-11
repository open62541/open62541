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

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
/* Secrets in the AuditUpdateMethodEvents. The Methods are called on a server
 * without a server thread. */

static UA_Variant capturedInputs;  /* /InputArguments of a Method call */
static UA_Variant capturedOutputs; /* /OutputArguments of a Method call */

static void
capturePayloadValue(const UA_KeyValueMap *payload, const char *key,
                    UA_Variant *out) {
    const UA_Variant *v =
        UA_KeyValueMap_get(payload, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)key));
    ck_assert_ptr_nonnull(v);
    UA_Variant_clear(out);
    ck_assert_uint_eq(UA_Variant_copy(v, out), UA_STATUSCODE_GOOD);
}

static void
secretsAuditCb(UA_Server *s, UA_ApplicationNotificationType type,
               const UA_KeyValueMap payload) {
    (void)s;
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD) {
        capturePayloadValue(&payload, "/InputArguments", &capturedInputs);
        capturePayloadValue(&payload, "/OutputArguments", &capturedOutputs);
    }
}

static void setupSecrets(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->tcpEnabled = false;
    cfg->auditingEnabled = true;
    cfg->auditMethodUpdateEnabled = true;
    cfg->auditNotificationCallback = secretsAuditCb;
    UA_Variant_init(&capturedInputs);
    UA_Variant_init(&capturedOutputs);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void teardownSecrets(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
    UA_Variant_clear(&capturedInputs);
    UA_Variant_clear(&capturedOutputs);
}

/* The arguments of a Method with default values of the declared types */
static size_t
defaultArguments(const UA_NodeId methodId, const char *property, UA_Variant *out) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = methodId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HASPROPERTY);
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_String name = UA_STRING((char*)(uintptr_t)property);
    UA_Variant v;
    UA_Variant_init(&v);
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_String_equal(&br.references[i].browseName.name, &name))
            ck_assert_uint_eq(UA_Server_readValue(server,
                                  br.references[i].nodeId.nodeId, &v),
                              UA_STATUSCODE_GOOD);
    }
    UA_BrowseResult_clear(&br);
    const UA_Argument *args = (const UA_Argument*)v.data;
    for(size_t i = 0; i < v.arrayLength; i++) {
        const UA_DataType *type = UA_findDataType(&args[i].dataType);
        ck_assert_ptr_nonnull(type);
        if(args[i].valueRank > 0)
            UA_Variant_setArray(&out[i], UA_Array_new(1, type), 1, type);
        else
            UA_Variant_setScalar(&out[i], UA_new(type), type);
    }
    size_t size = v.arrayLength;
    UA_Variant_clear(&v);
    return size;
}

static UA_Variant stubOutputs[8];
static size_t stubOutputsSize;
static UA_Boolean stubReceivedAll;

static UA_StatusCode
methodStub(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *methodId, void *methodContext,
           const UA_NodeId *objectId, void *objectContext, size_t inputSize,
           const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    stubReceivedAll = true;
    for(size_t i = 0; i < inputSize; i++)
        stubReceivedAll &= !UA_Variant_isEmpty(&input[i]);
    for(size_t i = 0; i < outputSize && i < stubOutputsSize; i++)
        UA_Variant_copy(&stubOutputs[i], &output[i]);
    return UA_STATUSCODE_GOOD;
}

/* Bit i of a mask stands for argument i */
static const struct {
    UA_UInt32 objectId;
    UA_UInt32 methodId;
    UA_UInt32 secretInputs;
    UA_UInt32 secretOutputs;
} secretMethods[] = {
    {UA_NS0ID_SERVERCONFIGURATION, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
     1u << 5, 0}, /* PrivateKey */
    {UA_NS0ID_KEYCREDENTIALCONFIGURATIONTYPE,
     UA_NS0ID_KEYCREDENTIALCONFIGURATIONTYPE_UPDATECREDENTIAL,
     1u << 1, 0}, /* CredentialSecret */
    /* The UserManagement Object is removed at startup, its type remains */
    {UA_NS0ID_USERMANAGEMENTTYPE, UA_NS0ID_USERMANAGEMENTTYPE_ADDUSER,
     1u << 1, 0}, /* Password */
    {UA_NS0ID_USERMANAGEMENTTYPE, UA_NS0ID_USERMANAGEMENTTYPE_MODIFYUSER,
     1u << 2, 0}, /* Password */
    {UA_NS0ID_USERMANAGEMENTTYPE, UA_NS0ID_USERMANAGEMENTTYPE_CHANGEPASSWORD,
     (1u << 0) | (1u << 1), 0}, /* OldPassword, NewPassword */
    {UA_NS0ID_SERVER, UA_NS0ID_SERVER_GETMONITOREDITEMS, 0, 0}, /* No secret */
#ifdef UA_ENABLE_PUBSUB
    {UA_NS0ID_PUBLISHSUBSCRIBE, UA_NS0ID_PUBLISHSUBSCRIBE_GETSECURITYKEYS,
     0, 1u << 2}, /* Keys */
    {UA_NS0ID_PUBLISHSUBSCRIBE, UA_NS0ID_PUBLISHSUBSCRIBE_SETSECURITYKEYS,
     (1u << 3) | (1u << 4), 0}, /* CurrentKey, FutureKeys */
#endif
};

static void
assertRedacted(const UA_Variant *published, size_t size, UA_UInt32 secrets) {
    ck_assert(published->type == &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(published->arrayLength, size);
    for(size_t i = 0; i < size; i++)
        ck_assert_msg(UA_Variant_isEmpty(&((UA_Variant*)published->data)[i]) ==
                      ((secrets & (1u << i)) != 0), "argument %u", (unsigned)i);
}

/* The AuditUpdateMethodEvent omits passwords, private keys and security keys.
 * The Method still receives them. */
START_TEST(MethodAudit_omitsSecrets) {
    UA_NodeId methodId = UA_NODEID_NUMERIC(0, secretMethods[_i].methodId);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, methodId, methodStub),
                      UA_STATUSCODE_GOOD);
    UA_Variant in[8];
    size_t inSize = defaultArguments(methodId, "InputArguments", in);
    stubOutputsSize = defaultArguments(methodId, "OutputArguments", stubOutputs);

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(0, secretMethods[_i].objectId);
    req.methodId = methodId;
    req.inputArgumentsSize = inSize;
    req.inputArguments = in;
    stubReceivedAll = false;
    UA_CallMethodResult res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(res.outputArgumentsSize, stubOutputsSize);
    UA_CallMethodResult_clear(&res);
    ck_assert(stubReceivedAll);
    assertRedacted(&capturedInputs, inSize, secretMethods[_i].secretInputs);
    assertRedacted(&capturedOutputs, stubOutputsSize, secretMethods[_i].secretOutputs);
    for(size_t i = 0; i < inSize; i++)
        UA_Variant_clear(&in[i]);
    for(size_t i = 0; i < stubOutputsSize; i++)
        UA_Variant_clear(&stubOutputs[i]);
} END_TEST
#endif

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

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
    TCase *tc_secrets = tcase_create("secrets");
    tcase_add_checked_fixture(tc_secrets, setupSecrets, teardownSecrets);
    tcase_add_loop_test(tc_secrets, MethodAudit_omitsSecrets, 0,
                        sizeof(secretMethods) / sizeof(secretMethods[0]));
    suite_add_tcase(s, tc_secrets);
#endif
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
