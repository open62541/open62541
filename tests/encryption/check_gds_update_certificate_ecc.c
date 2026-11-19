/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

/* UpdateCertificate without a PrivateKey for the ECC certificate types. The
 * certificate is for the current private key of the server or for the key of
 * the last CSR. The ECC SecurityPolicies must use that key. A certificate for
 * another key must be rejected, and the SecurityPolicy must stay unchanged and
 * usable. A SecurityPolicy that frees its context after a failed update
 * crashes in a later handshake or in UA_Server_delete. AddressSanitizer reports
 * the use after free. */

#include <open62541/server_config_default.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/driver/gds_receiver.h>

#include <check.h>
#include <thread_wrapper.h>
#include "test_helpers.h"
#include "certificates.h"
#include "csr_ca_sign.h"

#ifdef CAN_SIGN_CSR

/* Not the default port 4840. The server sets SO_REUSEPORT. A test server of
 * another process on the same port would answer some of the connections, and
 * the tests check which certificate the server presents. */
#define SERVER_PORT 4852
#define SERVER_URL "opc.tcp://localhost:4852"

/* The ApplicationUri in the SubjectAltName of the ECC test certificates */
#define APPLICATION_URI "urn:unconfigured:application"

/* The constructor of an ECC SecurityPolicy */
typedef UA_StatusCode (*EccPolicyCtor)(UA_SecurityPolicy *,
                                       const UA_ApplicationType,
                                       const UA_ByteString,
                                       const UA_ByteString,
                                       const UA_Logger *);

typedef struct {
    const char *name;
    UA_String policyUri;
    UA_Byte *cert;
    size_t certLen;
    UA_Byte *key;
    size_t keyLen;
    /* The deprecated non-AEAD ECC SecurityPolicies are not in the default
     * set. They are added explicitly. NULL for the SecurityPolicies of the
     * default set. */
    EccPolicyCtor deprecatedCtor;
    /* The SecurityPolicy can create a CSR. CreateSigningRequest does not
     * support the EdDSA keys of ECC_curve25519 and ECC_curve448 yet. */
    UA_Boolean csr;
} EccCurve;

static EccCurve eccCurves[] = {
    {"ECC_nistP256",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256"),
     CERT_P256_DER_DATA, CERT_P256_DER_LENGTH,
     KEY_P256_DER_DATA, KEY_P256_DER_LENGTH,
     UA_SecurityPolicy_EccNistP256, true},
    {"ECC_nistP384",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384"),
     CERT_P384_DER_DATA, CERT_P384_DER_LENGTH,
     KEY_P384_DER_DATA, KEY_P384_DER_LENGTH,
     UA_SecurityPolicy_EccNistP384, true},
    {"ECC_brainpoolP256r1",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1"),
     CERT_BP256R1_DER_DATA, CERT_BP256R1_DER_LENGTH,
     KEY_BP256R1_DER_DATA, KEY_BP256R1_DER_LENGTH,
     UA_SecurityPolicy_EccBrainpoolP256r1, true},
    {"ECC_brainpoolP384r1",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1"),
     CERT_BP384R1_DER_DATA, CERT_BP384R1_DER_LENGTH,
     KEY_BP384R1_DER_DATA, KEY_BP384R1_DER_LENGTH,
     UA_SecurityPolicy_EccBrainpoolP384r1, true},
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    {"ECC_curve25519",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519"),
     CERT_ED25519_DER_DATA, CERT_ED25519_DER_LENGTH,
     KEY_ED25519_DER_DATA, KEY_ED25519_DER_LENGTH,
     NULL, false},
    {"ECC_curve448",
     UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448"),
     CERT_ED448_DER_DATA, CERT_ED448_DER_LENGTH,
     KEY_ED448_DER_DATA, KEY_ED448_DER_LENGTH,
     NULL, false}
#endif
};

#define NUM_ECC_CURVES (sizeof(eccCurves) / sizeof(eccCurves[0]))

/* Several SecurityPolicies can use the certificate type of a curve, e.g.
 * ECC_nistP256, ECC_nistP256_AesGcm and ECC_nistP256_ChaChaPoly */
#define MAX_CURVE_POLICIES 8

static UA_Server *server;
static UA_GDSReceiver *receiver;
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE server_thread;
static size_t activeCurve;

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void
addDeprecatedPolicy(UA_SecurityPolicy **policies, size_t *policiesSize,
                    UA_ApplicationType applicationType, const UA_Logger *logging) {
    EccCurve *c = &eccCurves[activeCurve];
    if(!c->deprecatedCtor)
        return;
    UA_ByteString certificate = {c->certLen, c->cert};
    UA_ByteString privateKey = {c->keyLen, c->key};
    UA_SecurityPolicy *tmp = (UA_SecurityPolicy *)
        UA_realloc(*policies, sizeof(UA_SecurityPolicy) * (*policiesSize + 1));
    ck_assert_ptr_nonnull(tmp);
    *policies = tmp;
    UA_StatusCode res = c->deprecatedCtor(&tmp[*policiesSize], applicationType,
                                          certificate, privateKey, logging);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    (*policiesSize)++;
}

static void setup(void) {
    UA_atomic_store(&running, true);
    EccCurve *c = &eccCurves[activeCurve];
    UA_ByteString certificate = {c->certLen, c->cert};
    UA_ByteString privateKey = {c->keyLen, c->key};

    /* The CA of the GDS issues the new certificates. UpdateCertificate verifies
     * the issuer. The client uses the original certificate of the server. */
    UA_ByteString trustList[2] = {
        {CSR_CA_CERT_DER_LENGTH, CSR_CA_CERT_DER_DATA}, certificate};
    UA_ByteString revocationList[1] = {
        {CSR_CA_EMPTY_CRL_DER_LENGTH, CSR_CA_EMPTY_CRL_DER_DATA}};

    /* The RSA SecurityPolicies cannot use the ECC certificate and are not
     * added */
    server = UA_Server_newForUnitTestWithSecurityPolicies(SERVER_PORT, &certificate,
                                                          &privateKey, trustList, 2,
                                                          NULL, 0, revocationList, 1);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if(c->deprecatedCtor) {
        addDeprecatedPolicy(&config->securityPolicies, &config->securityPoliciesSize,
                            UA_APPLICATIONTYPE_SERVER, config->logging);
        UA_StatusCode res = UA_ServerConfig_addAllEndpoints(config);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);

    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    UA_StatusCode res = UA_Server_addDriver(server, &receiver->drv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

#define DEFINE_CURVE_SETUP(N) \
    static void setup_curve_##N(void) { activeCurve = N; setup(); }

DEFINE_CURVE_SETUP(0)
DEFINE_CURVE_SETUP(1)
DEFINE_CURVE_SETUP(2)
DEFINE_CURVE_SETUP(3)
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
DEFINE_CURVE_SETUP(4)
DEFINE_CURVE_SETUP(5)
#endif

static void (*curveSetups[])(void) = {
    setup_curve_0, setup_curve_1, setup_curve_2, setup_curve_3,
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    setup_curve_4, setup_curve_5
#endif
};

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Helper: the SecurityPolicy of the server with the URI */
static UA_SecurityPolicy *
getServerPolicy(const UA_String *policyUri) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&config->securityPolicies[i].policyUri, policyUri))
            return &config->securityPolicies[i];
    }
    return NULL;
}

/* Helper: the certificate type of the active curve */
static UA_NodeId
getCertificateType(void) {
    UA_SecurityPolicy *sp = getServerPolicy(&eccCurves[activeCurve].policyUri);
    ck_assert_ptr_nonnull(sp);
    return sp->certificateTypeId;
}

/* Helper: the SecurityPolicies of the server with the certificate type of the
 * active curve. UpdateCertificate updates all of them. */
static size_t
getCurvePolicies(UA_SecurityPolicy **policies) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_NodeId typeId = getCertificateType();
    size_t policiesSize = 0;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &config->securityPolicies[i];
        if(sp->policyType == UA_SECURITYPOLICYTYPE_NONE ||
           !UA_NodeId_equal(&sp->certificateTypeId, &typeId))
            continue;
        ck_assert_uint_lt(policiesSize, MAX_CURVE_POLICIES);
        policies[policiesSize++] = sp;
    }
    ck_assert_uint_gt(policiesSize, 0);
    return policiesSize;
}

/* Helper: open a new SecureChannel and Session with the SecurityPolicy. The
 * client uses the original certificate of the server. The handshake fails if
 * the private key of the server does not match the certificate it presents. */
static UA_StatusCode
connectClient(const UA_String *policyUri) {
    EccCurve *c = &eccCurves[activeCurve];
    UA_ByteString certificate = {c->certLen, c->cert};
    UA_ByteString privateKey = {c->keyLen, c->key};

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    addDeprecatedPolicy(&cc->securityPolicies, &cc->securityPoliciesSize,
                        UA_APPLICATIONTYPE_CLIENT, cc->logging);
    UA_StatusCode res = UA_String_copy(policyUri, &cc->securityPolicyUri);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);

    res = UA_Client_connect(client, SERVER_URL);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return res;
}

/* Helper: the certificate of the endpoints with the SecurityPolicy.
 * GetEndpoints uses SecurityPolicy#None and works with a broken server key as
 * well. */
static void
getEndpointCertificate(const UA_String *policyUri, UA_ByteString *certificate) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    size_t endpointsSize = 0;
    UA_EndpointDescription *endpoints = NULL;
    UA_StatusCode res =
        UA_Client_getEndpoints(client, SERVER_URL, &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_init(certificate);
    for(size_t i = 0; i < endpointsSize; i++) {
        if(!UA_String_equal(&endpoints[i].securityPolicyUri, policyUri))
            continue;
        res = UA_ByteString_copy(&endpoints[i].serverCertificate, certificate);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        break;
    }
    ck_assert_uint_gt(certificate->length, 0);
    UA_Array_delete(endpoints, endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
}

/* Helper: all endpoints with the certificate type of the active curve present
 * the certificate, and the handshake with each of their SecurityPolicies
 * succeeds */
static void
checkServerUsesCertificate(const UA_ByteString *certificate) {
    UA_SecurityPolicy *policies[MAX_CURVE_POLICIES];
    size_t policiesSize = getCurvePolicies(policies);
    for(size_t i = 0; i < policiesSize; i++) {
        UA_ByteString endpointCertificate;
        getEndpointCertificate(&policies[i]->policyUri, &endpointCertificate);
        ck_assert(UA_ByteString_equal(&endpointCertificate, certificate));
        UA_ByteString_clear(&endpointCertificate);
        ck_assert_uint_eq(connectClient(&policies[i]->policyUri), UA_STATUSCODE_GOOD);
    }
}

/* Positive control for the tests below. The server presents its original
 * certificate, and the client can connect. */
START_TEST(connect_original_certificate) {
    EccCurve *c = &eccCurves[activeCurve];
    UA_ByteString certificate = {c->certLen, c->cert};
    checkServerUsesCertificate(&certificate);
}
END_TEST

/* The GDS issues a new certificate for the current key of the server. No CSR is
 * needed for that. The SecurityPolicies keep their private key. */
START_TEST(update_keyless_current_key) {
    EccCurve *c = &eccCurves[activeCurve];
    UA_ByteString oldCertificate = {c->certLen, c->cert};
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    renewCertificate(&oldCertificate, &certificate);

    UA_StatusCode res =
        UA_GDSReceiver_updateCertificate(receiver, UA_NODEID_NULL, getCertificateType(),
                                         certificate, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    checkServerUsesCertificate(&certificate);
    UA_ByteString_clear(&certificate);
}
END_TEST

/* CreateSigningRequest with a new key. The GDS returns the certificate without
 * a PrivateKey. The SecurityPolicies use the key of the CSR. */
START_TEST(update_keyless_csr_key) {
    UA_NodeId typeId = getCertificateType();
    UA_Byte nonceData[32];
    memset(nonceData, 0x2a, sizeof(nonceData));
    UA_ByteString nonce = {sizeof(nonceData), nonceData};
    UA_Boolean regenerateKey = true;
    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_StatusCode res =
        UA_GDSReceiver_createSigningRequest(receiver, UA_NODEID_NULL, typeId, NULL,
                                            &regenerateKey, &nonce, &csr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ByteString certificate = UA_BYTESTRING_NULL;
    signCsr(&csr, &certificate);
    res = UA_GDSReceiver_updateCertificate(receiver, UA_NODEID_NULL, typeId,
                                           certificate, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    checkServerUsesCertificate(&certificate);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&csr);
}
END_TEST

/* A certificate for another key without a PrivateKey. The SecurityPolicy must
 * reject it, keep its certificate and still accept handshakes. The certificate
 * is the test certificate of another curve. */
START_TEST(securitypolicy_keyless_update_reject_other_key) {
    EccCurve *other = &eccCurves[(activeCurve + 1) % NUM_ECC_CURVES];
    UA_ByteString otherCertificate = {other->certLen, other->cert};
    UA_SecurityPolicy *policies[MAX_CURVE_POLICIES];
    size_t policiesSize = getCurvePolicies(policies);
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    for(size_t i = 0; i < policiesSize; i++) {
        UA_SecurityPolicy *sp = policies[i];
        UA_ByteString oldCertificate = UA_BYTESTRING_NULL;

        /* No assertions while the server is locked. A failed assertion would
         * leave the server thread blocked. */
        el->lock(el);
        UA_StatusCode copyRes = UA_ByteString_copy(&sp->localCertificate, &oldCertificate);
        UA_StatusCode updateRes =
            sp->updateCertificate(sp, otherCertificate, UA_BYTESTRING_NULL);
        UA_Boolean unchanged = UA_ByteString_equal(&sp->localCertificate, &oldCertificate);
        el->unlock(el);
        ck_assert_uint_eq(copyRes, UA_STATUSCODE_GOOD);
        ck_assert_uint_ne(updateRes, UA_STATUSCODE_GOOD);
        ck_assert(unchanged);
        ck_assert_uint_eq(connectClient(&sp->policyUri), UA_STATUSCODE_GOOD);
        UA_ByteString_clear(&oldCertificate);
    }
}
END_TEST

/* The second CSR discards the key of the first CSR. A certificate for the key
 * of the first CSR without a PrivateKey must be rejected. The SecurityPolicy
 * must keep its certificate and still accept handshakes. */
START_TEST(securitypolicy_keyless_update_reject_discarded_csr_key) {
    UA_SecurityPolicy *sp = getServerPolicy(&eccCurves[activeCurve].policyUri);
    ck_assert_ptr_nonnull(sp);
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    UA_ByteString key1 = UA_BYTESTRING_NULL;
    UA_ByteString key2 = UA_BYTESTRING_NULL;
    UA_ByteString oldCertificate = UA_BYTESTRING_NULL;

    /* No assertions while the server is locked */
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    el->lock(el);
    UA_StatusCode csrRes1 =
        sp->createSigningRequest(sp, NULL, NULL, &UA_KEYVALUEMAP_NULL, &csr1, &key1);
    UA_StatusCode csrRes2 =
        sp->createSigningRequest(sp, NULL, NULL, &UA_KEYVALUEMAP_NULL, &csr2, &key2);
    UA_StatusCode copyRes = UA_ByteString_copy(&sp->localCertificate, &oldCertificate);
    el->unlock(el);
    ck_assert_uint_eq(csrRes1, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(csrRes2, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(copyRes, UA_STATUSCODE_GOOD);

    UA_ByteString cert1 = UA_BYTESTRING_NULL;
    signCsr(&csr1, &cert1);

    el->lock(el);
    UA_StatusCode updateRes = sp->updateCertificate(sp, cert1, UA_BYTESTRING_NULL);
    UA_Boolean unchanged = UA_ByteString_equal(&sp->localCertificate, &oldCertificate);
    el->unlock(el);
    ck_assert_uint_ne(updateRes, UA_STATUSCODE_GOOD);
    ck_assert(unchanged);
    ck_assert_uint_eq(connectClient(&sp->policyUri), UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&cert1);
    UA_ByteString_clear(&oldCertificate);
    UA_ByteString_clear(&key2);
    UA_ByteString_clear(&key1);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

#endif /* CAN_SIGN_CSR */

static Suite *testSuite_gds_update_certificate_ecc(void) {
    Suite *s = suite_create("GDS UpdateCertificate ECC");
#ifdef CAN_SIGN_CSR
    for(size_t i = 0; i < NUM_ECC_CURVES; i++) {
        TCase *tc = tcase_create(eccCurves[i].name);
        tcase_add_checked_fixture(tc, curveSetups[i], teardown);
        tcase_add_test(tc, connect_original_certificate);
        tcase_add_test(tc, update_keyless_current_key);
        tcase_add_test(tc, securitypolicy_keyless_update_reject_other_key);
        if(eccCurves[i].csr) {
            tcase_add_test(tc, update_keyless_csr_key);
            tcase_add_test(tc, securitypolicy_keyless_update_reject_discarded_csr_key);
        }
        suite_add_tcase(s, tc);
    }
#endif
    return s;
}

int main(void) {
    Suite *s = testSuite_gds_update_certificate_ecc();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
