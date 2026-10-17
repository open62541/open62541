/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/driver/gds_receiver.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include "test_helpers.h"
#include "certificates.h"
#include "testing_clock.h"
#include "server/ua_server_internal.h"

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
callServerConfigurationMethod(UA_UInt32 methodId, size_t inputSize,
                              UA_Variant *input) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    request.methodId = UA_NODEID_NUMERIC(0, methodId);
    request.inputArgumentsSize = inputSize;
    request.inputArguments = input;

    UA_CallMethodResult result = UA_Server_call(server, &request);
    UA_StatusCode res = result.statusCode;
    UA_CallMethodResult_clear(&result);
    return res;
}

/* The UpdateCertificate Method with the test certificate and key */
static UA_StatusCode
callUpdateCertificate(UA_NodeId *certificateGroupId,
                      UA_NodeId *certificateTypeId) {
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    UA_String privateKeyFormat = UA_STRING_STATIC("DER");

    UA_Variant input[6];
    memset(input, 0, sizeof(input));
    UA_Variant_setScalar(&input[0], certificateGroupId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], certificateTypeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&input[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&input[4], &privateKeyFormat,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[5], &privateKey,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    return callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE, 6, input);
}

static UA_StatusCode
stageAndApplyCertificateUpdate(void) {
    UA_StatusCode res = callUpdateCertificate(&defaultApplicationGroup,
                                              &rsaSha256CertificateType);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL);
}

START_TEST(shutdownWhileApplyChangesPending) {
    startup();
    ck_assert_uint_eq(stageAndApplyCertificateUpdate(), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

/* The transaction is discarded when the queued ApplyChanges runs. An update
 * staged in between would be lost and must be refused. */
START_TEST(rejectUpdateWhileApplyChangesQueued) {
    startup();
    ck_assert_uint_eq(stageAndApplyCertificateUpdate(), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callUpdateCertificate(&defaultApplicationGroup,
                                            &rsaSha256CertificateType),
                      UA_STATUSCODE_BADTRANSACTIONPENDING);

    /* The delayed ApplyChanges completes the transaction */
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(stageAndApplyCertificateUpdate(), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(shutdownWhileCertificateClosurePending) {
    startup();
    ck_assert_uint_eq(updateCertificate(), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    serverStarted = false;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

/* Part 12, 7.10.5: a null CertificateGroupId selects the
 * DefaultApplicationGroup, an unsupported group or type is an invalid
 * argument */
START_TEST(updateCertificateArguments) {
    startup();
    UA_NodeId nullId = UA_NODEID_NULL;
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    ck_assert_uint_eq(callUpdateCertificate(&objectsFolder,
                                            &rsaSha256CertificateType),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(callUpdateCertificate(&defaultApplicationGroup, &nullId),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(callUpdateCertificate(&nullId, &rsaSha256CertificateType),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callServerConfigurationMethod(
                          UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL),
                      UA_STATUSCODE_GOOD);
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

/* Without an RSA endpoint, no certificate type can be updated. The
 * CertificateTypes are an empty array, and the methods are still bound. */
START_TEST(noUpdatableCertificateType) {
    UA_Server *plainServer = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(plainServer);
    UA_GDSReceiver *plainReceiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(plainReceiver);
    ck_assert_uint_eq(UA_Server_addDriver(plainServer, &plainReceiver->drv),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(plainServer), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(plainReceiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(plainServer, UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_CERTIFICATETYPES),
        &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_NODEID]));
    ck_assert_uint_eq(value.arrayLength, 0);
    UA_Variant_clear(&value);

    void *methodContext = NULL;
    ck_assert_uint_eq(UA_Server_getNodeContext(
                          plainServer,
                          UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES),
                          &methodContext),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(methodContext, plainReceiver);

    ck_assert_uint_eq(UA_Server_run_shutdown(plainServer), UA_STATUSCODE_GOOD);
    UA_Server_delete(plainServer);
} END_TEST

#ifdef UA_ENABLE_RBAC
/* Only the role setup needs the CertificateGroups folder. Without it, the role
 * setup fails. This is only logged, as the AccessControl can restrict the
 * methods as well. The receiver starts with its push methods. */
START_TEST(failedRoleSetupKeepsMethods) {
    const UA_NodeId folder = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS);
    const UA_UInt32 groups[] = {
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP
    };
    /* Detach the groups so that they are not deleted with the folder */
    for(size_t i = 0; i < sizeof(groups) / sizeof(groups[0]); i++) {
        ck_assert_uint_eq(UA_Server_deleteReference(
                              server, folder, UA_NS0ID(HASCOMPONENT), true,
                              UA_EXPANDEDNODEID_NUMERIC(0, groups[i]), true),
                          UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(UA_Server_deleteNode(server, folder, true),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    serverStarted = true;
    ck_assert_uint_eq(receiver->drv.state, UA_LIFECYCLESTATE_STARTED);

    void *methodContext = NULL;
    ck_assert_uint_eq(UA_Server_getNodeContext(
                          server,
                          UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE),
                          &methodContext),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(methodContext, receiver);
    ck_assert_uint_eq(stageAndApplyCertificateUpdate(), UA_STATUSCODE_GOOD);
} END_TEST
#endif

static void
setupLocalMethods(void) {
    setup();
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_GOOD);
}

static void
teardownLocalMethods(void) {
    receiver->drv.stop(&receiver->drv);
    teardown();
}

static unsigned rejectedGroups;
static int rejectedFailureGroup;
static unsigned rejectedCalls[2];

static UA_StatusCode
getRejectedListFixture(UA_CertificateGroup *group, UA_ByteString **list, size_t *size) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t index = group == &config->secureChannelPKI ? 0 : 1;
    rejectedCalls[index]++;
    UA_ByteString certificates[2] = {
        {CERT_DER_LENGTH, CERT_DER_DATA},
        {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA}
    };
    *size = (rejectedGroups & (1u << index)) ? (2 - index) : 0;
    UA_StatusCode res = UA_Array_copy(certificates, *size, (void **)list,
                                      &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    /* A failing backend can still return owned partial results. */
    if((int)index == rejectedFailureGroup)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    return UA_STATUSCODE_GOOD;
}

START_TEST(rejectedListsCombineCertificateGroups) {
    rejectedGroups = _i < 4 ? (unsigned)_i : 3;
    rejectedFailureGroup = _i < 4 ? -1 : _i - 4;
    memset(rejectedCalls, 0, sizeof(rejectedCalls));
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->secureChannelPKI.getRejectedList = getRejectedListFixture;
    config->sessionPKI.getRejectedList = getRejectedListFixture;

    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    request.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST);
    UA_CallMethodResult result = UA_Server_call(server, &request);
    ck_assert_uint_eq(rejectedCalls[0], 1);
    ck_assert_uint_eq(rejectedCalls[1], rejectedFailureGroup == 0 ? 0 : 1);
    if(rejectedFailureGroup >= 0) {
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
        for(size_t i = 0; i < result.outputArgumentsSize; i++)
            ck_assert(UA_Variant_isEmpty(&result.outputArguments[i]));
    } else {
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        UA_Variant *output = &result.outputArguments[0];
        ck_assert(UA_Variant_hasArrayType(output, &UA_TYPES[UA_TYPES_BYTESTRING]));
        size_t count = (rejectedGroups & 1u ? 2 : 0) + (rejectedGroups & 2u ? 1 : 0);
        ck_assert_uint_eq(output->arrayLength, count);
        UA_ByteString *certificates = (UA_ByteString *)output->data;
        UA_ByteString expected[2] = {
            {CERT_DER_LENGTH, CERT_DER_DATA},
            {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA}
        };
        size_t offset = 0;
        if(rejectedGroups & 1u) {
            ck_assert(UA_ByteString_equal(&certificates[0], &expected[0]));
            ck_assert(UA_ByteString_equal(&certificates[1], &expected[1]));
            offset = 2;
        }
        if(rejectedGroups & 2u)
            ck_assert(UA_ByteString_equal(&certificates[offset], &expected[0]));
    }
    UA_CallMethodResult_clear(&result);
} END_TEST

#define APP_TRUSTLIST_ID(SUFFIX) \
    UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_##SUFFIX

static void
setupLocalTrustLists(void) {
    setup();
    /* The lifecycle fixture accepts all certificates. File operations need
     * certificate groups that actually store and update trust lists. */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup *groups[2] = {&config->secureChannelPKI, &config->sessionPKI};
    UA_NodeId ids[2] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP)
    };
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    for(size_t i = 0; i < 2; i++) {
        groups[i]->clear(groups[i]);
        ck_assert_uint_eq(UA_CertificateGroup_Memorystore(groups[i], &ids[i], &list,
            config->logging, NULL), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(receiver->drv.start(&receiver->drv), UA_STATUSCODE_GOOD);
}

static UA_StatusCode
callTrustList(UA_UInt32 objectId, UA_UInt32 methodId, size_t inputSize,
               UA_Variant *input, UA_Variant *output) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, objectId);
    request.methodId = UA_NODEID_NUMERIC(0, methodId);
    request.inputArgumentsSize = inputSize;
    request.inputArguments = input;
    UA_CallMethodResult result = UA_Server_call(server, &request);
    UA_StatusCode res = result.statusCode;
    if(res == UA_STATUSCODE_GOOD && output) {
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        res = UA_Variant_copy(&result.outputArguments[0], output);
    }
    UA_CallMethodResult_clear(&result);
    return res;
}

static UA_StatusCode
callLocalTrustList(UA_UInt32 methodId, size_t inputSize, UA_Variant *input,
                   UA_Variant *output) {
    return callTrustList(
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
        methodId, inputSize, input, output);
}

static UA_UInt32
openLocalTrustList(UA_Byte mode) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Variant_init(&output);
    ck_assert_uint_eq(callLocalTrustList(APP_TRUSTLIST_ID(OPEN), 1, &input, &output),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT32]));
    UA_UInt32 handle = *(UA_UInt32 *)output.data;
    UA_Variant_clear(&output);
    return handle;
}

static UA_StatusCode
localFileOperation(UA_UInt32 methodId, UA_UInt32 handle, void *argument,
                    const UA_DataType *argumentType, UA_Variant *output) {
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    if(argument)
        UA_Variant_setScalar(&input[1], argument, argumentType);
    return callLocalTrustList(methodId, argument ? 2 : 1, input, output);
}

static void
assertTrustListOpenCount(UA_UInt16 count) {
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, APP_TRUSTLIST_ID(OPENCOUNT)), &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_uint_eq(*(UA_UInt16 *)value.data, count);
    UA_Variant_clear(&value);
}

static void
assertTrustListPosition(UA_UInt32 handle, UA_UInt64 position) {
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(GETPOSITION), handle,
        NULL, NULL, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64]));
    ck_assert_uint_eq(*(UA_UInt64 *)value.data, position);
    UA_Variant_clear(&value);
}

START_TEST(trustListReadPositions) {
    UA_UInt32 handle = openLocalTrustList(UA_OPENFILEMODE_READ);
    assertTrustListOpenCount(1);
    UA_Int32 length = 512;
    UA_Variant original;
    UA_Variant_init(&original);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(READ), handle, &length,
        &UA_TYPES[UA_TYPES_INT32], &original), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&original, &UA_TYPES[UA_TYPES_BYTESTRING]));
    UA_ByteString *bytes = (UA_ByteString *)original.data;
    ck_assert_uint_ge(bytes->length, 7);
    assertTrustListPosition(handle, bytes->length);

    UA_UInt64 position = 2;
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(SETPOSITION), handle, &position,
        &UA_TYPES[UA_TYPES_UINT64], NULL), UA_STATUSCODE_GOOD);
    assertTrustListPosition(handle, 2);
    length = 5;
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(READ), handle, &length,
        &UA_TYPES[UA_TYPES_INT32], &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTESTRING]));
    UA_ByteString expected = {5, bytes->data + 2};
    ck_assert(UA_ByteString_equal((UA_ByteString *)value.data, &expected));
    UA_Variant_clear(&value);
    assertTrustListPosition(handle, 7);

    position = UA_UINT64_MAX;
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(SETPOSITION), handle, &position,
        &UA_TYPES[UA_TYPES_UINT64], NULL), UA_STATUSCODE_GOOD);
    assertTrustListPosition(handle, bytes->length);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(READ), handle, &length,
        &UA_TYPES[UA_TYPES_INT32], &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTESTRING]));
    ck_assert_uint_eq(((UA_ByteString *)value.data)->length, 0);
    UA_Variant_clear(&value);
    assertTrustListPosition(handle, bytes->length);
    UA_Variant_clear(&original);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSE), handle,
        NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(0);
} END_TEST

START_TEST(trustListInvalidHandlePreservesOpenFile) {
    UA_UInt32 handle = openLocalTrustList(UA_OPENFILEMODE_READ);
    const UA_UInt32 methods[] = {
        APP_TRUSTLIST_ID(GETPOSITION), APP_TRUSTLIST_ID(SETPOSITION),
        APP_TRUSTLIST_ID(READ), APP_TRUSTLIST_ID(WRITE),
        APP_TRUSTLIST_ID(CLOSE), APP_TRUSTLIST_ID(CLOSEANDUPDATE)
    };
    UA_UInt64 position = 1;
    UA_Int32 length = 1;
    UA_ByteString bytes = UA_BYTESTRING("x");
    void *argument = NULL;
    const UA_DataType *type = NULL;
    if(_i == 1) {
        argument = &position;
        type = &UA_TYPES[UA_TYPES_UINT64];
    } else if(_i == 2) {
        argument = &length;
        type = &UA_TYPES[UA_TYPES_INT32];
    } else if(_i == 3) {
        argument = &bytes;
        type = &UA_TYPES[UA_TYPES_BYTESTRING];
    }
    ck_assert_uint_eq(localFileOperation(methods[_i], handle + 1, argument, type, NULL),
                      UA_STATUSCODE_BADINTERNALERROR);
    assertTrustListOpenCount(1);
    assertTrustListPosition(handle, 0);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSE), handle,
        NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(0);
} END_TEST

START_TEST(trustListRejectsInvalidOpenMode) {
    UA_Byte modes[] = {0, UA_OPENFILEMODE_WRITE, UA_OPENFILEMODE_APPEND,
                      UA_OPENFILEMODE_READ | UA_OPENFILEMODE_WRITE,
                      UA_OPENFILEMODE_READ | UA_OPENFILEMODE_ERASEEXISTING};
    UA_Variant input;
    UA_Variant_setScalar(&input, &modes[_i], &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_uint_eq(callLocalTrustList(APP_TRUSTLIST_ID(OPEN), 1, &input, NULL),
                      UA_STATUSCODE_BADINVALIDSTATE);
    assertTrustListOpenCount(0);
} END_TEST

START_TEST(trustListRejectsWrongAccessMode) {
    UA_Byte mode = _i == 0 ? UA_OPENFILEMODE_READ :
        (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING);
    UA_UInt32 handle = openLocalTrustList(mode);
    if(_i == 0) {
        UA_ByteString data = UA_BYTESTRING("x");
        ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &data,
            &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_BADINVALIDSTATE);
        ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSEANDUPDATE), handle,
            NULL, NULL, NULL), UA_STATUSCODE_BADINVALIDSTATE);
    } else {
        UA_Int32 length = 1;
        ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(READ), handle, &length,
            &UA_TYPES[UA_TYPES_INT32], NULL), UA_STATUSCODE_BADINVALIDSTATE);
    }
    assertTrustListOpenCount(1);
    assertTrustListPosition(handle, 0);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSE), handle,
        NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(0);
} END_TEST

START_TEST(trustListRejectedWritePreservesData) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&list, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
        &encoded, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(encoded.length, 4);
    UA_Server_getConfig(server)->maxTrustListSize = (UA_UInt32)encoded.length;
    UA_UInt32 handle = openLocalTrustList(UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING);
    UA_ByteString prefix = {4, encoded.data};
    UA_ByteString suffix = {encoded.length - 4, encoded.data + 4};
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &prefix,
        &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &encoded,
        &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_BADREQUESTTOOLARGE);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &suffix,
        &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_GOOD);
    UA_Variant output;
    UA_Variant_init(&output);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSEANDUPDATE), handle,
        NULL, NULL, &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(*(UA_Boolean *)output.data);
    UA_Variant_clear(&output);
    assertTrustListOpenCount(0);
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(trustListMalformedUpdateKeepsHandle) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&list, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
        &encoded, NULL), UA_STATUSCODE_GOOD);
    UA_UInt32 handle = openLocalTrustList(UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING);
    UA_ByteString truncated = {3, encoded.data};
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &truncated,
        &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSEANDUPDATE), handle,
        NULL, NULL, NULL), UA_STATUSCODE_BADINTERNALERROR);
    assertTrustListOpenCount(1);
    /* The failed update keeps the handle and its data available for a retry. */
    UA_ByteString suffix = {encoded.length - 3, encoded.data + 3};
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(WRITE), handle, &suffix,
        &UA_TYPES[UA_TYPES_BYTESTRING], NULL), UA_STATUSCODE_GOOD);
    UA_Variant output;
    UA_Variant_init(&output);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSEANDUPDATE), handle,
        NULL, NULL, &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(*(UA_Boolean *)output.data);
    UA_Variant_clear(&output);
    assertTrustListOpenCount(0);
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(trustListReadersHaveIndependentHandles) {
    UA_UInt32 first = openLocalTrustList(UA_OPENFILEMODE_READ);
    UA_UInt32 second = openLocalTrustList(UA_OPENFILEMODE_READ);
    ck_assert_uint_ne(first, second);
    assertTrustListOpenCount(2);
    UA_Byte writeMode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    UA_Variant input;
    UA_Variant_setScalar(&input, &writeMode, &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_uint_eq(callLocalTrustList(APP_TRUSTLIST_ID(OPEN), 1, &input, NULL),
                      UA_STATUSCODE_BADNOTWRITABLE);
    UA_UInt64 position = 2;
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(SETPOSITION), first, &position,
        &UA_TYPES[UA_TYPES_UINT64], NULL), UA_STATUSCODE_GOOD);
    assertTrustListPosition(first, 2);
    assertTrustListPosition(second, 0);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSE), first,
        NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(1);
    assertTrustListPosition(second, 0);
    ck_assert_uint_eq(localFileOperation(APP_TRUSTLIST_ID(CLOSE), second,
        NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(0);
} END_TEST

static void
setupSessionCleanup(void) {
    setupLocalTrustLists();
    /* Real Sessions require a running server, even for local file methods. */
    UA_Server_getConfig(server)->tcpEnabled = false;
    startup();
}

static UA_Session *
newLocalSession(void) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);
    request.requestedSessionTimeout = 120000;
    UA_Session *session = NULL;
    lockServer(server);
    UA_StatusCode res = UA_Session_create(server, NULL, &request, &session);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(session, NULL);
    return session;
}

static void
closeLocalSession(UA_Session *session) {
    lockServer(server);
    UA_Session_remove(server, session, UA_SHUTDOWNREASON_CLOSE);
    unlockServer(server);
}

static UA_StatusCode
callMethodForSession(const UA_NodeId *sessionId, UA_UInt32 objectNumericId,
                      UA_UInt32 methodNumericId, size_t inputSize,
                      UA_Variant *input, size_t outputSize, UA_Variant *output) {
    /* UA_Server_call uses the administrator Session. Invoke the registered
     * handler with an explicit Session to check file ownership and cleanup. */
    UA_NodeId objectId = UA_NODEID_NUMERIC(0, objectNumericId);
    UA_NodeId methodId = UA_NODEID_NUMERIC(0, methodNumericId);
    UA_MethodCallback method = NULL;
    void *context = NULL;
    ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server, methodId, &method),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getNodeContext(server, methodId, &context),
                      UA_STATUSCODE_GOOD);
    ck_assert(method != NULL);
    return method(server, sessionId, NULL, &methodId, context, &objectId, NULL,
                  inputSize, input, outputSize, output);
}

static UA_StatusCode
openTrustListForSession(const UA_NodeId *sessionId, UA_Byte mode, UA_UInt32 *handle) {
    UA_Variant input, output;
    UA_Variant_setScalar(&input, &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Variant_init(&output);
    UA_StatusCode res = callMethodForSession(sessionId,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
        APP_TRUSTLIST_ID(OPEN), 1, &input, 1, &output);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT32]));
        *handle = *(UA_UInt32 *)output.data;
    } else {
        ck_assert(UA_Variant_isEmpty(&output));
    }
    UA_Variant_clear(&output);
    return res;
}

static UA_StatusCode
fileMethodForSession(const UA_NodeId *sessionId, UA_UInt32 method,
                      UA_UInt32 handle, UA_Variant *output) {
    UA_Variant input;
    UA_Variant_setScalar(&input, &handle, &UA_TYPES[UA_TYPES_UINT32]);
    return callMethodForSession(sessionId,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
        method, 1, &input, output ? 1 : 0, output);
}

static void
runSessionCleanupTimer(void) {
    /* The receiver checks its owning Sessions every ten seconds. */
    UA_fakeSleep(10001);
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
}

START_TEST(trustListSessionClosurePreservesOtherReaders) {
    UA_Session *first = newLocalSession();
    UA_Session *second = newLocalSession();
    UA_NodeId firstId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_NodeId_copy(&first->sessionId, &firstId), UA_STATUSCODE_GOOD);
    UA_UInt32 firstHandle = 0, secondHandle = 0;
    ck_assert_uint_eq(openTrustListForSession(&firstId, UA_OPENFILEMODE_READ, &firstHandle),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTrustListForSession(&second->sessionId, UA_OPENFILEMODE_READ,
                                             &secondHandle), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(firstHandle, secondHandle);
    assertTrustListOpenCount(2);
    UA_Variant output;
    UA_Variant_init(&output);
    ck_assert_uint_eq(fileMethodForSession(&firstId, APP_TRUSTLIST_ID(GETPOSITION),
        secondHandle, &output), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert(UA_Variant_isEmpty(&output));
    runSessionCleanupTimer();
    assertTrustListOpenCount(2);

    closeLocalSession(first);
    runSessionCleanupTimer();
    assertTrustListOpenCount(1);
    ck_assert_uint_eq(fileMethodForSession(&firstId, APP_TRUSTLIST_ID(GETPOSITION),
        firstHandle, &output), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert(UA_Variant_isEmpty(&output));
    ck_assert_uint_eq(fileMethodForSession(&second->sessionId, APP_TRUSTLIST_ID(GETPOSITION),
        secondHandle, &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT64]));
    ck_assert_uint_eq(*(UA_UInt64 *)output.data, 0);
    UA_Variant_clear(&output);
    ck_assert_uint_eq(fileMethodForSession(&second->sessionId, APP_TRUSTLIST_ID(CLOSE),
        secondHandle, NULL), UA_STATUSCODE_GOOD);
    assertTrustListOpenCount(0);
    runSessionCleanupTimer();
    closeLocalSession(second);
    UA_NodeId_clear(&firstId);
} END_TEST

START_TEST(trustListSessionClosureDiscardsTransaction) {
    UA_Session *owner = newLocalSession();
    UA_Session *other = newLocalSession();
    UA_UInt32 handle = 0;
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    ck_assert_uint_eq(openTrustListForSession(&owner->sessionId, mode, &handle),
                      UA_STATUSCODE_GOOD);
    UA_ByteString pending = UA_BYTESTRING("unfinished write");
    UA_Variant input[2];
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &pending, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(callMethodForSession(&owner->sessionId,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
        APP_TRUSTLIST_ID(WRITE), 2, input, 0, NULL), UA_STATUSCODE_GOOD);
    UA_UInt32 otherHandle = 12345;
    ck_assert_uint_eq(openTrustListForSession(&other->sessionId, mode, &otherHandle),
                      UA_STATUSCODE_BADTRANSACTIONPENDING);
    ck_assert_uint_eq(otherHandle, 12345);
    ck_assert_uint_eq(callMethodForSession(&other->sessionId,
        UA_NS0ID_SERVERCONFIGURATION, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
        0, NULL, 0, NULL), UA_STATUSCODE_BADUSERACCESSDENIED);
    runSessionCleanupTimer();
    assertTrustListOpenCount(1);

    closeLocalSession(owner);
    runSessionCleanupTimer();
    assertTrustListOpenCount(0);
    ck_assert_uint_eq(callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL), UA_STATUSCODE_BADNOTHINGTODO);
    ck_assert_uint_eq(openTrustListForSession(&other->sessionId, UA_OPENFILEMODE_READ,
                                             &otherHandle), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fileMethodForSession(&other->sessionId, APP_TRUSTLIST_ID(CLOSE),
        otherHandle, NULL), UA_STATUSCODE_GOOD);
    runSessionCleanupTimer();
    closeLocalSession(other);
} END_TEST

#define USER_TRUSTLIST_ID(SUFFIX) \
    UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_##SUFFIX

static const UA_UInt32 trustListObjects[2] = {
    UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
    UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST
};

static UA_ByteString rollbackCertificates[2] = {
    {CERT_DER_LENGTH, CERT_DER_DATA},
    {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA}
};

static void
stageTrustList(size_t index, UA_ByteString *certificate) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    list.trustedCertificatesSize = 1;
    list.trustedCertificates = certificate;
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&list, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                                      &encoded, NULL), UA_STATUSCODE_GOOD);
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    UA_Variant input[2], output;
    UA_Variant_setScalar(&input[0], &mode, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Variant_init(&output);
    ck_assert_uint_eq(callTrustList(trustListObjects[index],
        index ? USER_TRUSTLIST_ID(OPEN) : APP_TRUSTLIST_ID(OPEN),
        1, input, &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UINT32]));
    UA_UInt32 handle = *(UA_UInt32 *)output.data;
    UA_Variant_clear(&output);
    UA_Variant_setScalar(&input[0], &handle, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&input[1], &encoded, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(callTrustList(trustListObjects[index],
        index ? USER_TRUSTLIST_ID(WRITE) : APP_TRUSTLIST_ID(WRITE),
        2, input, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callTrustList(trustListObjects[index],
        index ? USER_TRUSTLIST_ID(CLOSEANDUPDATE) : APP_TRUSTLIST_ID(CLOSEANDUPDATE),
        1, input, &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(*(UA_Boolean *)output.data);
    UA_Variant_clear(&output);
    UA_ByteString_clear(&encoded);
}

static UA_DateTime
trustListLastUpdate(size_t index) {
    UA_Variant output;
    UA_Variant_init(&output);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(0,
        index ? USER_TRUSTLIST_ID(LASTUPDATETIME) : APP_TRUSTLIST_ID(LASTUPDATETIME)),
        &output), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&output, &UA_TYPES[UA_TYPES_UTCTIME]));
    UA_DateTime result = *(UA_DateTime *)output.data;
    UA_Variant_clear(&output);
    return result;
}

static UA_StatusCode (*realGetTrustList[2])(UA_CertificateGroup *, UA_TrustListDataType *);
static UA_StatusCode (*realSetTrustList[2])(UA_CertificateGroup *, const UA_TrustListDataType *);
static size_t failureGroup;
static UA_Boolean failAfterMutation;
static unsigned setTrustListCalls[2];

static size_t
liveGroupIndex(UA_CertificateGroup *group) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    if(group == &config->secureChannelPKI)
        return 0;
    ck_assert_ptr_eq(group, &config->sessionPKI);
    return 1;
}

static UA_StatusCode
failingSetTrustList(UA_CertificateGroup *group, const UA_TrustListDataType *list) {
    size_t index = liveGroupIndex(group);
    setTrustListCalls[index]++;
    UA_Boolean fail = index == failureGroup && setTrustListCalls[index] == 1;
    if(fail && !failAfterMutation)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    UA_StatusCode res = realSetTrustList[index](group, list);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* A plugin may fail after changing its store. That group needs rollback too. */
    return fail ? UA_STATUSCODE_BADRESOURCEUNAVAILABLE : res;
}

static UA_StatusCode
failingSnapshotTrustList(UA_CertificateGroup *group, UA_TrustListDataType *list) {
    size_t index = liveGroupIndex(group);
    UA_StatusCode res = realGetTrustList[index](group, list);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Return owned data with the error to verify snapshot cleanup. */
    return index == failureGroup ? UA_STATUSCODE_BADRESOURCEUNAVAILABLE : res;
}

static void
assertStoredCertificate(UA_CertificateGroup *group, UA_ByteString *certificate) {
    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);
    list.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    ck_assert_uint_eq(group->getTrustList(group, &list), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(list.trustedCertificatesSize, 1);
    ck_assert(UA_ByteString_equal(&list.trustedCertificates[0], certificate));
    ck_assert_uint_eq(list.issuerCertificatesSize, 0);
    ck_assert_uint_eq(list.trustedCrlsSize, 0);
    ck_assert_uint_eq(list.issuerCrlsSize, 0);
    UA_TrustListDataType_clear(&list);
}

/* Fail the first or second group, before or after its data changes, and fail
 * either snapshot. No failed transaction may publish data or update metadata. */
START_TEST(trustListTransactionFailureIsAtomic) {
    failureGroup = (size_t)_i % 2;
    failAfterMutation = _i >= 2;
    UA_Boolean snapshotFailure = _i >= 4;
    memset(setTrustListCalls, 0, sizeof(setTrustListCalls));
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup *groups[2] = {&config->secureChannelPKI, &config->sessionPKI};
    UA_DateTime lastUpdate[2];
    for(size_t i = 0; i < 2; i++) {
        realGetTrustList[i] = groups[i]->getTrustList;
        realSetTrustList[i] = groups[i]->setTrustList;
        UA_TrustListDataType initial;
        UA_TrustListDataType_init(&initial);
        initial.specifiedLists = UA_TRUSTLISTMASKS_ALL;
        initial.trustedCertificatesSize = 1;
        initial.trustedCertificates = &rollbackCertificates[i];
        ck_assert_uint_eq(groups[i]->setTrustList(groups[i], &initial), UA_STATUSCODE_GOOD);
        lastUpdate[i] = trustListLastUpdate(i);
        stageTrustList(i, &rollbackCertificates[1 - i]);
        assertStoredCertificate(groups[i], &rollbackCertificates[i]);
    }
    for(size_t i = 0; i < 2; i++) {
        if(snapshotFailure)
            groups[i]->getTrustList = failingSnapshotTrustList;
        groups[i]->setTrustList = failingSetTrustList;
    }
    UA_fakeSleep(1);
    UA_StatusCode res = callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL);
    for(size_t i = 0; i < 2; i++) {
        groups[i]->getTrustList = realGetTrustList[i];
        groups[i]->setTrustList = realSetTrustList[i];
    }
    ck_assert_uint_eq(res, UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    for(size_t i = 0; i < 2; i++) {
        ck_assert_uint_eq(setTrustListCalls[i], snapshotFailure || i > failureGroup ? 0 : 2);
        assertStoredCertificate(groups[i], &rollbackCertificates[i]);
        ck_assert(trustListLastUpdate(i) == lastUpdate[i]);
    }
    ck_assert_uint_eq(callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL), UA_STATUSCODE_BADNOTHINGTODO);

    /* A fresh transaction succeeds after the failed one has been discarded. */
    for(size_t i = 0; i < 2; i++)
        stageTrustList(i, &rollbackCertificates[1 - i]);
    ck_assert_uint_eq(callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        assertStoredCertificate(groups[i], &rollbackCertificates[1 - i]);
        ck_assert(trustListLastUpdate(i) > lastUpdate[i]);
    }
    ck_assert_uint_eq(config->eventLoop->run(config->eventLoop, 0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL), UA_STATUSCODE_BADNOTHINGTODO);
} END_TEST

START_TEST(signingRequestMethodReturnsMatchingPublicKey) {
    UA_NodeId groupId = _i == 0 ? UA_NODEID_NULL : defaultApplicationGroup;
    UA_NodeId typeId = rsaSha256CertificateType;
    if(_i == 2)
        groupId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    if(_i == 3)
        typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);
    UA_String subject = UA_STRING("CN=GdsRequest");
    UA_Boolean regenerateKey = false;
    UA_ByteString nonce = UA_BYTESTRING("01234567890123456789012345678901");
    UA_Variant input[5];
    UA_Variant_setScalar(&input[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&input[2], &subject, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&input[3], &regenerateKey, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&input[4], &nonce, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    request.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST);
    request.inputArgumentsSize = 5;
    request.inputArguments = input;
    UA_CallMethodResult result = UA_Server_call(server, &request);
    if(_i >= 2) {
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);
        for(size_t i = 0; i < result.outputArgumentsSize; i++)
            ck_assert(UA_Variant_isEmpty(&result.outputArguments[i]));
    } else {
        ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(result.outputArgumentsSize, 1);
        ck_assert(UA_Variant_hasScalarType(&result.outputArguments[0],
                                           &UA_TYPES[UA_TYPES_BYTESTRING]));
        UA_ByteString *csr = (UA_ByteString *)result.outputArguments[0].data;
        UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
        ck_assert_uint_gt(csr->length, 0);
        ck_assert(!UA_ByteString_equal(csr, &certificate));
        ck_assert_uint_eq(UA_CertificateUtils_comparePublicKeys(csr, &certificate),
                          UA_STATUSCODE_GOOD);
        UA_ServerConfig *config = UA_Server_getConfig(server);
        for(size_t i = 0; i < config->securityPoliciesSize; i++) {
            UA_SecurityPolicy *policy = &config->securityPolicies[i];
            if(UA_NodeId_equal(&policy->certificateTypeId, &rsaSha256CertificateType))
                ck_assert(UA_ByteString_equal(&policy->localCertificate, &certificate));
        }
    }
    UA_CallMethodResult_clear(&result);
    /* Creating a request alone must not start an update transaction. */
    ck_assert_uint_eq(callServerConfigurationMethod(
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES, 0, NULL), UA_STATUSCODE_BADNOTHINGTODO);
} END_TEST

int
main(void) {
    Suite *suite = suite_create("GDS Receiver lifecycle");
    TCase *tc = tcase_create("lifecycle");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, shutdownWhileApplyChangesPending);
    tcase_add_test(tc, rejectUpdateWhileApplyChangesQueued);
    tcase_add_test(tc, shutdownWhileCertificateClosurePending);
    tcase_add_test(tc, updateCertificateArguments);
    tcase_add_test(tc, rejectDuplicateReceiver);
    tcase_add_test(tc, helpersBeforeStartupAndDuringShutdown);
    tcase_add_test(tc, removeStoppedReceiver);
#ifdef UA_ENABLE_RBAC
    tcase_add_test(tc, failedRoleSetupKeepsMethods);
#endif
    suite_add_tcase(suite, tc);

    TCase *tcPlain = tcase_create("no RSA endpoint");
    tcase_add_test(tcPlain, noUpdatableCertificateType);
    suite_add_tcase(suite, tcPlain);

    TCase *tcLocal = tcase_create("local GDS methods");
    tcase_add_checked_fixture(tcLocal, setupLocalMethods, teardownLocalMethods);
    tcase_add_loop_test(tcLocal, rejectedListsCombineCertificateGroups, 0, 6);
    suite_add_tcase(suite, tcLocal);

    TCase *tcFiles = tcase_create("local trust-list files");
    tcase_add_checked_fixture(tcFiles, setupLocalTrustLists, teardownLocalMethods);
    tcase_add_test(tcFiles, trustListReadPositions);
    tcase_add_loop_test(tcFiles, trustListInvalidHandlePreservesOpenFile, 0, 6);
    tcase_add_loop_test(tcFiles, trustListRejectsInvalidOpenMode, 0, 5);
    tcase_add_loop_test(tcFiles, trustListRejectsWrongAccessMode, 0, 2);
    tcase_add_test(tcFiles, trustListRejectedWritePreservesData);
    tcase_add_test(tcFiles, trustListMalformedUpdateKeepsHandle);
    tcase_add_test(tcFiles, trustListReadersHaveIndependentHandles);
    suite_add_tcase(suite, tcFiles);

    TCase *tcSessions = tcase_create("GDS session cleanup");
    tcase_add_checked_fixture(tcSessions, setupSessionCleanup, teardownLocalMethods);
    tcase_add_test(tcSessions, trustListSessionClosurePreservesOtherReaders);
    tcase_add_test(tcSessions, trustListSessionClosureDiscardsTransaction);
    suite_add_tcase(suite, tcSessions);

    TCase *tcRollback = tcase_create("GDS transaction rollback");
    tcase_add_checked_fixture(tcRollback, setupSessionCleanup, teardownLocalMethods);
    tcase_add_loop_test(tcRollback, trustListTransactionFailureIsAtomic, 0, 6);
    suite_add_tcase(suite, tcRollback);

    TCase *tcCsr = tcase_create("GDS signing request method");
    tcase_add_checked_fixture(tcCsr, setupLocalTrustLists, teardownLocalMethods);
    tcase_add_loop_test(tcCsr, signingRequestMethodReturnsMatchingPublicKey, 0, 4);
    suite_add_tcase(suite, tcCsr);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
