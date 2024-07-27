/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"
#include "ua_pubsub_keystorage.h"
#include "ua_server_internal.h"

#include <check.h>
#include "test_helpers.h"
#include "testing_clock.h"

#define UA_PUBSUB_KEYMATERIAL_NONCELENGTH 32
#define UA_AES128CTR_SIGNING_KEY_LENGTH          32
#define UA_AES128CTR_KEY_LENGTH                  32
#define UA_AES128CTR_KEYNONCE_LENGTH             4
#define UA_SymmetricKey_Length UA_AES128CTR_SIGNING_KEY_LENGTH + UA_AES128CTR_KEY_LENGTH + UA_AES128CTR_KEYNONCE_LENGTH

#define MSG_LENGTH_ENCRYPTED 85
#define MSG_LENGTH_DECRYPTED 39
#define MSG_HEADER "f111ba08016400014df4030100000008b02d012e01000000"
#define MSG_HEADER_NO_SEC "f101ba08016400014df4"
#define MSG_PAYLOAD_ENC "da434ce02ee19922c6e916c8154123baa25f67288e3378d613f3203909"
#define MSG_PAYLOAD_DEC "e1101054c2949f3a" \
                        "d701b4205f69841e" \
                        "5f6901000d7657c2" \
                        "949F3ad701"
#define MSG_SIG "6e08a9ff14b83ea2247792eeffc757c85ac99c0ffa79e4fbe5629783dc77b403"
#define MSG_SIG_INVALID "5e08a9ff14b83ea2247792eeffc757c85ac99c0ffa79e4fbe5629783dc77b403"

UA_Byte signingKeyPub[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKeyPub[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNoncePub[UA_AES128CTR_KEYNONCE_LENGTH] = {0};
UA_Server *server = NULL;
UA_ByteString currentKey;
UA_ByteString *futureKey = NULL;
UA_UInt32 futureKeySize;
UA_String SecurityGroupId;
UA_NodeId connection, writerGroup, readerGroup, publishedDataSet, dataSetWriter;


static UA_StatusCode
generateKeyData(const UA_PubSubSecurityPolicy *policy, UA_ByteString *key) {
    if(!key || !policy)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retVal;

    /* Can't not found in specification for pubsub key generation, so use the idea of
     * securechannel, see specification part6 p51 for more details*/

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

    retVal = policy->symmetricModule.generateKey(policy->policyContext, &secret, &seed, key);
    return retVal;
}

static void
addTestWriterGroup(UA_String securitygroupId){
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

    retval |= UA_Server_addWriterGroup(server, connection, &writerGroupConfig, &writerGroup);
    UA_Server_enableWriterGroup(server, writerGroup);
}

static void
addTestReaderGroup(UA_String securitygroupId){
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* To check status after running both publisher and subscriber */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup");

    /* Reader Group Encryption settings */
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityGroupId = securitygroupId;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retVal |=  UA_Server_addReaderGroup(server, connection, &readerGroupConfig, &readerGroup);
    UA_Server_enableReaderGroup(server, readerGroup);
}

static UA_PubSubKeyStorage*
createKeyStoragewithkeys(UA_UInt32 currentTokenId, UA_UInt32 keysize,
                         UA_Duration msKeyLifeTime, UA_Duration msTimeToNextKey,
                         UA_String testSecurityGroupId) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_Duration callbackTime;
    addTestWriterGroup(SecurityGroupId);
    addTestReaderGroup(SecurityGroupId);

    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *tKeyStorage =
        UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);

    size_t keyLength = server->config.pubSubConfig.securityPolicies->symmetricModule
                           .secureChannelNonceLength;
    UA_ByteString_allocBuffer(&currentKey, keyLength);
    generateKeyData(server->config.pubSubConfig.securityPolicies, &currentKey);

    futureKey = (UA_ByteString *)UA_calloc(keysize, sizeof(UA_ByteString));
    if(!futureKey)
        return NULL;

    for(size_t i = 0; i < keysize; i++) {
        UA_ByteString_allocBuffer(&futureKey[i], keyLength);
        generateKeyData(server->config.pubSubConfig.securityPolicies, &futureKey[i]);
    }

    retval = UA_PubSubKeyStorage_storeSecurityKeys(server, tKeyStorage,
                                                   currentTokenId, &currentKey, futureKey,
                                                   keysize, msKeyLifeTime);
    if(retval != UA_STATUSCODE_GOOD)
        return NULL;

    retval = UA_PubSubKeyStorage_activateKeyToChannelContext(server, UA_NODEID_NULL,
                                                             tKeyStorage->securityGroupID);
    if(retval != UA_STATUSCODE_GOOD)
        return NULL;

    callbackTime = msKeyLifeTime;
    if(msTimeToNextKey > 0)
        callbackTime = msTimeToNextKey;

    /*move to setSecurityKeysAction*/
    retval = UA_PubSubKeyStorage_addKeyRolloverCallback(
        server, tKeyStorage, (UA_ServerCallback)UA_PubSubKeyStorage_keyRolloverCallback, callbackTime,
        &tKeyStorage->callBackId);
    UA_UNLOCK(&server->serviceMutex);

    return tKeyStorage;
}

static UA_Byte *
hexstr_to_char(const char *hexstr) {
    size_t len = strlen(hexstr);
    if(len % 2 != 0)
        return NULL;
    size_t final_len = len / 2;
    UA_Byte *chrs = (UA_Byte *)malloc((final_len + 1) * sizeof(*chrs));
    for(size_t i = 0, j = 0; j < final_len; i += 2, j++)
        chrs[j] =
            (UA_Byte)((hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25);
    chrs[final_len] = '\0';
    return chrs;
}

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    SecurityGroupId = UA_STRING("TestSecurityGroup");

    UA_ServerConfig *config = &server->config;
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    UA_Server_run_startup(server);
    //add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ByteString_clear(&currentKey);
    if(futureKey) {
        UA_Array_delete(futureKey, futureKeySize, &UA_TYPES[UA_TYPES_STRING]);
        futureKey = NULL;
        futureKeySize = 0;
    }
}

START_TEST(TestPubSubKeyStorage_initialize) {
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_UInt32 maxPastkeyCount = 0;
    UA_UInt32 maxFuturekeyCount = 0;
    UA_PubSubKeyStorage *tKeyStorage = (UA_PubSubKeyStorage *)
        UA_calloc(1, sizeof(UA_PubSubKeyStorage));
    ck_assert_ptr_ne(tKeyStorage, NULL);

    UA_LOCK(&server->serviceMutex);

    retval =
        UA_PubSubKeyStorage_init(server, tKeyStorage,
                                 &SecurityGroupId, server->config.pubSubConfig.securityPolicies,
                                 maxPastkeyCount, maxFuturekeyCount);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(server->config.pubSubConfig.securityPolicies, tKeyStorage->policy);
    UA_PubSubKeyListItem *item;
    UA_UInt32 startingKeyId = 1;
    TAILQ_FOREACH(item, &tKeyStorage->keyList, keyListEntry){
        ck_assert_msg(item->keyID == startingKeyId, "Expected KeyIDs to be incremented by 1 starting from currentKeyId");
        startingKeyId++;
    }
    ck_assert_msg(UA_String_equal(&tKeyStorage->securityGroupID, &SecurityGroupId), "Expected SecurityGroupId to be equal to keystorage->securityGroupID");
    /*check if the keystorage is in the Server Keystorage list*/
    ck_assert_ptr_eq(server->pubSubManager.pubSubKeyList.lh_first, tKeyStorage);

    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestPubSubKeyStorageSetKeys){
    UA_UInt32 currentTokenId = 1;
    futureKeySize = 2;
    UA_Duration msTimeToNextKey = 2000;
    UA_String testSecurityGroupId = UA_STRING("TestSecurityGroup");
    UA_PubSubKeyStorage *tKeyStorage = createKeyStoragewithkeys(currentTokenId, futureKeySize, msTimeToNextKey, 0, testSecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyListItem *keyListIterator;
    ck_assert_ptr_ne(tKeyStorage, NULL);
    ck_assert_msg(UA_ByteString_equal(&currentKey, &tKeyStorage->keyList.tqh_first->key), "Expected CurrentKey to be equal to the first key in the KeyList");
    ck_assert_msg(UA_ByteString_equal(&currentKey, &tKeyStorage->currentItem->key), "Expected CurrentItem to be equal to the CurrentKey");
    keyListIterator = tKeyStorage->keyList.tqh_first->keyListEntry.tqe_next;
    for (size_t i = 0; i < futureKeySize; i++) {
        ck_assert_msg(UA_ByteString_equal(&futureKey[i], &keyListIterator->key), "Expected FutureKey to be equal to the second key in the KeyList");
        keyListIterator = keyListIterator->keyListEntry.tqe_next;
    }
    keyListIterator = TAILQ_LAST(&tKeyStorage->keyList, keyListItems);
    ck_assert_msg(UA_ByteString_equal(&futureKey[futureKeySize - 1], &keyListIterator->key), "Expected lastItem to be equal to the last FutureKey");
    ck_assert_msg(futureKeySize + 1 == tKeyStorage->keyListSize,"Expected KeyListSize to be equal to FutureKeySize + 1");
    ck_assert_msg(tKeyStorage->keyLifeTime == msTimeToNextKey, "Expected keyLifetime to be equal to the Keystorage->keyLifeTime");
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestPubSubKeyStorage_MovetoNextKeyCallback){
    UA_UInt32 currentTokenId = 1;
    futureKeySize = 2;
    UA_Duration msTimeToNextKey = 2000;
    UA_String testSecurityGroupId = UA_STRING("TestSecurityGroup");

    UA_PubSubKeyStorage *tKeyStorage = createKeyStoragewithkeys(currentTokenId, futureKeySize, msTimeToNextKey, 0, testSecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    ck_assert_ptr_ne(tKeyStorage, NULL);
    UA_PubSubKeyListItem *nextCurrentKey = TAILQ_NEXT(tKeyStorage->currentItem, keyListEntry);
    UA_fakeSleep(2000);
    UA_UNLOCK(&server->serviceMutex);

    UA_Server_run_iterate(server,false);
    ck_assert_ptr_eq(nextCurrentKey, tKeyStorage->currentItem);
    ck_assert_msg(UA_ByteString_equal(&nextCurrentKey->key, &tKeyStorage->currentItem->key), "Expected Current key to be the First Future key after first TimeToNextKey expires");
    /*securityTokenId must be updated after KeyLifeTime elapses*/
    ck_assert_uint_eq(nextCurrentKey->keyID, tKeyStorage->currentTokenId);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    ck_assert_uint_eq(wg->securityTokenId, nextCurrentKey->keyID);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    ck_assert_uint_eq(rg->securityTokenId, nextCurrentKey->keyID);
} END_TEST

START_TEST(TestPubSubKeystorage_ImportedKey){

    UA_StatusCode retval = UA_STATUSCODE_BAD;

    UA_Byte nonceData[8]= {1,2,3,4,5,6,7,8};
    UA_ByteString testMsgNonce = {8, nonceData};
    UA_ByteString buffer, expect_buf, signature;
    const char * msg_unenc = MSG_HEADER MSG_PAYLOAD_DEC;

    UA_UInt32 currentTokenId = 1;
    futureKeySize = 2;
    UA_Duration msTimeToNextKey = 2000;
    UA_String testSecurityGroupId = UA_STRING("TestSecurityGroup");

    buffer.length = MSG_LENGTH_DECRYPTED;
    buffer.data = hexstr_to_char(msg_unenc);
    UA_ByteString_copy(&buffer, &expect_buf);

    createKeyStoragewithkeys(currentTokenId, futureKeySize, msTimeToNextKey,0, testSecurityGroupId);
    UA_LOCK(&server->serviceMutex);

    /*encrypt and sign with Writer channelContext*/

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    retval = wg->config.securityPolicy->setMessageNonce(wg->securityPolicyContext, &testMsgNonce);
    retval =  wg->config.securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.encrypt( wg->securityPolicyContext, &buffer);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected retval to be GOOD");
    size_t sigSize = wg->config.securityPolicy->symmetricModule.cryptoModule.
                     signatureAlgorithm.getLocalSignatureSize(wg->securityPolicyContext);
    UA_ByteString_allocBuffer(&signature, sigSize);
    retval = wg->config.securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.sign(wg->securityPolicyContext,&buffer,&signature);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected retval to be GOOD: Error Code %s", UA_StatusCode_name(retval));

    /*decrypt and verify with the imported key in the ReaderGroup*/
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    retval = rg->config.securityPolicy->setMessageNonce(rg->securityPolicyContext, &testMsgNonce);
    retval = rg->config.securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.verify(rg->securityPolicyContext, &buffer,&signature);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected retval to be GOOD: Error Code %s", UA_StatusCode_name(retval));
    retval = rg->config.securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.decrypt(rg->securityPolicyContext,&buffer);
    ck_assert(memcmp(buffer.data, expect_buf.data, buffer.length) == 0);
    UA_ByteString_clear(&expect_buf);
    UA_ByteString_clear(&signature);
    UA_ByteString_clear(&buffer);

    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestPubSubKeyStorage_InitWithWriterGroup){
    addTestWriterGroup(SecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);
    ck_assert_ptr_ne(wg->keyStorage, NULL);
    ck_assert_ptr_eq(ks, wg->keyStorage);
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestPubSubKeyStorage_InitWithReaderGroup){
    addTestReaderGroup(SecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);
    ck_assert_ptr_ne(rg->keyStorage, NULL);
    ck_assert_ptr_eq(ks, rg->keyStorage);
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestAddingNewGroupToExistingKeyStorage){
    addTestWriterGroup(SecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);
    ck_assert_msg(ks->referenceCount == 1, "Expected the reference Count to be exactly 1 after adding one Group");
    UA_UNLOCK(&server->serviceMutex);
    addTestReaderGroup(SecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    ck_assert_msg(ks->referenceCount == 2, "Expected the reference Count to be exactly 2 after adding second Group same SecurityGroupId");
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    ck_assert_ptr_eq(ks, rg->keyStorage);
    ck_assert_ptr_eq(ks, wg->keyStorage);
    ck_assert_ptr_eq(rg->keyStorage, wg->keyStorage);
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

START_TEST(TestRemoveAPubSubGroupWithKeyStorage){
    addTestWriterGroup(SecurityGroupId);
    addTestReaderGroup(SecurityGroupId);
    UA_LOCK(&server->serviceMutex);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);
    UA_UInt32 refCountBefore = ks->referenceCount;
    UA_UNLOCK(&server->serviceMutex);
    UA_Server_removeWriterGroup(server, writerGroup);
    --refCountBefore;
    ck_assert_msg(ks->referenceCount == refCountBefore, "Expected keyStroage referenceCount to be One less then before after removing a Group");
    UA_Server_removeReaderGroup(server, readerGroup);
    UA_LOCK(&server->serviceMutex);
    ks = NULL;
    ks = UA_PubSubKeyStorage_findKeyStorage(server, SecurityGroupId);
    ck_assert_ptr_eq(ks, NULL);
    UA_UNLOCK(&server->serviceMutex);
} END_TEST

int
main(void) {
    int number_failed = 0;
    TCase *tc_pubsub_keystorage = tcase_create("PubSub KeyStorage");
    tcase_add_checked_fixture(tc_pubsub_keystorage, setup, teardown);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeyStorage_initialize);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeyStorageSetKeys);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeyStorage_MovetoNextKeyCallback);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeystorage_ImportedKey);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeyStorage_InitWithWriterGroup);
    tcase_add_test(tc_pubsub_keystorage, TestPubSubKeyStorage_InitWithReaderGroup);
    tcase_add_test(tc_pubsub_keystorage, TestAddingNewGroupToExistingKeyStorage);
    tcase_add_test(tc_pubsub_keystorage, TestRemoveAPubSubGroupWithKeyStorage);

    Suite *s =
        suite_create("PubSub Keystorage and handling keys for Publisher and Subscriber");
    suite_add_tcase(s, tc_pubsub_keystorage);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
