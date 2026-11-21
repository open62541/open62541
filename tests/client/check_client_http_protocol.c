/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <check.h>
#include <stdlib.h>

#include "ua_client_internal.h"

typedef struct {
    UA_ConnectionManager cm;
    UA_Boolean opened;
    UA_Boolean useTls;
    UA_UInt16 port;
    UA_String address;
    UA_String path;
} TestHttpManager;

static UA_StatusCode
captureOpen(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
            void *application, void *context,
            UA_ConnectionManager_connectionCallback callback) {
    (void)application;
    (void)context;
    (void)callback;
    TestHttpManager *manager = (TestHttpManager *)cm;
    const UA_UInt16 *port = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "port"), &UA_TYPES[UA_TYPES_UINT16]);
    const UA_Boolean *useTls = (const UA_Boolean *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "tls"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_String *address = (const UA_String *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "address"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *path = (const UA_String *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "path"), &UA_TYPES[UA_TYPES_STRING]);
    const char *hidden[] = {"tls-context", "certificate", "private-key", "ca-certificate"};
    for(size_t i = 0; i < 4; i++)
        ck_assert_ptr_null(UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)hidden[i])));
    ck_assert_ptr_nonnull(port);
    ck_assert_ptr_nonnull(useTls);
    ck_assert_ptr_nonnull(address);
    ck_assert_ptr_nonnull(path);
    manager->opened = true;
    manager->port = *port;
    manager->useTls = *useTls;
    manager->address = *address;
    manager->path = *path;
    return UA_STATUSCODE_GOOD;
}

START_TEST(httpConfigurationValidation) {
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *config = UA_Client_getConfig(client);

    ck_assert_uint_eq(__Client_validateHttpConnection(client, false),
                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
    config->httpAllowUnencrypted = true;
    ck_assert_uint_eq(__Client_validateHttpConnection(client, false),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(__Client_validateHttpConnection(client, true), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
}
END_TEST

START_TEST(httpProviderReceivesConnectionParameters) {
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    TestHttpManager manager;
    memset(&manager, 0, sizeof(manager));
    manager.cm.openConnection = captureOpen;
    const UA_String hostname = UA_STRING_STATIC("example.test");
    const UA_String path = UA_STRING_STATIC("/binary");
    ck_assert_uint_eq(__Client_openHttpConnection(
                          client, &manager.cm, &hostname, 8443, &path, true,
                          UA_SECURECHANNEL_ENCODING_BINARY),
                      UA_STATUSCODE_GOOD);
    ck_assert(manager.opened);
    ck_assert(manager.useTls);
    ck_assert_uint_eq(manager.port, 8443);
    ck_assert(UA_String_equal(&manager.address, &hostname));
    ck_assert(UA_String_equal(&manager.path, &path));
    ck_assert_uint_eq(client->channel.transport,
                      UA_SECURECHANNEL_TRANSPORT_HTTP);
    ck_assert_uint_eq(client->channel.encoding,
                      UA_SECURECHANNEL_ENCODING_BINARY);
    UA_Client_delete(client);
}
END_TEST

START_TEST(clientRequiresExplicitTransportManager) {
    const char *urls[] = {"opc.http://localhost:4840/ua",
                          "opc.https://localhost:4840/ua",
                          "opc.ws://localhost:4840/ua",
                          "opc.wss://localhost:4840/ua"};
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    UA_Client_getConfig(client)->httpAllowUnencrypted = true;
    ck_assert_uint_eq(UA_Client_connect(client, urls[_i]), UA_STATUSCODE_BADNOTSUPPORTED);
    /* Connecting must not install a provider implicitly either. */
    const UA_String http = UA_STRING_STATIC("http"), ws = UA_STRING_STATIC("websocket");
    for(UA_EventSource *es = UA_Client_getConfig(client)->eventLoop->eventSources;
        es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        ck_assert(!UA_String_equal(&cm->protocol, &http));
        ck_assert(!UA_String_equal(&cm->protocol, &ws));
    }
    UA_Client_delete(client);
} END_TEST

static void
configureDirectEndpoint(UA_ClientConfig *config, const char *url,
                        const char *profile, const char *policy,
                        UA_MessageSecurityMode mode,
                        UA_SecurityPolicyType policyType) {
    ck_assert_uint_gt(config->securityPoliciesSize, 0);
    config->securityPolicies[0].policyUri = UA_STRING((char *)(uintptr_t)policy);
    config->securityPolicies[0].policyType = policyType;
    config->endpoint.endpointUrl = UA_STRING_ALLOC(url);
    config->endpoint.transportProfileUri = UA_STRING_ALLOC(profile);
    config->endpoint.securityPolicyUri = UA_STRING_ALLOC(policy);
    config->endpoint.securityMode = mode;
    config->noSession = true;
}

START_TEST(httpSelectedSecurityPolicyValidation) {
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->httpAllowUnencrypted = true;
    configureDirectEndpoint(
        config, "opc.http://127.0.0.1:4840/binary",
        "http://open62541.org/UA-Profile/Transport/http-uabinary",
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256",
        UA_MESSAGESECURITYMODE_NONE, UA_SECURITYPOLICYTYPE_RSA);
    ck_assert_uint_eq(
        UA_Client_connect(client, "opc.http://127.0.0.1:4840/binary"),
        UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
    UA_Client_delete(client);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION
START_TEST(httpSelectedEnhancedSecurityPolicyValidation) {
    UA_Client *client = UA_Client_new();
    ck_assert_ptr_nonnull(client);
    UA_ClientConfig *config = UA_Client_getConfig(client);
    configureDirectEndpoint(
        config, "opc.https://127.0.0.1:4840/binary",
        "http://opcfoundation.org/UA-Profile/Transport/https-uabinary",
        "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm",
        UA_MESSAGESECURITYMODE_SIGNANDENCRYPT,
        UA_SECURITYPOLICYTYPE_ECC_AEAD);
    /* The policy declares the SecureChannelEnhancements, the URI alone does
     * not make it enhanced */
    config->securityPolicies[0].secureChannelEnhancements = true;
    ck_assert_uint_eq(
        UA_Client_connect(client, "opc.https://127.0.0.1:4840/binary"),
        UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
    UA_Client_delete(client);
}
END_TEST
#endif

int main(void) {
    Suite *suite = suite_create("OPC UA HTTP client protocol");
    TCase *tc = tcase_create("provider-neutral setup");
    tcase_add_test(tc, httpConfigurationValidation);
    tcase_add_loop_test(tc, clientRequiresExplicitTransportManager, 0, 4);
    tcase_add_test(tc, httpProviderReceivesConnectionParameters);
    tcase_add_test(tc, httpSelectedSecurityPolicyValidation);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, httpSelectedEnhancedSecurityPolicyValidation);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
