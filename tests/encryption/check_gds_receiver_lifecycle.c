/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/driver/gds_receiver.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include "test_helpers.h"
#include "certificates.h"

static UA_Server *server;
static UA_GDSReceiver *receiver;
static UA_Boolean serverStarted;

static UA_NodeId defaultApplicationGroup;
static UA_NodeId rsaSha256CertificateType;

static void
setup(void) {
    defaultApplicationGroup = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    rsaSha256CertificateType = UA_NODEID_NUMERIC(
        0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);

    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    server = UA_Server_newForUnitTestWithSecurityPolicies(
        4840, &certificate, &privateKey, NULL, 0, NULL, 0, NULL, 0);
    ck_assert_ptr_nonnull(server);

    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &receiver->drv),
                      UA_STATUSCODE_GOOD);
    serverStarted = false;
}

static void
teardown(void) {
    if(serverStarted)
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}

static void
startup(void) {
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    serverStarted = true;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);
}

static UA_StatusCode
updateCertificate(void) {
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    return UA_GDSReceiver_updateCertificate(
        receiver, defaultApplicationGroup, rsaSha256CertificateType,
        certificate, &privateKey);
}

static UA_StatusCode
createSigningRequest(void) {
    UA_ByteString csr = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_GDSReceiver_createSigningRequest(
        receiver, defaultApplicationGroup, rsaSha256CertificateType,
        NULL, NULL, NULL, &csr);
    UA_ByteString_clear(&csr);
    return res;
}

static void
checkHelpersDuringShutdown(void *application, void *context) {
    UA_Server *server_ = (UA_Server*)application;
    ck_assert_ptr_eq(server_, server);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server),
                      UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(updateCertificate(), UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(createSigningRequest(), UA_STATUSCODE_BADINVALIDSTATE);
    *(UA_Boolean*)context = true;
}

static UA_StatusCode
stageAndApplyCertificateUpdate(void) {
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    UA_String privateKeyFormat = UA_STRING_STATIC("DER");

    UA_Variant input[6];
    memset(input, 0, sizeof(input));
    UA_Variant_setScalar(&input[0], &defaultApplicationGroup,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &rsaSha256CertificateType,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&input[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[4], &privateKeyFormat,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[5], &privateKey,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    request.methodId = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE);
    request.inputArgumentsSize = 6;
    request.inputArguments = input;

    UA_CallMethodResult result = UA_Server_call(server, &request);
    UA_StatusCode res = result.statusCode;
    UA_CallMethodResult_clear(&result);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    request.methodId = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES);
    request.inputArgumentsSize = 0;
    request.inputArguments = NULL;
    result = UA_Server_call(server, &request);
    res = result.statusCode;
    UA_CallMethodResult_clear(&result);
    return res;
}

START_TEST(shutdownWhileApplyChangesPending) {
    startup();
    ck_assert_uint_eq(stageAndApplyCertificateUpdate(), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(shutdownWhileCertificateClosurePending) {
    startup();
    ck_assert_uint_eq(updateCertificate(), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(rejectDuplicateReceiver) {
    UA_GDSReceiver *duplicate = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(duplicate);
    ck_assert_uint_eq(UA_Server_addDriver(server, &duplicate->drv),
                      UA_STATUSCODE_BADALREADYEXISTS);
    ck_assert_uint_eq(duplicate->drv.free(&duplicate->drv),
                      UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(helpersBeforeStartupAndDuringShutdown) {
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    ck_assert_uint_eq(UA_GDSReceiver_updateCertificate(
                          NULL, defaultApplicationGroup,
                          rsaSha256CertificateType, certificate, &privateKey),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_GDSReceiver_updateCertificate(
                          receiver, defaultApplicationGroup,
                          rsaSha256CertificateType, UA_BYTESTRING_NULL,
                          &privateKey),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_GDSReceiver_createSigningRequest(
                          receiver, defaultApplicationGroup,
                          rsaSha256CertificateType, NULL, NULL, NULL, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(updateCertificate(), UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(createSigningRequest(), UA_STATUSCODE_BADINVALIDSTATE);

    startup();
    ck_assert_uint_eq(updateCertificate(), UA_STATUSCODE_GOOD);
    UA_Boolean callbackCalled = false;
    UA_DelayedCallback callback = {0};
    callback.callback = checkHelpersDuringShutdown;
    callback.application = server;
    callback.context = &callbackCalled;
    UA_EventLoop *eventLoop = UA_Server_getConfig(server)->eventLoop;
    eventLoop->addDelayedCallback(eventLoop, &callback);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert(callbackCalled);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(removeStoppedReceiver) {
    startup();
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
    void *methodContext = receiver;
    ck_assert_uint_eq(UA_Server_getNodeContext(
                          server,
                          UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES),
                          &methodContext),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(methodContext);
    ck_assert_uint_eq(UA_Server_removeDriver(server, &receiver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.free(&receiver->drv), UA_STATUSCODE_GOOD);
    receiver = NULL;

    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    ck_assert_uint_eq(UA_Server_addDriver(server, &receiver->drv),
                      UA_STATUSCODE_GOOD);
    startup();
    methodContext = NULL;
    ck_assert_uint_eq(UA_Server_getNodeContext(
                          server,
                          UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES),
                          &methodContext),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(methodContext, receiver);
} END_TEST

int
main(void) {
    Suite *suite = suite_create("GDS Receiver lifecycle");
    TCase *tc = tcase_create("lifecycle");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, shutdownWhileApplyChangesPending);
    tcase_add_test(tc, shutdownWhileCertificateClosurePending);
    tcase_add_test(tc, rejectDuplicateReceiver);
    tcase_add_test(tc, helpersBeforeStartupAndDuringShutdown);
    tcase_add_test(tc, removeStoppedReceiver);
    suite_add_tcase(suite, tc);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
