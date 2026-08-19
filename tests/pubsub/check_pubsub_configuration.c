/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Siemens AG (Author: Thomas Fischer)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include "../common.h"

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddPublisherUsingBinaryFile) {
    UA_PubSubManager *psm = getPSM(server);
    UA_ByteString publisherConfiguration = loadFile("../../tests/pubsub/check_publisher_configuration.bin");
    ck_assert(publisherConfiguration.length > 0);
    UA_Server_disableAllPubSubComponents(server);
    UA_StatusCode retVal = UA_Server_loadPubSubConfigFromByteString(server, publisherConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection;
    UA_WriterGroup *writerGroup;
    UA_DataSetWriter *dataSetWriter;
    size_t connectionCount = 0;
    size_t writerGroupCount = 0;
    size_t dataSetWriterCount = 0;
    UA_String tmp;
    TAILQ_FOREACH(connection, &psm->connections, listEntry) {
        connectionCount++;
        tmp = UA_STRING("UADP Connection 1");
        ck_assert(UA_String_equal(&tmp, &connection->config.name));
        LIST_FOREACH(writerGroup, &connection->writerGroups, listEntry){
            writerGroupCount++;
            tmp = UA_STRING("Demo WriterGroup");
            ck_assert(UA_String_equal(&tmp, &writerGroup->config.name));
            LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                dataSetWriterCount++;
                tmp = UA_STRING("Demo DataSetWriter");
                ck_assert(UA_String_equal(&tmp, &dataSetWriter->config.name));
            }
        }
    }
    ck_assert_uint_eq(connectionCount, 1);
    ck_assert_uint_eq(writerGroupCount, 1);
    ck_assert_uint_eq(dataSetWriterCount, 1);
    UA_ByteString_clear(&publisherConfiguration);
} END_TEST

START_TEST(AddSubscriberUsingBinaryFile) {
    UA_PubSubManager *psm = getPSM(server);
    UA_ByteString subscriberConfiguration = loadFile("../../tests/pubsub/check_subscriber_configuration.bin");
    ck_assert(subscriberConfiguration.length > 0);
    UA_Server_disableAllPubSubComponents(server);
    UA_StatusCode retVal = UA_Server_loadPubSubConfigFromByteString(server, subscriberConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection;
    UA_ReaderGroup *readerGroup;
    UA_DataSetReader *dataSetReader;
    size_t connectionCount = 0;
    size_t readerGroupCount = 0;
    size_t dataSetReaderCount = 0;
    UA_String tmp;
    TAILQ_FOREACH(connection, &psm->connections, listEntry) {
        connectionCount++;
        tmp = UA_STRING("UDPMC Connection 1");
        ck_assert(UA_String_equal(&tmp, &connection->config.name));
        LIST_FOREACH(readerGroup, &connection->readerGroups, listEntry){
            readerGroupCount++;
            tmp = UA_STRING("ReaderGroup1");
            ck_assert(UA_String_equal(&tmp, &readerGroup->config.name));
            LIST_FOREACH(dataSetReader, &readerGroup->readers, listEntry){
                dataSetReaderCount++;
                tmp = UA_STRING("DataSet Reader 1");
                ck_assert(UA_String_equal(&tmp, &dataSetReader->config.name));
            }
        }
    }
    ck_assert_uint_eq(connectionCount, 1);
    ck_assert_uint_eq(readerGroupCount, 1);
    ck_assert_uint_eq(dataSetReaderCount, 1);

    /* A subscriber-only configuration has no PublishedDataSets and must still
     * be serializable. */
    UA_ByteString savedConfiguration = UA_BYTESTRING_NULL;
    retVal = UA_Server_writePubSubConfigurationToByteString(server,
                                                            &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);
    UA_ByteString_clear(&savedConfiguration);
    UA_ByteString_clear(&subscriberConfiguration);
} END_TEST

START_TEST(SaveEmptyConfiguration) {
    UA_ByteString savedConfiguration = UA_BYTESTRING_NULL;
    UA_StatusCode retVal =
        UA_Server_writePubSubConfigurationToByteString(server,
                                                       &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);

    /* A configuration without PublishedDataSets and Connections must also be
     * loadable again. */
    retVal = UA_Server_loadPubSubConfigFromByteString(server, savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&savedConfiguration);
} END_TEST

START_TEST(SaveConfigurationWithEmptyComponents) {
    /* A Connection without groups, a WriterGroup without DataSetWriters, a
     * ReaderGroup without DataSetReaders and a DataSetReader without
     * TargetVariables must be serializable. */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    /* The first Connection stays without any group */
    UA_NodeId connection;
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connection);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup without writers");
    writerGroupConfig.publishingInterval = 100;
    retVal = UA_Server_addWriterGroup(server, connection, &writerGroupConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup without readers");
    UA_NodeId readerGroup;
    retVal = UA_Server_addReaderGroup(server, connection, &readerGroupConfig,
                                      &readerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(readerConfig));
    readerConfig.name = UA_STRING("DataSetReader without target variables");
    retVal = UA_Server_addDataSetReader(server, readerGroup, &readerConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_ByteString savedConfiguration = UA_BYTESTRING_NULL;
    retVal = UA_Server_writePubSubConfigurationToByteString(server,
                                                            &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);

    UA_Server_disableAllPubSubComponents(server);
    retVal = UA_Server_loadPubSubConfigFromByteString(server, savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&savedConfiguration);
} END_TEST

int main(void) {
    TCase *tc_pubsub_file_configuration = tcase_create("File Configuration");
    tcase_add_checked_fixture(tc_pubsub_file_configuration, setup, teardown);
    tcase_add_test(tc_pubsub_file_configuration, AddPublisherUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, AddSubscriberUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, SaveEmptyConfiguration);
    tcase_add_test(tc_pubsub_file_configuration, SaveConfigurationWithEmptyComponents);

    Suite *s = suite_create("PubSub file configuration");
    suite_add_tcase(s, tc_pubsub_file_configuration);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
