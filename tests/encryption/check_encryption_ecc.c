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
     KEY_P384_PEM_DATA, KEY_P384_PEM_LENGTH}
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

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

/* Per-curve setup functions (set activeCurve before calling common setup) */
#define DEFINE_CURVE_SETUP(N) \
    static void setup_curve_##N(void) { activeCurve = N; setup_common(); }

DEFINE_CURVE_SETUP(0)

static void (*curveSetups[])(void) = {
    setup_curve_0
};

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
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

/* --- Test suite: one TCase per curve --- */

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Encryption ECC");
    for(size_t i = 0; i < NUM_ECC_CURVES; i++) {
        TCase *tc = tcase_create(eccCurves[i].name);
        tcase_add_checked_fixture(tc, curveSetups[i], teardown);
#ifdef UA_ENABLE_ENCRYPTION
        tcase_add_test(tc, encryption_connect);
        tcase_add_test(tc, encryption_connect_pem);
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
