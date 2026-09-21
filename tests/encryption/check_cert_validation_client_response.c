/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>

#include "certificates.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_atomic(uintptr_t) running;
static THREAD_HANDLE server_thread;
static UA_StatusCode forcedVerifyStatus;

typedef enum {
    ENDPOINTS_UNCHANGED,
    ENDPOINTS_REORDERED,
    ENDPOINT_METADATA_OMITTED,
    ENDPOINT_SECURITYLEVEL_CHANGED
} EndpointMutation;

static EndpointMutation endpointMutation;

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_StatusCode
forceCertVerifyStatus(UA_CertificateGroup *certGroup,
                      const UA_ByteString *certificate) {
    (void)certGroup;
    (void)certificate;
    return forcedVerifyStatus;
}

static void
mutateCreateSessionEndpoints(UA_Server *server_,
                             UA_ApplicationNotificationType type,
                             const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN ||
       endpointMutation == ENDPOINTS_UNCHANGED)
        return;

    const UA_NodeId *serviceType =
        (const UA_NodeId*)payload.map[3].value.data;
    if(!UA_NodeId_equal(serviceType,
                        &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST].typeId))
        return;

    UA_ServerConfig *config = UA_Server_getConfig(server_);
    ck_assert_uint_gt(config->endpointsSize, 1);
    if(endpointMutation == ENDPOINTS_REORDERED) {
        UA_EndpointDescription tmp = config->endpoints[0];
        config->endpoints[0] = config->endpoints[config->endpointsSize - 1];
        config->endpoints[config->endpointsSize - 1] = tmp;
    } else if(endpointMutation == ENDPOINT_METADATA_OMITTED) {
        /* CreateSession may omit descriptive application metadata. Keep the
         * ApplicationUri, which is one of the required comparison fields. */
        UA_ApplicationDescription *ad = &config->applicationDescription;
        UA_LocalizedText_clear(&ad->applicationName);
        UA_String_clear(&ad->productUri);
        UA_String_clear(&ad->gatewayServerUri);
        UA_String_clear(&ad->discoveryProfileUri);
        UA_Array_delete(ad->discoveryUrls, ad->discoveryUrlsSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        ad->discoveryUrls = NULL;
        ad->discoveryUrlsSize = 0;
    } else {
        const UA_String policy = UA_STRING(
            "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
        for(size_t i = 0; i < config->endpointsSize; i++) {
            if(UA_String_equal(&config->endpoints[i].securityPolicyUri, &policy))
                config->endpoints[i].securityLevel++;
        }
    }
    endpointMutation = ENDPOINTS_UNCHANGED;
}

static void
setupServer(const char *applicationUri) {
    UA_atomic_store(&running, true);

    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    size_t trustListSize = 0;
    UA_ByteString *trustList = NULL;
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    size_t revocationListSize = 0;
    UA_ByteString *revocationList = NULL;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);
    config->secureChannelPKI.verifyCertificate = forceCertVerifyStatus;
    config->serviceNotificationCallback = mutateCreateSessionEndpoints;
    endpointMutation = ENDPOINTS_UNCHANGED;

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC(applicationUri);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void
setup(void) {
    setupServer("urn:open62541.unconfigured.application");
}

static void
setupMismatchingApplicationUri(void) {
    setupServer("urn:mismatching:application");
}

static void
teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_Client *
newSecureClient(void) {
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);

    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:unconfigured:application");
    return client;
}

START_TEST(testOpenSecureChannelCertificateFailuresHidden) {
    static const UA_StatusCode certErrors[] = {
        UA_STATUSCODE_BADCERTIFICATEINVALID,
        UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE,
        UA_STATUSCODE_BADCERTIFICATEPOLICYCHECKFAILED,
        UA_STATUSCODE_BADCERTIFICATEUNTRUSTED,
        UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN,
        UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN,
        UA_STATUSCODE_BADCERTIFICATEREVOKED,
        UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED
    };

    for(size_t i = 0; i < (sizeof(certErrors) / sizeof(certErrors[0])); i++) {
        forcedVerifyStatus = certErrors[i];

        UA_Client *client = newSecureClient();
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

        ck_assert_msg(retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED,
                      "verifyCertificate returned %s, but client connect returned %s",
                      UA_StatusCode_name(certErrors[i]), UA_StatusCode_name(retval));

        UA_Client_delete(client);
    }
}
END_TEST

START_TEST(testCreateSessionAcceptsMatchingServerApplicationUri) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;

    UA_Client *client = newSecureClient();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionAcceptsReorderedEndpoints) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;
    endpointMutation = ENDPOINTS_REORDERED;

    UA_Client *client = newSecureClient();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionAcceptsOmittedEndpointMetadata) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;
    endpointMutation = ENDPOINT_METADATA_OMITTED;

    UA_Client *client = newSecureClient();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionRejectsMismatchingServerApplicationUri) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;

    UA_Client *client = newSecureClient();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_msg(retval == UA_STATUSCODE_BADCERTIFICATEURIINVALID,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_Client_getState(client, &channelState, &sessionState, NULL);
    ck_assert_int_eq(channelState, UA_SECURECHANNELSTATE_CLOSED);
    ck_assert_int_eq(sessionState, UA_SESSIONSTATE_CLOSED);

    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionRejectsChangedSelectedEndpointByDefault) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;
    endpointMutation = ENDPOINT_SECURITYLEVEL_CHANGED;

    UA_Client *client = newSecureClient();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionWarnsForChangedSelectedEndpointWithWarnRule) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;
    endpointMutation = ENDPOINT_SECURITYLEVEL_CHANGED;

    UA_Client *client = newSecureClient();
    UA_Client_getConfig(client)->endpointDescriptionRule =
        UA_RULEHANDLING_WARN;
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(testCreateSessionIgnoresEndpointsForDirectConfiguration) {
    forcedVerifyStatus = UA_STATUSCODE_GOOD;

    UA_Client *discoveryClient = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(discoveryClient);
    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpoints(discoveryClient, "opc.tcp://localhost:4840",
                               &endpointsSize, &endpoints);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_delete(discoveryClient);

    const UA_String policy = UA_STRING(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    size_t selected = endpointsSize;
    for(size_t i = 0; i < endpointsSize; i++) {
        if(!UA_String_equal(&endpoints[i].securityPolicyUri, &policy))
            continue;
        if(selected == endpointsSize ||
           endpoints[i].securityLevel > endpoints[selected].securityLevel)
            selected = i;
    }
    ck_assert_uint_lt(selected, endpointsSize);

    UA_Client *client = newSecureClient();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->endpointDescriptionRule = UA_RULEHANDLING_ABORT;
    retval = UA_EndpointDescription_copy(&endpoints[selected], &cc->endpoint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    endpointMutation = ENDPOINT_SECURITYLEVEL_CHANGED;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                  "client connect returned %s",
                  UA_StatusCode_name(retval));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite *
testSuite_create(void) {
    Suite *s = suite_create("Certificate Validation Client Response");
    TCase *tc = tcase_create("OpenSecureChannel Certificate Failures Hidden");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, testOpenSecureChannelCertificateFailuresHidden);
    tcase_add_test(tc, testCreateSessionAcceptsMatchingServerApplicationUri);
    tcase_add_test(tc, testCreateSessionAcceptsReorderedEndpoints);
    tcase_add_test(tc, testCreateSessionAcceptsOmittedEndpointMetadata);
    tcase_add_test(tc, testCreateSessionRejectsChangedSelectedEndpointByDefault);
    tcase_add_test(tc,
                   testCreateSessionWarnsForChangedSelectedEndpointWithWarnRule);
    tcase_add_test(tc,
                   testCreateSessionIgnoresEndpointsForDirectConfiguration);
    suite_add_tcase(s, tc);

    TCase *tcApplicationUri = tcase_create("Server ApplicationUri");
    tcase_add_checked_fixture(tcApplicationUri,
                              setupMismatchingApplicationUri, teardown);
    tcase_add_test(tcApplicationUri,
                   testCreateSessionRejectsMismatchingServerApplicationUri);
    suite_add_tcase(s, tcApplicationUri);
    return s;
}

int
main(void) {
    Suite *s = testSuite_create();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
