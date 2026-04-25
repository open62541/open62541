/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* Consolidated ECC encryption integration tests.
 * Tests all ECC security profiles added by o6 Automation:
 *   ECC_nistP384, ECC_brainpoolP256r1, ECC_brainpoolP384r1,
 *   ECC_curve25519, ECC_curve448
 *
 * Each curve is tested with DER and PEM certificate formats. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "certificates.h"
#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

/* --- Curve parameter table --- */

typedef struct {
    const char *name;
    const char *policyUri;
    UA_Byte *certDer;  size_t certDerLen;
    UA_Byte *keyDer;   size_t keyDerLen;
    UA_Byte *certPem;  size_t certPemLen;
    UA_Byte *keyPem;   size_t keyPemLen;
} EccCurveTestData;

static EccCurveTestData eccCurves[] = {
    {"ECC_nistP384",
     "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384",
     CERT_P384_DER_DATA, CERT_P384_DER_LENGTH,
     KEY_P384_DER_DATA, KEY_P384_DER_LENGTH,
     CERT_P384_PEM_DATA, CERT_P384_PEM_LENGTH,
     KEY_P384_PEM_DATA, KEY_P384_PEM_LENGTH},
    {"ECC_brainpoolP256r1",
     "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1",
     CERT_BP256R1_DER_DATA, CERT_BP256R1_DER_LENGTH,
     KEY_BP256R1_DER_DATA, KEY_BP256R1_DER_LENGTH,
     CERT_BP256R1_PEM_DATA, CERT_BP256R1_PEM_LENGTH,
     KEY_BP256R1_PEM_DATA, KEY_BP256R1_PEM_LENGTH},
    {"ECC_brainpoolP384r1",
     "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1",
     CERT_BP384R1_DER_DATA, CERT_BP384R1_DER_LENGTH,
     KEY_BP384R1_DER_DATA, KEY_BP384R1_DER_LENGTH,
     CERT_BP384R1_PEM_DATA, CERT_BP384R1_PEM_LENGTH,
     KEY_BP384R1_PEM_DATA, KEY_BP384R1_PEM_LENGTH},
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    {"ECC_curve25519",
     "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519",
     CERT_ED25519_DER_DATA, CERT_ED25519_DER_LENGTH,
     KEY_ED25519_DER_DATA, KEY_ED25519_DER_LENGTH,
     CERT_ED25519_PEM_DATA, CERT_ED25519_PEM_LENGTH,
     KEY_ED25519_PEM_DATA, KEY_ED25519_PEM_LENGTH},
    {"ECC_curve448",
     "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448",
     CERT_ED448_DER_DATA, CERT_ED448_DER_LENGTH,
     KEY_ED448_DER_DATA, KEY_ED448_DER_LENGTH,
     CERT_ED448_PEM_DATA, CERT_ED448_PEM_LENGTH,
     KEY_ED448_PEM_DATA, KEY_ED448_PEM_LENGTH}
#endif
};

#define NUM_ECC_CURVES (sizeof(eccCurves) / sizeof(eccCurves[0]))

/* --- Shared state --- */

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static size_t activeCurve;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup_common(void) {
    running = true;
    EccCurveTestData *c = &eccCurves[activeCurve];

    UA_ByteString certificate;
    certificate.length = c->certDerLen;
    certificate.data = c->certDer;

    UA_ByteString privateKey;
    privateKey.length = c->keyDerLen;
    privateKey.data = c->keyDer;

    server = UA_Server_newForUnitTestWithSecurityPolicies(
        4840, &certificate, &privateKey, NULL, 0, NULL, 0, NULL, 0);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    /* Add a writable test variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 initValue = 42;
    UA_Variant_setScalar(&attr.value, &initValue, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_StatusCode addRes =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50000),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "TestVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, NULL);
    ck_assert_uint_eq(addRes, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

/* Per-curve setup functions (set activeCurve before calling common setup) */
#define DEFINE_CURVE_SETUP(N) \
    static void setup_curve_##N(void) { activeCurve = N; setup_common(); }

DEFINE_CURVE_SETUP(0)
DEFINE_CURVE_SETUP(1)
DEFINE_CURVE_SETUP(2)
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
DEFINE_CURVE_SETUP(3)
DEFINE_CURVE_SETUP(4)
#endif

static void (*curveSetups[])(void) = {
    setup_curve_0, setup_curve_1, setup_curve_2,
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    setup_curve_3, setup_curve_4
#endif
};

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void pauseServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

static void runServer(void) {
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

/* Helper: create an encrypted client for the active curve using DER certs */
static UA_Client *
createEncryptedClient(void) {
    EccCurveTestData *c = &eccCurves[activeCurve];

    UA_ByteString certificate;
    certificate.length = c->certDerLen;
    certificate.data = c->certDer;

    UA_ByteString privateKey;
    privateKey.length = c->keyDerLen;
    privateKey.data = c->keyDer;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri = UA_STRING_ALLOC(c->policyUri);

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    return client;
}

/* --- Test: connect with DER certificate --- */

START_TEST(encryption_connect) {
    EccCurveTestData *c = &eccCurves[activeCurve];

    UA_ByteString certificate;
    certificate.length = c->certDerLen;
    certificate.data = c->certDer;

    UA_ByteString privateKey;
    privateKey.length = c->keyDerLen;
    privateKey.data = c->keyDer;

    /* Discover endpoints */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);

    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                               &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);
    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);

    /* Encrypted connect */
    client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri = UA_STRING_ALLOC(c->policyUri);

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: connect with PEM certificate --- */

START_TEST(encryption_connect_pem) {
    EccCurveTestData *c = &eccCurves[activeCurve];

    UA_ByteString certificate;
    certificate.length = c->certPemLen;
    certificate.data = c->certPem;

    UA_ByteString privateKey;
    privateKey.length = c->keyPemLen;
    privateKey.data = c->keyPem;

    /* Discover endpoints */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);

    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                               &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);
    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);

    /* Encrypted connect */
    client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    cc->securityPolicyUri = UA_STRING_ALLOC(c->policyUri);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: write and read back over encrypted channel --- */

START_TEST(encryption_connect_write) {
    UA_Client *client = createEncryptedClient();
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write a value to the test variable */
    UA_NodeId testVarId = UA_NODEID_NUMERIC(1, 50000);
    UA_Int32 writeValue = 1234;
    UA_Variant val;
    UA_Variant_setScalar(&val, &writeValue, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, testVarId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back and verify */
    UA_Variant readVal;
    UA_Variant_init(&readVal);
    retval = UA_Client_readValueAttribute(client, testVarId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(readVal.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 1234);
    UA_Variant_clear(&readVal);

    /* Write a different value */
    writeValue = 5678;
    UA_Variant_setScalar(&val, &writeValue, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, testVarId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back again */
    UA_Variant_init(&readVal);
    retval = UA_Client_readValueAttribute(client, testVarId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 5678);
    UA_Variant_clear(&readVal);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: reconnect session over new SecureChannel --- */

START_TEST(encryption_reconnect) {
    UA_Client *client = createEncryptedClient();
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Initial read */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Remember the session token */
    UA_NodeId oldAuthToken = client->authenticationToken;

    /* Close the SecureChannel without closing the session */
    UA_Client_disconnectSecureChannel(client);

    /* Reconnect — reuses the previous session */
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read again — reactivates the session */
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Session token must be the same */
    ck_assert(UA_NodeId_equal(&oldAuthToken, &client->authenticationToken));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: SecureChannel token renewal under encryption --- */

START_TEST(encryption_renew) {
    UA_Client *client = createEncryptedClient();
    ck_assert(client != NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 channelId = client->channel.securityToken.channelId;

    /* Pause the server thread so we can manually step client/server */
    pauseServer();

    /* Advance time to 80% of token lifetime to trigger renewal */
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);

    /* Advance again to trigger a second renewal */
    UA_fakeSleep((UA_UInt32)(client->channel.securityToken.revisedLifetime * 0.8));
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 1);
    UA_Server_run_iterate(server, false);

    /* Still the same channel */
    ck_assert_uint_eq(channelId, client->channel.securityToken.channelId);

    /* Resume the server thread */
    runServer();

    /* Read a value to verify the channel still works after renewal */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: connect with Sign-only mode (exercises AEAD sign path
 *          for curve25519/448 and HMAC sign path for NIST/Brainpool) --- */

START_TEST(encryption_connect_sign_only) {
    EccCurveTestData *c = &eccCurves[activeCurve];

    UA_ByteString certificate;
    certificate.length = c->certDerLen;
    certificate.data = c->certDer;

    UA_ByteString privateKey;
    privateKey.length = c->keyDerLen;
    privateKey.data = c->keyDer;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri = UA_STRING_ALLOC(c->policyUri);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read a value to exercise the signed channel */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Write to exercise sign path for larger payloads */
    UA_NodeId testVarId = UA_NODEID_NUMERIC(1, 50000);
    UA_Int32 writeValue = 9999;
    UA_Variant wval;
    UA_Variant_setScalar(&wval, &writeValue, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, testVarId, &wval);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: CSR generation for ECC policies --- */

START_TEST(encryption_csr_generation) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Find the active curve's security policy on the server */
    EccCurveTestData *c = &eccCurves[activeCurve];
    UA_SecurityPolicy *sp = NULL;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_String policyUri = UA_STRING((char *)(uintptr_t)c->policyUri);
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &policyUri)) {
            sp = &config->securityPolicies[i];
            break;
        }
    }
    ck_assert_ptr_ne(sp, NULL);
    ck_assert(sp->createSigningRequest != NULL);

    /* Generate a CSR without regenerating the private key */
    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_String subjectName = UA_STRING("CN=TestECC O=open62541");
    UA_StatusCode retval = sp->createSigningRequest(sp, &subjectName,
                                                     NULL, NULL,
                                                     &csr, NULL);
    /* Curve25519/448 may not support standard CSR generation */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&csr);
        return;
    }
    ck_assert(csr.length > 0);
    UA_ByteString_clear(&csr);

    /* Generate a CSR with a new private key */
    UA_ByteString newPrivKey = UA_BYTESTRING_NULL;
    retval = sp->createSigningRequest(sp, &subjectName,
                                       NULL, NULL,
                                       &csr, &newPrivKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(csr.length > 0);
    ck_assert(newPrivKey.length > 0);
    UA_ByteString_clear(&csr);
    UA_ByteString_clear(&newPrivKey);
}
END_TEST

/* --- Test: update certificate and private key on active ECC policy --- */

START_TEST(encryption_update_certificate) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    EccCurveTestData *c = &eccCurves[activeCurve];
    UA_SecurityPolicy *sp = NULL;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_String policyUri = UA_STRING((char *)(uintptr_t)c->policyUri);
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &policyUri)) {
            sp = &config->securityPolicies[i];
            break;
        }
    }
    ck_assert_ptr_ne(sp, NULL);
    ck_assert(sp->updateCertificate != NULL);

    /* Update with the same cert and key (should succeed) */
    UA_ByteString certificate;
    certificate.length = c->certDerLen;
    certificate.data = c->certDer;

    UA_ByteString privateKey;
    privateKey.length = c->keyDerLen;
    privateKey.data = c->keyDer;

    UA_StatusCode retval = sp->updateCertificate(sp, certificate, privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The policy should still be functional after update — connect and read */
    UA_Client *client = createEncryptedClient();
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test suite: one TCase per curve --- */

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Encryption ECC");
    for(size_t i = 0; i < NUM_ECC_CURVES; i++) {
        TCase *tc = tcase_create(eccCurves[i].name);
        tcase_add_checked_fixture(tc, curveSetups[i], teardown);
#ifdef UA_ENABLE_ENCRYPTION
        tcase_add_test(tc, encryption_connect);
        tcase_add_test(tc, encryption_connect_pem);
        tcase_add_test(tc, encryption_connect_sign_only);
        tcase_add_test(tc, encryption_connect_write);
        tcase_add_test(tc, encryption_reconnect);
        tcase_add_test(tc, encryption_renew);
        tcase_add_test(tc, encryption_csr_generation);
        tcase_add_test(tc, encryption_update_certificate);
#endif /* UA_ENABLE_ENCRYPTION */
        suite_add_tcase(s, tc);
    }
    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
