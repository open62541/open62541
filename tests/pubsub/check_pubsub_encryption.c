/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/securitypolicy_default.h>

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"

#include <check.h>
#include <stdlib.h>

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

UA_Server *server = NULL;
UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3;

static size_t generatedNonceLength;

static UA_StatusCode
stubNewContext(UA_PubSubSecurityPolicy *policy, const UA_ByteString *signingKey,
               const UA_ByteString *encryptingKey, const UA_ByteString *keyNonce,
               void **context) {
    (void)signingKey;
    (void)encryptingKey;
    (void)keyNonce;
    *context = policy;
    return UA_STATUSCODE_GOOD;
}

static void
stubDeleteContext(UA_PubSubSecurityPolicy *policy, void *context) {
    (void)policy;
    (void)context;
}

static UA_StatusCode
stubVerify(const UA_PubSubSecurityPolicy *policy, void *context,
           const UA_ByteString *message, const UA_ByteString *signature) {
    (void)policy;
    (void)context;
    (void)message;
    (void)signature;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubSign(const UA_PubSubSecurityPolicy *policy, void *context,
         const UA_ByteString *message, UA_ByteString *signature) {
    return stubVerify(policy, context, message, signature);
}

static size_t
stubGetSize(const UA_PubSubSecurityPolicy *policy, const void *context) {
    (void)policy;
    (void)context;
    return 0;
}

static UA_StatusCode
stubCrypt(const UA_PubSubSecurityPolicy *policy, void *context,
          UA_ByteString *data) {
    (void)policy;
    (void)context;
    (void)data;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubSetKeys(UA_PubSubSecurityPolicy *policy, void *context,
            const UA_ByteString *signingKey, const UA_ByteString *encryptingKey,
            const UA_ByteString *keyNonce) {
    (void)policy;
    (void)context;
    (void)signingKey;
    (void)encryptingKey;
    (void)keyNonce;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubGenerateKey(UA_PubSubSecurityPolicy *policy, void *context,
                const UA_ByteString *secret, const UA_ByteString *seed,
                UA_ByteString *out) {
    (void)policy;
    (void)context;
    (void)secret;
    (void)seed;
    memset(out->data, 0, out->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubGenerateNonce(UA_PubSubSecurityPolicy *policy, void *context,
                  UA_ByteString *out) {
    (void)policy;
    (void)context;
    generatedNonceLength = out->length;
    memset(out->data, 0x5a, out->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
stubSetMessageNonce(UA_PubSubSecurityPolicy *policy, void *context,
                    const UA_ByteString *nonce) {
    (void)context;
    return nonce->length == policy->messageNonceLength ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSECURITYCHECKSFAILED;
}

static void
stubClear(UA_PubSubSecurityPolicy *policy) {
    (void)policy;
}

static UA_PubSubSecurityPolicy
makeStubPolicy(size_t messageNonceLength) {
    UA_PubSubSecurityPolicy policy;
    memset(&policy, 0, sizeof(policy));
    policy.policyUri = UA_STRING("urn:open62541:test:variable-nonce");
    policy.newGroupContext = stubNewContext;
    policy.deleteGroupContext = stubDeleteContext;
    policy.verify = stubVerify;
    policy.sign = stubSign;
    policy.getSignatureSize = stubGetSize;
    policy.getSignatureKeyLength = stubGetSize;
    policy.getEncryptionKeyLength = stubGetSize;
    policy.encrypt = stubCrypt;
    policy.decrypt = stubCrypt;
    policy.setSecurityKeys = stubSetKeys;
    policy.generateKey = stubGenerateKey;
    policy.generateNonce = stubGenerateNonce;
    policy.keyMaterialLength = 4;
    policy.messageNonceLength = messageNonceLength;
    policy.setMessageNonce = stubSetMessageNonce;
    policy.clear = stubClear;
    return policy;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    UA_StatusCode retVal = UA_Server_run_startup(server);
    //add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(SinglePublishDataSetField) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    retVal |= UA_Server_enableWriterGroup(server, writerGroup1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    writerGroupConfig.name = UA_STRING("WriterGroup 2");
    writerGroupConfig.publishingInterval = 50;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup2);
    retVal |= UA_Server_enableWriterGroup(server, writerGroup2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_enableAllPubSubComponents(server);

    writerGroupConfig.name = UA_STRING("WriterGroup 3");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1).addResult;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    pdsConfig.name = UA_STRING("PublishedDataSet 2");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet2).addResult;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    retVal |= UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL).result;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    retVal |= UA_Server_addDataSetWriter(server, writerGroup3, publishedDataSet1,
                                         &dataSetWriterConfig, &dataSetWriter1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

    UA_Server_setWriterGroupEncryptionKeys(server, writerGroup3, 1, sk, ek, kn);

    retVal |= UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup3);
    UA_WriterGroup_publishCallback(psm, wg);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(SecurityPolicyContractIsValidatedAtGroupCreation) {
    UA_PubSubSecurityPolicy policy = makeStubPolicy(0);
    ck_assert_uint_eq(policy.keyMaterialLength, 4);
    ck_assert_uint_eq(policy.messageNonceLength, 0);

    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("invalid-policy-writer");
    wgc.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgc.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    wgc.securityPolicy = &policy;
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, connection1, &wgc, NULL),
                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);

    UA_ReaderGroupConfig rgc;
    memset(&rgc, 0, sizeof(rgc));
    rgc.name = UA_STRING("invalid-policy-reader");
    rgc.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    rgc.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    rgc.securityPolicy = &policy;
    ck_assert_uint_eq(UA_Server_addReaderGroup(server, connection1, &rgc, NULL),
                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);

    /* Publishers and subscribers with externally supplied keys do not need
     * key derivation (for example, the PKCS#11 policies). */
    policy = makeStubPolicy(8);
    policy.generateKey = NULL;
    UA_NodeId wgId, rgId;
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, connection1, &wgc, &wgId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addReaderGroup(server, connection1, &rgc, &rgId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_removeWriterGroup(server, wgId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_removeReaderGroup(server, rgId), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(CustomMessageNonceLengthIsUsed) {
    UA_PubSubSecurityPolicy policy = makeStubPolicy(12);
    ck_assert_uint_eq(policy.messageNonceLength, 12);

    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("variable-nonce-writer");
    wgc.publishingInterval = 100;
    wgc.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgc.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    wgc.securityPolicy = &policy;
    UA_NodeId wgId;
    ck_assert_uint_eq(UA_Server_addWriterGroup(server, connection1, &wgc, &wgId),
                      UA_STATUSCODE_GOOD);

    UA_PublishedDataSetConfig pdc;
    memset(&pdc, 0, sizeof(pdc));
    pdc.name = UA_STRING("variable-nonce-data");
    UA_NodeId pdsId;
    ck_assert_uint_eq(UA_Server_addPublishedDataSet(server, &pdc, &pdsId).addResult,
                      UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig field;
    memset(&field, 0, sizeof(field));
    field.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    field.field.variable.fieldNameAlias = UA_STRING("state");
    field.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    field.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_addDataSetField(server, pdsId, &field, NULL).result,
                      UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("variable-nonce-dsw");
    ck_assert_uint_eq(UA_Server_addDataSetWriter(server, wgId, pdsId, &dswc, NULL),
                      UA_STATUSCODE_GOOD);

    UA_ByteString empty = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_Server_setWriterGroupEncryptionKeys(server, wgId, 1,
                                                             empty, empty, empty),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_enableAllPubSubComponents(server),
                      UA_STATUSCODE_GOOD);
    generatedNonceLength = 0;
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    UA_WriterGroup_publishCallback(psm, wg);
    ck_assert_uint_eq(generatedNonceLength, 8);
    ck_assert_uint_eq(wg->config.securityPolicy->messageNonceLength, 12);

    UA_Server_removeWriterGroup(server, wgId);
    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

int main(void) {
    TCase *tc_pubsub_publish = tcase_create("PubSub publish DataSetFields");
    tcase_add_checked_fixture(tc_pubsub_publish, setup, teardown);
    tcase_add_test(tc_pubsub_publish, SinglePublishDataSetField);
    tcase_add_test(tc_pubsub_publish, SecurityPolicyContractIsValidatedAtGroupCreation);
    tcase_add_test(tc_pubsub_publish, CustomMessageNonceLengthIsUsed);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_pubsub_publish);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
