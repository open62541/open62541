/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Siemens AG (Author: Thomas Fischer)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types_generated_encoding_binary.h>

#include "../common.h"

#include "testing_config.h"
#include "ua_pubsub.h"
#include "pubsub/ua_pubsub_config.h"
#include "ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new_testing();
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddPublisherUsingBinaryFile) {
    UA_ByteString publisherConfiguration = loadFile("../tests/pubsub/check_publisher_configuration.bin");
    UA_StatusCode retVal = UA_PubSubManager_loadPubSubConfigFromByteString(server, publisherConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection;
    UA_WriterGroup *writerGroup;
    UA_DataSetWriter *dataSetWriter;
    size_t connectionCount = 0;
    size_t writerGroupCount = 0;
    size_t dataSetWriterCount = 0;
    TAILQ_FOREACH(connection, &server->pubSubManager.connections, listEntry) {
        connectionCount++;
        char* expectedConnectionName = "UADP Connection 1";
        ck_assert_str_eq(expectedConnectionName, (char*)connection->config->name.data);
        LIST_FOREACH(writerGroup, &connection->writerGroups, listEntry){
            writerGroupCount++;
            char* expectedWgName = "Demo WriterGroup";
            ck_assert_str_eq(expectedWgName, (char*)writerGroup->config.name.data);
            LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                dataSetWriterCount++;
                char* expectedWriterName = "Demo DataSetWriter";
                ck_assert_str_eq(expectedWriterName, (char*)dataSetWriter->config.name.data);
            }
        }
    }
    ck_assert_int_eq(connectionCount, 1);
    ck_assert_int_eq(writerGroupCount, 1);
    ck_assert_int_eq(dataSetWriterCount, 1);
} END_TEST

START_TEST(AddSubscriberUsingBinaryFile) {
    UA_ByteString subscriberConfiguration = loadFile("../tests/pubsub/check_subscriber_configuration.bin");
    UA_StatusCode retVal = UA_PubSubManager_loadPubSubConfigFromByteString(server, subscriberConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection;
    UA_ReaderGroup *readerGroup;
    UA_DataSetReader *dataSetReader;
    size_t connectionCount = 0;
    size_t readerGroupCount = 0;
    size_t dataSetReaderCount = 0;
    TAILQ_FOREACH(connection, &server->pubSubManager.connections, listEntry) {
        connectionCount++;
        char* expectedConnectionName = "UDPMC Connection 1";
        ck_assert_str_eq(expectedConnectionName, (char*)connection->config->name.data);
        LIST_FOREACH(readerGroup, &connection->readerGroups, listEntry){
            readerGroupCount++;
            char* expectedRgName = "ReaderGroup1";
            ck_assert_str_eq(expectedRgName, (char*)readerGroup->config.name.data);
            LIST_FOREACH(dataSetReader, &readerGroup->readers, listEntry){
                dataSetReaderCount++;
                char* expectedReaderName = "DataSet Reader 1";
                ck_assert_str_eq(expectedReaderName, (char*)dataSetReader->config.name.data);
            }
        }
    }
    ck_assert_int_eq(connectionCount, 1);
    ck_assert_int_eq(readerGroupCount, 1);
    ck_assert_int_eq(dataSetReaderCount, 1);
} END_TEST

int main(void) {
    TCase *tc_pubsub_file_configuration = tcase_create("File Configuration");
    tcase_add_checked_fixture(tc_pubsub_file_configuration, setup, teardown);
    tcase_add_test(tc_pubsub_file_configuration, AddPublisherUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, AddSubscriberUsingBinaryFile);

    Suite *s = suite_create("PubSub file configuration");
    suite_add_tcase(s, tc_pubsub_file_configuration);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
