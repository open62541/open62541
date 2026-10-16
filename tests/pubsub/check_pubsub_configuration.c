/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Siemens AG (Author: Thomas Fischer)
 * Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include "../common.h"

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "pubsub_config_test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

UA_Server *server = NULL;

#define PDS_NAME        "Config2 PDS"
#define SSDS_NAME       "Config2 SSDS"
#define CONNECTION_NAME "Config2 Connection"
#define WG_NAME         "Config2 WriterGroup"
#define DSW_NAME        "Config2 DataSetWriter"
#define RG_NAME         "Config2 ReaderGroup"
#define DSR_NAME        "Config2 DataSetReader"
#define DSR2_NAME       "Config2 DataSetReader SSDS"

#define PUBLISHER_ID    2234
#define WG_ID           100
#define DSW_ID          62541
#define DSW2_ID         62542

static UA_NodeId pubVarId32, pubVarId64, subVarId32, subVarId64, ssdsVarId;

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
    UA_StatusCode retVal =
        UA_PubSubTest_applyConfigFile(server, &publisherConfiguration, false);
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
    retVal = UA_Server_readPubSubConfiguration(
        server, &roundtripConfiguration);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(roundtripConfiguration.length, 0);
    UA_Server_disableAllPubSubComponents(server);
    retVal = UA_PubSubTest_applyConfigFile(server, &roundtripConfiguration, true);
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
    UA_StatusCode retVal =
        UA_PubSubTest_applyConfigFile(server, &subscriberConfiguration, false);
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
    retVal = UA_Server_readPubSubConfiguration(server,
                                                            &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);

    UA_Server_disableAllPubSubComponents(server);
    retVal = UA_PubSubTest_applyConfigFile(server, &savedConfiguration, true);
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
        UA_Server_readPubSubConfiguration(server,
                                                       &savedConfiguration);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(savedConfiguration.length, 0);

    /* The empty file has no elements to reference */
    size_t refsSize = 1;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    retVal = UA_PubSubConfiguration_createReferences(
        &savedConfiguration, UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD,
        &refsSize, &refs);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(refsSize, 0);
    retVal = UA_PubSubTest_applyConfigFile(server, &savedConfiguration, false);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNOTHINGTODO);
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
    res = UA_Server_readPubSubConfiguration(server, &saved);
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
    UA_StatusCode res = UA_PubSubTest_applyConfigFile(server, &input, false);
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
    res = UA_Server_readPubSubConfiguration(server, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_PubSubTest_applyConfigFile(server, &encoded, true);
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
    UA_StatusCode res = UA_PubSubTest_applyConfigFile(server, &input, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *connection = TAILQ_FIRST(&psm->connections);
    ck_assert_ptr_nonnull(connection);
    UA_WriterGroup *group = LIST_FIRST(&connection->writerGroups);
    ck_assert_ptr_nonnull(group);
    connection->config.enabled = false;
    group->config.enabled = true;

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    res = UA_Server_readPubSubConfiguration(server, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_PubSubTest_applyConfigFile(server, &encoded, true);
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
    UA_PubSubConfigurationUpdateResult result;
    ck_assert_uint_eq(UA_Server_updatePubSubConfiguration(NULL, &empty, 0, NULL,
                                                          false, &result),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_updatePubSubConfiguration(server, NULL, 0, NULL,
                                                          false, &result),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_readPubSubConfiguration(NULL,
                                                                     &empty),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_readPubSubConfiguration(server,
                                                                     NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    size_t refsSize = 0;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    const UA_UInt32 add = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD;
    ck_assert_uint_eq(UA_PubSubConfiguration_createReferences(NULL, add,
                                                              &refsSize, &refs),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_PubSubConfiguration_createReferences(&empty, add,
                                                              NULL, &refs),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
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

/* Add the variables used as publisher sources and subscriber targets */
static void
addVariables(UA_Server *srv) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 initVal32 = 42;
    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_StatusCode res =
        UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50001),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Pub UInt32"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &pubVarId32);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_UInt64 initVal64 = 43;
    UA_Variant_setScalar(&vAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50002),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Pub UInt64"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &pubVarId64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50003),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Sub UInt32"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &subVarId32);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50004),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Sub UInt64"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &subVarId64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50005),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "SSDS UInt32"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &ssdsVarId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Add the variables to a second server, keep the NodeIds of the first */
static void
addVariablesKeepIds(UA_Server *srv) {
    UA_NodeId keep32 = pubVarId32, keep64 = pubVarId64,
        keepS32 = subVarId32, keepS64 = subVarId64, keepSsds = ssdsVarId;
    addVariables(srv);
    pubVarId32 = keep32; pubVarId64 = keep64;
    subVarId32 = keepS32; subVarId64 = keepS64; ssdsVarId = keepSsds;
}

static void
fillMetaData2Fields(UA_DataSetMetaDataType *md) {
    UA_DataSetMetaDataType_init(md);
    md->name = UA_STRING(PDS_NAME);
    md->fieldsSize = 2;
    md->fields = (UA_FieldMetaData*)
        UA_Array_new(md->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    UA_FieldMetaData_init(&md->fields[0]);
    md->fields[0].name = UA_STRING("UInt32 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &md->fields[0].dataType);
    md->fields[0].builtInType = UA_NS0ID_UINT32;
    md->fields[0].valueRank = -1;
    UA_FieldMetaData_init(&md->fields[1]);
    md->fields[1].name = UA_STRING("UInt64 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId, &md->fields[1].dataType);
    md->fields[1].builtInType = UA_NS0ID_UINT64;
    md->fields[1].valueRank = -1;
}

/* The export writes UA_*Config.enabled, not the runtime state. Set it for a
 * connection subtree that was enabled at runtime. */
static void
markConfiguredEnabled(UA_PubSubConnection *c) {
    ck_assert_ptr_nonnull(c);
    c->config.enabled = true;
    UA_WriterGroup *wg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        wg->config.enabled = true;
        UA_DataSetWriter *dsw;
        LIST_FOREACH(dsw, &wg->writers, listEntry)
            dsw->config.enabled = true;
    }
    UA_ReaderGroup *rg;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        rg->config.enabled = true;
        UA_DataSetReader *dsr;
        LIST_FOREACH(dsr, &rg->readers, listEntry)
            dsr->config.enabled = true;
    }
}

/* Full configuration via the C API: a PDS (2 fields), an SSDS and a
 * connection with WG + DSW and RG + two DSRs (TargetVariables, SSDS link) */
static void
buildFullConfig(UA_Server *srv, UA_NodeId *connId) {
    addVariables(srv);

    /* PDS with two fields, a folder path and an extension field */
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.name = UA_STRING(PDS_NAME);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_String pdsFolder[2] = {UA_STRING_STATIC("Fixtures"),
                              UA_STRING_STATIC("Config2")};
    pdsConfig.dataSetFolder = pdsFolder;
    pdsConfig.dataSetFolderSize = 2;
    UA_KeyValuePair pdsExtension;
    pdsExtension.key = UA_QUALIFIEDNAME(0, "ExtensionField1");
    UA_UInt32 extensionValue = 7;
    UA_Variant_setScalar(&pdsExtension.value, &extensionValue,
                         &UA_TYPES[UA_TYPES_UINT32]);
    pdsConfig.extensionFields.map = &pdsExtension;
    pdsConfig.extensionFields.mapSize = 1;
    UA_NodeId pdsId;
    UA_AddPublishedDataSetResult pdsRes =
        UA_Server_addPublishedDataSet(srv, &pdsConfig, &pdsId);
    ck_assert_int_eq(pdsRes.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(fieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("UInt32 Field");
    fieldConfig.field.variable.promotedField = false;
    fieldConfig.field.variable.publishParameters.publishedVariable = pubVarId32;
    fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult fieldRes =
        UA_Server_addDataSetField(srv, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    fieldConfig.field.variable.fieldNameAlias = UA_STRING("UInt64 Field");
    fieldConfig.field.variable.publishParameters.publishedVariable = pubVarId64;
    fieldRes = UA_Server_addDataSetField(srv, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4801);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(srv, &connectionConfig, connId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig wgConfig;
    memset(&wgConfig, 0, sizeof(wgConfig));
    wgConfig.name = UA_STRING(WG_NAME);
    wgConfig.writerGroupId = WG_ID;
    wgConfig.publishingInterval = 100.0;
    wgConfig.keepAliveTime = 5000.0;
    wgConfig.priority = 10;
    wgConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgConfig.maxNetworkMessageSize = 1400;
    wgConfig.headerLayoutUri = UA_STRING("http://opcfoundation.org/UA/PubSub-Layouts/UADP-Cyclic-Fixed");
    UA_String wgLocales[1] = {UA_STRING_STATIC("en-US")};
    wgConfig.localeIds = wgLocales;
    wgConfig.localeIdsSize = 1;
    UA_UadpWriterGroupMessageDataType wgMessage;
    UA_UadpWriterGroupMessageDataType_init(&wgMessage);
    wgMessage.networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    UA_ExtensionObject_setValueNoDelete(&wgConfig.messageSettings, &wgMessage,
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_NodeId wgId;
    res = UA_Server_addWriterGroup(srv, *connId, &wgConfig, &wgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* No dataSetName: the export uses the name of the connected PDS */
    UA_DataSetWriterConfig dswConfig;
    memset(&dswConfig, 0, sizeof(dswConfig));
    dswConfig.name = UA_STRING(DSW_NAME);
    dswConfig.dataSetWriterId = DSW_ID;
    dswConfig.keyFrameCount = 10;
    dswConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    UA_NodeId dswId;
    res = UA_Server_addDataSetWriter(srv, wgId, pdsId, &dswConfig, &dswId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_ReaderGroupConfig rgConfig;
    memset(&rgConfig, 0, sizeof(rgConfig));
    rgConfig.name = UA_STRING(RG_NAME);
    rgConfig.maxNetworkMessageSize = 1400;
    UA_NodeId rgId;
    res = UA_Server_addReaderGroup(srv, *connId, &rgConfig, &rgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* DataSetReader with inline TargetVariables */
    UA_DataSetReaderConfig dsrConfig;
    memset(&dsrConfig, 0, sizeof(dsrConfig));
    dsrConfig.name = UA_STRING(DSR_NAME);
    dsrConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsrConfig.publisherId.id.uint16 = PUBLISHER_ID;
    dsrConfig.writerGroupId = WG_ID;
    dsrConfig.dataSetWriterId = DSW_ID;
    dsrConfig.messageReceiveTimeout = 400.0;
    dsrConfig.keyFrameCount = 10;
    dsrConfig.headerLayoutUri = UA_STRING("http://opcfoundation.org/UA/PubSub-Layouts/UADP-Cyclic-Fixed");
    UA_KeyValuePair dsrProperty;
    dsrProperty.key = UA_QUALIFIEDNAME(0, "ReaderProp1");
    UA_UInt32 dsrPropertyValue = 11;
    UA_Variant_setScalar(&dsrProperty.value, &dsrPropertyValue,
                         &UA_TYPES[UA_TYPES_UINT32]);
    dsrConfig.dataSetReaderProperties.map = &dsrProperty;
    dsrConfig.dataSetReaderProperties.mapSize = 1;
    fillMetaData2Fields(&dsrConfig.dataSetMetaData);
    UA_FieldTargetDataType targets[2];
    UA_FieldTargetDataType_init(&targets[0]);
    targets[0].attributeId = UA_ATTRIBUTEID_VALUE;
    targets[0].targetNodeId = subVarId32;
    UA_FieldTargetDataType_init(&targets[1]);
    targets[1].attributeId = UA_ATTRIBUTEID_VALUE;
    targets[1].targetNodeId = subVarId64;
    dsrConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsrConfig.subscribedDataSet.target.targetVariablesSize = 2;
    dsrConfig.subscribedDataSet.target.targetVariables = targets;
    UA_NodeId dsrId;
    res = UA_Server_addDataSetReader(srv, rgId, &dsrConfig, &dsrId);
    /* Shallow free: the fields hold only literals and numeric NodeIds */
    UA_free(dsrConfig.dataSetMetaData.fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_SubscribedDataSetConfig ssdsConfig;
    memset(&ssdsConfig, 0, sizeof(ssdsConfig));
    ssdsConfig.name = UA_STRING(SSDS_NAME);
    ssdsConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    UA_String ssdsFolder[1] = {UA_STRING_STATIC("Config2")};
    ssdsConfig.dataSetFolder = ssdsFolder;
    ssdsConfig.dataSetFolderSize = 1;
    UA_DataSetMetaDataType *md = &ssdsConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init(md);
    md->name = UA_STRING(SSDS_NAME);
    md->fieldsSize = 1;
    md->fields = (UA_FieldMetaData*)
        UA_Array_new(md->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    UA_FieldMetaData_init(&md->fields[0]);
    md->fields[0].name = UA_STRING("SSDS UInt32 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &md->fields[0].dataType);
    md->fields[0].builtInType = UA_NS0ID_UINT32;
    md->fields[0].valueRank = -1;
    UA_FieldTargetDataType ssdsTarget;
    UA_FieldTargetDataType_init(&ssdsTarget);
    ssdsTarget.attributeId = UA_ATTRIBUTEID_VALUE;
    ssdsTarget.targetNodeId = ssdsVarId;
    ssdsConfig.subscribedDataSet.target.targetVariablesSize = 1;
    ssdsConfig.subscribedDataSet.target.targetVariables = &ssdsTarget;
    UA_NodeId ssdsId;
    res = UA_Server_addSubscribedDataSet(srv, &ssdsConfig, &ssdsId);
    UA_free(md->fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Second DataSetReader linked to the SSDS by name */
    UA_DataSetReaderConfig dsr2Config;
    memset(&dsr2Config, 0, sizeof(dsr2Config));
    dsr2Config.name = UA_STRING(DSR2_NAME);
    dsr2Config.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsr2Config.publisherId.id.uint16 = PUBLISHER_ID;
    dsr2Config.writerGroupId = WG_ID;
    dsr2Config.dataSetWriterId = DSW2_ID;
    fillMetaData2Fields(&dsr2Config.dataSetMetaData);
    dsr2Config.dataSetMetaData.fieldsSize = 1; /* match the SSDS */
    dsr2Config.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsr2Config.linkedStandaloneSubscribedDataSetName = UA_STRING(SSDS_NAME);
    UA_NodeId dsr2Id;
    res = UA_Server_addDataSetReader(srv, rgId, &dsr2Config, &dsr2Id);
    UA_free(dsr2Config.dataSetMetaData.fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Compare two configurations. The ConfigurationVersion always differs. */
static void
compareConfig2(const UA_PubSubConfiguration2DataType *a,
               const UA_PubSubConfiguration2DataType *b) {
    UA_PubSubConfiguration2DataType ca = *a, cb = *b;
    ca.configurationVersion = 0;
    cb.configurationVersion = 0;
    ck_assert(UA_order(&ca, &cb, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]) ==
              UA_ORDER_EQ);
}

/* Extract the Config2 body from an encoded configuration file */
static void
decodeConfig2File(const UA_ByteString *buffer,
                  UA_PubSubConfiguration2DataType *config) {
    UA_StatusCode res = UA_PubSubTest_decodeConfigFile(buffer, config);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Snapshot of the running config via UA_Server_readPubSubConfiguration */
START_TEST(ReadPubSubConfiguration) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_PubSubConfiguration2DataType cfg;
    UA_StatusCode res = UA_PubSubTest_readConfig(server, &cfg);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(cfg.connectionsSize, 1);
    ck_assert_uint_eq(cfg.publishedDataSetsSize, 1);
    ck_assert_uint_eq(cfg.subscribedDataSetsSize, 1);

    UA_String tmp = UA_STRING(CONNECTION_NAME);
    ck_assert(UA_String_equal(&cfg.connections[0].name, &tmp));
    ck_assert_uint_eq(cfg.connections[0].writerGroupsSize, 1);
    ck_assert_uint_eq(cfg.connections[0].readerGroupsSize, 1);
    ck_assert_uint_eq(cfg.connections[0].writerGroups[0].dataSetWritersSize, 1);
    ck_assert_uint_eq(cfg.connections[0].readerGroups[0].dataSetReadersSize, 2);

    /* Top-level enabled (PubSubManager runs), components start disabled */
    ck_assert(cfg.enabled);
    ck_assert(!cfg.connections[0].enabled);
    ck_assert(!cfg.connections[0].writerGroups[0].enabled);

    /* The DSW dataSetName is taken from the connected PDS */
    tmp = UA_STRING(PDS_NAME);
    UA_DataSetWriterDataType *dsw =
        &cfg.connections[0].writerGroups[0].dataSetWriters[0];
    ck_assert(UA_String_equal(&dsw->dataSetName, &tmp));

    UA_PublishedDataSetDataType *pds = &cfg.publishedDataSets[0];
    ck_assert_int_eq((int)pds->dataSetSource.encoding,
                     (int)UA_EXTENSIONOBJECT_DECODED);
    ck_assert(pds->dataSetSource.content.decoded.type ==
              &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
    UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
        pds->dataSetSource.content.decoded.data;
    ck_assert_uint_eq(pdi->publishedDataSize, 2);
    ck_assert(UA_NodeId_equal(&pdi->publishedData[0].publishedVariable,
                              &pubVarId32));
    ck_assert(UA_NodeId_equal(&pdi->publishedData[1].publishedVariable,
                              &pubVarId64));

    /* The second DSR references the SSDS by name */
    UA_DataSetReaderDataType *dsr2 =
        &cfg.connections[0].readerGroups[0].dataSetReaders[1];
    ck_assert(dsr2->subscribedDataSet.encoding == UA_EXTENSIONOBJECT_DECODED);
    ck_assert(dsr2->subscribedDataSet.content.decoded.type ==
              &UA_TYPES[UA_TYPES_STANDALONESUBSCRIBEDDATASETREFDATATYPE]);
    UA_StandaloneSubscribedDataSetRefDataType *ref =
        (UA_StandaloneSubscribedDataSetRefDataType*)
        dsr2->subscribedDataSet.content.decoded.data;
    tmp = UA_STRING(SSDS_NAME);
    ck_assert(UA_String_equal(&ref->dataSetName, &tmp));

    UA_PubSubConfiguration2DataType_clear(&cfg);
} END_TEST

/* Export server A, load on server B, compare the configurations and states */
START_TEST(ExportImportRoundTrip) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *conn;
    TAILQ_FOREACH(conn, &getPSM(server)->connections, listEntry)
        markConfiguredEnabled(conn);

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_readPubSubConfiguration(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(exportA.length > 0);

    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    /* The target/published variables must exist on B as well */
    addVariablesKeepIds(serverB);

    res = UA_PubSubTest_applyConfigFile(serverB, &exportA, false);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubManager *psmB = getPSM(serverB);
    ck_assert_uint_eq(psmB->connectionsSize, 1);
    ck_assert_uint_eq(psmB->publishedDataSetsSize, 1);
    ck_assert_uint_eq(psmB->subscribedDataSetsSize, 1);

    UA_PubSubConfiguration2DataType cfgA, cfgB;
    res = UA_PubSubTest_readConfig(server, &cfgA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    res = UA_PubSubTest_readConfig(serverB, &cfgB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    compareConfig2(&cfgA, &cfgB);

    /* Enabled, not Operational: the readers wait for the first message */
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psmB->connections, listEntry) {
        ck_assert(UA_PubSubState_isEnabled(c->head.state));
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            ck_assert(UA_PubSubState_isEnabled(wg->head.state));
        }
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            ck_assert(UA_PubSubState_isEnabled(rg->head.state));
        }
    }

    /* The round trip is lossless */
    UA_ByteString exportB = UA_BYTESTRING_NULL;
    res = UA_Server_readPubSubConfiguration(serverB, &exportB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubConfiguration2DataType fileA, fileB;
    decodeConfig2File(&exportA, &fileA);
    decodeConfig2File(&exportB, &fileB);
    compareConfig2(&fileA, &fileB);

    UA_PubSubConfiguration2DataType_clear(&fileA);
    UA_PubSubConfiguration2DataType_clear(&fileB);
    UA_PubSubConfiguration2DataType_clear(&cfgA);
    UA_PubSubConfiguration2DataType_clear(&cfgB);
    UA_ByteString_clear(&exportA);
    UA_ByteString_clear(&exportB);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* A disabled connection in the file stays disabled, its sibling is enabled */
START_TEST(MixedEnabledFlags) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("Disabled Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID + 1;
    UA_NodeId conn2Id;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(server, &connectionConfig, &conn2Id);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_enablePubSubConnection(server, connId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    markConfiguredEnabled(UA_PubSubConnection_find(getPSM(server), connId));

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_readPubSubConfiguration(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    addVariablesKeepIds(serverB);

    res = UA_PubSubTest_applyConfigFile(serverB, &exportA, false);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psmB = getPSM(serverB);
    UA_String nameEnabled = UA_STRING(CONNECTION_NAME);
    UA_String nameDisabled = UA_STRING("Disabled Connection");
    size_t found = 0;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psmB->connections, listEntry) {
        if(UA_String_equal(&c->config.name, &nameEnabled)) {
            ck_assert(UA_PubSubState_isEnabled(c->head.state));
            found++;
        } else if(UA_String_equal(&c->config.name, &nameDisabled)) {
            ck_assert_int_eq((int)c->head.state, (int)UA_PUBSUBSTATE_DISABLED);
            found++;
        }
    }
    ck_assert_uint_eq(found, 2);

    UA_ByteString_clear(&exportA);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* Namespace indices are remapped on load, unknown namespaces are added */
START_TEST(NamespaceRemapOnLoad) {
    UA_UInt16 nsA = UA_Server_addNamespace(server, "http://config2.test/nsA");

    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 initVal32 = 42;
    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_NodeId nsVarId;
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(nsA, 60001),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(nsA, "NS Var"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &nsVarId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.name = UA_STRING("NS PDS");
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_NodeId pdsId;
    UA_AddPublishedDataSetResult pdsRes =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId);
    ck_assert_int_eq(pdsRes.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(fieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("NS Field");
    fieldConfig.field.variable.publishParameters.publishedVariable = nsVarId;
    fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult fieldRes =
        UA_Server_addDataSetField(server, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_readPubSubConfiguration(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Another namespace on B moves nsA to a different index */
    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    UA_Server_addNamespace(serverB, "http://config2.test/other");
    addVariablesKeepIds(serverB);

    /* The published variable must exist on B. The loader must map to this
     * namespace entry instead of adding a duplicate. */
    UA_UInt16 nsB = UA_Server_addNamespace(serverB, "http://config2.test/nsA");
    ck_assert_uint_ne(nsB, nsA);
    UA_NodeId nsVarIdB;
    res = UA_Server_addVariableNode(serverB, UA_NODEID_NUMERIC(nsB, 60001),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(nsB, "NS Var"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &nsVarIdB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    res = UA_PubSubTest_applyConfigFile(serverB, &exportA, false);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    size_t nsBIndex = 0;
    res = getNamespaceByName(serverB, UA_STRING("http://config2.test/nsA"),
                             &nsBIndex);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(nsBIndex, nsA); /* different index than on server A */

    /* The published variable NodeId was remapped to the new index */
    UA_PubSubConfiguration2DataType cfgB;
    res = UA_PubSubTest_readConfig(serverB, &cfgB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_String pdsName = UA_STRING("NS PDS");
    UA_Boolean foundPds = false;
    for(size_t i = 0; i < cfgB.publishedDataSetsSize; i++) {
        UA_PublishedDataSetDataType *pds = &cfgB.publishedDataSets[i];
        if(!UA_String_equal(&pds->name, &pdsName))
            continue;
        foundPds = true;
        UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
            pds->dataSetSource.content.decoded.data;
        ck_assert_uint_eq(pdi->publishedDataSize, 1);
        ck_assert_uint_eq(pdi->publishedData[0].publishedVariable.namespaceIndex,
                          (UA_UInt16)nsBIndex);
    }
    ck_assert(foundPds);

    UA_PubSubConfiguration2DataType_clear(&cfgB);
    UA_ByteString_clear(&exportA);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* Malformed files are rejected by the update and by createReferences */
static void
assertInvalidFile(const UA_ByteString *file) {
    UA_PubSubConfigurationRefDataType ref;
    UA_PubSubConfigurationRefDataType_init(&ref);
    ref.configurationMask = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION;
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res =
        UA_Server_updatePubSubConfiguration(server, file, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_BADTYPEMISMATCH);
    size_t refsSize = 1;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    res = UA_PubSubConfiguration_createReferences(
        file, UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD, &refsSize, &refs);
    ck_assert_int_eq(res, UA_STATUSCODE_BADTYPEMISMATCH);
    ck_assert_uint_eq(refsSize, 0);
}

START_TEST(InvalidFileBody) {
    UA_Byte malformedData[] = {0xff, 0xff, 0xff, 0xff};
    UA_ByteString malformed = {sizeof(malformedData), malformedData};
    assertInvalidFile(&malformed);
    UA_ByteString garbage = UA_BYTESTRING("this is not a pubsub config");
    assertInvalidFile(&garbage);

    /* Wrong body types, including the legacy PubSubConfigurationDataType */
    UA_UABinaryFileDataType binFile;
    UA_UABinaryFileDataType_init(&binFile);
    UA_Int32 wrongBody = 42;
    UA_Variant_setScalar(&binFile.body, &wrongBody, &UA_TYPES[UA_TYPES_INT32]);
    UA_ExtensionObject eo;
    UA_ExtensionObject_setValueNoDelete(&eo, &binFile,
                                        &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res =
        UA_encodeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &buf, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    assertInvalidFile(&buf);
    UA_ByteString_clear(&buf);

    UA_PubSubConfigurationDataType legacy;
    UA_PubSubConfigurationDataType_init(&legacy);
    UA_Variant_setScalar(&binFile.body, &legacy,
                         &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);
    res = UA_encodeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &buf, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    assertInvalidFile(&buf);
    UA_ByteString_clear(&buf);

    ck_assert_uint_eq(getPSM(server)->connectionsSize, 0);
} END_TEST

static void
exportConfig(UA_Server *srv, UA_ByteString *file) {
    UA_ByteString_init(file);
    UA_StatusCode res = UA_Server_readPubSubConfiguration(srv, file);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

static void
checkRef(const UA_PubSubConfigurationRefDataType *ref, UA_UInt32 mask,
         UA_UInt16 elementIndex, UA_UInt16 connectionIndex, UA_UInt16 groupIndex) {
    ck_assert_uint_eq(ref->configurationMask, mask);
    ck_assert_uint_eq(ref->elementIndex, elementIndex);
    ck_assert_uint_eq(ref->connectionIndex, connectionIndex);
    ck_assert_uint_eq(ref->groupIndex, groupIndex);
}

/* createReferences returns a reference per file element, in file order */
START_TEST(CreateReferencesAllElements) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);
    UA_ByteString file;
    exportConfig(server, &file);

    size_t refsSize = 0;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    UA_StatusCode res = UA_PubSubConfiguration_createReferences(
        &file, UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD, &refsSize, &refs);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(refsSize, 8);
    const UA_UInt32 add = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD;
    checkRef(&refs[0], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
    checkRef(&refs[1], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET, 0, 0, 0);
    checkRef(&refs[2], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    checkRef(&refs[3], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    checkRef(&refs[4], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    checkRef(&refs[5], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);
    checkRef(&refs[6], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, 0, 0, 0);
    checkRef(&refs[7], add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, 1, 0, 0);
    UA_Array_delete(refs, refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
    UA_ByteString_clear(&file);
} END_TEST

/* Match applies to connections and groups only. Masks outside the operations
 * of CloseAndUpdate are rejected. */
START_TEST(CreateReferencesMaskRules) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);
    UA_ByteString file;
    exportConfig(server, &file);

    const UA_UInt32 match = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH;
    size_t refsSize = 0;
    UA_PubSubConfigurationRefDataType *refs = NULL;
    UA_StatusCode res =
        UA_PubSubConfiguration_createReferences(&file, match, &refsSize, &refs);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(refsSize, 3);
    checkRef(&refs[0], match | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    checkRef(&refs[1], match | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    checkRef(&refs[2], match | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);
    UA_Array_delete(refs, refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);

    const UA_UInt32 addMatch = match | UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD;
    res = UA_PubSubConfiguration_createReferences(&file, addMatch, &refsSize, &refs);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(refsSize, 8);
    ck_assert_uint_eq(refs[0].configurationMask,
                      UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET);
    ck_assert_uint_eq(refs[2].configurationMask,
                      addMatch | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION);
    ck_assert_uint_eq(refs[4].configurationMask,
                      UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER);
    UA_Array_delete(refs, refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);

    UA_UInt32 invalid[3] = {
        0,
        UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE,
        UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION};
    for(size_t i = 0; i < 3; i++) {
        res = UA_PubSubConfiguration_createReferences(&file, invalid[i],
                                                      &refsSize, &refs);
        ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
        ck_assert_uint_eq(refsSize, 0);
        ck_assert_ptr_null(refs);
    }
    UA_ByteString_clear(&file);
} END_TEST

/* The Remove and Add references of a file in one complete update replace the
 * elements. When the update fails, the prior configuration is restored. */
START_TEST(ReplaceWithRemoveAndAdd) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);
    UA_PubSubManager *psm = getPSM(server);
    UA_ByteString file;
    exportConfig(server, &file);
    UA_PubSubConfiguration2DataType before;
    decodeConfig2File(&file, &before);

    UA_StatusCode res = UA_PubSubTest_applyConfigFile(server, &file, true);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 1);

    UA_ByteString after;
    exportConfig(server, &after);
    UA_PubSubConfiguration2DataType afterCfg;
    decodeConfig2File(&after, &afterCfg);
    compareConfig2(&before, &afterCfg);
    UA_PubSubConfiguration2DataType_clear(&afterCfg);
    UA_ByteString_clear(&after);

    /* A fixed NetworkMessageNumber fails the writer: the update rolls back */
    UA_PubSubConfiguration2DataType bad;
    decodeConfig2File(&file, &bad);
    UA_DataSetWriterDataType *dsw =
        &bad.connections[0].writerGroups[0].dataSetWriters[0];
    UA_ExtensionObject_clear(&dsw->messageSettings);
    UA_UadpDataSetWriterMessageDataType *uadp =
        UA_UadpDataSetWriterMessageDataType_new();
    uadp->networkMessageNumber = 1;
    UA_ExtensionObject_setValue(&dsw->messageSettings, uadp,
                                &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]);
    UA_ByteString badFile;
    res = UA_PubSubTest_encodeConfigFile(&bad, &badFile);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_UInt32 version = psm->configurationVersion;
    res = UA_PubSubTest_applyConfigFile(server, &badFile, true);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(psm->configurationVersion, version);

    exportConfig(server, &after);
    decodeConfig2File(&after, &afterCfg);
    compareConfig2(&before, &afterCfg);

    UA_PubSubConfiguration2DataType_clear(&afterCfg);
    UA_PubSubConfiguration2DataType_clear(&before);
    UA_PubSubConfiguration2DataType_clear(&bad);
    UA_ByteString_clear(&after);
    UA_ByteString_clear(&badFile);
    UA_ByteString_clear(&file);
} END_TEST

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

    TCase *tc_config2 = tcase_create("PubSubConfiguration2");
    tcase_add_checked_fixture(tc_config2, setup, teardown);
    tcase_add_test(tc_config2, ReadPubSubConfiguration);
    tcase_add_test(tc_config2, ExportImportRoundTrip);
    tcase_add_test(tc_config2, MixedEnabledFlags);
    tcase_add_test(tc_config2, NamespaceRemapOnLoad);
    tcase_add_test(tc_config2, InvalidFileBody);
    tcase_add_test(tc_config2, CreateReferencesAllElements);
    tcase_add_test(tc_config2, CreateReferencesMaskRules);
    tcase_add_test(tc_config2, ReplaceWithRemoveAndAdd);

    Suite *s = suite_create("PubSub file configuration");
    suite_add_tcase(s, tc_pubsub_file_configuration);
    suite_add_tcase(s, tc_config2);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
