/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include "test_helpers.h"
#include "../encryption/certificates.h"
#include "ua_pubsub.h"
#include "ua_pubsub_keystorage.h"
#include "ua_server_internal.h"

#include <check.h>
#include "thread_wrapper.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32

UA_Server *server = NULL;
UA_ByteString currentKey;
UA_ByteString *futureKey = NULL;
UA_String securityGroupId;
UA_NodeId connection, writerGroup, readerGroup, publishedDataSet, dataSetWriter;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_StatusCode
generateKeyData(const UA_PubSubSecurityPolicy *policy, UA_ByteString *key) {
    if(!key || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    UA_Byte secretBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString secret;
    secret.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;
    secret.data = secretBytes;

    UA_Byte seedBytes[UA_PUBSUB_KEYMATERIAL_NONCELENGTH];
    UA_ByteString seed;
    seed.data = seedBytes;
    seed.length = UA_PUBSUB_KEYMATERIAL_NONCELENGTH;

    retVal = policy->symmetricModule.generateNonce(policy->policyContext, &secret);
    retVal |= policy->symmetricModule.generateNonce(policy->policyContext, &seed);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal =
        policy->symmetricModule.generateKey(policy->policyContext, &secret, &seed, key);
    return retVal;
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

    /* Secure client connect */
    return UA_Client_connect(client, "opc.tcp://localhost:4840");
}

static UA_StatusCode
callSetSecurityKey(UA_Client *client, UA_String pSecurityGroupId, UA_UInt32 currentTokenId, UA_UInt32 futureKeySize){
    UA_NodeId parentId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    UA_NodeId methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_SETSECURITYKEYS);
    size_t inputSize = 7;
    UA_Variant inputs[7];
    size_t outputSize;
    UA_Variant *output;

    UA_Variant_setScalar(&inputs[0], &pSecurityGroupId, &UA_TYPES[UA_TYPES_STRING]);

    UA_String policUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");
    UA_Variant_setScalar(&inputs[1], &policUri, &UA_TYPES[UA_TYPES_STRING]);

    UA_Variant_setScalar(&inputs[2], &currentTokenId, &UA_TYPES[UA_TYPES_UINT32]);

    size_t keyLength = server->config.pubSubConfig.securityPolicies->symmetricModule.secureChannelNonceLength;
    UA_ByteString_allocBuffer(&currentKey, keyLength);
    generateKeyData(server->config.pubSubConfig.securityPolicies, &currentKey);
    UA_Variant_setScalar(&inputs[3], &currentKey, &UA_TYPES[UA_TYPES_BYTESTRING]);

    futureKey = (UA_ByteString *)UA_calloc(futureKeySize, sizeof(UA_ByteString));

    for (size_t i = 0; i < futureKeySize; i++) {
        UA_ByteString_allocBuffer(&futureKey[i], keyLength);
        generateKeyData(server->config.pubSubConfig.securityPolicies, &futureKey[i]);
    }
    UA_Variant_setArrayCopy(&inputs[4], futureKey, futureKeySize, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_Duration msTimeNextKey = 0;
    UA_Variant_setScalar(&inputs[5], &msTimeNextKey, &UA_TYPES[UA_TYPES_DURATION]);

    UA_Duration mskeyLifeTime = 2000;
    UA_Variant_setScalar(&inputs[6], &mskeyLifeTime, &UA_TYPES[UA_TYPES_DURATION]);

    UA_StatusCode retval = UA_Client_call(client, parentId, methodId, inputSize, inputs, &outputSize, &output);
    UA_ByteString_clear(&currentKey);
    UA_Variant_clear(&inputs[4]);
    UA_Array_delete(futureKey, futureKeySize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return retval;
}


static void
addTestWriterGroup(UA_String securitygroupId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityGroupId = securitygroupId;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retval |=
        UA_Server_addWriterGroup(server, connection, &writerGroupConfig, &writerGroup);
    UA_Server_enableWriterGroup(server, writerGroup);
}

static void
addTestReaderGroup(UA_String securitygroupId) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* To check status after running both publisher and subscriber */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup");

    /* Reader Group Encryption settings */
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityGroupId = securitygroupId;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retVal |=
        UA_Server_addReaderGroup(server, connection, &readerGroupConfig, &readerGroup);
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

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    // add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection);

    securityGroupId = UA_STRING("TestSecurityGroup");

    addTestReaderGroup(securityGroupId);
    addTestWriterGroup(securityGroupId);

    retVal |= UA_Server_run_startup(server);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    THREAD_CREATE(server_thread, serverloop);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(TestSetSecurityKeys_InsufficientSecurityMode) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    }
    retval = callSetSecurityKey(client, securityGroupId, 1, 2);
    ck_assert_msg(retval == UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT, "Expected BAD_SECURITYMODEINSUFFICIENT but erorr code : %s \n", UA_StatusCode_name(retval));
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT);
    UA_Client_delete(client);
} END_TEST

START_TEST(TestSetSecurityKeys_MissingSecurityGroup) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = encyrptedclientconnect(client);
    UA_String wrongSecurityGroupId = UA_STRING("WrongSecurityGroupId");
    retval = callSetSecurityKey(client, wrongSecurityGroupId, 1, 2);
    ck_assert_msg(retval == UA_STATUSCODE_BADNOTFOUND, "Expected BAD_BADNOTFOUND but erorr code : %s \n", UA_StatusCode_name(retval));
    UA_Client_delete(client);
} END_TEST

START_TEST(TestSetSecurityKeys_GOOD) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_UInt32 futureKeySize = 2;
    UA_UInt32 currentTokenId = 1;
    UA_UInt32 startingTokenId = 1;

    UA_StatusCode retval = encyrptedclientconnect(client);

    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
    UA_UNLOCK(&server->serviceMutex);

    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n",
                  UA_StatusCode_name(retval));
    ck_assert_uint_eq(ks->currentItem->keyID, currentTokenId);

    startingTokenId = currentTokenId;
    UA_PubSubKeyListItem *iterator = TAILQ_FIRST(&ks->keyList);
    for (size_t i = 0; i < futureKeySize + 1; i++) {
        ck_assert_ptr_ne(iterator, NULL);
        ck_assert_uint_eq(iterator->keyID, startingTokenId);
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        startingTokenId++;
    }
    ck_assert_uint_eq(ks->keyListSize, futureKeySize+1);
    UA_Client_delete(client);
} END_TEST

START_TEST(TestSetSecurityKeys_UpdateCurrentKeyFromExistingList){
    UA_Client *client = UA_Client_newForUnitTest();
    UA_UInt32 futureKeySize = 2;
    UA_UInt32 currentTokenId = 1;

    UA_StatusCode retval = encyrptedclientconnect(client);

    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
    UA_UNLOCK(&server->serviceMutex);

    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));

    futureKeySize = 0;
    currentTokenId = 3;
    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));
    ck_assert_uint_eq(ks->currentItem->keyID, currentTokenId);
    UA_Client_delete(client);
} END_TEST

START_TEST(TestSetSecurityKeys_UpdateCurrentKeyFromExistingListAndAddNewFutureKeys){
    UA_Client *client = UA_Client_newForUnitTest();
    UA_UInt32 futureKeySize = 2;
    UA_UInt32 currentTokenId = 1;
    UA_UInt32 startingTokenId = 1;
    size_t keyListSize;
    UA_StatusCode retval = encyrptedclientconnect(client);

    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
    UA_UNLOCK(&server->serviceMutex);

    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));

    keyListSize = ks->keyListSize;
    futureKeySize = 3;
    currentTokenId = 2;
    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));
    ck_assert_uint_eq(ks->currentItem->keyID, currentTokenId);
    /*After updating: KeyListSize = Pervious KeyListSize + FutureKeySize - Duplicated Keys with matching KeyID */
    ck_assert_uint_eq(keyListSize + futureKeySize - 1, ks->keyListSize);
    UA_PubSubKeyListItem *iterator = TAILQ_FIRST(&ks->keyList);
    for (size_t i = 0; i < ks->keyListSize; i++) {
        ck_assert_ptr_ne(iterator, NULL);
        ck_assert_uint_eq(iterator->keyID, startingTokenId);
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        startingTokenId++;
    }
    UA_Client_delete(client);
} END_TEST

START_TEST(TestSetSecurityKeys_ReplaceExistingKeyListWithFetchedKeyList){
    UA_Client *client = UA_Client_newForUnitTest();
    UA_UInt32 futureKeySize = 2;
    UA_UInt32 currentTokenId = 1;
    UA_UInt32 startingTokenId = 1;

    UA_StatusCode retval = encyrptedclientconnect(client);

    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, securityGroupId);
    UA_UNLOCK(&server->serviceMutex);

    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));

    futureKeySize = 3;
    currentTokenId = 4;
    startingTokenId = currentTokenId;
    retval = callSetSecurityKey(client, securityGroupId, currentTokenId, futureKeySize);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode Good but erorr code : %s \n", UA_StatusCode_name(retval));
    UA_PubSubKeyListItem *iterator = TAILQ_FIRST(&ks->keyList);
    for (size_t i = 0; i < futureKeySize + 1; i++) {
        ck_assert_ptr_ne(iterator, NULL);
        ck_assert_uint_eq(iterator->keyID, startingTokenId);
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        startingTokenId++;
    }
    ck_assert_uint_eq(ks->keyListSize, futureKeySize+1);
    UA_Client_delete(client);
} END_TEST

int
main(void) {
    int number_failed = 0;
    TCase *tc_pubsub_sks_push = tcase_create("PubSub SKS Push");
    tcase_add_checked_fixture(tc_pubsub_sks_push, setup, teardown);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_InsufficientSecurityMode);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_MissingSecurityGroup);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_GOOD);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_UpdateCurrentKeyFromExistingList);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_UpdateCurrentKeyFromExistingListAndAddNewFutureKeys);
    tcase_add_test(tc_pubsub_sks_push, TestSetSecurityKeys_ReplaceExistingKeyListWithFetchedKeyList);
    Suite *s = suite_create("PubSub SKS Push");
    suite_add_tcase(s, tc_pubsub_sks_push);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
