/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

/* CreateSigningRequest with RegeneratePrivateKey followed by UpdateCertificate
 * without a PrivateKey. The server keeps the private key of the last CSR only.
 * A certificate for the key of an earlier CSR must be rejected by
 * UpdateCertificate. Otherwise the server presents a certificate that does not
 * match its private key and every following handshake fails. */

#include <open62541/server_config_default.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/driver/gds_receiver.h>

#include <check.h>
#include <thread_wrapper.h>
#include "test_helpers.h"
#include "certificates.h"
#include "certificates_csr_ca.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/evp.h>
#include <openssl/x509.h>
#define CAN_SIGN_CSR
#elif defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x03050000
#include "crypto/mbedtls/securitypolicy_mbedtls_compat.h"
#define CAN_SIGN_CSR
#endif
#endif

/* Not the default port 4840. The server sets SO_REUSEPORT. A test server of
 * another process on the same port would answer some of the connections, and
 * the tests check which certificate the server presents. */
#define SERVER_PORT 4851
#define SERVER_URL "opc.tcp://localhost:4851"

UA_Server *server;
UA_GDSReceiver *receiver;
UA_atomic(uintptr_t) running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    UA_atomic_store(&running, true);
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};

    /* Trust the CAs of the client certificate and the CA that signs the CSRs
     * of the server. UpdateCertificate verifies the issuer. */
    UA_ByteString trustList[3] = {
        {INTERMEDIATE_CERT_DER_LENGTH, INTERMEDIATE_CERT_DER_DATA},
        {ROOT_CERT_DER_LENGTH, ROOT_CERT_DER_DATA},
        {CSR_CA_CERT_DER_LENGTH, CSR_CA_CERT_DER_DATA}};
    UA_ByteString revocationList[3] = {
        {ROOT_EMPTY_CRL_PEM_LENGTH, ROOT_EMPTY_CRL_PEM_DATA},
        {INTERMEDIATE_EMPTY_CRL_PEM_LENGTH, INTERMEDIATE_EMPTY_CRL_PEM_DATA},
        {CSR_CA_EMPTY_CRL_DER_LENGTH, CSR_CA_EMPTY_CRL_DER_DATA}};

    server = UA_Server_newForUnitTestWithSecurityPolicies(SERVER_PORT, &certificate, &privateKey,
                                                          trustList, 3, trustList, 3,
                                                          revocationList, 3);
    ck_assert(server != NULL);

    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    UA_StatusCode res = UA_Server_addDriver(server, &receiver->drv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The ApplicationUri of the server certificate */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.unconfigured.application");

#ifdef UA_ENABLE_RBAC
    /* The GDS Methods are restricted to the SecurityAdmin role. Map the
     * anonymous session of the test client to it. */
    UA_NodeId secAdminId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    UA_Role secAdmin;
    res = UA_Server_getRoleById(server, secAdminId, &secAdmin);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType *)
        UA_realloc(secAdmin.identityMappingRules,
                   (secAdmin.identityMappingRulesSize + 1) *
                   sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(rules);
    secAdmin.identityMappingRules = rules;
    UA_IdentityMappingRuleType_init(&rules[secAdmin.identityMappingRulesSize]);
    rules[secAdmin.identityMappingRulesSize].criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    secAdmin.identityMappingRulesSize++;
    res = UA_Server_updateRole(server, &secAdmin);
    UA_Role_clear(&secAdmin);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
#endif

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Helper: create a client for the Aes128_Sha256_RsaOaep endpoint, which uses
 * an RsaSha256ApplicationCertificate. Not yet connected. */
static UA_Client *
newSecureClient(void) {
    UA_ByteString certificate = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    UA_ByteString privateKey = {APPLICATION_KEY_DER_LENGTH, APPLICATION_KEY_DER_DATA};

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    return client;
}

/* Helper: create a client connected to the test server */
static UA_Client *
createSecureClient(void) {
    UA_Client *client = newSecureClient();
    UA_StatusCode res = UA_Client_connect(client, SERVER_URL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return client;
}

/* Helper: open a new SecureChannel and Session to the test server. The
 * handshake fails if the private key of the server does not match the
 * certificate it presents. */
static UA_StatusCode
connectNewClient(void) {
    UA_Client *client = newSecureClient();
    UA_StatusCode res = UA_Client_connect(client, SERVER_URL);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return res;
}

/* Helper: the certificate of the Aes128_Sha256_RsaOaep endpoint. GetEndpoints
 * uses SecurityPolicy#None and works with a broken server key as well. */
static void
getEndpointCertificate(UA_ByteString *certificate) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    size_t endpointsSize = 0;
    UA_EndpointDescription *endpoints = NULL;
    UA_StatusCode res =
        UA_Client_getEndpoints(client, SERVER_URL, &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    UA_ByteString_init(certificate);
    for(size_t i = 0; i < endpointsSize; i++) {
        if(!UA_String_equal(&endpoints[i].securityPolicyUri, &policyUri))
            continue;
        res = UA_ByteString_copy(&endpoints[i].serverCertificate, certificate);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        break;
    }
    ck_assert_uint_gt(certificate->length, 0);
    UA_Array_delete(endpoints, endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
}

/* Helper: call a Method of the ServerConfiguration Object. Copies the first
 * output argument to out if out is not NULL. */
static UA_StatusCode
callServerConfigurationMethod(UA_Client *client, UA_UInt32 methodId,
                              size_t inputSize, UA_Variant *input,
                              UA_Variant *out) {
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = inputSize;
    callMethodRequest.inputArguments = input;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, methodId);

    UA_CallRequest callReq;
    UA_CallRequest_init(&callReq);
    callReq.methodsToCallSize = 1;
    callReq.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReq);
    UA_StatusCode res = response.responseHeader.serviceResult;
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(1, response.resultsSize);
        res = response.results[0].statusCode;
    }
    if(res == UA_STATUSCODE_GOOD && out) {
        ck_assert_uint_gt(response.results[0].outputArgumentsSize, 0);
        res = UA_Variant_copy(&response.results[0].outputArguments[0], out);
    }
    UA_CallResponse_clear(&response);
    return res;
}

/* Helper: call CreateSigningRequest for the RsaSha256ApplicationCertificate of
 * the DefaultApplicationGroup */
static UA_StatusCode
callCreateSigningRequest(UA_Client *client, UA_Boolean regenerateKey,
                         UA_ByteString *csr) {
    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_String subjectName = UA_STRING_NULL;
    UA_ByteString nonce = UA_BYTESTRING_NULL;

    UA_Variant inputArguments[5];
    UA_Variant_setScalar(&inputArguments[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[2], &subjectName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[3], &regenerateKey, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&inputArguments[4], &nonce, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res =
        callServerConfigurationMethod(client, UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST,
                                      5, inputArguments, &out);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&out, &UA_TYPES[UA_TYPES_BYTESTRING]));
        res = UA_ByteString_copy((UA_ByteString*)out.data, csr);
    }
    UA_Variant_clear(&out);
    return res;
}

/* Helper: call UpdateCertificate without a PrivateKey. The server uses the
 * private key of the last CSR or its current private key. */
static UA_StatusCode
callUpdateCertificate(UA_Client *client, UA_ByteString *certificate) {
    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_String privateKeyFormat = UA_STRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;

    UA_Variant inputArguments[6];
    UA_Variant_setScalar(&inputArguments[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[2], certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&inputArguments[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&inputArguments[4], &privateKeyFormat, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[5], &privateKey, &UA_TYPES[UA_TYPES_BYTESTRING]);

    return callServerConfigurationMethod(client, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
                                         6, inputArguments, NULL);
}

static UA_StatusCode
callApplyChanges(UA_Client *client) {
    return callServerConfigurationMethod(client, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
                                         0, NULL, NULL);
}

/* Helper: the first SecurityPolicy with an RsaSha256ApplicationCertificate */
static UA_SecurityPolicy *
getRsaSha256Policy(void) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &config->securityPolicies[i];
        if(sp->policyType != UA_SECURITYPOLICYTYPE_NONE &&
           UA_NodeId_equal(&sp->certificateTypeId, &typeId))
            return sp;
    }
    return NULL;
}

#ifdef CAN_SIGN_CSR

static UA_UInt32 serialNumber = 1;

/* Helper: issue a certificate for the public key, the subject and the
 * SubjectAltName of a CSR with CSR_CA_CERT_DER. The client of the test accepts
 * all server certificates, but checks the ApplicationUri in the
 * SubjectAltName. */
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
static void
signCsr(const UA_ByteString *csr, UA_ByteString *certificate) {
    const unsigned char *p = csr->data;
    X509_REQ *req = d2i_X509_REQ(NULL, &p, (long)csr->length);
    ck_assert_ptr_nonnull(req);
    p = CSR_CA_CERT_DER_DATA;
    X509 *issuer = d2i_X509(NULL, &p, CSR_CA_CERT_DER_LENGTH);
    ck_assert_ptr_nonnull(issuer);
    p = CSR_CA_KEY_DER_DATA;
    EVP_PKEY *issuerKey = d2i_AutoPrivateKey(NULL, &p, CSR_CA_KEY_DER_LENGTH);
    ck_assert_ptr_nonnull(issuerKey);
    EVP_PKEY *subjectKey = X509_REQ_get_pubkey(req);
    ck_assert_ptr_nonnull(subjectKey);

    X509 *crt = X509_new();
    ck_assert_ptr_nonnull(crt);
    ck_assert_int_eq(X509_set_version(crt, 2), 1);
    ck_assert_int_eq(ASN1_INTEGER_set(X509_get_serialNumber(crt),
                                      (long)serialNumber++), 1);
    ck_assert_int_eq(X509_set_issuer_name(crt, X509_get_subject_name(issuer)), 1);
    ck_assert_int_eq(X509_set_subject_name(crt, X509_REQ_get_subject_name(req)), 1);
    ck_assert_ptr_nonnull(X509_gmtime_adj(X509_getm_notBefore(crt), 0));
    ck_assert_ptr_nonnull(X509_gmtime_adj(X509_getm_notAfter(crt), 60 * 60 * 24));
    ck_assert_int_eq(X509_set_pubkey(crt, subjectKey), 1);

    STACK_OF(X509_EXTENSION) *exts = X509_REQ_get_extensions(req);
    for(int i = 0; i < sk_X509_EXTENSION_num(exts); i++)
        ck_assert_int_eq(X509_add_ext(crt, sk_X509_EXTENSION_value(exts, i), -1), 1);
    sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);

    ck_assert_int_gt(X509_sign(crt, issuerKey, EVP_sha256()), 0);
    int len = i2d_X509(crt, NULL);
    ck_assert_int_gt(len, 0);
    UA_StatusCode res = UA_ByteString_allocBuffer(certificate, (size_t)len);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    unsigned char *out = certificate->data;
    ck_assert_int_eq(i2d_X509(crt, &out), len);

    X509_free(crt);
    EVP_PKEY_free(subjectKey);
    EVP_PKEY_free(issuerKey);
    X509_free(issuer);
    X509_REQ_free(req);
}
#else
#define MAX_SAN_ENTRIES 8
static void
signCsr(const UA_ByteString *csr, UA_ByteString *certificate) {
    mbedtls_x509_csr req;
    mbedtls_x509_crt issuer;
    mbedtls_pk_context issuerKey;
    mbedtls_x509write_cert crt;
    mbedtls_x509_csr_init(&req);
    mbedtls_x509_crt_init(&issuer);
    mbedtls_pk_init(&issuerKey);
    mbedtls_x509write_crt_init(&crt);

    ck_assert_int_eq(mbedtls_x509_csr_parse_der(&req, csr->data, csr->length), 0);
    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&issuer, CSR_CA_CERT_DER_DATA,
                                                CSR_CA_CERT_DER_LENGTH), 0);
    ck_assert_int_eq(UA_mbedTLS_compat_parsePrivateKey(&issuerKey, CSR_CA_KEY_DER_DATA,
                                                       CSR_CA_KEY_DER_LENGTH, NULL, 0), 0);

    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&crt, &req.pk);
    mbedtls_x509write_crt_set_issuer_key(&crt, &issuerKey);
    char name[512];
    ck_assert_int_gt(mbedtls_x509_dn_gets(name, sizeof(name), &req.subject), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_subject_name(&crt, name), 0);
    ck_assert_int_gt(mbedtls_x509_dn_gets(name, sizeof(name), &issuer.subject), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_issuer_name(&crt, name), 0);
    unsigned char serial[4] = {
        (unsigned char)(serialNumber >> 24), (unsigned char)(serialNumber >> 16),
        (unsigned char)(serialNumber >> 8), (unsigned char)serialNumber};
    serialNumber++;
    ck_assert_int_eq(mbedtls_x509write_crt_set_serial_raw(&crt, serial,
                                                          sizeof(serial)), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_validity(&crt, "20240101000000",
                                                        "20991231235959"), 0);

    mbedtls_x509_san_list san[MAX_SAN_ENTRIES];
    size_t sanSize = 0;
    for(const mbedtls_x509_sequence *cur = &req.subject_alt_names;
        cur && cur->buf.p; cur = cur->next) {
        ck_assert_uint_lt(sanSize, MAX_SAN_ENTRIES);
        ck_assert_int_eq(mbedtls_x509_parse_subject_alt_name(&cur->buf,
                                                             &san[sanSize].node), 0);
        san[sanSize].next = NULL;
        if(sanSize > 0)
            san[sanSize - 1].next = &san[sanSize];
        sanSize++;
    }
    if(sanSize > 0)
        ck_assert_int_eq(mbedtls_x509write_crt_set_subject_alternative_name(&crt, san), 0);

    unsigned char buf[4096];
    int len = UA_mbedTLS_compat_writeCertificateDer(&crt, buf, sizeof(buf));
    ck_assert_int_gt(len, 0);
    UA_StatusCode res = UA_ByteString_allocBuffer(certificate, (size_t)len);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    memcpy(certificate->data, buf + sizeof(buf) - (size_t)len, (size_t)len);

    for(size_t i = 0; i < sanSize; i++)
        mbedtls_x509_free_subject_alt_name(&san[i].node);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&issuerKey);
    mbedtls_x509_crt_free(&issuer);
    mbedtls_x509_csr_free(&req);
}
#endif

/* Positive control for the tests below. The certificate for the key of the
 * last CSR is accepted, and the server presents it after ApplyChanges. */
START_TEST(update_certificate_accept_last_csr_key) {
    UA_Client *client = createSecureClient();
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr2), UA_STATUSCODE_GOOD);

    UA_ByteString cert2 = UA_BYTESTRING_NULL;
    signCsr(&csr2, &cert2);
    ck_assert_uint_eq(callUpdateCertificate(client, &cert2), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callApplyChanges(client), UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    UA_ByteString endpointCertificate;
    getEndpointCertificate(&endpointCertificate);
    ck_assert(UA_ByteString_equal(&endpointCertificate, &cert2));
    ck_assert_uint_eq(connectNewClient(), UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&endpointCertificate);
    UA_ByteString_clear(&cert2);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

/* The second CreateSigningRequest discards the key of the first CSR.
 * UpdateCertificate with a certificate for the first CSR must fail. The CTT
 * expects Bad_CertificateInvalid. Part 12, 7.10.5 also lists
 * Bad_SecurityChecksFailed for UpdateCertificate. */
START_TEST(update_certificate_reject_discarded_csr_key) {
    UA_Client *client = createSecureClient();
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr2), UA_STATUSCODE_GOOD);

    UA_ByteString cert1 = UA_BYTESTRING_NULL;
    signCsr(&csr1, &cert1);
    ck_assert_uint_eq(callUpdateCertificate(client, &cert1),
                      UA_STATUSCODE_BADCERTIFICATEINVALID);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_ByteString_clear(&cert1);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

/* Same sequence as above, but ApplyChanges is called if UpdateCertificate
 * accepts the certificate. The server must not install the certificate with
 * the key of the second CSR. Clients must still be able to connect. */
START_TEST(apply_changes_discarded_csr_key_keeps_server_reachable) {
    UA_Client *client = createSecureClient();
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr2), UA_STATUSCODE_GOOD);

    UA_ByteString cert1 = UA_BYTESTRING_NULL;
    signCsr(&csr1, &cert1);
    if(callUpdateCertificate(client, &cert1) == UA_STATUSCODE_GOOD)
        callApplyChanges(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    ck_assert_uint_eq(connectNewClient(), UA_STATUSCODE_GOOD);
    UA_ByteString endpointCertificate;
    getEndpointCertificate(&endpointCertificate);
    ck_assert(!UA_ByteString_equal(&endpointCertificate, &cert1));

    UA_ByteString_clear(&endpointCertificate);
    UA_ByteString_clear(&cert1);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

/* The SecurityPolicy itself must not pair a certificate with the key of the
 * last CSR if the keys do not match. */
START_TEST(securitypolicy_update_reject_csr_key_mismatch) {
    UA_SecurityPolicy *sp = getRsaSha256Policy();
    ck_assert_ptr_nonnull(sp);
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    UA_ByteString key1 = UA_BYTESTRING_NULL;
    UA_ByteString key2 = UA_BYTESTRING_NULL;
    UA_ByteString oldCertificate = UA_BYTESTRING_NULL;

    /* No assertions while the server is locked. A failed assertion would leave
     * the server thread blocked. */
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

    UA_ByteString_clear(&cert1);
    UA_ByteString_clear(&oldCertificate);
    UA_ByteString_clear(&key2);
    UA_ByteString_clear(&key1);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

/* A CreateSigningRequest without RegeneratePrivateKey replaces a CSR with a
 * new key. The server signs the new CSR with its current key. A certificate
 * for the key of the replaced CSR must be rejected. */
START_TEST(update_certificate_reject_key_of_replaced_csr) {
    UA_Client *client = createSecureClient();
    UA_ByteString csr1 = UA_BYTESTRING_NULL;
    UA_ByteString csr2 = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(callCreateSigningRequest(client, true, &csr1), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callCreateSigningRequest(client, false, &csr2), UA_STATUSCODE_GOOD);

    UA_ByteString cert1 = UA_BYTESTRING_NULL;
    signCsr(&csr1, &cert1);
    ck_assert_uint_eq(callUpdateCertificate(client, &cert1),
                      UA_STATUSCODE_BADCERTIFICATEINVALID);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_ByteString_clear(&cert1);
    UA_ByteString_clear(&csr2);
    UA_ByteString_clear(&csr1);
}
END_TEST

#endif /* CAN_SIGN_CSR */

/* CreateSigningRequest with RegeneratePrivateKey, then without, then with
 * again. The SecurityPolicy must not keep a pointer to a freed CSR key. The
 * GDS receiver calls a SecurityPolicy once for each of its endpoints (Sign and
 * SignAndEncrypt). So a dangling key is freed again in the same call already,
 * or else by the third call or when the server is deleted. AddressSanitizer or
 * Valgrind report the double free. */
START_TEST(csr_keep_key_after_regenerated_key) {
    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_Boolean regenerateKey[3] = {true, false, true};
    for(size_t i = 0; i < 3; i++) {
        UA_ByteString csr = UA_BYTESTRING_NULL;
        UA_StatusCode res =
            UA_GDSReceiver_createSigningRequest(receiver, groupId, typeId, NULL,
                                                &regenerateKey[i], NULL, &csr);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_gt(csr.length, 0);
        UA_ByteString_clear(&csr);
    }
}
END_TEST

static Suite *testSuite_gds_csr_key(void) {
    Suite *s = suite_create("GDS CSR Key");
#ifdef CAN_SIGN_CSR
    TCase *tc_key = tcase_create("CSR Key");
    tcase_add_checked_fixture(tc_key, setup, teardown);
    tcase_add_test(tc_key, update_certificate_accept_last_csr_key);
    tcase_add_test(tc_key, update_certificate_reject_discarded_csr_key);
    tcase_add_test(tc_key, apply_changes_discarded_csr_key_keeps_server_reachable);
    tcase_add_test(tc_key, securitypolicy_update_reject_csr_key_mismatch);
    suite_add_tcase(s, tc_key);
#endif

    /* A CSR without a new key replaces a CSR with a new key. A dangling CSR
     * key in a SecurityPolicy would crash the test binary, so these tests run
     * last. They can be run alone with CK_RUN_CASE="CSR Key Replaced". */
    TCase *tc_replaced = tcase_create("CSR Key Replaced");
    tcase_add_checked_fixture(tc_replaced, setup, teardown);
    tcase_add_test(tc_replaced, csr_keep_key_after_regenerated_key);
#ifdef CAN_SIGN_CSR
    tcase_add_test(tc_replaced, update_certificate_reject_key_of_replaced_csr);
#endif
    suite_add_tcase(s, tc_replaced);
    return s;
}

int main(void) {
    Suite *s = testSuite_gds_csr_key();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
