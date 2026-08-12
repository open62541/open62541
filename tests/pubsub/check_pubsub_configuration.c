/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Siemens AG (Author: Thomas Fischer)
 * Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include "../common.h"

#include "test_helpers.h"
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
    UA_WriterGroup *configuredWriterGroup = NULL;
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
            configuredWriterGroup = writerGroup;
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

    /* Populate fields that historically disappeared during save/load. */
    writerGroup = configuredWriterGroup;
    ck_assert_ptr_nonnull(writerGroup);
    UA_String securityGroup = UA_STRING("security-group-roundtrip");
    UA_String sksEndpoint = UA_STRING("opc.tcp://sks.example:4840");
    UA_String_clear(&writerGroup->config.securityGroupId);
    retVal = UA_String_copy(&securityGroup,
                            &writerGroup->config.securityGroupId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    writerGroup->config.maxNetworkMessageSize = 123456u;
    writerGroup->config.securityKeyServices = UA_EndpointDescription_new();
    ck_assert_ptr_nonnull(writerGroup->config.securityKeyServices);
    writerGroup->config.securityKeyServicesSize = 1;
    retVal = UA_String_copy(&sksEndpoint,
        &writerGroup->config.securityKeyServices[0].endpointUrl);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    UA_UInt32 propertyValue = 42;
    retVal = UA_KeyValueMap_setScalar(&writerGroup->config.groupProperties,
        UA_QUALIFIEDNAME(2, "roundtrip-property"), &propertyValue,
        &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PublishedDataSet *pds = TAILQ_FIRST(&psm->publishedDataSets);
    ck_assert_ptr_nonnull(pds);
    ck_assert_uint_gt(pds->dataSetMetaData.fieldsSize, 0);
    UA_DataSetMetaDataType *metadata = &pds->dataSetMetaData;
    metadata->configurationVersion.majorVersion = 123u;
    metadata->configurationVersion.minorVersion = 456u;
    metadata->dataSetClassId =
        UA_GUID("10203040-5060-7080-90a0-b0c0d0e0f001");
    UA_FieldMetaData *field = &metadata->fields[0];
    field->fieldFlags |= 0x0001u;
    field->maxStringLength = 77u;
    field->dataSetFieldId =
        UA_GUID("01020304-0506-0708-090a-0b0c0d0e0f10");
    UA_LocalizedText_clear(&field->description);
    UA_LocalizedText description =
        UA_LOCALIZEDTEXT("en", "roundtrip-description");
    retVal = UA_LocalizedText_copy(&description, &field->description);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    UA_DataSetMetaDataType expectedMetadata;
    UA_DataSetMetaDataType_init(&expectedMetadata);
    retVal = UA_DataSetMetaDataType_copy(metadata, &expectedMetadata);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    UA_ByteString roundtripConfiguration = UA_BYTESTRING_NULL;
    retVal = UA_Server_writePubSubConfigurationToByteString(
        server, &roundtripConfiguration);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(roundtripConfiguration.length, 0);
    UA_Server_disableAllPubSubComponents(server);
    retVal = UA_Server_loadPubSubConfigFromByteString(server,
                                                      roundtripConfiguration);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    writerGroup = LIST_FIRST(&connection->writerGroups);
    ck_assert_ptr_nonnull(writerGroup);
    ck_assert(UA_String_equal(&writerGroup->config.securityGroupId,
                              &securityGroup));
    ck_assert_uint_eq(writerGroup->config.maxNetworkMessageSize, 123456u);
    ck_assert_uint_eq(writerGroup->config.securityKeyServicesSize, 1);
    ck_assert(UA_String_equal(
        &writerGroup->config.securityKeyServices[0].endpointUrl,
        &sksEndpoint));
    ck_assert_uint_eq(writerGroup->config.groupProperties.mapSize, 1);

    pds = TAILQ_FIRST(&psm->publishedDataSets);
    ck_assert_ptr_nonnull(pds);
    ck_assert(UA_DataSetMetaDataType_equal(&expectedMetadata,
                                           &pds->dataSetMetaData));

    UA_DataSetMetaDataType_clear(&expectedMetadata);
    UA_ByteString_clear(&roundtripConfiguration);
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
    UA_ReaderGroup *configuredReaderGroup = NULL;
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
            configuredReaderGroup = readerGroup;
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

    readerGroup = configuredReaderGroup;
    ck_assert_ptr_nonnull(readerGroup);
    UA_String securityGroup = UA_STRING("reader-security-group-roundtrip");
    UA_String sksEndpoint = UA_STRING("opc.tcp://reader-sks.example:4840");
    UA_String_clear(&readerGroup->config.securityGroupId);
    retVal = UA_String_copy(&securityGroup, &readerGroup->config.securityGroupId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    readerGroup->config.maxNetworkMessageSize = 654321u;
    readerGroup->config.securityKeyServices = UA_EndpointDescription_new();
    ck_assert_ptr_nonnull(readerGroup->config.securityKeyServices);
    readerGroup->config.securityKeyServicesSize = 1;
    retVal = UA_String_copy(&sksEndpoint,
        &readerGroup->config.securityKeyServices[0].endpointUrl);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    /* A subscriber-only configuration has no PublishedDataSets and must still
     * be serializable. */
    UA_ByteString savedConfiguration = UA_BYTESTRING_NULL;
    retVal = UA_Server_writePubSubConfigurationToByteString(server,
                                                            &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);

    UA_Server_disableAllPubSubComponents(server);
    retVal = UA_Server_loadPubSubConfigFromByteString(server,
                                                      savedConfiguration);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    readerGroup = LIST_FIRST(&connection->readerGroups);
    ck_assert_ptr_nonnull(readerGroup);
    ck_assert(UA_String_equal(&readerGroup->config.securityGroupId,
                              &securityGroup));
    ck_assert_uint_eq(readerGroup->config.maxNetworkMessageSize, 654321u);
    ck_assert_uint_eq(readerGroup->config.securityKeyServicesSize, 1);
    ck_assert(UA_String_equal(
        &readerGroup->config.securityKeyServices[0].endpointUrl,
        &sksEndpoint));
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
    UA_ByteString_clear(&savedConfiguration);
} END_TEST

int main(void) {
    TCase *tc_pubsub_file_configuration = tcase_create("File Configuration");
    tcase_add_checked_fixture(tc_pubsub_file_configuration, setup, teardown);
    tcase_add_test(tc_pubsub_file_configuration, AddPublisherUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, AddSubscriberUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, SaveEmptyConfiguration);

    Suite *s = suite_create("PubSub file configuration");
    suite_add_tcase(s, tc_pubsub_file_configuration);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
