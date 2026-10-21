/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/driver/gds_receiver.h>

#include "ua_server_internal.h"

#include <check.h>
#include <thread_wrapper.h>
#include "test_helpers.h"
#include "certificates.h"

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
#include "mp_printf.h"
#define TEST_PATH_MAX 256
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

UA_Server *server;
UA_GDSReceiver *receiver;
UA_atomic(uintptr_t) running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void startServerWithReceiver(void);

static void setup(void) {
    UA_atomic_store(&running, true);
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

    startServerWithReceiver();
}

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
/* Same TrustList as setup, but kept in a filestore */
static void setupFilestore(void) {
    UA_atomic_store(&running, true);
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};

    char storePathDir[TEST_PATH_MAX];
    getcwd(storePathDir, TEST_PATH_MAX - 32);
    mp_snprintf(storePathDir, TEST_PATH_MAX, "%s/pki-gds-informationmodel",
                storePathDir);
    server = UA_Server_newForUnitTestWithSecurityPolicies_Filestore(4840, &certificate,
                                                                    &privateKey,
                                                                    UA_STRING(storePathDir));
    ck_assert(server != NULL);

    /* Replace the files left over from previous test cases */
    UA_ByteString cas[2] = {
        {INTERMEDIATE_CERT_DER_LENGTH, INTERMEDIATE_CERT_DER_DATA},
        {ROOT_CERT_DER_LENGTH, ROOT_CERT_DER_DATA}};
    UA_ByteString crls[2] = {
        {ROOT_EMPTY_CRL_PEM_LENGTH, ROOT_EMPTY_CRL_PEM_DATA},
        {INTERMEDIATE_EMPTY_CRL_PEM_LENGTH, INTERMEDIATE_EMPTY_CRL_PEM_DATA}};
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    trustList.trustedCertificates = cas;
    trustList.trustedCertificatesSize = 2;
    trustList.trustedCrls = crls;
    trustList.trustedCrlsSize = 2;
    trustList.issuerCertificates = cas;
    trustList.issuerCertificatesSize = 2;
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    UA_StatusCode res = certGroup->setTrustList(certGroup, &trustList);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    startServerWithReceiver();
}
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

static void startServerWithReceiver(void) {
    receiver = UA_GDSReceiver_new();
    ck_assert_ptr_nonnull(receiver);
    UA_StatusCode res = UA_Server_addDriver(server, &receiver->drv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_STRING_ALLOC("urn:open62541.unconfigured.application");

#ifdef UA_ENABLE_RBAC
    /* When RBAC is enabled the GDS TrustList methods are restricted to the
     * SecurityAdmin role (see initGDSRolePermissions). This functional test
     * uses an anonymous session, so grant the SecurityAdmin role an Anonymous
     * identity-mapping rule to exercise the GDS mechanics under RBAC. */
    UA_NodeId secAdminId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    UA_Role secAdmin;
    UA_StatusCode roleRes = UA_Server_getRoleById(server, secAdminId, &secAdmin);
    ck_assert_uint_eq(roleRes, UA_STATUSCODE_GOOD);
    UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType *)
        UA_realloc(secAdmin.identityMappingRules,
                   (secAdmin.identityMappingRulesSize + 1) *
                   sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(rules);
    secAdmin.identityMappingRules = rules;
    UA_IdentityMappingRuleType_init(&rules[secAdmin.identityMappingRulesSize]);
    rules[secAdmin.identityMappingRulesSize].criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    secAdmin.identityMappingRulesSize++;
    roleRes = UA_Server_updateRole(server, &secAdmin);
    UA_Role_clear(&secAdmin);
    ck_assert_uint_eq(roleRes, UA_STATUSCODE_GOOD);
#endif

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
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN);

    UA_CallRequest callOpenTrustList;
    UA_CallRequest_init(&callOpenTrustList);
    callOpenTrustList.methodsToCallSize = 1;
    callOpenTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callOpenTrustList);
    ck_assert_uint_eq(1, response.resultsSize);
    UA_StatusCode res = response.results[0].statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);
        UA_Variant_copy(&response.results[0].outputArguments[0], fileHandler);
    }

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return res;
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
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ);

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
setPositionTrustList(UA_Client *client, UA_UInt32 fileHandler, UA_UInt64 position) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(2, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[1], &position, &UA_TYPES[UA_TYPES_UINT64]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION);

    UA_CallRequest callSetPosition;
    UA_CallRequest_init(&callSetPosition);
    callSetPosition.methodsToCallSize = 1;
    callSetPosition.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callSetPosition);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, response.results[0].outputArgumentsSize);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getPositionTrustList(UA_Client *client, UA_UInt32 fileHandler, UA_UInt64 *position) {
    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], &fileHandler, &UA_TYPES[UA_TYPES_UINT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION);

    UA_CallRequest callGetPosition;
    UA_CallRequest_init(&callGetPosition);
    callGetPosition.methodsToCallSize = 1;
    callGetPosition.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callGetPosition);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(1, response.results[0].outputArgumentsSize);
    ck_assert(UA_Variant_hasScalarType(&response.results[0].outputArguments[0],
                                       &UA_TYPES[UA_TYPES_UINT64]));

    *position = *(UA_UInt64*)response.results[0].outputArguments[0].data;

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
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE);

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
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE);

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
    ck_assert_uint_eq(0, response.results[0].outputArgumentsSize);
    UA_StatusCode res = response.results[0].statusCode;

    UA_CallResponse_clear(&response);

    return res;
}

/* Helper: create a secure client connected to the test server */
static UA_Client *
createSecureClient(void) {
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
                                         NULL, 0,
                                         NULL, 0);
    cc->certificateVerification.clear(&cc->certificateVerification);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    return client;
}

/* Helper: read and decode the TrustList opened with OpenWithMasks */
static void
readTrustListWithMask(UA_Client *client, UA_UInt32 mask,
                      UA_TrustListDataType *trustList) {
    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_StatusCode retval = openTrustListWithMask(client, mask, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;

    UA_Variant data;
    UA_Variant_init(&data);
    retval = readTrustList(client, fd, 65535, &data);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_decodeBinary((UA_ByteString*)data.data, trustList,
                             &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&data);
    UA_Variant_clear(&fileHandler);
}

static void
assertMaskedList(const UA_ByteString *list, size_t listSize, UA_Boolean selected,
                 const UA_ByteString *all, size_t allSize) {
    ck_assert_uint_eq(listSize, selected ? allSize : 0);
    for(size_t i = 0; i < listSize; i++)
        ck_assert(UA_ByteString_equal(&list[i], &all[i]));
}

/* Helper: call AddCertificate on the DefaultApplicationGroup TrustList */
static UA_StatusCode
callAddCertificate(UA_Client *client, UA_ByteString *cert, UA_Boolean isTrusted) {
    UA_Variant inputArguments[2];
    UA_Variant_setScalar(&inputArguments[0], cert, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&inputArguments[1], &isTrusted, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE);

    UA_CallRequest callReq;
    UA_CallRequest_init(&callReq);
    callReq.methodsToCallSize = 1;
    callReq.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReq);
    ck_assert_uint_eq(1, response.resultsSize);
    UA_StatusCode res = response.results[0].statusCode;
    UA_CallResponse_clear(&response);
    return res;
}

/* Helper: call RemoveCertificate on the DefaultApplicationGroup TrustList */
static UA_StatusCode
callRemoveCertificate(UA_Client *client, UA_String *thumbprint, UA_Boolean isTrusted) {
    UA_Variant inputArguments[2];
    UA_Variant_setScalar(&inputArguments[0], thumbprint, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[1], &isTrusted, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE);

    UA_CallRequest callReq;
    UA_CallRequest_init(&callReq);
    callReq.methodsToCallSize = 1;
    callReq.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReq);
    ck_assert_uint_eq(1, response.resultsSize);
    UA_StatusCode res = response.results[0].statusCode;
    UA_CallResponse_clear(&response);
    return res;
}

/* Test: UpdateCertificate without a PrivateKey rejects a certificate that does
 * not parse, instead of staging it into the transaction */
START_TEST(update_certificate_reject_unparseable) {
    UA_Client *client = createSecureClient();

    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    UA_ByteString cert = UA_BYTESTRING("not a certificate");
    UA_String keyFormat = UA_STRING_NULL;
    UA_ByteString key = UA_BYTESTRING_NULL;

    UA_Variant inputArguments[6];
    UA_Variant_setScalar(&inputArguments[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[2], &cert, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&inputArguments[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&inputArguments[4], &keyFormat, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[5], &key, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 6;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE);

    UA_CallRequest callReq;
    UA_CallRequest_init(&callReq);
    callReq.methodsToCallSize = 1;
    callReq.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReq);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_BADCERTIFICATEINVALID);
    UA_CallResponse_clear(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Helper: stage the current server certificate again with UpdateCertificate
 * for the given certificate type */
static UA_StatusCode
callUpdateCertificateOfType(UA_Client *client, UA_UInt32 certificateTypeId) {
    UA_NodeId groupId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId typeId = UA_NODEID_NUMERIC(0, certificateTypeId);
    UA_ByteString certificate = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString privateKey = {KEY_DER_LENGTH, KEY_DER_DATA};
    UA_String privateKeyFormat = UA_STRING("DER");

    UA_Variant inputArguments[6];
    UA_Variant_setScalar(&inputArguments[0], &groupId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[1], &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&inputArguments[2], &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setArray(&inputArguments[3], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Variant_setScalar(&inputArguments[4], &privateKeyFormat, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[5], &privateKey, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 6;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE);

    UA_CallRequest callReq;
    UA_CallRequest_init(&callReq);
    callReq.methodsToCallSize = 1;
    callReq.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReq);
    ck_assert_uint_eq(1, response.resultsSize);
    UA_StatusCode res = response.results[0].statusCode;
    UA_CallResponse_clear(&response);
    return res;
}

/* Helper: stage the current server certificate again with UpdateCertificate */
static UA_StatusCode
callUpdateCertificate(UA_Client *client) {
    return callUpdateCertificateOfType(client,
                                       UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
}

/* Helper: stage an empty TrustList in the transaction of the client. The CAs
 * of the client certificate are no longer trusted after ApplyChanges. */
static void
stageEmptyTrustList(UA_Client *client) {
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_encodeBinary(&trustList,
                                           &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                                           &encTrustList, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    UA_Variant_clear(&fileHandler);
    retval = writeTrustList(client, fd, encTrustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant applyRequiredVar;
    UA_Variant_init(&applyRequiredVar);
    retval = closeAndUpdateTrustList(client, fd, &applyRequiredVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&applyRequiredVar);
    UA_ByteString_clear(&encTrustList);
}

/* Helper: wait until the server has closed the SecureChannel of the client */
static UA_Boolean
secureChannelClosed(UA_Client *client) {
    UA_SecureChannelState channelState = UA_SECURECHANNELSTATE_OPEN;
    for(size_t i = 0; i < 100; i++) {
        UA_Client_run_iterate(client, 10);
        UA_Client_getState(client, &channelState, NULL, NULL);
        if(channelState == UA_SECURECHANNELSTATE_CLOSED)
            return true;
    }
    return false;
}

/* ApplyChanges re-verifies the open SecureChannels against the new TrustList.
 * The SecureChannel of a client that is no longer trusted is closed. */
START_TEST(apply_trustlist_closes_untrusted_channel) {
    UA_Client *client = createSecureClient();
    UA_Client_getConfig(client)->noReconnect = true;

    stageEmptyTrustList(client);
    UA_StatusCode retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(secureChannelClosed(client));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Same as above, but the transaction also updates the certificate of a type
 * the SecureChannel of the client does not use (RsaMin, the client uses
 * Aes128_Sha256_RsaOaep with RsaSha256). The SecureChannel is still re-verified
 * against the new TrustList and closed. */
START_TEST(apply_trustlist_and_certificate_closes_untrusted_channel) {
    UA_Client *client = createSecureClient();
    UA_Client_getConfig(client)->noReconnect = true;

    UA_StatusCode retval =
        callUpdateCertificateOfType(client, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    stageEmptyTrustList(client);
    retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(secureChannelClosed(client));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: AddCertificate succeeds for a non-CA end-entity certificate */
START_TEST(add_certificate_success) {
    UA_Client *client = createSecureClient();

    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: AddCertificate rejects CA certificates */
START_TEST(add_certificate_reject_ca) {
    UA_Client *client = createSecureClient();

    UA_ByteString caCert;
    caCert.length = ROOT_CERT_DER_LENGTH;
    caCert.data = ROOT_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &caCert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: AddCertificate rejects isTrustedCertificate=false */
START_TEST(add_certificate_reject_issuer) {
    UA_Client *client = createSecureClient();

    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = false;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEINVALID);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Helper: is the certificate in the TrustedCertificates of the
 * DefaultApplicationGroup? */
static UA_Boolean
isTrustedCertificate(const UA_ByteString *cert) {
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    lockServer(server);
    UA_StatusCode retval = certGroup->getTrustList(certGroup, &trustList);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Boolean found = UA_TrustListDataType_contains(&trustList, cert,
                                                     UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    UA_TrustListDataType_clear(&trustList);
    return found;
}

/* Helper: verify a certificate with the DefaultApplicationGroup */
static UA_StatusCode
verifyCertificate(const UA_ByteString *cert) {
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode retval = certGroup->verifyCertificate(certGroup, cert);
    unlockServer(server);
    return retval;
}

/* Part 12 §7.8.2.6: AddCertificate rejects a certificate that does not parse
 * with Bad_CertificateInvalid. The certificate is not added to the TrustList
 * and certificates are still verified afterwards. */
START_TEST(add_certificate_reject_malformed) {
    UA_Client *client = createSecureClient();

    /* A truncated certificate does not parse */
    UA_ByteString malformed = {CERT_DER_LENGTH / 2, CERT_DER_DATA};
    UA_StatusCode retval = callAddCertificate(client, &malformed, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEINVALID);
    ck_assert(!isTrustedCertificate(&malformed));

    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    ck_assert_uint_eq(verifyCertificate(&cert), UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.6: AddCertificate rejects a certificate whose issuer CA is not
 * in the TrustList. Building the certificate chain fails first, so the
 * StatusCode is Bad_CertificateChainIncomplete (Part 4, Table 100), not
 * Bad_CertificateUntrusted. Bad_CertificateUntrusted belongs to the Trust List
 * Check, which comes after the Build Certificate Chain step. */
START_TEST(add_certificate_reject_unknown_issuer) {
    UA_Client *client = createSecureClient();

    /* Remove the CA certificates from the TrustList */
    UA_TrustListDataType noCas;
    UA_TrustListDataType_init(&noCas);
    noCas.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
        UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode retval = certGroup->setTrustList(certGroup, &noCas);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    ck_assert_uint_eq(verifyCertificate(&cert), UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE);

    retval = callAddCertificate(client, &cert, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE);
    ck_assert(!isTrustedCertificate(&cert));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.6: AddCertificate rejects a certificate that is in the CRL of
 * its issuer CA with Bad_CertificateRevoked */
START_TEST(add_certificate_reject_revoked) {
    UA_Client *client = createSecureClient();

    /* Add the CRL of the intermediate CA that revokes the certificate */
    UA_ByteString revokingCrl = {INTERMEDIATE_CRL_PEM_LENGTH, INTERMEDIATE_CRL_PEM_DATA};
    UA_TrustListDataType crls;
    UA_TrustListDataType_init(&crls);
    crls.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCRLS;
    crls.trustedCrls = &revokingCrl;
    crls.trustedCrlsSize = 1;
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode retval = certGroup->addToTrustList(certGroup, &crls);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString cert = {APPLICATION_CERT_DER_LENGTH, APPLICATION_CERT_DER_DATA};
    ck_assert_uint_eq(verifyCertificate(&cert), UA_STATUSCODE_BADCERTIFICATEREVOKED);

    retval = callAddCertificate(client, &cert, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEREVOKED);
    ck_assert(!isTrustedCertificate(&cert));

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.6: AddCertificate returns Bad_NotWritable while the
 * TrustList is open for reading */
START_TEST(add_certificate_reject_while_open_read) {
    UA_Client *client = createSecureClient();

    /* Open the TrustList for reading */
    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = 0x01; /* Read */
    UA_StatusCode retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Try to add — should fail because the list is open */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTWRITABLE);

    /* Close the TrustList */
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.6: AddCertificate returns Bad_InvalidState while the
 * TrustList is open for writing and CloseAndUpdate has not been called */
START_TEST(add_certificate_reject_while_open_write) {
    UA_Client *client = createSecureClient();

    /* Open the TrustList for writing */
    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    UA_StatusCode retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Try to add — should fail because the list is open */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDSTATE);

    /* Close without changes ends the transaction. The certificate can be
     * added afterwards. */
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: RemoveCertificate succeeds for a previously added certificate */
START_TEST(remove_certificate_success) {
    UA_Client *client = createSecureClient();

    /* First add the certificate */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Compute the thumbprint of the certificate we just added */
    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40; /* SHA1 = 20 bytes = 40 hex chars */
    thumbprint.data = (UA_Byte*)UA_malloc(thumbprint.length);
    ck_assert(thumbprint.data != NULL);
    retval = UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Remove the certificate by thumbprint */
    retval = callRemoveCertificate(client, &thumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_String_clear(&thumbprint);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: RemoveCertificate fails for a non-existent thumbprint */
START_TEST(remove_certificate_not_found) {
    UA_Client *client = createSecureClient();

    UA_String bogusThumbprint = UA_STRING("0000000000000000000000000000000000000000");
    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callRemoveCertificate(client, &bogusThumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.7: RemoveCertificate returns Bad_NotWritable while the
 * TrustList is open for reading */
START_TEST(remove_certificate_reject_while_open_read) {
    UA_Client *client = createSecureClient();

    /* First add a certificate so there's something to try removing */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Compute its thumbprint */
    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte*)UA_malloc(thumbprint.length);
    ck_assert(thumbprint.data != NULL);
    retval = UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Open the TrustList for reading */
    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = 0x01; /* Read */
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Try to remove — should fail because the list is open */
    retval = callRemoveCertificate(client, &thumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTWRITABLE);

    /* Close the TrustList */
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_String_clear(&thumbprint);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.7: RemoveCertificate returns Bad_InvalidState while the
 * TrustList is open for writing and CloseAndUpdate has not been called */
START_TEST(remove_certificate_reject_while_open_write) {
    UA_Client *client = createSecureClient();

    /* First add a certificate so there's something to try removing */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Compute its thumbprint */
    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte*)UA_malloc(thumbprint.length);
    ck_assert(thumbprint.data != NULL);
    retval = UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Open the TrustList for writing */
    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Try to remove — should fail because the list is open */
    retval = callRemoveCertificate(client, &thumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDSTATE);

    /* Close without changes ends the transaction. The certificate can be
     * removed afterwards. */
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = callRemoveCertificate(client, &thumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_String_clear(&thumbprint);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static UA_StatusCode
failingGetTrustList(UA_CertificateGroup *certGroup,
                    UA_TrustListDataType *trustList) {
    return UA_STATUSCODE_BADINTERNALERROR;
}

/* Test: A failed RemoveCertificate leaves no transaction behind */
START_TEST(remove_certificate_failure_clears_transaction) {
    UA_Client *client = createSecureClient();

    /* Copying the TrustList into the transaction fails */
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode (*getTrustList)(UA_CertificateGroup *, UA_TrustListDataType *) =
        certGroup->getTrustList;
    certGroup->getTrustList = failingGetTrustList;
    unlockServer(server);

    UA_String thumbprint = UA_STRING("0000000000000000000000000000000000000000");
    UA_StatusCode retval = callRemoveCertificate(client, &thumbprint, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    lockServer(server);
    certGroup->getTrustList = getTrustList;
    unlockServer(server);

    /* No transaction is pending, so the TrustList can be changed again */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;
    retval = callAddCertificate(client, &cert, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: RemoveCertificate with IsTrustedCertificate=false removes the CRLs of
 * the issuer CA from the issuer CRLs */
START_TEST(remove_issuer_certificate_removes_issuer_crls) {
    UA_Client *client = createSecureClient();

    /* The default configuration puts all CRLs into the TrustedCrls. Move the
     * CRL of the intermediate CA to the IssuerCrls. */
    UA_ByteString intermediateCa = {INTERMEDIATE_CERT_DER_LENGTH,
                                    INTERMEDIATE_CERT_DER_DATA};
    UA_ByteString intermediateCaCrl = {INTERMEDIATE_EMPTY_CRL_PEM_LENGTH,
                                       INTERMEDIATE_EMPTY_CRL_PEM_DATA};
    UA_TrustListDataType trustedCrls;
    UA_TrustListDataType_init(&trustedCrls);
    trustedCrls.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCRLS;
    trustedCrls.trustedCrls = &intermediateCaCrl;
    trustedCrls.trustedCrlsSize = 1;
    UA_TrustListDataType issuerCrls;
    UA_TrustListDataType_init(&issuerCrls);
    issuerCrls.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCRLS;
    issuerCrls.issuerCrls = &intermediateCaCrl;
    issuerCrls.issuerCrlsSize = 1;
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode retval = certGroup->removeFromTrustList(certGroup, &trustedCrls);
    if(retval == UA_STATUSCODE_GOOD)
        retval = certGroup->addToTrustList(certGroup, &issuerCrls);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte thumbprintBuf[40]; /* SHA1 = 20 bytes = 40 hex chars */
    UA_String thumbprint = {sizeof(thumbprintBuf), thumbprintBuf};
    retval = UA_CertificateUtils_getThumbprint(&intermediateCa, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = callRemoveCertificate(client, &thumbprint, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    lockServer(server);
    retval = certGroup->getTrustList(certGroup, &trustList);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The CA and its CRL are removed from the issuer lists */
    ck_assert(!UA_TrustListDataType_contains(&trustList, &intermediateCa,
                                             UA_TRUSTLISTMASKS_ISSUERCERTIFICATES));
    ck_assert(!UA_TrustListDataType_contains(&trustList, &intermediateCaCrl,
                                             UA_TRUSTLISTMASKS_ISSUERCRLS));

    /* The trusted lists are not changed */
    ck_assert(UA_TrustListDataType_contains(&trustList, &intermediateCa,
                                            UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    UA_TrustListDataType_clear(&trustList);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

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

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

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

    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    UA_Variant bufferVar;
    UA_Variant_init(&bufferVar);
    retval = readTrustList(client, fd, 20000, &bufferVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString data = *(UA_ByteString*)bufferVar.data;
    ck_assert_uint_ne(data.length, 0);

    /* The position is a UInt64. Positions beyond the end of the file are
     * clamped to its length. */
    retval = setPositionTrustList(client, fd, (UA_UInt64)1 << 32);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt64 position = 0;
    retval = getPositionTrustList(client, fd, &position);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(position, data.length);

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

    fd = *(UA_UInt32*)fileHandler.data;
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

/* Part 12 §7.10.9: ApplyChanges without an active transaction returns
 * Bad_NothingToDo */
START_TEST(apply_changes_without_transaction) {
    UA_Client *client = createSecureClient();

    UA_StatusCode retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTHINGTODO);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.2: Open with the write bit continues the transaction of the
 * session. Other sessions cannot open the TrustList meanwhile. */
START_TEST(open_continues_transaction) {
    UA_Client *client = createSecureClient();
    UA_Client *other = createSecureClient();

    /* The TrustList that is written back */
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    lockServer(server);
    UA_StatusCode retval = certGroup->getTrustList(certGroup, &trustList);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                             &encTrustList, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* UpdateCertificate starts the transaction */
    retval = callUpdateCertificate(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING;
    retval = openTrustList(other, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTRANSACTIONPENDING);

    /* Close discards the written data, but not the certificate update */
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    UA_Variant_clear(&fileHandler);
    retval = writeTrustList(client, fd, encTrustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = openTrustList(other, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTRANSACTIONPENDING);

    /* Write the TrustList in the same transaction */
    retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    fd = *(UA_UInt32*)fileHandler.data;
    UA_Variant_clear(&fileHandler);
    retval = writeTrustList(client, fd, encTrustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Part 12 §7.10.9: Nothing is applied while the TrustList is open for
     * writing */
    retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDSTATE);

    UA_Variant applyRequiredVar;
    UA_Variant_init(&applyRequiredVar);
    retval = closeAndUpdateTrustList(client, fd, &applyRequiredVar);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&applyRequiredVar);

    retval = applyChanges(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType_clear(&trustList);
    UA_ByteString_clear(&encTrustList);
    UA_Client_disconnect(other);
    UA_Client_delete(other);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Part 12 §7.8.2.6, §7.8.2.7: AddCertificate and RemoveCertificate return
 * Bad_TransactionPending while a transaction is pending and the TrustList is
 * closed */
START_TEST(add_remove_certificate_reject_transaction_pending) {
    UA_Client *client = createSecureClient();

    /* First add a certificate so there's something to try removing */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;

    UA_Boolean isTrusted = true;
    UA_StatusCode retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Compute its thumbprint */
    UA_String thumbprint = UA_STRING_NULL;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte*)UA_malloc(thumbprint.length);
    ck_assert(thumbprint.data != NULL);
    retval = UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* UpdateCertificate starts the transaction */
    retval = callUpdateCertificate(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = callAddCertificate(client, &cert, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTRANSACTIONPENDING);
    retval = callRemoveCertificate(client, &thumbprint, isTrusted);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTRANSACTIONPENDING);

    UA_String_clear(&thumbprint);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(read_trustlist_reject_negative_length) {
    UA_Client *client = createSecureClient();

    UA_Variant fileHandler;
    UA_Variant_init(&fileHandler);
    UA_Byte mode = UA_OPENFILEMODE_READ;
    UA_StatusCode retval = openTrustList(client, mode, &fileHandler);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 fd = *(UA_UInt32*)fileHandler.data;
    UA_Int32 lengthToRead = -1;
    UA_Variant inputArguments[2];
    UA_Variant_setScalar(&inputArguments[0], &fd, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[1], &lengthToRead, &UA_TYPES[UA_TYPES_INT32]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ);

    UA_CallRequest callReadTrustList;
    UA_CallRequest_init(&callReadTrustList);
    callReadTrustList.methodsToCallSize = 1;
    callReadTrustList.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReadTrustList);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert_uint_eq(response.results[0].statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_CallResponse_clear(&response);
    retval = closeTrustList(client, fd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_clear(&fileHandler);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test: OpenWithMasks returns only the lists selected by the mask */
START_TEST(open_trustlist_with_mask) {
    /* The default configuration puts all CRLs into the TrustedCrls */
    UA_ByteString intermediateCaCrl = {INTERMEDIATE_EMPTY_CRL_PEM_LENGTH,
                                       INTERMEDIATE_EMPTY_CRL_PEM_DATA};
    UA_TrustListDataType issuerCrls;
    UA_TrustListDataType_init(&issuerCrls);
    issuerCrls.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCRLS;
    issuerCrls.issuerCrls = &intermediateCaCrl;
    issuerCrls.issuerCrlsSize = 1;
    UA_CertificateGroup *certGroup = &UA_Server_getConfig(server)->secureChannelPKI;
    lockServer(server);
    UA_StatusCode retval = certGroup->addToTrustList(certGroup, &issuerCrls);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client *client = createSecureClient();

    UA_TrustListDataType all;
    UA_TrustListDataType_init(&all);
    readTrustListWithMask(client, UA_TRUSTLISTMASKS_ALL, &all);
    ck_assert_uint_ne(all.trustedCertificatesSize, 0);
    ck_assert_uint_ne(all.trustedCrlsSize, 0);
    ck_assert_uint_ne(all.issuerCertificatesSize, 0);
    ck_assert_uint_ne(all.issuerCrlsSize, 0);

    for(UA_UInt32 mask = 0; mask <= UA_TRUSTLISTMASKS_ALL; mask++) {
        UA_TrustListDataType trustList;
        UA_TrustListDataType_init(&trustList);
        readTrustListWithMask(client, mask, &trustList);
        ck_assert_uint_eq(trustList.specifiedLists, mask);
        assertMaskedList(trustList.trustedCertificates, trustList.trustedCertificatesSize,
                         (mask & UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES) != 0,
                         all.trustedCertificates, all.trustedCertificatesSize);
        assertMaskedList(trustList.trustedCrls, trustList.trustedCrlsSize,
                         (mask & UA_TRUSTLISTMASKS_TRUSTEDCRLS) != 0,
                         all.trustedCrls, all.trustedCrlsSize);
        assertMaskedList(trustList.issuerCertificates, trustList.issuerCertificatesSize,
                         (mask & UA_TRUSTLISTMASKS_ISSUERCERTIFICATES) != 0,
                         all.issuerCertificates, all.issuerCertificatesSize);
        assertMaskedList(trustList.issuerCrls, trustList.issuerCrlsSize,
                         (mask & UA_TRUSTLISTMASKS_ISSUERCRLS) != 0,
                         all.issuerCrls, all.issuerCrlsSize);
        UA_TrustListDataType_clear(&trustList);
    }

    UA_TrustListDataType_clear(&all);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(add_certificate_replaced_input_metadata) {
    UA_Client *client = createSecureClient();

    UA_StatusCode retval = UA_Client_deleteNode(
        client,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE_INPUTARGUMENTS),
        true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Argument argument;
    UA_Argument_init(&argument);
    argument.name = UA_STRING("Certificate");
    argument.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    argument.valueRank = UA_VALUERANK_SCALAR;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "InputArguments");
    attr.dataType = UA_TYPES[UA_TYPES_ARGUMENT].typeId;
    attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDimension = 1;
    attr.arrayDimensionsSize = 1;
    attr.arrayDimensions = &arrayDimension;
    UA_Variant_setArray(&attr.value, &argument, 1,
                        &UA_TYPES[UA_TYPES_ARGUMENT]);

    retval = UA_Client_addVariableNode(
        client,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE_INPUTARGUMENTS),
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(0, "InputArguments"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attr, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Prime the method-call stack with a Boolean as the second argument. The
     * shortened AddCertificate definition must not let its callback reuse it. */
    UA_String thumbprint = UA_STRING("not-found");
    UA_Boolean trusted = true;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_Variant removeInput[2];
    UA_Variant_init(&removeInput[0]);
    UA_Variant_init(&removeInput[1]);
    UA_Variant_setScalar(&removeInput[0], &thumbprint,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&removeInput[1], &trusted,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant addInput;
    UA_Variant_init(&addInput);
    UA_Variant_setScalar(&addInput, &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_CallMethodRequest methods[2];
    UA_CallMethodRequest_init(&methods[0]);
    UA_CallMethodRequest_init(&methods[1]);
    methods[0].objectId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    methods[0].methodId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE);
    methods[0].inputArgumentsSize = 2;
    methods[0].inputArguments = removeInput;
    methods[1].objectId = methods[0].objectId;
    methods[1].methodId = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE);
    methods[1].inputArgumentsSize = 1;
    methods[1].inputArguments = &addInput;

    UA_CallRequest request;
    UA_CallRequest_init(&request);
    request.methodsToCallSize = 2;
    request.methodsToCall = methods;
    UA_CallResponse response = UA_Client_Service_call(client, request);
    ck_assert_uint_eq(response.resultsSize, 2);
    ck_assert_uint_eq(response.results[0].statusCode,
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(response.results[1].statusCode,
                      UA_STATUSCODE_BADTYPEMISMATCH);
    UA_CallResponse_clear(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* A file transfer driver serves the TrustList (Part 12, 7.8.2.1) */
START_TEST(trustlist_file_properties) {
    UA_Client *client = createSecureClient();

    /* The Size has no meaning for a TrustList */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode retval = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SIZE),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTSUPPORTED);
    UA_Variant_clear(&value);

    /* Closing the Session closes the TrustList */
    UA_Variant fileHandle;
    UA_Variant_init(&fileHandle);
    retval = openTrustList(client, UA_OPENFILEMODE_READ, &fileHandle);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&fileHandle);
    const UA_NodeId openCount = UA_NODEID_NUMERIC(0,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT);
    retval = UA_Client_readValueAttribute(client, openCount, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt16*)value.data, 1);
    UA_Variant_clear(&value);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    retval = UA_Server_readValue(server, openCount, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*(UA_UInt16*)value.data, 0);
    UA_Variant_clear(&value);
}
END_TEST

/* Exercise each callback contract directly, independently of method metadata. */
START_TEST(gds_callback_argument_counts) {
    const struct {
        UA_UInt32 methodId;
        UA_UInt32 objectId;
        size_t inputSize;
        size_t outputSize;
    } contracts[] = {
        {UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
         UA_NS0ID_SERVERCONFIGURATION, 6, 1},
        {UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST,
         UA_NS0ID_SERVERCONFIGURATION, 5, 1},
        {UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST,
         UA_NS0ID_SERVERCONFIGURATION, 0, 1},
        {UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
         UA_NS0ID_SERVERCONFIGURATION, 0, 0},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST, 2, 0},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST, 2, 0},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST, 1, 1},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST, 1, 1},
    };
    UA_Variant input[7];
    UA_Variant output[2];
    memset(input, 0, sizeof(input));
    memset(output, 0, sizeof(output));
    for(size_t i = 0; i < sizeof(contracts) / sizeof(contracts[0]); i++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, contracts[i].methodId);
        UA_NodeId objectId = UA_NODEID_NUMERIC(0, contracts[i].objectId);
        UA_MethodCallback callback = NULL;
        UA_StatusCode retval =
            UA_Server_getMethodNodeCallback(server, methodId, &callback);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(callback != NULL);

        /* Both too few and too many arguments must be rejected before use. */
        size_t inputCounts[2] = {contracts[i].inputSize + 1,
                                contracts[i].inputSize ? contracts[i].inputSize - 1 : 1};
        size_t outputCounts[2] = {contracts[i].outputSize + 1,
                                 contracts[i].outputSize ? contracts[i].outputSize - 1 : 1};
        for(size_t j = 0; j < 2; j++) {
            retval = callback(server, &UA_NODEID_NULL, NULL, &methodId, NULL,
                              &objectId, NULL, inputCounts[j], input,
                              contracts[i].outputSize, output);
            ck_assert_uint_eq(retval, UA_STATUSCODE_BADTYPEMISMATCH);
            retval = callback(server, &UA_NODEID_NULL, NULL, &methodId, NULL,
                              &objectId, NULL, contracts[i].inputSize, input,
                              outputCounts[j], output);
            UA_StatusCode expectedOutputStatus =
                outputCounts[j] < contracts[i].outputSize ?
                    UA_STATUSCODE_BADARGUMENTSMISSING :
                    UA_STATUSCODE_BADTOOMANYARGUMENTS;
            ck_assert_uint_eq(retval, expectedOutputStatus);
        }
    }

    /* The file transfer driver serves the FileType Methods of the TrustLists */
    const UA_UInt32 fileMethods[6][3] = {
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN,
         UA_NS0ID_FILETYPE_OPEN},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE,
         UA_NS0ID_FILETYPE_CLOSE},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ,
         UA_NS0ID_FILETYPE_READ},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE,
         UA_NS0ID_FILETYPE_WRITE},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION,
         UA_NS0ID_FILETYPE_GETPOSITION},
        {UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION,
         UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION,
         UA_NS0ID_FILETYPE_SETPOSITION}
    };
    for(size_t i = 0; i < 6; i++) {
        UA_MethodCallback typeCallback = NULL;
        ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server,
                              UA_NODEID_NUMERIC(0, fileMethods[i][2]), &typeCallback),
                          UA_STATUSCODE_GOOD);
        ck_assert(typeCallback != NULL);
        for(size_t j = 0; j < 2; j++) {
            UA_MethodCallback callback = NULL;
            ck_assert_uint_eq(UA_Server_getMethodNodeCallback(server,
                                  UA_NODEID_NUMERIC(0, fileMethods[i][j]), &callback),
                              UA_STATUSCODE_GOOD);
            ck_assert(callback == typeCallback);
        }
    }
}
END_TEST

static void teardown(void) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("GDS TrustList Informationmodel Methods");
    TCase *tc_cert = tcase_create("GDS TrustList Informationmodel Methods");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, rw_trustlist);
    tcase_add_test(tc_cert, trustlist_file_properties);
    tcase_add_test(tc_cert, open_trustlist_with_mask);
    tcase_add_test(tc_cert, apply_changes_without_transaction);
    tcase_add_test(tc_cert, open_continues_transaction);
    tcase_add_test(tc_cert, gds_callback_argument_counts);
    tcase_add_test(tc_cert, read_trustlist_reject_negative_length);
    tcase_add_test(tc_cert, add_certificate_replaced_input_metadata);
    tcase_add_test(tc_cert, update_certificate_reject_unparseable);
    tcase_add_test(tc_cert, add_certificate_success);
    tcase_add_test(tc_cert, add_certificate_reject_ca);
    tcase_add_test(tc_cert, add_certificate_reject_issuer);
    tcase_add_test(tc_cert, add_certificate_reject_malformed);
    tcase_add_test(tc_cert, add_certificate_reject_unknown_issuer);
    tcase_add_test(tc_cert, add_certificate_reject_revoked);
    tcase_add_test(tc_cert, add_certificate_reject_while_open_read);
    tcase_add_test(tc_cert, add_certificate_reject_while_open_write);
    tcase_add_test(tc_cert, remove_certificate_success);
    tcase_add_test(tc_cert, remove_certificate_not_found);
    tcase_add_test(tc_cert, remove_certificate_reject_while_open_read);
    tcase_add_test(tc_cert, remove_certificate_reject_while_open_write);
    tcase_add_test(tc_cert, remove_certificate_failure_clears_transaction);
    tcase_add_test(tc_cert, remove_issuer_certificate_removes_issuer_crls);
    tcase_add_test(tc_cert, add_remove_certificate_reject_transaction_pending);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_cert_filestore = tcase_create("GDS TrustList Informationmodel Methods Filestore");
    tcase_add_checked_fixture(tc_cert_filestore, setupFilestore, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert_filestore, add_certificate_success);
    tcase_add_test(tc_cert_filestore, add_certificate_reject_malformed);
    tcase_add_test(tc_cert_filestore, add_certificate_reject_unknown_issuer);
    tcase_add_test(tc_cert_filestore, add_certificate_reject_revoked);
    tcase_add_test(tc_cert_filestore, apply_trustlist_closes_untrusted_channel);
    tcase_add_test(tc_cert_filestore, apply_trustlist_and_certificate_closes_untrusted_channel);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert_filestore);
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */
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
