/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;
UA_NodeId connection1, publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, dataSetWriterIdent;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
            UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;

    UA_Server_run_startup(server);
    //add connection
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.numeric = 62541;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Test PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);

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
            connection->channel->receive(connection->channel, &buffer, NULL, 10000);
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

START_TEST(CheckNMandDSMcalculation){
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;

    //maximum DSM in one NM = 10
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 10;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(wgm);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 10;
    dataSetWriterConfig.keyFrameCount = 1;
    //add 10 dataSetWriter
    for(UA_UInt16 i = 0; i < 10; i++){
        dataSetWriterConfig.dataSetWriterId = (UA_UInt16) (dataSetWriterConfig.dataSetWriterId + 1);
        UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                                   &dataSetWriterConfig, &dataSetWriterIdent);
    }

    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connection1);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        ck_assert(rv == UA_STATUSCODE_GOOD);
    }

    //change publish interval triggers implicit one publish callback run | alternatively run UA_Server_iterate
    writerGroupConfig.publishingInterval = 100000;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);

    UA_ByteString buffer = UA_BYTESTRING_ALLOC("");
    UA_NetworkMessage networkMessage;
    receiveSingleMessage(buffer, connection, &networkMessage);
    //ck_assert_int_eq(networkMessage.publisherId.publisherIdUInt32 , 62541);
    ck_assert_int_eq(networkMessage.payloadHeader.dataSetPayloadHeader.count, 10);
    for(size_t i = 10; i > 0; i--){
        ck_assert_int_eq(*(networkMessage.payloadHeader.dataSetPayloadHeader.dataSetWriterIds+(i-1)), 21-i);
    }
    UA_NetworkMessage_clear(&networkMessage);

    //change publish interval triggers implicit one publish callback run | alternatively run UA_Server_iterate
    writerGroupConfig.publishingInterval = 200000;
    //maximum DSM in one NM = 5
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 5;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    UA_NetworkMessage networkMessage1, networkMessage2;
    receiveSingleMessage(buffer, connection, &networkMessage1);
    receiveSingleMessage(buffer, connection, &networkMessage2);
    ck_assert_int_eq(networkMessage1.payloadHeader.dataSetPayloadHeader.count, 5);
    ck_assert_int_eq(networkMessage1.payloadHeader.dataSetPayloadHeader.count, 5);
    UA_NetworkMessage_clear(&networkMessage1);
    UA_NetworkMessage_clear(&networkMessage2);

    //change publish interval triggers implicit one publish callback run | alternatively run UA_Server_iterate
    writerGroupConfig.publishingInterval = 300000;
    //maximum DSM in one NM = 20
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 20;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    UA_NetworkMessage networkMessage3;
    receiveSingleMessage(buffer, connection, &networkMessage3);
    ck_assert_int_eq(networkMessage3.payloadHeader.dataSetPayloadHeader.count, 10);
    UA_NetworkMessage_clear(&networkMessage3);

    //change publish interval triggers implicit one publish callback run | alternatively run UA_Server_iterate
    writerGroupConfig.publishingInterval = 400000;
    //maximum DSM in one NM = 1
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 1;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    UA_NetworkMessage messageArray[10];
    for (int j = 0; j < 10; ++j) {
        receiveSingleMessage(buffer, connection, &(messageArray[j]));
        ck_assert_int_eq(messageArray[j].payloadHeader.dataSetPayloadHeader.count, 1);
        UA_NetworkMessage_clear(&messageArray[j]);
    }

    //change publish interval triggers implicit one publish callback run | alternatively run UA_Server_iterate
    writerGroupConfig.publishingInterval = 500000;
    //maximum DSM in one NM = 0 -> should be equal to 1
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 0;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    for (int j = 0; j < 10; ++j) {
        receiveSingleMessage(buffer, connection, &(messageArray[j]));
        ck_assert_int_eq(messageArray[j].payloadHeader.dataSetPayloadHeader.count, 1);
        UA_NetworkMessage_clear(&messageArray[j]);
    }

    } END_TEST

START_TEST(CheckNMandDSMBufferCalculation){
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = 10;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;

        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;

        //maximum DSM in one NM = 10
        writerGroupConfig.maxEncapsulatedDataSetMessageCount = 10;
        UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroupIdent);
        UA_Server_setWriterGroupOperational(server, writerGroupIdent);
        UA_UadpWriterGroupMessageDataType_delete(wgm);

        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 10;
        dataSetWriterConfig.keyFrameCount = 1;
        //add 10 dataSetWriter
        for(UA_UInt16 i = 0; i < 10; i++){
            dataSetWriterConfig.dataSetWriterId = (UA_UInt16) (dataSetWriterConfig.dataSetWriterId + 1);
            UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                                       &dataSetWriterConfig, &dataSetWriterIdent);
        }

        UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent);

    } END_TEST

int main(void) {
    TCase *tc_add_pubsub_DSMandNMcalculation = tcase_create("PubSub NM and DSM");
    tcase_add_checked_fixture(tc_add_pubsub_DSMandNMcalculation, setup, teardown);
    tcase_add_test(tc_add_pubsub_DSMandNMcalculation, CheckNMandDSMcalculation);
    tcase_add_test(tc_add_pubsub_DSMandNMcalculation, CheckNMandDSMBufferCalculation);

    Suite *s = suite_create("PubSub NM and DSM calculation");
    suite_add_tcase(s, tc_add_pubsub_DSMandNMcalculation);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
