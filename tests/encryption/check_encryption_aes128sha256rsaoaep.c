/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 *
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

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
    certificate.length = server_cert_der_len;
    certificate.data = server_cert_der;

    UA_ByteString privateKey;
    privateKey.length = server_key_der_len;
    privateKey.data = server_key_der;

    /* TODO test trustList
    if(argc > 3)
        trustListSize = (size_t)argc-3;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(argv[i+3]);
    */

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, NULL);

 	UA_ServerConfig_PKIStore_removeContentAll(UA_ServerConfig_PKIStore_getDefault(server));
 	UA_ServerConfig_PKIStore_storeCertificate(
 		UA_ServerConfig_PKIStore_getDefault(server),
 		UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
 		&certificate
 	);
 	UA_ServerConfig_PKIStore_storeCertificate(
 		UA_ServerConfig_PKIStore_getDefault(server),
 		UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE),
 		&certificate
 	);
 	UA_ServerConfig_PKIStore_storePrivateKey(
 		UA_ServerConfig_PKIStore_getDefault(server),
 		UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
 		&privateKey
 	);
 	UA_ServerConfig_PKIStore_storePrivateKey(
 		UA_ServerConfig_PKIStore_getDefault(server),
 		UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE),
 		&privateKey
 	);

 	UA_ServerConfig_PKIStore_storeTrustList(
 		UA_ServerConfig_PKIStore_getDefault(server),
 		1, &certificate,
 		0, NULL,
 		0, NULL,
 		0, NULL
 	);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
     config->applicationDescription.applicationUri =
         UA_STRING_ALLOC("urn:open62541.server.application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(encryption_connect) {
    UA_Client *client = NULL;
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;

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
    client = UA_Client_new();
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc);

    UA_ByteString_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    cc->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    ck_assert(client != NULL);

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
    client = UA_Client_new();
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc);

    UA_ByteString_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");
    cc->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    ck_assert(client != NULL);
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

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Encryption");
    TCase *tc_encryption = tcase_create("Encryption Aes128Sha256RsaOaep security policy");
    tcase_add_checked_fixture(tc_encryption, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption, encryption_connect);
    tcase_add_test(tc_encryption, encryption_connect_pem);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption);
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
