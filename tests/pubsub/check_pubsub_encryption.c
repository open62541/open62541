/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/securitypolicy_default.h>

#include "test_helpers.h"
#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

UA_Server *server = NULL;
UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3;

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
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
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
    writerGroupConfig.name = UA_STRING("WriterGroup 3");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);
    retVal |= UA_Server_enableWriterGroup(server, writerGroup3);
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

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup3);
    UA_WriterGroup_publishCallback(server, wg);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    TCase *tc_pubsub_publish = tcase_create("PubSub publish DataSetFields");
    tcase_add_checked_fixture(tc_pubsub_publish, setup, teardown);
    tcase_add_test(tc_pubsub_publish, SinglePublishDataSetField);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_pubsub_publish);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
