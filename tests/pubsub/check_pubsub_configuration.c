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

static UA_WriterGroup *
findWriterGroupByName(UA_PubSubConnection *connection, const char *name) {
    UA_String expected = UA_STRING((char*)(uintptr_t)name);
    UA_WriterGroup *wg;
    LIST_FOREACH(wg, &connection->writerGroups, listEntry) {
        if(UA_String_equal(&wg->config.name, &expected))
            return wg;
    }
    return NULL;
}

static void
addSecondWriterGroup(UA_PubSubConnection *connection,
                     UA_WriterGroup *templateGroup) {
    UA_WriterGroupConfig config;
    memset(&config, 0, sizeof(config));
    UA_StatusCode res =
        UA_WriterGroupConfig_copy(&templateGroup->config, &config);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&config.name);
    config.name = UA_STRING_ALLOC("Second WriterGroup");
    ck_assert_ptr_nonnull(config.name.data);
    config.writerGroupId++;
    config.enabled = false;

    UA_NodeId id;
    res = UA_Server_addWriterGroup(server, connection->head.identifier,
                                   &config, &id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_WriterGroupConfig_clear(&config);
}

static void
assertWriterGroupEnabled(UA_PubSubConnection *connection, const char *name,
                         UA_Boolean expected) {
    UA_WriterGroup *wg = findWriterGroupByName(connection, name);
    ck_assert_ptr_nonnull(wg);
    ck_assert_uint_eq(wg->config.enabled, expected);
}

START_TEST(AddPublisherUsingBinaryFile) {
    UA_PubSubManager *psm = getPSM(server);
    UA_ByteString publisherConfiguration =
        loadFile(UA_TEST_PUBSUB_CONFIG_DIR "check_publisher_configuration.bin");
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
    writerGroup->config.localeIds = (UA_String*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(writerGroup->config.localeIds);
    writerGroup->config.localeIdsSize = 2;
    writerGroup->config.localeIds[0] = UA_STRING_ALLOC("de-DE");
    writerGroup->config.localeIds[1] = UA_STRING_ALLOC("en-US");
    writerGroup->config.headerLayoutUri =
        UA_STRING_ALLOC("urn:open62541:test:writer-layout");
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
    ck_assert_uint_eq(writerGroup->config.localeIdsSize, 2);
    UA_String deDe = UA_STRING("de-DE");
    UA_String enUs = UA_STRING("en-US");
    UA_String writerLayout = UA_STRING("urn:open62541:test:writer-layout");
    ck_assert(UA_String_equal(&writerGroup->config.localeIds[0], &deDe));
    ck_assert(UA_String_equal(&writerGroup->config.localeIds[1], &enUs));
    ck_assert(UA_String_equal(&writerGroup->config.headerLayoutUri,
                              &writerLayout));
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
    UA_ByteString subscriberConfiguration =
        loadFile(UA_TEST_PUBSUB_CONFIG_DIR "check_subscriber_configuration.bin");
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

    dataSetReader = LIST_FIRST(&readerGroup->readers);
    ck_assert_ptr_nonnull(dataSetReader);
    dataSetReader->config.keyFrameCount = 7;
    dataSetReader->config.headerLayoutUri =
        UA_STRING_ALLOC("urn:open62541:test:reader-layout");
    UA_UInt32 readerProperty = 73;
    retVal = UA_KeyValueMap_setScalar(
        &dataSetReader->config.dataSetReaderProperties,
        UA_QUALIFIEDNAME(3, "reader-roundtrip"), &readerProperty,
        &UA_TYPES[UA_TYPES_UINT32]);
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
    dataSetReader = LIST_FIRST(&readerGroup->readers);
    ck_assert_ptr_nonnull(dataSetReader);
    ck_assert_uint_eq(dataSetReader->config.keyFrameCount, 7);
    UA_String readerLayout = UA_STRING("urn:open62541:test:reader-layout");
    ck_assert(UA_String_equal(&dataSetReader->config.headerLayoutUri,
                              &readerLayout));
    ck_assert_uint_eq(dataSetReader->config.dataSetReaderProperties.mapSize, 1);
    const UA_Variant *readerPropertyValue = UA_KeyValueMap_get(
        &dataSetReader->config.dataSetReaderProperties,
        UA_QUALIFIEDNAME(3, "reader-roundtrip"));
    ck_assert_ptr_nonnull(readerPropertyValue);
    ck_assert_ptr_eq(readerPropertyValue->type, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(*(UA_UInt32*)readerPropertyValue->data, readerProperty);
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

    retVal = UA_Server_loadPubSubConfigFromByteString(server,
                                                       savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&savedConfiguration);
} END_TEST

START_TEST(SaveConfigurationWithEmptyComponents) {
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &address,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(
        "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    UA_StatusCode res =
        UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId connection;
    res = UA_Server_addPubSubConnection(server, &connectionConfig, &connection);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig writerGroup = {0};
    writerGroup.name = UA_STRING("WriterGroup without writers");
    writerGroup.publishingInterval = 100;
    res = UA_Server_addWriterGroup(server, connection, &writerGroup, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ReaderGroupConfig readerGroup = {0};
    readerGroup.name = UA_STRING("ReaderGroup without readers");
    UA_NodeId readerGroupId;
    res = UA_Server_addReaderGroup(server, connection, &readerGroup,
                                   &readerGroupId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_DataSetReaderConfig reader = {0};
    reader.name = UA_STRING("DataSetReader without target variables");
    res = UA_Server_addDataSetReader(server, readerGroupId, &reader, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ByteString saved = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &saved);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(saved.length, 0);
    UA_ByteString_clear(&saved);
}
END_TEST

/* Before the identity-based restore fix, WriterGroups were paired with the
 * decoded array by linked-list position. Creating them inserts at the list
 * head, so a round trip swaps mixed enabled flags between two groups. */
START_TEST(EnabledFlagsAreRestoredByComponentIdentity) {
    UA_ByteString input =
        loadFile(UA_TEST_PUBSUB_CONFIG_DIR "check_publisher_configuration.bin");
    ck_assert_uint_gt(input.length, 0);
    UA_Server_disableAllPubSubComponents(server);
    UA_StatusCode res =
        UA_Server_loadPubSubConfigFromByteString(server, input);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    UA_WriterGroup *first = LIST_FIRST(&connection->writerGroups);
    ck_assert_ptr_nonnull(first);
    addSecondWriterGroup(connection, first);

    first = findWriterGroupByName(connection, "Demo WriterGroup");
    UA_WriterGroup *second =
        findWriterGroupByName(connection, "Second WriterGroup");
    ck_assert_ptr_nonnull(first);
    ck_assert_ptr_nonnull(second);
    connection->config.enabled = true;
    first->config.enabled = true;
    second->config.enabled = false;

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_loadPubSubConfigFromByteString(server, encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    ck_assert_uint_eq(connection->config.enabled, true);
    assertWriterGroupEnabled(connection, "Demo WriterGroup", true);
    assertWriterGroupEnabled(connection, "Second WriterGroup", false);

    UA_ByteString_clear(&encoded);
    UA_ByteString_clear(&input);
} END_TEST

/* A disabled parent still has to retain the desired enabled state of its
 * children. The old second-phase loop skipped every child when the connection
 * was disabled and silently rewrote enabled=true to false. */
START_TEST(DisabledParentPreservesChildEnabledIntent) {
    UA_ByteString input =
        loadFile(UA_TEST_PUBSUB_CONFIG_DIR "check_publisher_configuration.bin");
    ck_assert_uint_gt(input.length, 0);
    UA_Server_disableAllPubSubComponents(server);
    UA_StatusCode res =
        UA_Server_loadPubSubConfigFromByteString(server, input);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    UA_WriterGroup *group = LIST_FIRST(&connection->writerGroups);
    ck_assert_ptr_nonnull(group);
    connection->config.enabled = false;
    group->config.enabled = true;

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_loadPubSubConfigFromByteString(server, encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    ck_assert_uint_eq(connection->config.enabled, false);
    assertWriterGroupEnabled(connection, "Demo WriterGroup", true);

    UA_ByteString_clear(&encoded);
    UA_ByteString_clear(&input);
} END_TEST

START_TEST(FileConfigurationRejectsNullArguments) {
    UA_ByteString empty = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_Server_loadPubSubConfigFromByteString(NULL, empty),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_writePubSubConfigurationToByteString(NULL,
                                                                     &empty),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_writePubSubConfigurationToByteString(server,
                                                                     NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(FileConfigurationRejectsMalformedEncoding) {
    UA_Byte malformedData[] = {0xff, 0xff, 0xff, 0xff};
    UA_ByteString malformed = {sizeof(malformedData), malformedData};
    ck_assert_uint_ne(UA_Server_loadPubSubConfigFromByteString(server, malformed),
                      UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(DataSetWriterTransportSettingsAreCopied) {
    UA_BrokerDataSetWriterTransportDataType transport;
    UA_BrokerDataSetWriterTransportDataType_init(&transport);
    transport.queueName = UA_STRING("writer/topic");
    UA_DataSetWriterConfig source;
    memset(&source, 0, sizeof(source));
    ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(&source.transportSettings,
        &transport, &UA_TYPES[UA_TYPES_BROKERDATASETWRITERTRANSPORTDATATYPE]),
        UA_STATUSCODE_GOOD);
    UA_DataSetWriterConfig copy;
    ck_assert_uint_eq(UA_DataSetWriterConfig_copy(&source, &copy), UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(source.transportSettings.content.decoded.data,
                     copy.transportSettings.content.decoded.data);
    UA_DataSetWriterConfig_clear(&source);
    UA_BrokerDataSetWriterTransportDataType *copied =
        (UA_BrokerDataSetWriterTransportDataType*)copy.transportSettings.content.decoded.data;
    ck_assert(UA_String_equal(&copied->queueName, &transport.queueName));
    UA_DataSetWriterConfig_clear(&copy);
} END_TEST

/* Configuration validation only needs the EventLoop, without listening for
 * client connections or enabling any PubSub components. */
static void
setupFileValidation(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
}

static void
teardownFileValidation(void) {
    UA_Server_delete(server);
}

static UA_StatusCode
loadEncodedFile(UA_ExtensionObject *file) {
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(file, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                                      &encoded, NULL), UA_STATUSCODE_GOOD);
    UA_StatusCode res = UA_Server_loadPubSubConfigFromByteString(server, encoded);
    UA_ByteString_clear(&encoded);
    return res;
}

static UA_StatusCode
loadFileConfig(UA_PubSubConfigurationDataType *config) {
    UA_UABinaryFileDataType file = {0};
    UA_Variant_setScalar(&file.body, config,
                         &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);
    UA_ExtensionObject wrapper;
    UA_ExtensionObject_setValueNoDelete(&wrapper, &file,
                                       &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE]);
    return loadEncodedFile(&wrapper);
}

static void
assertEmptyFileConfig(void) {
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert(TAILQ_EMPTY(&psm->connections));
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert(TAILQ_EMPTY(&psm->publishedDataSets));
}

static void
initFileConnection(UA_PubSubConnectionDataType *connection,
                   UA_NetworkAddressUrlDataType *address, UA_UInt32 *publisherId) {
    UA_PubSubConnectionDataType_init(connection);
    connection->name = UA_STRING("File connection");
    connection->transportProfileUri = UA_STRING(
        "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Variant_setScalar(&connection->publisherId, publisherId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_ExtensionObject_setValueNoDelete(&connection->address, address,
                                       &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
}

START_TEST(FileConfigurationRejectsWrongWrapper) {
    UA_ExtensionObject wrapper;
    UA_ExtensionObject_init(&wrapper);
    UA_UABinaryFileDataType file = {0};
    UA_ApplicationDescription application = {0};
    UA_PubSubConfigurationDataType config = {0};
    UA_UInt32 number = 17;
    UA_StatusCode expected = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(_i == 1) {
        UA_ExtensionObject_setValueNoDelete(&wrapper, &application,
                                           &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    } else if(_i >= 2) {
        UA_ExtensionObject_setValueNoDelete(&wrapper, &file,
                                           &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE]);
        expected = UA_STATUSCODE_BADTYPEMISMATCH;
        if(_i == 3)
            UA_Variant_setScalar(&file.body, &number, &UA_TYPES[UA_TYPES_UINT32]);
        if(_i == 4) {
            UA_Variant_setArray(&file.body, &config, 1,
                                &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);
            expected = UA_STATUSCODE_BADNOTIMPLEMENTED;
        }
    }
    ck_assert_uint_eq(loadEncodedFile(&wrapper), expected);
    assertEmptyFileConfig();
} END_TEST

START_TEST(FileConfigurationRejectsInvalidConnection) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 23;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_Int32 invalidPublisherId = -1;
    UA_ApplicationDescription invalidAddress = {0};
    switch(_i) {
    case 0:
        UA_Variant_setScalar(&connection.publisherId, &invalidPublisherId,
                             &UA_TYPES[UA_TYPES_INT32]);
        break;
    case 1:
        UA_ExtensionObject_init(&connection.address);
        break;
    case 2:
        connection.transportProfileUri = UA_STRING("urn:unsupported:transport");
        break;
    default:
        UA_ExtensionObject_setValueNoDelete(&connection.address, &invalidAddress,
                                           &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        break;
    }
    UA_PubSubConfigurationDataType config = {0};
    config.connectionsSize = 1;
    config.connections = &connection;
    ck_assert_uint_eq(loadFileConfig(&config), UA_STATUSCODE_BADINTERNALERROR);
    assertEmptyFileConfig();
} END_TEST

START_TEST(FileConfigurationRollsBackPublishedDataSets) {
    UA_PublishedDataSetDataType published[2];
    memset(published, 0, sizeof(published));
    UA_PublishedDataItemsDataType items = {0};
    published[0].name = UA_STRING("First dataset");
    UA_ExtensionObject_setValueNoDelete(&published[0].dataSetSource, &items,
                                       &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
    published[1].name = UA_STRING("Invalid dataset");
    UA_PublishedEventsDataType events = {0};
    UA_ApplicationDescription wrongSource = {0};
    UA_FieldMetaData field = {0};
    UA_PublishedVariableDataType variable = {0};
    UA_PublishedDataItemsDataType invalidItems = {0};
    UA_StatusCode expected = UA_STATUSCODE_BADINTERNALERROR;
    switch(_i) {
    case 0:
        break; /* Missing data source */
    case 1:
        UA_ExtensionObject_setValueNoDelete(&published[1].dataSetSource, &events,
                                           &UA_TYPES[UA_TYPES_PUBLISHEDEVENTSDATATYPE]);
        expected = UA_STATUSCODE_BADNOTIMPLEMENTED;
        break;
    case 2:
        UA_ExtensionObject_setValueNoDelete(&published[1].dataSetSource, &wrongSource,
                                           &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        break;
    case 3:
        published[1] = published[0];
        expected = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        break;
    default:
        published[1].dataSetMetaData.fieldsSize = 1;
        published[1].dataSetMetaData.fields = &field;
        UA_ExtensionObject_setValueNoDelete(&published[1].dataSetSource, &invalidItems,
                                           &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
        if(_i == 5) {
            variable.publishedVariable = UA_NODEID_NUMERIC(1, 999999);
            variable.attributeId = UA_ATTRIBUTEID_VALUE;
            invalidItems.publishedDataSize = 1;
            invalidItems.publishedData = &variable;
            expected = UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
        break;
    }
    UA_PubSubConfigurationDataType config = {0};
    config.publishedDataSetsSize = 2;
    config.publishedDataSets = published;
    ck_assert_uint_eq(loadFileConfig(&config), expected);
    assertEmptyFileConfig();
} END_TEST

START_TEST(FileConfigurationRollsBackWriterGroups) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 23;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_WriterGroupDataType groups[2];
    memset(groups, 0, sizeof(groups));
    UA_UadpWriterGroupMessageDataType message = {0};
    groups[0].name = UA_STRING("First writer group");
    groups[0].publishingInterval = 100;
    UA_ExtensionObject_setValueNoDelete(&groups[0].messageSettings, &message,
                                       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    groups[1] = groups[0];
    groups[1].name = UA_STRING("Invalid writer group");
    groups[1].writerGroupId = 1;
    UA_ApplicationDescription wrongSettings = {0};
    UA_DataSetWriterDataType writer = {0};
    UA_StatusCode expected = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(_i == 0) {
        UA_ExtensionObject_init(&groups[1].messageSettings);
    } else if(_i == 1) {
        UA_ExtensionObject_setValueNoDelete(&groups[1].messageSettings, &wrongSettings,
                                           &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    } else {
        writer.name = UA_STRING("Writer with missing dataset");
        writer.dataSetName = UA_STRING("Missing dataset");
        writer.keyFrameCount = 1;
        groups[1].dataSetWritersSize = 1;
        groups[1].dataSetWriters = &writer;
        expected = UA_STATUSCODE_BADNOTFOUND;
    }
    connection.writerGroupsSize = 2;
    connection.writerGroups = groups;
    UA_PublishedDataItemsDataType items = {0};
    UA_PublishedDataSetDataType published = {0};
    published.name = UA_STRING("Existing dataset");
    UA_ExtensionObject_setValueNoDelete(&published.dataSetSource, &items,
                                       &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
    UA_PubSubConfigurationDataType config = {0};
    config.publishedDataSetsSize = 1;
    config.publishedDataSets = &published;
    config.connectionsSize = 1;
    config.connections = &connection;
    ck_assert_uint_eq(loadFileConfig(&config), expected);
    assertEmptyFileConfig();
} END_TEST

START_TEST(FileConfigurationRollsBackReaders) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 23;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_TargetVariablesDataType targets = {0};
    UA_DataSetReaderDataType readers[2];
    memset(readers, 0, sizeof(readers));
    readers[0].name = UA_STRING("First reader");
    UA_ExtensionObject_setValueNoDelete(&readers[0].subscribedDataSet, &targets,
                                       &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);
    readers[1].name = UA_STRING("Invalid reader");
    UA_SubscribedDataSetMirrorDataType mirror = {0};
    UA_ApplicationDescription wrongSource = {0};
    UA_Int32 invalidPublisherId = -1;
    UA_StatusCode expected = UA_STATUSCODE_BADINVALIDARGUMENT;
    switch(_i) {
    case 0:
        break; /* Missing subscribed dataset */
    case 1:
        UA_ExtensionObject_setValueNoDelete(&readers[1].subscribedDataSet, &mirror,
                                           &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE]);
        break;
    case 2:
        UA_ExtensionObject_setValueNoDelete(&readers[1].subscribedDataSet, &wrongSource,
                                           &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
        expected = UA_STATUSCODE_BADINTERNALERROR;
        break;
    default:
        UA_Variant_setScalar(&readers[1].publisherId, &invalidPublisherId,
                             &UA_TYPES[UA_TYPES_INT32]);
        expected = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    UA_ReaderGroupDataType group = {0};
    group.name = UA_STRING("Reader group");
    group.dataSetReadersSize = 2;
    group.dataSetReaders = readers;
    connection.readerGroupsSize = 1;
    connection.readerGroups = &group;
    UA_PubSubConfigurationDataType config = {0};
    config.connectionsSize = 1;
    config.connections = &connection;
    ck_assert_uint_eq(loadFileConfig(&config), expected);
    assertEmptyFileConfig();
} END_TEST

START_TEST(FileConfigurationPreservesPublisherAndTransport) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 23;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_UInt16 shortId = UA_UINT16_MAX;
    UA_UInt64 longId = UA_UINT64_MAX;
    UA_String stringId = UA_STRING("Plant/A");
    if(_i == 0)
        UA_Variant_setScalar(&connection.publisherId, &shortId, &UA_TYPES[UA_TYPES_UINT16]);
    else if(_i == 1)
        UA_Variant_setScalar(&connection.publisherId, &longId, &UA_TYPES[UA_TYPES_UINT64]);
    else
        UA_Variant_setScalar(&connection.publisherId, &stringId, &UA_TYPES[UA_TYPES_STRING]);

    UA_DatagramConnectionTransportDataType transport;
    UA_DatagramConnectionTransportDataType_init(&transport);
    UA_ExtensionObject_setValueNoDelete(&transport.discoveryAddress, &address,
                                       &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_ExtensionObject_setValueNoDelete(&connection.transportSettings, &transport,
                                       &UA_TYPES[UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE]);
    UA_PubSubConfigurationDataType config = {0};
    config.connectionsSize = 1;
    config.connections = &connection;
    ck_assert_uint_eq(loadFileConfig(&config), UA_STATUSCODE_GOOD);

    for(size_t round = 0; round < 2; round++) {
        UA_PubSubManager *psm = getPSM(server);
        ck_assert_uint_eq(psm->connectionsSize, 1);
        UA_PubSubConnection *restored = TAILQ_FIRST(&psm->connections);
        ck_assert_ptr_nonnull(restored);
        UA_Variant actual;
        UA_PublisherId_toVariant(&restored->config.publisherId, &actual);
        ck_assert(UA_Variant_equal(&actual, &connection.publisherId));
        UA_Variant *settings = &restored->config.connectionTransportSettings;
        ck_assert(UA_Variant_hasScalarType(settings,
            &UA_TYPES[UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE]));
        UA_DatagramConnectionTransportDataType *restoredTransport =
            (UA_DatagramConnectionTransportDataType*)settings->data;
        UA_ExtensionObject *discovery = &restoredTransport->discoveryAddress;
        ck_assert_uint_eq(discovery->encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert_ptr_eq(discovery->content.decoded.type,
                          &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        ck_assert_ptr_nonnull(discovery->content.decoded.data);
        UA_NetworkAddressUrlDataType *restoredAddress =
            (UA_NetworkAddressUrlDataType*)discovery->content.decoded.data;
        ck_assert(UA_NetworkAddressUrlDataType_equal(restoredAddress, &address));
        ck_assert_ptr_ne(restoredAddress->url.data, address.url.data);
        if(round == 0) {
            UA_ByteString saved = UA_BYTESTRING_NULL;
            ck_assert_uint_eq(UA_Server_writePubSubConfigurationToByteString(server, &saved),
                              UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(UA_Server_loadPubSubConfigFromByteString(server, saved),
                              UA_STATUSCODE_GOOD);
            UA_ByteString_clear(&saved);
        }
    }
} END_TEST

START_TEST(FileConfigurationKeepsActiveConfiguration) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 23;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_PubSubConfigurationDataType config = {0};
    config.enabled = true;
    config.connectionsSize = 1;
    config.connections = &connection;
    ck_assert_uint_eq(loadFileConfig(&config), UA_STATUSCODE_GOOD);
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STARTED);
    UA_PubSubConnection *original = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(original);

    UA_ByteString saved = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_Server_writePubSubConfigurationToByteString(server, &saved),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_loadPubSubConfigFromByteString(server, saved),
                      UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_ptr_eq(TAILQ_FIRST(&psm->connections), original);
    ck_assert_uint_eq(original->config.publisherId.idType, UA_PUBLISHERIDTYPE_UINT32);
    ck_assert_uint_eq(original->config.publisherId.id.uint32, publisherId);
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STARTED);

    UA_Server_disableAllPubSubComponents(server);
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_loadPubSubConfigFromByteString(server, saved),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&saved);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STARTED);
    UA_Server_disableAllPubSubComponents(server);
} END_TEST

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
static UA_NodeId
configurationMethod(const char *name) {
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, (char *)(uintptr_t)name);
    UA_BrowsePathResult result = UA_Server_browseSimplifiedBrowsePath(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE), 1, &browseName);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.targetsSize, 1);
    UA_NodeId method = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_NodeId_copy(&result.targets[0].targetId.nodeId, &method),
                      UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&result);
    return method;
}

static UA_StatusCode
callConfigurationMethod(UA_NodeId method, UA_ByteString *configuration) {
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    request.methodId = method;
    UA_Variant input;
    if(configuration) {
        UA_Variant_setScalar(&input, configuration, &UA_TYPES[UA_TYPES_BYTESTRING]);
        request.inputArgumentsSize = 1;
        request.inputArguments = &input;
    }
    UA_CallMethodResult result = UA_Server_call(server, &request);
    UA_StatusCode res = result.statusCode;
    ck_assert_uint_eq(result.outputArgumentsSize, 0);
    UA_CallMethodResult_clear(&result);
    return res;
}

static void
createMethodConfiguration(UA_Boolean enabled) {
    UA_NetworkAddressUrlDataType address = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_UInt32 publisherId = 73;
    UA_PubSubConnectionDataType connection;
    initFileConnection(&connection, &address, &publisherId);
    UA_PublishedDataItemsDataType items = {0};
    UA_PublishedDataSetDataType dataSet = {0};
    dataSet.name = UA_STRING("Method dataset");
    UA_ExtensionObject_setValueNoDelete(&dataSet.dataSetSource, &items,
                                       &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
    UA_PubSubConfigurationDataType config = {0};
    config.enabled = enabled;
    config.connectionsSize = 1;
    config.connections = &connection;
    config.publishedDataSetsSize = 1;
    config.publishedDataSets = &dataSet;
    ck_assert_uint_eq(loadFileConfig(&config), UA_STATUSCODE_GOOD);
}

START_TEST(ConfigurationDeleteMethodClearsAndPermitsReload) {
    if(_i > 0)
        createMethodConfiguration(_i == 2);
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId remove = configurationMethod("Delete PubSub config");
    UA_NodeId load = configurationMethod("PubSub configuration");
    UA_ByteString saved = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_Server_writePubSubConfigurationToByteString(server, &saved),
                      UA_STATUSCODE_GOOD);
    UA_NodeId connectionId = UA_NODEID_NULL;
    UA_NodeId dataSetId = UA_NODEID_NULL;
    if(_i > 0) {
        ck_assert_uint_eq(UA_NodeId_copy(&TAILQ_FIRST(&psm->connections)->head.identifier,
                                        &connectionId), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_NodeId_copy(&TAILQ_FIRST(&psm->publishedDataSets)->head.identifier,
                                        &dataSetId), UA_STATUSCODE_GOOD);
    }
    /* Two accepted deletes must remain safe while their callbacks are queued. */
    ck_assert_uint_eq(callConfigurationMethod(remove, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callConfigurationMethod(remove, NULL), UA_STATUSCODE_GOOD);
    if(_i > 0) {
        ck_assert_uint_eq(psm->connectionsSize, 1);
        ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    }
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
    assertEmptyFileConfig();
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STOPPED);
    if(_i > 0) {
        UA_NodeClass nodeClass;
        ck_assert_uint_eq(UA_Server_readNodeClass(server, connectionId, &nodeClass),
                          UA_STATUSCODE_BADNODEIDUNKNOWN);
        ck_assert_uint_eq(UA_Server_readNodeClass(server, dataSetId, &nodeClass),
                          UA_STATUSCODE_BADNODEIDUNKNOWN);
    }

    /* The configuration methods survive deletion and can restore the saved file. */
    ck_assert_uint_eq(callConfigurationMethod(load, &saved), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, _i > 0 ? 1 : 0);
    ck_assert_uint_eq(psm->publishedDataSetsSize, _i > 0 ? 1 : 0);
    if(_i > 0) {
        UA_PubSubConnection *connection = TAILQ_FIRST(&psm->connections);
        ck_assert_uint_eq(connection->config.publisherId.idType, UA_PUBLISHERIDTYPE_UINT32);
        ck_assert_uint_eq(connection->config.publisherId.id.uint32, 73);
        UA_String expected = UA_STRING("Method dataset");
        ck_assert(UA_String_equal(&TAILQ_FIRST(&psm->publishedDataSets)->config.name, &expected));
    }
    ck_assert_uint_eq(psm->drv.state,
                      _i == 2 ? UA_LIFECYCLESTATE_STARTED : UA_LIFECYCLESTATE_STOPPED);
    UA_Server_disableAllPubSubComponents(server);
    UA_ByteString_clear(&saved);
    UA_NodeId_clear(&connectionId);
    UA_NodeId_clear(&dataSetId);
    UA_NodeId_clear(&remove);
    UA_NodeId_clear(&load);
} END_TEST

START_TEST(ConfigurationLoadMethodRejectsMalformedFileWithoutMutation) {
    createMethodConfiguration(false);
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *connection = TAILQ_FIRST(&psm->connections);
    UA_PublishedDataSet *dataSet = TAILQ_FIRST(&psm->publishedDataSets);
    UA_NodeId load = configurationMethod("PubSub configuration");
    UA_ByteString malformed = UA_BYTESTRING("invalid binary configuration");
    ck_assert_uint_eq(callConfigurationMethod(load, &malformed), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    ck_assert_ptr_eq(TAILQ_FIRST(&psm->connections), connection);
    ck_assert_ptr_eq(TAILQ_FIRST(&psm->publishedDataSets), dataSet);
    ck_assert_uint_eq(psm->drv.state, UA_LIFECYCLESTATE_STOPPED);
    UA_NodeId_clear(&load);
} END_TEST
#endif

int main(void) {
    TCase *tc_pubsub_file_configuration = tcase_create("File Configuration");
    tcase_add_checked_fixture(tc_pubsub_file_configuration, setup, teardown);
    tcase_add_test(tc_pubsub_file_configuration, AddPublisherUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, AddSubscriberUsingBinaryFile);
    tcase_add_test(tc_pubsub_file_configuration, SaveEmptyConfiguration);
    tcase_add_test(tc_pubsub_file_configuration,
                   SaveConfigurationWithEmptyComponents);
    tcase_add_test(tc_pubsub_file_configuration, DataSetWriterTransportSettingsAreCopied);
    tcase_add_test(tc_pubsub_file_configuration,
                   EnabledFlagsAreRestoredByComponentIdentity);
    tcase_add_test(tc_pubsub_file_configuration,
                   DisabledParentPreservesChildEnabledIntent);
    tcase_add_test(tc_pubsub_file_configuration,
                   FileConfigurationRejectsNullArguments);
    tcase_add_test(tc_pubsub_file_configuration,
                   FileConfigurationRejectsMalformedEncoding);

    Suite *s = suite_create("PubSub file configuration");
    suite_add_tcase(s, tc_pubsub_file_configuration);

    TCase *tc_validation = tcase_create("File validation");
    tcase_add_checked_fixture(tc_validation, setupFileValidation,
                              teardownFileValidation);
    tcase_add_loop_test(tc_validation, FileConfigurationRejectsWrongWrapper, 0, 5);
    tcase_add_loop_test(tc_validation, FileConfigurationRejectsInvalidConnection, 0, 4);
    tcase_add_loop_test(tc_validation, FileConfigurationRollsBackPublishedDataSets, 0, 6);
    tcase_add_loop_test(tc_validation, FileConfigurationRollsBackWriterGroups, 0, 3);
    tcase_add_loop_test(tc_validation, FileConfigurationRollsBackReaders, 0, 4);
    tcase_add_loop_test(tc_validation, FileConfigurationPreservesPublisherAndTransport, 0, 3);
    tcase_add_test(tc_validation, FileConfigurationKeepsActiveConfiguration);
    suite_add_tcase(s, tc_validation);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    TCase *tc_methods = tcase_create("Configuration methods");
    tcase_add_checked_fixture(tc_methods, setupFileValidation, teardownFileValidation);
    tcase_add_loop_test(tc_methods, ConfigurationDeleteMethodClearsAndPermitsReload, 0, 3);
    tcase_add_test(tc_methods, ConfigurationLoadMethodRejectsMalformedFileWithoutMutation);
    suite_add_tcase(s, tc_methods);
#endif

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
