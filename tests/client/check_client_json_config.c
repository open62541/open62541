/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_file_based.h>
#include <open62541/server.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/server_config_default.h>

#include "../common.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

#include <check.h>

#if defined(_MSC_VER)
# pragma warning(disable: 4146)
#endif

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    UA_StatusCode retval;
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Instatiate a new AccessControl plugin that knows username/pw */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->allowNonePolicyPassword = true;

    retval = UA_ServerConfig_addEndpoint(config, UA_SECURITY_POLICY_NONE_URI,
                                         UA_MESSAGESECURITYMODE_NONE);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    /* Add a UserTokenPolicy that allows anonymous access */
    retval = UA_AccessControl_default(config, true, NULL, 0, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static char const * const file_name = "client_json_config.json5";

START_TEST(loadClientConfig) {

    UA_ByteString jsonConfig = loadFile(file_name);

    /* test setup failure */
    ck_assert(!UA_ByteString_equal(&jsonConfig, &UA_BYTESTRING_NULL));

    UA_ClientConfig clientConfig;
    UA_StatusCode res = UA_ClientConfig_loadFromFile(&clientConfig, jsonConfig);

    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* expected static strings */
    const UA_String expected_client_applicationUri = UA_STRING_STATIC("urn:open62541.client.application");
    const UA_String expected_client_productUri = UA_STRING_STATIC("urn:product.test.org");
    const UA_String expected_client_appName_text = UA_STRING_STATIC("Test Application");
    const UA_String expected_client_appName_locale = UA_STRING_STATIC("en-EN");
    const UA_String expected_client_gatewayServerUri = UA_STRING_NULL;
    const UA_String expected_endpointUrl = UA_STRING_STATIC("opc.tcp://localhost:4840");
    const UA_String expected_sessionLocale_0 = UA_STRING_STATIC("en_US");
    const UA_String expected_sessionLocale_1 = UA_STRING_STATIC("de_DE");
    const UA_String expected_applicationUri_server = UA_STRING_STATIC("urn:open62541.unconfigured.application");

    /* check fields
     *
     * The following tests were generated with AI with the following prompt:
     *
     * In `check_json_config.c` I need you to create test cases for the
     * individual fields.  For the json object defined by
     * `client_json_config.json5`, create test cases in `check_json_config.c`
     * at line 39.  See `client.h` for the structure of each field in
     * `UA_ClientConfig` and nested fields.
     */

    /* test timeout field */
    ck_assert_uint_eq(clientConfig.timeout, 1000);

    /* test applicationDescription fields */
    ck_assert(UA_String_equal(&clientConfig.clientDescription.applicationUri,
                              &expected_client_applicationUri));
    ck_assert(UA_String_equal(&clientConfig.clientDescription.productUri,
                              &expected_client_productUri));
    ck_assert(UA_String_equal(&clientConfig.clientDescription.applicationName.text,
                              &expected_client_appName_text));
    ck_assert(UA_String_equal(&clientConfig.clientDescription.applicationName.locale,
                              &expected_client_appName_locale));
    ck_assert_uint_eq(clientConfig.clientDescription.applicationType, 1); /* Client */
    ck_assert(UA_String_equal(&clientConfig.clientDescription.gatewayServerUri,
                              &expected_client_gatewayServerUri));
    ck_assert_uint_eq(clientConfig.clientDescription.discoveryUrlsSize, 0);

    /* test connection configuration */
    ck_assert(UA_String_equal(&clientConfig.endpointUrl,
                              &expected_endpointUrl));

    /* test session configuration */
    ck_assert_uint_eq(clientConfig.sessionLocaleIdsSize, 2);
    ck_assert(UA_String_equal(&clientConfig.sessionLocaleIds[0],
                              &expected_sessionLocale_0));
    ck_assert(UA_String_equal(&clientConfig.sessionLocaleIds[1],
                              &expected_sessionLocale_1));

    /* test session name */
    const UA_String expected_sessionName = UA_STRING_STATIC("TestSession");
    ck_assert(UA_String_equal(&clientConfig.sessionName,
                              &expected_sessionName));

    /* test local connection configuration */
    ck_assert_uint_eq(clientConfig.localConnectionConfig.protocolVersion, 0);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.recvBufferSize, 65536);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.sendBufferSize, 65536);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.localMaxMessageSize, 65536);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.remoteMaxMessageSize, 65536);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.localMaxChunkCount, 8);
    ck_assert_uint_eq(clientConfig.localConnectionConfig.remoteMaxChunkCount, 8);

    /* test endpoint object */
    /*
    const UA_String expected_endpoint_endpointUrl = UA_STRING_STATIC("opc.tcp://localhost:4840");
    const UA_String expected_endpoint_securityPolicyUri = UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");
    ck_assert(UA_String_equal(&clientConfig.endpoint.endpointUrl,
                              &expected_endpoint_endpointUrl));
    ck_assert(UA_String_equal(&clientConfig.endpoint.securityPolicyUri,
                              &expected_endpoint_securityPolicyUri));
    */

    /* test client-level security URIs */
    const UA_String expected_client_securityPolicyUri = UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");
    const UA_String expected_client_authSecurityPolicyUri = UA_STRING_STATIC("");
    ck_assert(UA_String_equal(&clientConfig.securityPolicyUri,
                              &expected_client_securityPolicyUri));
    ck_assert(UA_String_equal(&clientConfig.authSecurityPolicyUri,
                              &expected_client_authSecurityPolicyUri));

    /* test basic connection behavior flags */
    ck_assert_uint_eq(clientConfig.noSession, false);
    ck_assert_uint_eq(clientConfig.noReconnect, false);
    ck_assert_uint_eq(clientConfig.noNewSession, false);

    /* test advanced connection settings */
    ck_assert_uint_eq(clientConfig.secureChannelLifeTime, 0);
    ck_assert_uint_eq(clientConfig.requestedSessionTimeout, 1000);
    ck_assert_uint_eq(clientConfig.connectivityCheckInterval, 0);
    ck_assert_uint_eq(clientConfig.tcpReuseAddr, true);

    /* test security/filtering fields */
    ck_assert(UA_String_equal(&clientConfig.applicationUri,
                              &expected_applicationUri_server));
    ck_assert_uint_eq(clientConfig.allowNonePolicyPassword, false);

#ifdef UA_ENABLE_ENCRYPTION
    /* test encryption-related fields */
    ck_assert_uint_eq(clientConfig.maxTrustListSize, 20);
    ck_assert_uint_eq(clientConfig.maxRejectedListSize, 20);
#endif

    /* test namespace array (empty in this config) */
    ck_assert_uint_eq(clientConfig.namespacesSize, 0);

    /* test outstanding publish requests */
    ck_assert_uint_eq(clientConfig.outStandingPublishRequests, 0);

    UA_ByteString_clear(&jsonConfig);
    UA_ClientConfig_clear(&clientConfig);
}
END_TEST

START_TEST(UA_server_and_client_config) {
    UA_ByteString jsonConfig = loadFile(file_name);

    /* test setup failure */
    ck_assert(!UA_ByteString_equal(&jsonConfig, &UA_BYTESTRING_NULL));

    UA_Client *client = UA_Client_newFromFile(jsonConfig);

    ck_assert_ptr_ne(client, NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // Do something here

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_ByteString_clear(&jsonConfig);
}
END_TEST

START_TEST(loadClientFromConfigAndConnectToServer) {
    UA_ByteString jsonConfig = loadFile(file_name);

    /* test setup failure */
    ck_assert(!UA_ByteString_equal(&jsonConfig, &UA_BYTESTRING_NULL));

    UA_Client *client = UA_Client_newFromFile(jsonConfig);

    ck_assert_ptr_ne(client, NULL);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // Do something here

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_ByteString_clear(&jsonConfig);
}
END_TEST

START_TEST(loadClientAndClientConfigAndCompare) {
    UA_ByteString jsonConfig = loadFile(file_name);

    /* test setup failure */
    ck_assert(!UA_ByteString_equal(&jsonConfig, &UA_BYTESTRING_NULL));

    UA_Client *client = UA_Client_newFromFile(jsonConfig);
    ck_assert_ptr_ne(client, NULL);

    UA_ClientConfig clientConfig;
    UA_StatusCode res = UA_ClientConfig_loadFromFile(&clientConfig, jsonConfig);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* parsing generates the same client configuration */
    const UA_ClientConfig *cc2 = UA_Client_getConfig(client);
    ck_assert_ptr_ne(cc2, NULL);

    /* test timeout field */
    ck_assert_uint_eq(cc2->timeout, clientConfig.timeout);

    /* test applicationDescription fields */
    ck_assert(UA_String_equal(&cc2->clientDescription.applicationUri,
                              &clientConfig.clientDescription.applicationUri));
    ck_assert(UA_String_equal(&cc2->clientDescription.productUri,
                              &clientConfig.clientDescription.productUri));
    ck_assert(UA_String_equal(&cc2->clientDescription.applicationName.text,
                              &clientConfig.clientDescription.applicationName.text));
    ck_assert(UA_String_equal(&cc2->clientDescription.applicationName.locale,
                              &clientConfig.clientDescription.applicationName.locale));
    ck_assert_uint_eq(cc2->clientDescription.applicationType,
                      clientConfig.clientDescription.applicationType);
    ck_assert(UA_String_equal(&cc2->clientDescription.gatewayServerUri,
                              &clientConfig.clientDescription.gatewayServerUri));
    ck_assert(UA_String_equal(&cc2->clientDescription.discoveryProfileUri,
                              &clientConfig.clientDescription.discoveryProfileUri));
    ck_assert_uint_eq(cc2->clientDescription.discoveryUrlsSize,
                      clientConfig.clientDescription.discoveryUrlsSize);

    /* test connection configuration */
    ck_assert(UA_String_equal(&cc2->endpointUrl,
                              &clientConfig.endpointUrl));

    /* test session configuration */
    ck_assert_uint_eq(cc2->sessionLocaleIdsSize, clientConfig.sessionLocaleIdsSize);
    ck_assert(UA_String_equal(&cc2->sessionLocaleIds[0],
                              &clientConfig.sessionLocaleIds[0]));
    ck_assert(UA_String_equal(&cc2->sessionLocaleIds[1],
                              &clientConfig.sessionLocaleIds[1]));

    /* test session name */
    ck_assert(UA_String_equal(&cc2->sessionName,
                              &clientConfig.sessionName));

    /* test local connection configuration */
    ck_assert_uint_eq(cc2->localConnectionConfig.protocolVersion,
                      clientConfig.localConnectionConfig.protocolVersion);
    ck_assert_uint_eq(cc2->localConnectionConfig.recvBufferSize,
                      clientConfig.localConnectionConfig.recvBufferSize);
    ck_assert_uint_eq(cc2->localConnectionConfig.sendBufferSize,
                      clientConfig.localConnectionConfig.sendBufferSize);
    ck_assert_uint_eq(cc2->localConnectionConfig.localMaxMessageSize,
                      clientConfig.localConnectionConfig.localMaxMessageSize);
    ck_assert_uint_eq(cc2->localConnectionConfig.remoteMaxMessageSize,
                      clientConfig.localConnectionConfig.remoteMaxMessageSize);
    ck_assert_uint_eq(cc2->localConnectionConfig.localMaxChunkCount,
                      clientConfig.localConnectionConfig.localMaxChunkCount);
    ck_assert_uint_eq(cc2->localConnectionConfig.remoteMaxChunkCount,
                      clientConfig.localConnectionConfig.remoteMaxChunkCount);

    /* test client-level security URIs */
    ck_assert(UA_String_equal(&cc2->securityPolicyUri,
                              &clientConfig.securityPolicyUri));
    ck_assert(UA_String_equal(&cc2->authSecurityPolicyUri,
                              &clientConfig.authSecurityPolicyUri));

    /* test basic connection behavior flags */
    ck_assert_uint_eq(cc2->noSession, clientConfig.noSession);
    ck_assert_uint_eq(cc2->noReconnect, clientConfig.noReconnect);
    ck_assert_uint_eq(cc2->noNewSession, clientConfig.noNewSession);

    /* test advanced connection settings */
    ck_assert_uint_eq(cc2->secureChannelLifeTime, clientConfig.secureChannelLifeTime);
    ck_assert_uint_eq(cc2->requestedSessionTimeout, clientConfig.requestedSessionTimeout);
    ck_assert_uint_eq(cc2->connectivityCheckInterval, clientConfig.connectivityCheckInterval);
    ck_assert_uint_eq(cc2->tcpReuseAddr, clientConfig.tcpReuseAddr);

    /* test security/filtering fields */
    ck_assert(UA_String_equal(&cc2->applicationUri,
                              &clientConfig.applicationUri));
    ck_assert_uint_eq(cc2->allowNonePolicyPassword, clientConfig.allowNonePolicyPassword);

#ifdef UA_ENABLE_ENCRYPTION
    /* test encryption-related fields */
    ck_assert_uint_eq(cc2->maxTrustListSize, clientConfig.maxTrustListSize);
    ck_assert_uint_eq(cc2->maxRejectedListSize, clientConfig.maxRejectedListSize);
#endif

    /* test namespace array */
    ck_assert_uint_eq(cc2->namespacesSize, clientConfig.namespacesSize);

    /* test outstanding publish requests */
    ck_assert_uint_eq(cc2->outStandingPublishRequests, clientConfig.outStandingPublishRequests);

    UA_Client_delete(client);
    UA_ClientConfig_clear(&clientConfig);
    UA_ByteString_clear(&jsonConfig);
}
END_TEST

static Suite *testSuite_client_from_json(void) {
    Suite *s = suite_create("Loading Client and ClientConfig from JSON");

    TCase *tc_clientConfig = tcase_create("ClientConfig");
    tcase_add_test(tc_clientConfig, loadClientConfig);
    suite_add_tcase(s, tc_clientConfig);

    TCase *tc_client = tcase_create("Client");
    tcase_add_test(tc_client, loadClientAndClientConfigAndCompare);
    suite_add_tcase(s, tc_client);

    TCase *tc_both = tcase_create("Client and Server");
    tcase_add_checked_fixture(tc_both, setup, teardown);
    tcase_add_test(tc_both, loadClientFromConfigAndConnectToServer);
    suite_add_tcase(s,tc_both);

    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;
    s  = testSuite_client_from_json();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
