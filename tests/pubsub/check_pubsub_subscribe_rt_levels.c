/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Kalycito Infotech Private Limited (Author: Suriya Narayanan)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/pubsub_udp.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#include <check.h>
#include <stdio.h>

UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, dataSetWriterIdent, dataSetFieldIdent, readerGroupIdentifier, readerIdentifier;

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
    connectionConfig.publisherId.numeric = 2234;
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

static void receiveSingleMessageRT(UA_PubSubConnection *connection, UA_DataSetReader *dataSetReader) {
    UA_ByteString buffer;
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

    /* Decode only the necessary offset and update the networkMessage */
    if(UA_NetworkMessage_updateBufferedNwMessage(&dataSetReader->bufferedMessage, &buffer) != UA_STATUSCODE_GOOD) {
        ck_abort_msg("PubSub receive. Unknown field type!");
    }

    /* Check the decoded message is the expected one */
    if((dataSetReader->bufferedMessage.nm->groupHeader.writerGroupId != dataSetReader->config.writerGroupId) ||
       (*dataSetReader->bufferedMessage.nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != dataSetReader->config.dataSetWriterId)) {
        ck_abort_msg("PubSub receive. Unknown message received. Will not be processed.");
    }

    UA_Server_DataSetReader_process(server, dataSetReader,
                                    dataSetReader->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages);

    /* Delete the payload value of every dsf's decoded */
     UA_DataSetMessage *dsm = dataSetReader->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages;
     if(dsm->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
         for(UA_UInt16 i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
             UA_Variant_deleteMembers(&dsm->data.keyFrameData.dataSetFields[i].value);
         }
     }

    UA_ByteString_clear(&buffer);
}

START_TEST(SubscribeSingleFieldWithFixedOffsets) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
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
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
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

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    /* Data Set Reader */
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask           = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;
    /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
    targetVariable */
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* UInt32 DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                   &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_UINT32;
    pMetaData->fields[0].valueRank = -1; /* scalar */
    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    /* Add Subscribed Variables */
    UA_NodeId folderId;
    UA_NodeId newnodeId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if (folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
      }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    /* Variable to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50002),
                                       folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = 1;
    targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                      UA_calloc(targetVars.targetVariablesSize,
                                      sizeof(UA_FieldTargetDataType));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
    targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[0].targetNodeId = newnodeId;
    retVal = UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                            &targetVars);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_TargetVariablesDataType_deleteMembers(&targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);

    UA_DataSetReader *dataSetReader = UA_ReaderGroup_findDSRbyId(server, readerIdentifier);
    receiveSingleMessageRT(connection, dataSetReader);

   /* Read data received by the Subscriber */
    UA_Variant *subscribedNodeData = UA_Variant_new();
    retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 50002), subscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert((*(UA_Int32 *)subscribedNodeData->data) == 1000);

    UA_Variant_deleteMembers(subscribedNodeData);
    UA_free(subscribedNodeData);
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(SetupInvalidPubSubConfig) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
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
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
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
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = (UA_UInt32) 1000;
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    attributes.value = variant;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,  1000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "variable"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attributes, NULL, NULL);
    dsfConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, 1000);
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    /* Not using static value source */
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    /* Data Set Reader */
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask           = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;
    /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
    targetVariable */
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* DateTime DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId,
                   &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].valueRank = -1; /* scalar */
    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_NodeId readerIdentifier2;
    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    /* Add Subscribed Variables */
    UA_NodeId folderId;
    UA_NodeId newnodeId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if (folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
      }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    /* Variable to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed DateTime");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed DateTime");
    vAttr.dataType    = UA_TYPES[UA_TYPES_DATETIME].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50002),
                                       folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed DateTime"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = 1;
    targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                      UA_calloc(targetVars.targetVariablesSize,
                                      sizeof(UA_FieldTargetDataType));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
    targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[0].targetNodeId = newnodeId;
    retVal = UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                            &targetVars);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_TargetVariablesDataType_deleteMembers(&targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_Variant_deleteMembers(&variant);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTIMPLEMENTED); // Multiple DSR not supported

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    retVal = UA_Server_removeDataSetReader(server, readerIdentifier2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTSUPPORTED); // DateTime not supported
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_BADNOTSUPPORTED); // Static value source only supported
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    TCase *tc_pubsub_subscribe_rt = tcase_create("PubSub RT subscribe with fixed offsets");
    tcase_add_checked_fixture(tc_pubsub_subscribe_rt, setup, teardown);
    tcase_add_test(tc_pubsub_subscribe_rt, SubscribeSingleFieldWithFixedOffsets);
    tcase_add_test(tc_pubsub_subscribe_rt, SetupInvalidPubSubConfig);

    Suite *s = suite_create("PubSub RT configuration levels");
    suite_add_tcase(s, tc_pubsub_subscribe_rt);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
