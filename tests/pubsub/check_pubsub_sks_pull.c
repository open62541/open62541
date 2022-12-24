/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"
#include "ua_pubsub_keystorage.h"
#include "ua_server_internal.h"

#include <check.h>
#include "testing_clock.h"
#include "../encryption/certificates.h"
#include "thread_wrapper.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32
#define policUri "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"

UA_Server *sksServer = NULL;
UA_String securityGroupId;
UA_NodeId sgNodeId;
UA_UInt32 maxKeyCount;
UA_NodeId connection;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(sksServer, true);
    return 0;
}

static void
addSecurityGroup(void) {
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

    UA_Server_addSecurityGroup(sksServer, securityGroupParent, &config, &outNodeId);
    UA_String_copy(&config.securityGroupName, &securityGroupId);

    UA_String allowedUsername = UA_STRING("user1");

    UA_ByteString *username = UA_ByteString_new();
    UA_ByteString_copy(&allowedUsername, username);
    UA_Server_setNodeContext(sksServer, outNodeId, username);
    UA_NodeId_copy(&outNodeId, &sgNodeId);
}

static UA_Boolean
getUserExecutableOnObject_sks(UA_Server *server, UA_AccessControl *ac,
                              const UA_NodeId *sessionId, void *sessionContext,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext) {
    /* For the CTT, recognize whether two sessions are  */
    if(objectContext && sessionContext) {
        UA_ByteString *username = (UA_ByteString *)objectContext;
        UA_ByteString *sessionUsername = (UA_ByteString *)sessionContext;
        if(!UA_ByteString_equal(username, sessionUsername))
            return false;
    }
    return true;
}

static void
setup(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = server_cert_der_len;
    certificate.data = server_cert_der;

    UA_ByteString privateKey;
    privateKey.length = server_key_der_len;
    privateKey.data = server_key_der;

    sksServer = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(sksServer);
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, NULL);

	UA_ServerConfig_PKIStore_removeContentAll(UA_ServerConfig_PKIStore_getDefault(sksServer));
	UA_ServerConfig_PKIStore_storeCertificate(
		UA_ServerConfig_PKIStore_getDefault(sksServer),
		UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
		&certificate
	);
	UA_ServerConfig_PKIStore_storeCertificate(
		UA_ServerConfig_PKIStore_getDefault(sksServer),
		UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE),
		&certificate
	);
	UA_ServerConfig_PKIStore_storePrivateKey(
		UA_ServerConfig_PKIStore_getDefault(sksServer),
		UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
		&privateKey
	);
	UA_ServerConfig_PKIStore_storePrivateKey(
		UA_ServerConfig_PKIStore_getDefault(sksServer),
		UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE),
		&privateKey
	);

	UA_ServerConfig_PKIStore_storeTrustList(
		UA_ServerConfig_PKIStore_getDefault(sksServer),
		1, &certificate,
		0, NULL,
		0, NULL,
		0, NULL
	);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      &config->logger);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(sksServer, &connectionConfig, &connection);

    /*User Access Control*/
    config->accessControl.getUserExecutableOnObject = getUserExecutableOnObject_sks;

    addSecurityGroup();

    UA_Server_run_startup(sksServer);
    THREAD_CREATE(server_thread, serverloop);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(sksServer);
    UA_Server_delete(sksServer);
}

static UA_StatusCode
encyrptedclientconnect(UA_Client *client, const char *username, const char *password ) {
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

    UA_ClientConfig_setDefaultEncryption(cc);
    cc->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    /* Secure client connect */
    return UA_Client_connectUsername(client, "opc.tcp://localhost:4840", username, password);
}

static UA_CallResponse
callGetSecurityKeys(UA_Client *client, UA_String sksSecurityGroupId,
                    UA_UInt32 startingTokenId, UA_UInt32 requestedKeyCount) {
    UA_Variant *inputArguments = (UA_Variant *)UA_calloc(3, (sizeof(UA_Variant)));

    UA_Variant_setScalarCopy(&inputArguments[0], &sksSecurityGroupId,
                             &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&inputArguments[1], &startingTokenId,
                             &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&inputArguments[2], &requestedKeyCount,
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
    return UA_Client_Service_call(client, callMethodRequestFromClient);
}

START_TEST(getSecuritykeysBadSecurityModeInsufficient) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    }
    UA_StatusCode expectedCode = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
    UA_CallResponse response = callGetSecurityKeys(client, securityGroupId, 1, 1);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
}
END_TEST

START_TEST(getSecuritykeysBadNotFound) {
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_String badSecurityGroupId = UA_STRING("BadSecurityGroupId");
    UA_StatusCode expectedCode = UA_STATUSCODE_BADNOTFOUND;
    UA_CallResponse response = callGetSecurityKeys(sksClient, badSecurityGroupId, 1, 1);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
}
END_TEST

START_TEST(getSecuritykeysBadUserAccessDenied) {
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user2", "password1");
    UA_StatusCode expectedCode = UA_STATUSCODE_BADUSERACCESSDENIED;
    UA_UInt32 reqkeyCount = 1;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, 1, reqkeyCount);
    /* set SecurityGroupNodeContext to username
        compare the SGNodeContext with sessioncontext in getUserExecutableOnObject
    */
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
                  UA_StatusCode_name(response.results->statusCode));
}
END_TEST

START_TEST(getSecuritykeysGoodAndValidOutput) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 1;
    UA_CallResponse response = callGetSecurityKeys(sksClient, securityGroupId, 1, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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
}
END_TEST

START_TEST(requestCurrentKeyWithFutureKeys) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 1;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response = callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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

}
END_TEST

START_TEST(requestCurrentKeyOnly) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 0;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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
}
END_TEST

START_TEST(requestPastKey) {
    /*wait for one keyLifeTime*/
    UA_fakeSleep(2000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = 0;
    UA_UInt32 reqStartingTokenId = 1;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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
}
END_TEST

START_TEST(requestUnknownStartingTokenId){
    UA_fakeSleep(1000);
    UA_sleep_ms(4000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = UA_UINT32_MAX;
    UA_UInt32 reqStartingTokenId = UA_UINT32_MAX;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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

}END_TEST

START_TEST(requestMaxFutureKeys) {
    UA_fakeSleep(1000);
    UA_Client *sksClient = UA_Client_new();
    encyrptedclientconnect(sksClient, "user1", "password");
    UA_StatusCode expectedCode = UA_STATUSCODE_GOOD;
    UA_UInt32 reqkeyCount = UA_UINT32_MAX;
    UA_UInt32 reqStartingTokenId = 0;
    UA_CallResponse response =
        callGetSecurityKeys(sksClient, securityGroupId, reqStartingTokenId, reqkeyCount);
    ck_assert_msg(response.results->statusCode == expectedCode,
                  "Expected %s but erorr code : %s \n", UA_StatusCode_name(expectedCode),
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
