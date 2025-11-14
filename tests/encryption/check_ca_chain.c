/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Sören Krecker
 *
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "certificates_ca.h"
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

static UA_StatusCode
privateKeyPasswordClientCallback(UA_ClientConfig *cc, UA_ByteString *password) {
    *password = UA_STRING_ALLOC("ca1passwd");
    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
privateKeyPasswordServerCallback(UA_ServerConfig *cc, UA_ByteString *password) {
    *password = UA_STRING_ALLOC("ca1passwd");
    return UA_STATUSCODE_GOOD;
}

static void
setup(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = SERVER_CERT_PEM_LENGTH;
    certificate.data = SERVER_CERT_PEM_DATA;

    UA_ByteString privateKey;
    privateKey.length = SERVER_KEY_PEM_LENGTH;
    privateKey.data = SERVER_KEY_PEM_DATA;

    /* Load the issuerList */
    size_t issuerListSize = 1;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize + 1);
    issuerList[0].data = CA_CERT_PEM_DATA;
    issuerList[0].length = CA_CERT_PEM_LENGTH;

    /* Load the issuerList */
    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize + 1);
    trustList[0].data = CA_CERT_PEM_DATA;
    trustList[0].length = CA_CERT_PEM_LENGTH;

    /* Revocation lists are supported, but not used here */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->privateKeyPasswordCallback = privateKeyPasswordServerCallback;
    UA_String_clear(&config->applicationDescription.applicationUri);
    UA_ServerConfig_setDefaultWithSecurityPolicies(
        config, 4840, &certificate, &privateKey, trustList, trustListSize, issuerList,
        issuerListSize, revocationList, revocationListSize);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.catest.server");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(encryption_connect_Basic256Sha256_ca) {
    UA_Client *client = NULL;
    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize + 1);
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CLIENT_CERT_PEM_LENGTH;
    certificate.data = CLIENT_CERT_PEM_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = CLIENT_KEY_PEM_LENGTH;
    privateKey.data = CLIENT_KEY_PEM_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    trustList[0].length = SERVER_CERT_PEM_LENGTH;
    trustList[0].data = SERVER_CERT_PEM_DATA;

    /* The Get endpoint (discovery service) is done with
     * security mode as none to see the server's capability
     * and certificate */
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    ck_assert(client != NULL);
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                                  &endpointArraySize, &endpointArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointArraySize > 0);

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client);

    /* Secure client initialization */
    client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->privateKeyPasswordCallback = privateKeyPasswordClientCallback;
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey, trustList,
                                         trustListSize, revocationList,
                                         revocationListSize);
    UA_CertificateVerification_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("URN:open62541.catest.client");

    /*for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }*/

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

static Suite *
testSuite_encryption(void) {
    Suite *s = suite_create("EncryptionCA");
    TCase *tc_encryption = tcase_create("Encryption security policy with CA chain");
    tcase_add_checked_fixture(tc_encryption, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption, encryption_connect_Basic256Sha256_ca);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s, tc_encryption);
    return s;
}

int
main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
