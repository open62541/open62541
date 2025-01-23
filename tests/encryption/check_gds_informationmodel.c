/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>

#include "ua_server_internal.h"

#include <check.h>
#include <thread_wrapper.h>
#include "test_helpers.h"
#include "certificates.h"

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
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;

    /* Load the trustlist */
    size_t trustListSize = 2;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = intermediateCa;
    trustList[1] = rootCa;

    /* Load the issuerList */
    size_t issuerListSize = 2;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
    issuerList[0] = intermediateCa;
    issuerList[1] = rootCa;

    /* Loading of a revocation list currently unsupported */
    size_t revocationListSize = 2;
    UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
    revocationList[0] = rootCaCrl;
    revocationList[1] = intermediateCaCrl;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize, issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static UA_StatusCode
openTrustList(UA_Client *client, UA_Byte mode, UA_Variant* fileHandler) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &mode, &UA_TYPES[UA_TYPES_BYTE]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN);

    UA_CallRequest callOpenTrustList;
    UA_CallRequest_init(&callOpenTrustList);
    callOpenTrustList.methodsToCallSize = 1;
    callOpenTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callOpenTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);

    UA_Variant_copy(&response.results[0].outputArguments[0], fileHandler);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
openTrustListWithMask(UA_Client *client, UA_UInt32 mask, UA_Variant* fileHandler) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &mask, &UA_TYPES[UA_TYPES_UINT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS);

    UA_CallRequest callOpenTrustListWithMask;
    UA_CallRequest_init(&callOpenTrustListWithMask);
    callOpenTrustListWithMask.methodsToCallSize = 1;
    callOpenTrustListWithMask.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callOpenTrustListWithMask);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);

    UA_Variant_copy(&response.results[0].outputArguments[0], fileHandler);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readTrustList(UA_Client *client, UA_UInt32 fileHandler, UA_Int32 lengthToRead, UA_Variant* data) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(2, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[1], &lengthToRead, &UA_TYPES[UA_TYPES_INT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ);

    UA_CallRequest callReadTrustList;
    UA_CallRequest_init(&callReadTrustList);
    callReadTrustList.methodsToCallSize = 1;
    callReadTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReadTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);

    UA_Variant_copy(&response.results[0].outputArguments[0], data);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeTrustList(UA_Client *client, UA_UInt32 fileHandler, UA_ByteString data) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(2, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[1], &data, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE);

    UA_CallRequest callWriteTrustList;
    UA_CallRequest_init(&callWriteTrustList);
    callWriteTrustList.methodsToCallSize = 1;
    callWriteTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callWriteTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, response.results[0].outputArgumentsSize);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
closeTrustList(UA_Client *client, UA_UInt32 fileHandler) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE);

    UA_CallRequest callCloseTrustList;
    UA_CallRequest_init(&callCloseTrustList);
    callCloseTrustList.methodsToCallSize = 1;
    callCloseTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callCloseTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, response.results[0].outputArgumentsSize);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
closeAndUpdateTrustList(UA_Client *client, UA_UInt32 fileHandler, UA_Variant* applyRequired) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE);

    UA_CallRequest callCloseTrustList;
    UA_CallRequest_init(&callCloseTrustList);
    callCloseTrustList.methodsToCallSize = 1;
    callCloseTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callCloseTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);

    UA_Variant_copy(&response.results[0].outputArguments[0], applyRequired);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
applyChanges(UA_Client *client) {
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 0;
    callMethodRequest.inputArguments = NULL;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES);

    UA_CallRequest callApplyChanges;
    UA_CallRequest_init(&callApplyChanges);
    callApplyChanges.methodsToCallSize = 1;
    callApplyChanges.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callApplyChanges);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, response.results[0].outputArgumentsSize);

    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

START_TEST(rw_trustlist) {
    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Secure client initialization */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0,
                                         NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
    ck_assert(client != NULL);

    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_UInt32 mask = UA_TRUSTLISTMASKS_ALL;
    retval = openTrustListWithMask(client, mask, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 fd = *(UA_Int32*)fileHandler.data;
    UA_Variant bufferVar;
    UA_Variant_init(&bufferVar);
    retval = readTrustList(client, fd, 20000, &bufferVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString data = *(UA_ByteString*)bufferVar.data;
    ck_assert_uint_ne(data.length, 0);

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    retval =
        UA_decodeBinary(&data, &trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_Variant_clear(&bufferVar);

    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    fd = *(UA_Int32*)fileHandler.data;
    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], &encTrustList, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString chunk1 = {.length = 1000, .data = encTrustList.data};
    UA_ByteString chunk2 = {.length = 1000, .data = encTrustList.data+1000};
    UA_ByteString chunk3 = {.length = encTrustList.length - 2000, .data = encTrustList.data+2000};

    retval = writeTrustList(client, fd, chunk1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeTrustList(client, fd, chunk2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = writeTrustList(client, fd, chunk3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant applyRequiredVar;
    UA_Variant_init(&applyRequiredVar);
    retval = closeAndUpdateTrustList(client, fd, &applyRequiredVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean isApplyRequired = *(UA_Boolean*)applyRequiredVar.data;
    ck_assert_uint_eq(isApplyRequired, UA_TRUE);

    retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);

    UA_Variant_clear(&fileHandler);
    UA_Variant_clear(&bufferVar);
    UA_Variant_clear(&applyRequiredVar);
    UA_TrustListDataType_clear(&trustList);
    UA_ByteString_clear(&encTrustList);
    UA_Client_delete(client);
}
END_TEST

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Update TrustList Informationmodel Methods");
    TCase *tc_cert = tcase_create("Update TrustList Informationmodel Methods");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, rw_trustlist);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);
    return s;
}

int main(void) {
    Suite *s = testSuite_create_certificate();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
