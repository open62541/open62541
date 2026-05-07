/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 *
 */

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

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    /* Load the trustlist */
    size_t trustListSize = 0;
    UA_ByteString *trustList = NULL;

    /* Load the issuerList */
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;

    /* TODO test trustList
    if(argc > 3)
        trustListSize = (size_t)argc-3;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i+3]);
    */

    /* Loading of a revocation list currently unsupported */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);

    /* Add a writable test variable */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 initValue = 42;
    UA_Variant_setScalar(&vattr.value, &initValue, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_StatusCode addRes =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50000),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "TestVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vattr, NULL, NULL);
    ck_assert_uint_eq(addRes, UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
static void setup2(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    char storePathDir[4096];
    getcwd(storePathDir, 4096);

    const UA_String storePath = UA_STRING(storePathDir);
    server =
        UA_Server_newForUnitTestWithSecurityPolicies_Filestore(4840, &certificate,
                                                               &privateKey, storePath);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

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

/* Helper: create an encrypted client for ECC_nistP256 with DER certs */
static UA_Client *
createEncryptedClient_P256(void) {
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    return client;
}

START_TEST(encryption_connect) {
    UA_Client *client = NULL;
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* The Get endpoint (discovery service) is done with
     * security mode as none to see the server's capability
     * and certificate */
    client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* TODO test trustList Load revocationList is not supported now
    if(argc > MIN_ARGS) {
        trustListSize = (size_t)argc-MIN_ARGS;
        retval = UA_ByteString_allocBuffer(trustList, trustListSize);
        if(retval != UA_STATUSCODE_GOOD) {
            cleanupClient(client, remoteCertificate);
            return (int)retval;
        }

        for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
            trustList[trustListCount] = loadFile(argv[trustListCount+3]);
        }
    }
    */

    UA_Client_delete(client);

    /* Secure client initialization */
    client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");
    ck_assert(client != NULL);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }

    /* Secure client connect */
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

START_TEST(encryption_connect_pem) {
    UA_Client *client = NULL;
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_PEM_LENGTH;
    certificate.data = CERT_P256_PEM_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_PEM_LENGTH;
    privateKey.data = KEY_P256_PEM_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* The Get endpoint (discovery service) is done with
     * security mode as none to see the server's capability
     * and certificate */
    client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* TODO test trustList Load revocationList is not supported now
    if(argc > MIN_ARGS) {
        trustListSize = (size_t)argc-MIN_ARGS;
        retval = UA_ByteString_allocBuffer(trustList, trustListSize);
        if(retval != UA_STATUSCODE_GOOD) {
            cleanupClient(client, remoteCertificate);
            return (int)retval;
        }

        for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
            trustList[trustListCount] = loadFile(argv[trustListCount+3]);
        }
    }
    */

    UA_Client_delete(client);

    /* Secure client initialization */
    client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");
    ck_assert(client != NULL);

    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }

    /* Secure client connect */
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
    UA_Client *client = createEncryptedClient_P256();
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
    UA_Client *client = createEncryptedClient_P256();
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
    UA_Client *client = createEncryptedClient_P256();
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

/* --- Test: connect with Sign-only mode --- */

START_TEST(encryption_connect_sign_only) {
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGN;

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client,
                 UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Write to exercise sign path */
    UA_NodeId testVarId = UA_NODEID_NUMERIC(1, 50000);
    UA_Int32 writeValue = 7777;
    UA_Variant wval;
    UA_Variant_setScalar(&wval, &writeValue, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, testVarId, &wval);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* --- Test: CSR generation for ECC_nistP256 --- */

START_TEST(encryption_csr_generation) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_String policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");
    UA_SecurityPolicy *sp = NULL;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &policyUri)) {
            sp = &config->securityPolicies[i];
            break;
        }
    }
    ck_assert_ptr_ne(sp, NULL);
    ck_assert(sp->createSigningRequest != NULL);

    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_String subjectName = UA_STRING("CN=TestECC_P256 O=open62541");
    UA_StatusCode retval = sp->createSigningRequest(sp, &subjectName,
                                                     NULL, NULL,
                                                     &csr, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(csr.length > 0);
    UA_ByteString_clear(&csr);

    /* CSR with new private key generation */
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

/* --- Test: update certificate and private key --- */

START_TEST(encryption_update_certificate) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_String policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");
    UA_SecurityPolicy *sp = NULL;
    for(size_t i = 0; i < config->securityPoliciesSize; i++) {
        if(UA_String_equal(&config->securityPolicies[i].policyUri, &policyUri)) {
            sp = &config->securityPolicies[i];
            break;
        }
    }
    ck_assert_ptr_ne(sp, NULL);
    ck_assert(sp->updateCertificate != NULL);

    /* Update with the same cert/key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    UA_StatusCode retval = sp->updateCertificate(sp, certificate, privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify the policy still works after update */
    UA_Client *client = createEncryptedClient_P256();
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

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Encryption");
    TCase *tc_encryption = tcase_create("Encryption ECC_nistP256");
    tcase_add_checked_fixture(tc_encryption, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption, encryption_connect);
    tcase_add_test(tc_encryption, encryption_connect_pem);
    tcase_add_test(tc_encryption, encryption_connect_sign_only);
    tcase_add_test(tc_encryption, encryption_connect_write);
    tcase_add_test(tc_encryption, encryption_reconnect);
    tcase_add_test(tc_encryption, encryption_renew);
    tcase_add_test(tc_encryption, encryption_csr_generation);
    tcase_add_test(tc_encryption, encryption_update_certificate);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_encryption_filestore = tcase_create("Encryption ECC_nistP256 security policy filestore");
    tcase_add_checked_fixture(tc_encryption_filestore, setup2, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption_filestore, encryption_connect);
    tcase_add_test(tc_encryption_filestore, encryption_connect_pem);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption_filestore);
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
