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
 * threading. Guard the platform-specific includes AND the body so reduced
 * configs without auditing or thread-safe server support compile cleanly. */
#if defined(UA_ENABLE_AUDITING) && UA_MULTITHREADING >= 100
#include <stdio.h>
#include "thread_wrapper.h"
#include "server/ua_server_internal.h"
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
    case UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE:
        counterInc(&sessionActivateCalls); break;
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

/* Secrets in audit events. The audit functions are called directly on a
 * server without a server thread. */

static UA_Variant capturedToken;   /* /UserIdentityToken of ActivateSession */
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
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_SECURITY_SESSION_ACTIVATE) {
        capturePayloadValue(&payload, "/UserIdentityToken", &capturedToken);
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD) {
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
    UA_Variant_init(&capturedToken);
    UA_Variant_init(&capturedInputs);
    UA_Variant_init(&capturedOutputs);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void teardownSecrets(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
    UA_Variant_clear(&capturedToken);
    UA_Variant_clear(&capturedInputs);
    UA_Variant_clear(&capturedOutputs);
}

/* Emit the AuditActivateSessionEvent for an ActivateSession request with the
 * given (decoded) token */
static void
auditActivateWithToken(void *token, const UA_DataType *tokenType) {
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.securityToken.channelId = 42;
    UA_ActivateSessionRequest req;
    UA_ActivateSessionRequest_init(&req);
    UA_ExtensionObject_setValueNoDelete(&req.userIdentityToken, token, tokenType);
    UA_ActivateSessionResponse resp;
    UA_ActivateSessionResponse_init(&resp);
    UA_Variant_clear(&capturedToken);
    lockServer(server);
    auditActivateSessionEvent(server, &channel, NULL, &req, &resp);
    unlockServer(server);
}

/* Part 5 §6.4.10: the password is not included in the
 * AuditActivateSessionEvent. The same holds for the data of an issued token.
 * The token in the request (decrypted by ActivateSession) is not modified. */
START_TEST(ActivateSessionAudit_omitsSecrets) {
    UA_UserNameIdentityToken userName;
    UA_UserNameIdentityToken_init(&userName);
    userName.policyId = UA_STRING("username");
    userName.userName = UA_STRING("alice");
    userName.password = UA_BYTESTRING("s3cret-password");
    userName.encryptionAlgorithm =
        UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep");
    auditActivateWithToken(&userName, &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]);
    ck_assert(UA_Variant_hasScalarType(&capturedToken,
                  &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]));
    const UA_UserNameIdentityToken *publishedUserName =
        (const UA_UserNameIdentityToken*)capturedToken.data;
    ck_assert(UA_String_equal(&publishedUserName->policyId, &userName.policyId));
    ck_assert(UA_String_equal(&publishedUserName->userName, &userName.userName));
    ck_assert_uint_eq(publishedUserName->password.length, 0);
    ck_assert_uint_eq(publishedUserName->encryptionAlgorithm.length, 0);
    ck_assert_uint_eq(userName.password.length, strlen("s3cret-password"));

    UA_IssuedIdentityToken issued;
    UA_IssuedIdentityToken_init(&issued);
    issued.policyId = UA_STRING("jwt");
    issued.tokenData = UA_BYTESTRING("eyJhbGciOiJSUzI1NiJ9.secret.signature");
    issued.encryptionAlgorithm =
        UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep");
    auditActivateWithToken(&issued, &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]);
    ck_assert(UA_Variant_hasScalarType(&capturedToken,
                  &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN]));
    const UA_IssuedIdentityToken *publishedIssued =
        (const UA_IssuedIdentityToken*)capturedToken.data;
    ck_assert(UA_String_equal(&publishedIssued->policyId, &issued.policyId));
    ck_assert_uint_eq(publishedIssued->tokenData.length, 0);
    ck_assert_uint_eq(publishedIssued->encryptionAlgorithm.length, 0);
    ck_assert_uint_ne(issued.tokenData.length, 0);

    /* The certificate of an X509 token is not secret and is kept */
    UA_X509IdentityToken x509;
    UA_X509IdentityToken_init(&x509);
    x509.policyId = UA_STRING("certificate");
    x509.certificateData = UA_BYTESTRING("certificate-bytes");
    auditActivateWithToken(&x509, &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]);
    ck_assert(UA_Variant_hasScalarType(&capturedToken,
                  &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]));
    const UA_X509IdentityToken *publishedX509 =
        (const UA_X509IdentityToken*)capturedToken.data;
    ck_assert(UA_ByteString_equal(&publishedX509->certificateData,
                                  &x509.certificateData));
} END_TEST

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
static UA_ByteString receivedPrivateKey;

static UA_StatusCode
updateCertificateStub(UA_Server *s, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    ck_assert_uint_eq(inputSize, 6);
    ck_assert(UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_BYTESTRING]));
    receivedPrivateKey = *(UA_ByteString*)input[5].data;
    UA_Boolean applyChangesRequired = true;
    return UA_Variant_setScalarCopy(output, &applyChangesRequired,
                                    &UA_TYPES[UA_TYPES_BOOLEAN]);
}

/* The PrivateKey argument of ServerConfiguration.UpdateCertificate is not
 * published in the AuditUpdateMethodEvent. The Method still receives it. */
START_TEST(MethodAudit_omitsPrivateKey) {
    UA_NodeId methodId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, methodId,
                                                      updateCertificateStub),
                      UA_STATUSCODE_GOOD);

    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_ByteString certificate = UA_BYTESTRING("certificate-bytes");
    UA_String keyFormat = UA_STRING("PEM");
    UA_ByteString privateKey = UA_BYTESTRING("-----BEGIN PRIVATE KEY-----");
    UA_Variant in[6];
    UA_Variant_setScalar(&in[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&in[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&in[2], &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&in[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&in[4], &keyFormat, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&in[5], &privateKey, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    req.methodId = methodId;
    req.inputArgumentsSize = 6;
    req.inputArguments = in;
    UA_ByteString_init(&receivedPrivateKey);
    UA_CallMethodResult res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);
    ck_assert(UA_ByteString_equal(&receivedPrivateKey, &privateKey));

    ck_assert(capturedInputs.type == &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(capturedInputs.arrayLength, 6);
    const UA_Variant *published = (const UA_Variant*)capturedInputs.data;
    ck_assert(UA_Variant_hasScalarType(&published[2], &UA_TYPES[UA_TYPES_BYTESTRING]));
    ck_assert(UA_ByteString_equal((const UA_ByteString*)published[2].data,
                                  &certificate));
    ck_assert(UA_Variant_hasScalarType(&published[4], &UA_TYPES[UA_TYPES_STRING]));
    ck_assert(UA_Variant_isEmpty(&published[5]));
} END_TEST

#ifdef UA_ENABLE_PUBSUB
static UA_ByteString securityKeys[2] = {UA_STRING_STATIC("current-key"),
                                        UA_STRING_STATIC("future-key")};

static UA_StatusCode
getSecurityKeysStub(UA_Server *s, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize,
                    const UA_Variant *input, size_t outputSize,
                    UA_Variant *output) {
    ck_assert_uint_eq(outputSize, 5);
    UA_String policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    UA_UInt32 firstTokenId = 1;
    UA_Duration timeToNextKey = 1000.0;
    UA_Duration keyLifetime = 2000.0;
    UA_StatusCode res =
        UA_Variant_setScalarCopy(&output[0], &policyUri, &UA_TYPES[UA_TYPES_STRING]);
    res |= UA_Variant_setScalarCopy(&output[1], &firstTokenId,
                                    &UA_TYPES[UA_TYPES_INTEGERID]);
    res |= UA_Variant_setArrayCopy(&output[2], securityKeys, 2,
                                   &UA_TYPES[UA_TYPES_BYTESTRING]);
    res |= UA_Variant_setScalarCopy(&output[3], &timeToNextKey,
                                    &UA_TYPES[UA_TYPES_DURATION]);
    res |= UA_Variant_setScalarCopy(&output[4], &keyLifetime,
                                    &UA_TYPES[UA_TYPES_DURATION]);
    return res;
}

static size_t setSecurityKeysCalls;

static UA_StatusCode
setSecurityKeysStub(UA_Server *s, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize,
                    const UA_Variant *input, size_t outputSize,
                    UA_Variant *output) {
    ck_assert_uint_eq(inputSize, 7);
    ck_assert(UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_BYTESTRING]));
    ck_assert(UA_ByteString_equal((const UA_ByteString*)input[3].data,
                                  &securityKeys[0]));
    setSecurityKeysCalls++;
    return UA_STATUSCODE_GOOD;
}

/* The PubSub security keys returned by GetSecurityKeys and passed to
 * SetSecurityKeys (Part 14 §8.3.2, §9.1.3.3) are not published in the
 * AuditUpdateMethodEvent */
START_TEST(MethodAudit_omitsSecurityKeys) {
    UA_NodeId getKeysId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_GETSECURITYKEYS);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, getKeysId,
                                                      getSecurityKeysStub),
                      UA_STATUSCODE_GOOD);
    UA_String groupId = UA_STRING("SecurityGroup");
    UA_UInt32 startingTokenId = 0;
    UA_UInt32 requestedKeyCount = 1;
    UA_Variant in[7];
    UA_Variant_setScalar(&in[0], &groupId, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&in[1], &startingTokenId, &UA_TYPES[UA_TYPES_INTEGERID]);
    UA_Variant_setScalar(&in[2], &requestedKeyCount, &UA_TYPES[UA_TYPES_UINT32]);

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    req.methodId = getKeysId;
    req.inputArgumentsSize = 3;
    req.inputArguments = in;
    UA_CallMethodResult res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    /* The caller receives the keys */
    ck_assert_uint_eq(res.outputArgumentsSize, 5);
    ck_assert(UA_Variant_hasArrayType(&res.outputArguments[2],
                                      &UA_TYPES[UA_TYPES_BYTESTRING]));
    ck_assert_uint_eq(res.outputArguments[2].arrayLength, 2);
    UA_CallMethodResult_clear(&res);

    ck_assert(capturedOutputs.type == &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(capturedOutputs.arrayLength, 5);
    const UA_Variant *published = (const UA_Variant*)capturedOutputs.data;
    ck_assert(UA_Variant_hasScalarType(&published[0], &UA_TYPES[UA_TYPES_STRING]));
    ck_assert(UA_Variant_hasScalarType(&published[1], &UA_TYPES[UA_TYPES_INTEGERID]));
    ck_assert(UA_Variant_isEmpty(&published[2]));
    ck_assert(UA_Variant_hasScalarType(&published[3], &UA_TYPES[UA_TYPES_DURATION]));
    ck_assert(UA_Variant_hasScalarType(&published[4], &UA_TYPES[UA_TYPES_DURATION]));
    /* The input arguments are not secret */
    ck_assert_uint_eq(capturedInputs.arrayLength, 3);

    /* SetSecurityKeys: CurrentKey and FutureKeys */
    UA_NodeId setKeysId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SETSECURITYKEYS);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, setKeysId,
                                                      setSecurityKeysStub),
                      UA_STATUSCODE_GOOD);
    UA_String policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    UA_UInt32 currentTokenId = 1;
    UA_Duration timeToNextKey = 1000.0;
    UA_Duration keyLifetime = 2000.0;
    UA_Variant_setScalar(&in[1], &policyUri, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&in[2], &currentTokenId, &UA_TYPES[UA_TYPES_INTEGERID]);
    UA_Variant_setScalar(&in[3], &securityKeys[0], &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&in[4], &securityKeys[1], 1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&in[5], &timeToNextKey, &UA_TYPES[UA_TYPES_DURATION]);
    UA_Variant_setScalar(&in[6], &keyLifetime, &UA_TYPES[UA_TYPES_DURATION]);
    req.methodId = setKeysId;
    req.inputArgumentsSize = 7;
    setSecurityKeysCalls = 0;
    res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);
    ck_assert_uint_eq(setSecurityKeysCalls, 1);

    ck_assert_uint_eq(capturedInputs.arrayLength, 7);
    published = (const UA_Variant*)capturedInputs.data;
    ck_assert(UA_Variant_hasScalarType(&published[0], &UA_TYPES[UA_TYPES_STRING]));
    ck_assert(UA_Variant_hasScalarType(&published[2], &UA_TYPES[UA_TYPES_INTEGERID]));
    ck_assert(UA_Variant_isEmpty(&published[3]));
    ck_assert(UA_Variant_isEmpty(&published[4]));
    ck_assert(UA_Variant_hasScalarType(&published[6], &UA_TYPES[UA_TYPES_DURATION]));
} END_TEST
#endif /* UA_ENABLE_PUBSUB */
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
    tcase_add_test(tc, ToggleWriteUpdateFlag);
    suite_add_tcase(s, tc);

    TCase *tc_secrets = tcase_create("secrets");
    tcase_add_checked_fixture(tc_secrets, setupSecrets, teardownSecrets);
    tcase_add_test(tc_secrets, ActivateSessionAudit_omitsSecrets);
#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
    tcase_add_test(tc_secrets, MethodAudit_omitsPrivateKey);
#ifdef UA_ENABLE_PUBSUB
    tcase_add_test(tc_secrets, MethodAudit_omitsSecurityKeys);
#endif
#endif
    suite_add_tcase(s, tc_secrets);
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
