/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Kalycito Infotech Private Limited
 */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include "testing_clock.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>

UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, writerGroupIdent1, dataSetWriterIdent, dataSetWriterIdent1, dataSetFieldIdent, readerGroupIdentifier, readerIdentifier;

UA_UInt32    *subValue;
UA_DataValue *subDataValueRT;
UA_NodeId    subNodeId;

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
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
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
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* If the external data source is written over the information model, the
 * externalDataWriteCallback will be triggered. The user has to take care and assure
 * that the write leads not to synchronization issues and race conditions. */
static UA_StatusCode
externalDataWriteCallback(UA_Server *serverLocal, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, const UA_NumericRange *range,
                          const UA_DataValue *data){
    if(UA_NodeId_equal(nodeId, &subNodeId)){
        memcpy(subValue, data->value.data, sizeof(UA_UInt32));
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
externalDataReadNotificationCallback(UA_Server *serverLocal, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeid,
                                     void *nodeContext, const UA_NumericRange *range){
    //allow read without any preparation
    return UA_STATUSCODE_GOOD;
}

START_TEST(SubscribeMultipleMessagesRT) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    ck_assert(connection);
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


    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup2");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent1) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);

    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    // Create Variant and configure as DataSetField source
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = 1000;
    UA_DataValue *dataValue = UA_DataValue_new();
    UA_Variant_setScalar(&dataValue->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.rtValueSource.staticValueSource = &dataValue;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

    /* add data set writers */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);

    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 2");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent1, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent1) == UA_STATUSCODE_GOOD);
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
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
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

    /* Add Subscribed Variables */
    UA_NodeId folderId;
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

    retVal = UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    /* Variable to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50002),
                                       folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &subNodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    subValue = UA_UInt32_new();
    *subValue = 0;
    subDataValueRT = UA_DataValue_new();
    UA_Variant_setScalar(&subDataValueRT->value, subValue, &UA_TYPES[UA_TYPES_UINT32]);
    subDataValueRT->hasValue = true;
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &subDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, subNodeId, valueBackend);

    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = subNodeId;

    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable);

    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableReaderGroup(server, readerGroupIdentifier));

    while(true) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 50002), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_Boolean eq = ((*(UA_Int32 *)subscribedNodeData->data) == 1000);
        UA_Variant_delete(subscribedNodeData);
        if(eq)
            break;
    }

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);
    UA_DataValue_delete(dataValue);
    UA_free(subValue);
    UA_free(subDataValueRT);
} END_TEST

START_TEST(SubscribeMultipleMessagesWithoutRT) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    ck_assert(connection);
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

    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup2");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgm = UA_UadpWriterGroupMessageDataType_new();
    wgm->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                      (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = wgm;
    writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent1) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(wgm);

    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    // Create Variant and configure as DataSetField source
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = 1000;
    UA_DataValue *dataValue = UA_DataValue_new();
    UA_Variant_setScalar(&dataValue->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.rtValueSource.staticValueSource = &dataValue;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

    /* add data set writers */
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 2");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent1, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent1) == UA_STATUSCODE_GOOD);

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
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
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

    retVal = UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = newnodeId;

    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    UA_FieldTargetDataType_clear(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);

    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent1) == UA_STATUSCODE_GOOD);
    UA_DataValue_delete(dataValue);
} END_TEST

START_TEST(SetupInvalidPubSubConfig) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
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
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,  1000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "variable"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attributes, NULL, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
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

    retVal = UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables     = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = newnodeId;

    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_NodeId readerIdentifier2;
    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);

    UA_FieldTargetDataType_clear(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_Variant_clear(&variant);

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
    TCase *tc_pubsub_subscribe_rt = tcase_create("PubSub RT subscribe receive multiple messages");
    tcase_add_checked_fixture(tc_pubsub_subscribe_rt, setup, teardown);
    tcase_add_test(tc_pubsub_subscribe_rt, SubscribeMultipleMessagesRT);
    tcase_add_test(tc_pubsub_subscribe_rt, SubscribeMultipleMessagesWithoutRT);
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
