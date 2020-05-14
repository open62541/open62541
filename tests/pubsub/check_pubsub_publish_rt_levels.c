/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/pubsub_udp.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#include <check.h>
#include <stdio.h>

UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, dataSetWriterIdent, dataSetFieldIdent;

static UA_StatusCode
addMinimalPubSubConfiguration(void){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    /* Add one PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;
    /* Add one PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Add one DataSetField to the PDS */
    UA_AddPublishedDataSetResult addResult = UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
    return addResult.addResult;
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
            UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void receiveSingleMessage(UA_ByteString buffer, UA_PubSubConnection *connection, UA_NetworkMessage *networkMessage) {
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        ck_abort_msg("Message buffer allocation failed!");
    }
    UA_StatusCode retval =
            connection->channel->receive(connection->channel, &buffer, NULL, 1000000);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        buffer.length = 512;
        UA_ByteString_clear(&buffer);
        ck_abort_msg("Expected message not received!");
    }
    memset(networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, networkMessage);
    UA_ByteString_clear(&buffer);
}

START_TEST(PublishSingleFieldWithStaticValueSource) {
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        ck_assert(rv == UA_STATUSCODE_GOOD);
    }
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    /* Create Variant and configure as DataSetField source */
    UA_UInt32 intValue = 1000;
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, &intValue, &UA_TYPES[UA_TYPES_UINT32]);
    UA_DataValue staticValueSource;
    memset(&staticValueSource, 0, sizeof(staticValueSource));
    staticValueSource.value = variant;
    dsfConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.staticValueSource.value = variant;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_ByteString buffer;
    UA_ByteString_init(&buffer);
    UA_NetworkMessage networkMessage;
    receiveSingleMessage(buffer, connection, &networkMessage);
    ck_assert((*((UA_UInt32 *)networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data)) == 1000);
    UA_NetworkMessage_deleteMembers(&networkMessage);
    } END_TEST

START_TEST(PublishSingleFieldWithDifferentBinarySizes) {
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        ck_assert(rv == UA_STATUSCODE_GOOD);
    }
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Test WriterGroup");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    /* Create Variant and configure as DataSetField source */
    UA_String stringValue = UA_STRING_ALLOC("12345");
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_DataValue staticValueSource;
    memset(&staticValueSource, 0, sizeof(staticValueSource));
    staticValueSource.value = variant;
    dsfConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.staticValueSource.value = variant;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_ByteString buffer;
    UA_ByteString_init(&buffer);
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(networkMessage));
    receiveSingleMessage(buffer, connection, &networkMessage);
    UA_String compareString = UA_STRING("12345");
    ck_assert(UA_String_equal(((UA_String *) networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data), &compareString) == UA_TRUE);
    UA_NetworkMessage_deleteMembers(&networkMessage);
    compareString = UA_STRING("123456789");
    stringValue.data = (UA_Byte *) UA_realloc(stringValue.data, 9);
    stringValue.length = 9;
    memcpy(stringValue.data, "123456789", 9);
    UA_ByteString_init(&buffer);
    memset(&networkMessage, 0, sizeof(networkMessage));
    ck_assert(UA_Server_setWriterGroupDisabled(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    receiveSingleMessage(buffer, connection, &networkMessage);
    ck_assert(UA_String_equal(((UA_String *) networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data), &compareString) == UA_TRUE);
    UA_NetworkMessage_deleteMembers(&networkMessage);
    UA_String_deleteMembers(&stringValue);
} END_TEST

START_TEST(SetupInvalidPubSubConfigWithStaticValueSource) {
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        ck_assert(rv == UA_STATUSCODE_GOOD);
    }
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Test WriterGroup");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;

    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent,
                              &dataSetFieldConfig, &dataSetFieldIdent);
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_BADCONFIGURATIONERROR);
} END_TEST

START_TEST(PublishSingleFieldWithFixedOffsets) {
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        ck_assert(rv == UA_STATUSCODE_GOOD);
    }
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    // Create Variant and configure as DataSetField source
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = (UA_UInt32) 1000;
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    UA_DataValue staticValueSource;
    memset(&staticValueSource, 0, sizeof(staticValueSource));
    staticValueSource.value = variant;
    dsfConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.staticValueSource.value = variant;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_ByteString buffer;
    UA_ByteString_init(&buffer);
    UA_NetworkMessage networkMessage;
    receiveSingleMessage(buffer, connection, &networkMessage);
    ck_assert((*((UA_UInt32 *)networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data)) == 1000);
    UA_NetworkMessage_deleteMembers(&networkMessage);
} END_TEST

START_TEST(PublishPDSWithMultipleFieldsAndFixedOffset) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
        if(connection != NULL) {
            UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
            ck_assert(rv == UA_STATUSCODE_GOOD);
        }
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = 10;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        // Create Variant and configure as DataSetField source
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = (UA_UInt32) 1000;
        UA_Variant variant;
        memset(&variant, 0, sizeof(UA_Variant));
        UA_Variant_setScalar(&variant, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        UA_DataValue staticValueSource;
        memset(&staticValueSource, 0, sizeof(staticValueSource));
        staticValueSource.value = variant;
        dsfConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.staticValueSource.value = variant;
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
        UA_UInt32 *intValue2 = UA_UInt32_new();
        *intValue2 = (UA_UInt32) 2000;
        UA_Variant variant2;
        memset(&variant2, 0, sizeof(UA_Variant));
        UA_Variant_setScalar(&variant2, intValue2, &UA_TYPES[UA_TYPES_UINT32]);
        UA_DataValue staticValueSource2;
        memset(&staticValueSource2, 0, sizeof(staticValueSource));
        staticValueSource2.value = variant2;
        dsfConfig.field.variable.staticValueSource.value = variant2;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_ByteString buffer;
        UA_ByteString_init(&buffer);
        UA_NetworkMessage networkMessage;
        receiveSingleMessage(buffer, connection, &networkMessage);
        ck_assert((*((UA_UInt32 *)networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data)) == 1000);
        ck_assert(*((UA_UInt32 *) networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields[1].value.data) == 2000);
        UA_NetworkMessage_deleteMembers(&networkMessage);
        *intValue = (UA_UInt32) 1001;
        *intValue2 = (UA_UInt32) 2001;
        UA_ByteString_init(&buffer);
        memset(&networkMessage, 0, sizeof(networkMessage));
        ck_assert(UA_Server_setWriterGroupDisabled(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        receiveSingleMessage(buffer, connection, &networkMessage);
        ck_assert((*((UA_UInt32 *)networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields->value.data)) == 1001);
        ck_assert(*((UA_UInt32 *) networkMessage.payload.dataSetPayload.dataSetMessages->data.keyFrameData.dataSetFields[1].value.data) == 2001);
        UA_NetworkMessage_deleteMembers(&networkMessage);
} END_TEST

int main(void) {
    TCase *tc_pubsub_rt_static_value_source = tcase_create("PubSub RT publish with static value sources");
    tcase_add_checked_fixture(tc_pubsub_rt_static_value_source, setup, teardown);
    tcase_add_test(tc_pubsub_rt_static_value_source, PublishSingleFieldWithStaticValueSource);
    tcase_add_test(tc_pubsub_rt_static_value_source, PublishSingleFieldWithDifferentBinarySizes);
    tcase_add_test(tc_pubsub_rt_static_value_source, SetupInvalidPubSubConfigWithStaticValueSource);


    TCase *tc_pubsub_rt_fixed_offsets = tcase_create("PubSub RT publish with fixed offsets");
    tcase_add_checked_fixture(tc_pubsub_rt_fixed_offsets, setup, teardown);
    tcase_add_test(tc_pubsub_rt_fixed_offsets, PublishSingleFieldWithFixedOffsets);
    tcase_add_test(tc_pubsub_rt_fixed_offsets, PublishPDSWithMultipleFieldsAndFixedOffset);

    Suite *s = suite_create("PubSub RT configuration levels");
    suite_add_tcase(s, tc_pubsub_rt_static_value_source);
    suite_add_tcase(s, tc_pubsub_rt_fixed_offsets);


    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
