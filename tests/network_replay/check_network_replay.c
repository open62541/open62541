/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <check.h>

#include "test_helpers.h"
#include "testing_networklayers.h"
#include "../encryption/certificates.h"

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

/* For replay with encryption the local nonce needs to be generated in a
 * reproducible fashion. This works as the non-cryptographic RNG is statically
 * initialized. This is *not* the RNG that gets used otherwise for
 * encryption. */
static UA_StatusCode
reproducibleNonce(void *policyContext, UA_ByteString *out) {
    (void)policyContext;
    for(size_t i = 0; i < out->length; i += 4) {
        *(UA_UInt32*)(out->data + i) = UA_UInt32_random();
    }
    return UA_STATUSCODE_GOOD;
}

static UA_Client *
createReplayClient(const char *pcap) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_EventLoop *el = cc->eventLoop;

    /* Remove the default TCP ConnectionManager */
    UA_String tcpName = UA_STRING("tcp");
    for(UA_EventSource *es = el->eventSources; es; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(UA_String_equal(&tcpName, &cm->protocol)) {
            el->deregisterEventSource(el, es);
            cm->eventSource.free(&cm->eventSource);
            break;
        }
    }

    /* Change the path to the location of the current executable (Linux only) */
    char exe_path[PATH_MAX];
    ssize_t pathlen = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if(pathlen < PATH_MAX)
        exe_path[pathlen] = '\0'; /* Null-terminate the string */
    else
        exe_path[PATH_MAX-1] = '\0';
    char *last_slash = strrchr(exe_path, '/'); /* Find the last slash to isolate the directory */
    if(last_slash != NULL)
        *last_slash = '\0'; /* Remove the executable name to get the directory */
    chdir(exe_path); /* Change the current working directory */

    /* Add the replay ConnectionManager */
    UA_ConnectionManager *pcap_cm =
        ConnectionManage_replayPCAP(pcap, true);
    el->registerEventSource(el, &pcap_cm->eventSource);

    return client;
}

START_TEST(unified_cpp_none) {
    UA_Client *client = createReplayClient("../../../tests/network_replay/unified_cpp_none.pcap");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:48010");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant namespaceArray;
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, 2255),
                                           &namespaceArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&namespaceArray);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION
START_TEST(unified_cpp_basic256sha256) {
    UA_Client *client =
        createReplayClient("../../../tests/network_replay/unified_cpp_basic256sha256.pcap");

    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Secure client initialization */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    /* Replace the nonce-generating function in the SecurityPolicies */
    for(size_t i = 0; i < cc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &cc->securityPolicies[i];
        sp->symmetricModule.generateNonce = reproducibleNonce;
    }

    /* Reset the rng */
    UA_random_seed_deterministic(0);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:48010");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant namespaceArray;
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, 2255),
                                           &namespaceArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&namespaceArray);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(prosys_basic256sha256) {
    UA_Client *client =
        createReplayClient("../../../tests/network_replay/prosys_basic256sha256.pcap");

    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Secure client initialization */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    /* Replace the nonce-generating function in the SecurityPolicies */
    for(size_t i = 0; i < cc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &cc->securityPolicies[i];
        sp->symmetricModule.generateNonce = reproducibleNonce;
    }

    /* Reset the rng */
    UA_random_seed_deterministic(0);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://192.168.56.1:53530");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant namespaceArray;
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, 2255),
                                           &namespaceArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&namespaceArray);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST
#endif

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
START_TEST(softing_eccp256) {
    UA_Client *client =
        createReplayClient("../../../tests/network_replay/softing_eccp256.pcap");

    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    /* Secure client initialization */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");

    /* Replace the nonce-generating function in the SecurityPolicies */
    for(size_t i = 0; i < cc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &cc->securityPolicies[i];
        sp->symmetricModule.generateNonce = reproducibleNonce;
    }

    /* Reset the rng */
    UA_random_seed_deterministic(0);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://127.0.0.1:4880");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant namespaceArray;
    retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(0, 2255),
                                           &namespaceArray);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&namespaceArray);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST
#endif

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_test(tc_client, unified_cpp_none);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_client, unified_cpp_basic256sha256);
    tcase_add_test(tc_client, prosys_basic256sha256);
#endif
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    tcase_add_test(tc_client, softing_eccp256);
#endif
    suite_add_tcase(s, tc_client);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
