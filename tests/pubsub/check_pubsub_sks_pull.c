/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "ua_pubsub.h"
#include "ua_pubsub_keystorage.h"
#include "ua_server_internal.h"

#include <check.h>
#include "testing_clock.h"
#include "../encryption/certificates.h"
#include "thread_wrapper.h"
#include "open62541/plugin/accesscontrol_default.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32
#define policUri "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"

static UA_UsernamePasswordLogin userNamePW[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password2")}
};

UA_Server *sksServer = NULL;
UA_String securityGroupId;
UA_NodeId sgNodeId;
UA_UInt32 maxKeyCount;
UA_NodeId connection;
UA_Boolean running;
UA_ByteString allowedUsername;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(sksServer, true);
    return 0;
}

typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
    UA_UsernamePasswordLoginCallback loginCallback;
    void *loginContext;
    UA_CertificateGroup verifyX509;
} AccessControlContext;

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define CERTIFICATE_POLICY "open62541-certificate-policy"
#define USERNAME_POLICY "open62541-username-policy"
const UA_String anonymousPolicy = UA_STRING_STATIC(ANONYMOUS_POLICY);
const UA_String certificatePolicy = UA_STRING_STATIC(CERTIFICATE_POLICY);
const UA_String usernamePolicy = UA_STRING_STATIC(USERNAME_POLICY);

static void
addSecurityGroup(void) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId securityGroupParent =
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SECURITYGROUPS);
    UA_NodeId outNodeId;
    UA_SecurityGroupConfig config;
    memset(&config, 0, sizeof(UA_SecurityGroupConfig));
    config.keyLifeTime = 2000;
    config.securityPolicyUri = UA_STRING(policUri);
    config.securityGroupName = UA_STRING("TestSecurityGroup");
    config.maxFutureKeyCount = 1;
    config.maxPastKeyCount = 1;

    maxKeyCount = config.maxPastKeyCount + 1 + config.maxFutureKeyCount;

    retVal |= UA_Server_addSecurityGroup(sksServer, securityGroupParent, &config, &outNodeId);
    UA_String_copy(&config.securityGroupName, &securityGroupId);

    allowedUsername = UA_STRING("user1");
    retVal |= UA_Server_setNodeContext(sksServer, outNodeId, &allowedUsername);
    retVal |= UA_NodeId_copy(&outNodeId, &sgNodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static UA_Boolean
getUserExecutableOnObject_sks(UA_Server *server, UA_AccessControl *ac,
                              const UA_NodeId *sessionId, void *sessionContext,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext) {
    if(!objectContext)
        return true;

    UA_Session *session = getSessionById(server, sessionId);
    UA_ByteString *username = (UA_ByteString *)objectContext;
    return UA_ByteString_equal(username, &session->clientUserIdOfSession);
}

static void
setup(void) {
    running = true;

    /* Load certificate and private key */
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
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    sksServer = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                             trustList, trustListSize,
                                                             issuerList, issuerListSize,
                                                             revocationList, revocationListSize);

    UA_ServerConfig *config = UA_Server_getConfig(sksServer);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_String basic256sha256 = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    UA_AccessControl_default(config, true, &basic256sha256, 2, userNamePW);

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(sksServer, &connectionConfig, &connection);

    /*User Access Control*/
    config->accessControl.getUserExecutableOnObject = getUserExecutableOnObject_sks;

    addSecurityGroup();

    retVal |= UA_Server_run_startup(sksServer);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void
teardown(void) {
    UA_String_clear(&securityGroupId);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(sksServer);
    UA_Server_delete(sksServer);
}

static void
cleanupSessionContext(void) {
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &sksServer->sessions, pointers, temp) {
        if(current->session.context) {
            UA_ExtensionObject *handle = (UA_ExtensionObject*)current->session.context;
            UA_ExtensionObject_clear(handle);
            UA_free(current->session.context);
            current->session.context = NULL;
        }
    }
}

static UA_StatusCode
encyrptedclientconnect(UA_Client *client) {
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
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey, trustList,
                                         trustListSize, revocationList,
                                         revocationListSize);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    return UA_STATUSCODE_GOOD;
}

static UA_CallResponse
callGetSecurityKeys(UA_Client *client, UA_String sksSecurityGroupId,
                    UA_UInt32 startingTokenId, UA_UInt32 requestedKeyCount) {
    UA_Variant *inputArguments = (UA_Variant *)UA_calloc(3, (sizeof(UA_Variant)));

    UA_Variant_setScalar(&inputArguments[0], &sksSecurityGroupId,
                             &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[1], &startingTokenId,
                             &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&inputArguments[2], &requestedKeyCount,
                             &UA_TYPES[UA_TYPES_UINT32]);

    // Call method from client
    UA_CallRequest callMethodRequestFromClient;
    UA_CallRequest_init(&callMethodRequestFromClient);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);

    callMethodRequestFromClient.methodsToCall = &item;
    callMethodRequestFromClient.methodsToCallSize = 1;
    item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_GETSECURITYKEYS);
    item.inputArguments = (UA_Variant *)inputArguments;
    item.inputArgumentsSize = 3;
    UA_CallResponse response = UA_Client_Service_call(client, callMethodRequestFromClient);
    UA_free(inputArguments);
    return response;
}

START_TEST(getSecuritykeysBadSecurityModeInsufficient) {
    UA_Client *client = UA_Client_newForUnitTest();
    encyrptedclientconnect(client);
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGN;
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
    UA_CallResponse response = callGetSecurityKeys(client, securityGroupId, 1, 1);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(client);
}
END_TEST

START_TEST(getSecuritykeysBadNotFound) {
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_String badSecurityGroupId = UA_STRING("BadSecurityGroupId");
    UA_StatusCode expectedCode = UA_STATUSCODE_BADNOTFOUND;
    UA_CallResponse response = callGetSecurityKeys(sksClient, badSecurityGroupId, 1, 1);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(getSecuritykeysBadUserAccessDenied) {
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user2", "password2");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_BADUSERACCESSDENIED;
    UA_UInt32 reqkeyCount = 1;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, 1, reqkeyCount);
    /* set SecurityGroupNodeContext to username
        compare the SGNodeContext with sessioncontext in getUserExecutableOnObject
    */
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(getSecuritykeysGoodAndValidOutput) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 1;
    UA_CallResponse response = callGetSecurityKeys(sksClient, securityGroupId, 1, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_Variant *output = response.results->outputArguments;
    /* check types */
    ck_assert(output[0].type == &UA_TYPES[UA_TYPES_STRING]);
    ck_assert(output[1].type == &UA_TYPES[UA_TYPES_INTEGERID] ||
              output[1].type == &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert(output[2].type == &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert(output[3].type == &UA_TYPES[UA_TYPES_DURATION] ||
              output[3].type == &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(output[4].type == &UA_TYPES[UA_TYPES_DURATION] ||
              output[4].type == &UA_TYPES[UA_TYPES_DOUBLE]);

    /* epected values */
    UA_String *securityPolicyUri = (UA_String *)output[0].data;
    UA_String expectedUri = UA_STRING(policUri);
    ck_assert(UA_String_equal(securityPolicyUri, &expectedUri) == UA_TRUE);

    UA_IntegerId expectedToken = 1;
    UA_IntegerId firstTokenId = *(UA_IntegerId *)output[1].data;
    ck_assert(expectedToken == firstTokenId);

    size_t retKeyCount = output[2].arrayLength;
    UA_UInt32 totalReqkeyCount = reqkeyCount + 1;
    ck_assert(retKeyCount == totalReqkeyCount || retKeyCount == maxKeyCount );
    UA_ByteString *keys = (UA_ByteString *)output[2].data;
    for(size_t i = 0; i < retKeyCount; i++) {
        ck_assert(UA_ByteString_equal(&keys[i], &UA_BYTESTRING_NULL) == UA_FALSE);
    }
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(requestCurrentKeyWithFutureKeys) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 1;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response = callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(sksServer, sgNodeId);
    UA_Variant *output = response.results->outputArguments;

    UA_UInt32 firstTokenId = *(UA_UInt32 *) output[1].data;
    size_t retKeyCount = output[2].arrayLength;
    ck_assert(reqkeyCount + 1 == retKeyCount);

    UA_ByteString *retKeys = (UA_ByteString *)output[2].data;
    UA_PubSubKeyListItem *iterator = sg->keyStorage->currentItem;
    for (size_t i = 0; i < retKeyCount; i++)
    {
        ck_assert(UA_ByteString_equal(&retKeys[i], &iterator->key) == UA_TRUE);
        ck_assert(firstTokenId == iterator->keyID);
        ++firstTokenId;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
    }
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(requestCurrentKeyOnly) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 0;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(sksServer, sgNodeId);
    UA_Variant *output = response.results->outputArguments;

    UA_UInt32 firstTokenId = *(UA_UInt32 *)output[1].data;
    size_t retKeyCount = output[2].arrayLength;
    ck_assert(retKeyCount == 1);

    UA_ByteString *retKeys = (UA_ByteString *)output[2].data;
    UA_PubSubKeyListItem *iterator = sg->keyStorage->currentItem;
    for(size_t i = 0; i < retKeyCount; i++) {
        ck_assert(UA_ByteString_equal(&retKeys[i], &iterator->key) == UA_TRUE);
        ck_assert(firstTokenId == iterator->keyID);
        ++firstTokenId;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
    }
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(requestPastKey) {
    /*wait for one keyLifeTime*/
    UA_fakeSleep(2000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 0;
    UA_UInt32 reqStartingTokenId = 1;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(sksServer, sgNodeId);
    UA_Variant *output = response.results->outputArguments;

    UA_UInt32 firstTokenId = *(UA_UInt32 *)output[1].data;
    size_t retKeyCount = output[2].arrayLength;
    ck_assert(retKeyCount == 1);

    UA_ByteString *retKeys = (UA_ByteString *)output[2].data;
    UA_PubSubKeyListItem *firstItem = TAILQ_FIRST(&sg->keyStorage->keyList);
    ck_assert(firstItem->keyID != sg->keyStorage->currentItem->keyID);
    ck_assert(firstItem->keyID == firstTokenId);
    ck_assert(UA_ByteString_equal(retKeys, &firstItem->key) == UA_TRUE);
    ck_assert(UA_ByteString_equal(retKeys, &sg->keyStorage->currentItem->key) != UA_TRUE);
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

START_TEST(requestUnknownStartingTokenId){
    UA_fakeSleep(1000);
    UA_realSleep(4000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = UA_UINT32_MAX;
    UA_UInt32 reqStartingTokenId = UA_UINT32_MAX;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(sksServer, sgNodeId);
    UA_Variant *output = response.results->outputArguments;

    UA_UInt32 firstTokenId = *(UA_UInt32 *)output[1].data;
    size_t retKeyCount = output[2].arrayLength;
    ck_assert(retKeyCount == sg->keyStorage->keyListSize);

    UA_ByteString *retKeys = (UA_ByteString *)output[2].data;
    UA_PubSubKeyListItem *iterator = TAILQ_FIRST(&sg->keyStorage->keyList);
    for(size_t i = 0; i < retKeyCount; i++) {
        ck_assert(UA_ByteString_equal(&retKeys[i], &iterator->key) == UA_TRUE);
        ck_assert(firstTokenId == iterator->keyID);
        ++firstTokenId;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
    }
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}END_TEST

START_TEST(requestMaxFutureKeys) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_newForUnitTest();
    encyrptedclientconnect(sksClient);
    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connectUsername(sksClient, "opc.tcp://localhost:4840", "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(sksClient);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = UA_UINT32_MAX;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert(response.results != NULL);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but error code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
    ck_assert_uint_eq(response.results->outputArgumentsSize, 5);

    UA_SecurityGroup *sg = UA_SecurityGroup_findSGbyId(sksServer, sgNodeId);
    UA_Variant *output = response.results->outputArguments;

    UA_UInt32 firstTokenId = *(UA_UInt32 *)output[1].data;
    size_t retKeyCount = output[2].arrayLength;
    ck_assert(retKeyCount == sg->keyStorage->maxFutureKeyCount + 1 );

    UA_ByteString *retKeys = (UA_ByteString *)output[2].data;
    UA_PubSubKeyListItem *iterator = sg->keyStorage->currentItem;
    for(size_t i = 0; i < retKeyCount; i++) {
        ck_assert(UA_ByteString_equal(&retKeys[i], &iterator->key) == UA_TRUE);
        ck_assert(firstTokenId == iterator->keyID);
        ++firstTokenId;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
    }
    UA_CallResponse_clear(&response);
    cleanupSessionContext();
    UA_Client_delete(sksClient);
}
END_TEST

int
main(void) {
    int number_failed = 0;
    TCase *tc_pubsub_sks_pull = tcase_create("PubSub SKS Pull");
    tcase_add_checked_fixture(tc_pubsub_sks_pull, setup, teardown);
    tcase_add_test(tc_pubsub_sks_pull, getSecuritykeysBadSecurityModeInsufficient);
    tcase_add_test(tc_pubsub_sks_pull, getSecuritykeysBadNotFound);
    tcase_add_test(tc_pubsub_sks_pull, getSecuritykeysBadUserAccessDenied);
    tcase_add_test(tc_pubsub_sks_pull, getSecuritykeysGoodAndValidOutput);
    tcase_add_test(tc_pubsub_sks_pull, requestCurrentKeyWithFutureKeys);
    tcase_add_test(tc_pubsub_sks_pull, requestCurrentKeyOnly);
    tcase_add_test(tc_pubsub_sks_pull, requestPastKey);
    tcase_add_test(tc_pubsub_sks_pull, requestUnknownStartingTokenId);
    tcase_add_test(tc_pubsub_sks_pull, requestMaxFutureKeys);
    Suite *s = suite_create("PubSub SKS Pull");
    suite_add_tcase(s, tc_pubsub_sks_pull);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
