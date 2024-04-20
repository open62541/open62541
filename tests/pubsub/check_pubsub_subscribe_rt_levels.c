/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Kalycito Infotech Private Limited (Author: Suriya Narayanan)
 */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include "testing_clock.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>

UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, dataSetWriterIdent, dataSetFieldIdent, readerGroupIdentifier, readerIdentifier;

UA_UInt32    *subValue;
UA_DataValue *subDataValueRT;
UA_NodeId    subNodeId;

/* utility function to trigger server process loop and wait until pubsub callbacks are executed */
static void ServerDoProcess(
    const UA_UInt32 sleep_ms,             /* use at least publishing interval */
    const UA_UInt32 noOfIterateCycles)
{
    UA_Server_run_iterate(server, true);
    for (UA_UInt32 i = 0; i < noOfIterateCycles; i++) {
        UA_fakeSleep(sleep_ms);
        UA_Server_run_iterate(server, true);
    }
}

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

START_TEST(SubscribeSingleFieldWithFixedOffsets) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    // Create Variant and configure as DataSetField source
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = 1000;
    UA_DataValue *dataValue = UA_DataValue_new();
    UA_Variant_setScalar(&dataValue->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    dsfConfig.field.variable.fieldNameAlias = UA_STRING("Published Int32");
    dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.rtValueSource.staticValueSource = &dataValue;
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

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

    /* add dataset writer */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Data Set Reader */
    UA_UInt16 publisherIdentifier = 2234;
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING ("DataSetReader Test");
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

    subValue        = UA_UInt32_new();
    subDataValueRT  = UA_DataValue_new();
    subDataValueRT->hasValue = UA_TRUE;
    UA_Variant_setScalar(&subDataValueRT->value, subValue, &UA_TYPES[UA_TYPES_UINT32]);
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &subDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, subNodeId, valueBackend);

    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables     = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));

    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = subNodeId;

    retVal = UA_Server_addDataSetReader (server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    UA_FieldTargetDataType_clear(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

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
        UA_Variant_clear(subscribedNodeData);
        UA_free(subscribedNodeData);
        if(eq)
            break;
    }
    UA_DataValue_delete(dataValue);
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    UA_free(subValue);
    UA_free(subDataValueRT);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, publishedDataSetIdent));
} END_TEST

START_TEST(SetupInvalidPubSubConfigReader) {
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

        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));

        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);

        // UA_free(intValue);
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

        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables     = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));

        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = subNodeId;

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
        // UA_Variant_clear(&variant);

        ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTIMPLEMENTED); // Multiple DSR not supported

        ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
        retVal = UA_Server_removeDataSetReader(server, readerIdentifier2);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTSUPPORTED); // DateTime not supported
        ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, publishedDataSetIdent));
    } END_TEST

START_TEST(SetupInvalidPubSubConfig) {
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
    ck_assert_int_eq(UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,  1000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "variable"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attributes, NULL, NULL), UA_STATUSCODE_GOOD);
    dsfConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, 1000);
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    /* Not using static value source */
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_BADCONFIGURATIONERROR);

    UA_Variant_clear(&variant);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, publishedDataSetIdent));
} END_TEST

/* additional SubscriberBeforeWriteCallback test data */
#define NUMVARS 2
static UA_UInt32 sSubscriberWriteValue[NUMVARS] = {0};
static UA_NodeId sSubscribeWriteCb_TargetVar_Id[NUMVARS];
static void SubscriberBeforeWriteCallback(UA_Server *srv,
                       const UA_NodeId *readerId,
                       const UA_NodeId *readerGroupId,
                       const UA_NodeId *targetVariableId,
                       void *targetVariableContext,
                       UA_DataValue **externalDataValue) {

    int i = 0;
    ck_assert(srv != 0);
    ck_assert(UA_NodeId_equal(readerId, &readerIdentifier) == UA_TRUE);
    ck_assert(UA_NodeId_equal(readerGroupId, &readerGroupIdentifier) == UA_TRUE);
    for (i = 0; i < NUMVARS; i++) {
        if (UA_NodeId_equal(targetVariableId, &sSubscribeWriteCb_TargetVar_Id[i]) == UA_TRUE) {
            ck_assert(targetVariableContext != 0);
            ck_assert_uint_eq(10+i, *((UA_UInt32*) targetVariableContext));
            ck_assert(externalDataValue != 0);
            ck_assert_uint_eq((**externalDataValue).value.type->memSize, sizeof(sSubscriberWriteValue[i]));
            memcpy(&sSubscriberWriteValue[i], (**externalDataValue).value.data, (**externalDataValue).value.type->memSize);
            break;
        }
    }
    /* check if variable has been found */
    ck_assert(i < NUMVARS);
}

static void PublishSubscribeWithWriteCallback_Helper(
    UA_NodeId *publisherNode,
    UA_UInt32 **publisherData,
    UA_Boolean useRawEncoding) {

    /* test fast-path with subscriber write callback */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PublishSubscribeWithWriteCallback_Helper(): useRawEncoding = %s",
        (useRawEncoding == UA_TRUE) ? "true" : "false");

    /* configure the connection */
    int i;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    ck_assert(connection != 0);

    /* Data Set Field */
    UA_DataSetFieldConfig dataSetFieldConfig;
    UA_NodeId dataSetFieldIdents[NUMVARS];
    for (i = 0; i < NUMVARS; i++) {
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable  = publisherNode[i];
        dataSetFieldConfig.field.variable.publishParameters.attributeId        = UA_ATTRIBUTEID_VALUE;
        dataSetFieldConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
        retVal = UA_Server_addDataSetField (server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdents[i]).result;
        ck_assert(retVal == UA_STATUSCODE_GOOD);
    }
    /* Writer group */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name               = UA_STRING("WriterGroup Test");
    writerGroupConfig.rtLevel            = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.publishingInterval = 2;
    writerGroupConfig.enabled            = UA_FALSE;
    writerGroupConfig.writerGroupId      = 1;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    /* Message settings in WriterGroup to include necessary headers */
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    retVal |= UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* DataSetWriter */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
    dataSetWriterConfig.dataSetWriterId = 1;
    dataSetWriterConfig.keyFrameCount   = 10;
    if (useRawEncoding) {
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
    } else {
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    }
    retVal |= UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    retVal |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);

    /* Data Set Reader */
    /* Parameters to filter received NetworkMessage */
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name             = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = 1;
    readerConfig.dataSetWriterId  = 1;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dsReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dsReaderMessage->networkMessageContentMask =    (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    dsReaderMessage->publishingInterval = writerGroupConfig.publishingInterval;
    readerConfig.messageSettings.content.decoded.data = dsReaderMessage;
    readerConfig.messageReceiveTimeout = writerGroupConfig.publishingInterval * 10;
    if (useRawEncoding) {
        readerConfig.expectedEncoding = UA_PUBSUB_RT_RAW;
        readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
    } else {
        readerConfig.expectedEncoding = UA_PUBSUB_RT_UNKNOWN;   /* is this a good default value */
        readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    }

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name       = UA_STRING ("DataSet Test");
    /* Static definition of number of fields size to NUMVARS to create targetVariables */
    pMetaData->fieldsSize = NUMVARS;
    pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for (i = 0; i < NUMVARS; i++) {
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT32].typeId,
                        &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[i].valueRank   = -1; /* scalar */
    }
    retVal |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                            &readerIdentifier);
    UA_UadpDataSetReaderMessageDataType_delete(dsReaderMessage);
    dsReaderMessage = 0;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_FieldTargetVariable targetVar[NUMVARS];
    UA_UInt32 DummyTargetVariableContext[NUMVARS];
    memset(&targetVar, 0, sizeof(targetVar));
    for (i = 0; i < NUMVARS; i++) {
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVar[i].targetVariable);
        targetVar[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar[i].targetVariable.targetNodeId = sSubscribeWriteCb_TargetVar_Id[i];
        targetVar[i].beforeWrite                 = SubscriberBeforeWriteCallback;  /* set subscriber write callback */
        DummyTargetVariableContext[i]            = 10+i;
        targetVar[i].targetVariableContext       = &DummyTargetVariableContext[i];
    }
    retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                            NUMVARS, &targetVar[0]);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    for (i = 0; i < NUMVARS; i++) {
        UA_FieldTargetDataType_clear(&targetVar[i].targetVariable);
    }
    UA_free(pMetaData->fields);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier));
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent));
    ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_enableReaderGroup(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);

    for (i = 0; i < NUMVARS; i++) {
        /* run server - publisher and subscriber */
        *(publisherData[i]) = 42 + i;
        sSubscriberWriteValue[i] = 0;
    }
    ServerDoProcess((UA_UInt32) writerGroupConfig.publishingInterval, 3);
    for (i = 0; i < NUMVARS; i++) {
        /* check that subscriber write callback has been called - verify received value */
        ck_assert_uint_eq(*(publisherData[i]), sSubscriberWriteValue[i]);

        /* set new publisher data and test again */
        *(publisherData[i]) = 42 + NUMVARS + i;
        sSubscriberWriteValue[i] = 0;
    }
    ServerDoProcess((UA_UInt32) writerGroupConfig.publishingInterval, 3);
    for (i = 0; i < NUMVARS; i++) {
        /* check that subscriber write callback has been called - verify received value */
        ck_assert_uint_eq(*(publisherData[i]), sSubscriberWriteValue[i]);

        /* set new publisher data and test again for checking buffered data handling */
        *(publisherData[i]) = 42 + 2*NUMVARS + i;
        sSubscriberWriteValue[i] = 0;
    }
    ServerDoProcess((UA_UInt32) writerGroupConfig.publishingInterval, 3);
    for (i = 0; i < NUMVARS; i++) {
        ck_assert_uint_eq(*(publisherData[i]), sSubscriberWriteValue[i]);
    }

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setWriterGroupDisabled(server, writerGroupIdent));
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setReaderGroupDisabled(server, readerGroupIdentifier));

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent));
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier));

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePubSubConnection(server, connectionIdentifier));

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, publishedDataSetIdent));

    UA_NodeId_clear(&connectionIdentifier);
    UA_NodeId_clear(&publishedDataSetIdent);
    UA_NodeId_clear(&writerGroupIdent);
    UA_NodeId_clear(&dataSetWriterIdent);
    for (i = 0; i < NUMVARS; i++) {
        UA_NodeId_clear(&dataSetFieldIdents[i]);
    }
    UA_NodeId_clear(&readerGroupIdentifier);
    UA_NodeId_clear(&readerIdentifier);
}

START_TEST(PublishSubscribeWithWriteCallback) {

    int i;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PublishSubscribeWithWriteCallback() test start");

    /* Create variables to publish integer data */
    UA_NodeId publisherNode[NUMVARS];
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Integer");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Integer");
    attr.dataType              = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_DataValue *publisherDataValue[NUMVARS];
    UA_UInt32 *publisherData[NUMVARS];
    UA_StatusCode retVal;
    UA_ValueBackend valueBackend;

    for (i = 0; i < NUMVARS; i++) {
        publisherDataValue[i] = UA_DataValue_new();
        ck_assert(publisherDataValue[i] != 0);
        publisherData[i] = UA_UInt32_new();
        ck_assert(publisherData[i] != 0);
        *publisherData[i] = 42*(i+1);
        UA_Variant_setScalar(&publisherDataValue[i]->value, publisherData[i], &UA_TYPES[UA_TYPES_UINT32]);
        retVal = UA_Server_addVariableNode(server,
                                           UA_NODEID_NUMERIC(1, 50001+i),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                           UA_QUALIFIEDNAME(1, "Published Integer"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           attr, NULL, &publisherNode[i]);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* add external value backend for fast-path */
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &publisherDataValue[i];
        retVal = UA_Server_setVariableNode_valueBackend(server, publisherNode[i], valueBackend);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    }

    /* Add Subscribed Variables */
    UA_NodeId folderId;
    UA_String folderName = UA_STRING("Subscribed Variables");
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
    /* Variables to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_DataValue *subscriberDataValue[NUMVARS];
    UA_UInt32 *subscriberData[NUMVARS];
    for (i = 0; i < NUMVARS; i++) {
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 50001 + NUMVARS + i),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &sSubscribeWriteCb_TargetVar_Id[i]);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        subscriberDataValue[i] = UA_DataValue_new();
        ck_assert(subscriberDataValue[i] != 0);
        subscriberData[i] = UA_UInt32_new();
        ck_assert(subscriberData[i] != 0);
        *subscriberData[i] = 0;
        UA_Variant_setScalar(&subscriberDataValue[i]->value, subscriberData[i], &UA_TYPES[UA_TYPES_UINT32]);

        /* add external value backend for fast-path */
        memset(&valueBackend, 0, sizeof(valueBackend));
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &subscriberDataValue[i];
        retVal = UA_Server_setVariableNode_valueBackend(server, sSubscribeWriteCb_TargetVar_Id[i], valueBackend);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    }

    PublishSubscribeWithWriteCallback_Helper(publisherNode, publisherData, UA_FALSE);
    PublishSubscribeWithWriteCallback_Helper(publisherNode, publisherData, UA_TRUE);

    /* cleanup */
    for (i = 0; i < NUMVARS; i++) {
       UA_DataValue_delete(subscriberDataValue[i]);
       subscriberDataValue[i] = 0;
       UA_DataValue_delete(publisherDataValue[i]);
       publisherDataValue[i] = 0;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PublishSubscribeWithWriteCallback() test end");
} END_TEST


int main(void) {
    TCase *tc_pubsub_subscribe_rt = tcase_create("PubSub RT subscribe with fixed offsets");
    tcase_add_checked_fixture(tc_pubsub_subscribe_rt, setup, teardown);
    tcase_add_test(tc_pubsub_subscribe_rt, SetupInvalidPubSubConfig);
    tcase_add_test(tc_pubsub_subscribe_rt, SetupInvalidPubSubConfigReader);
    tcase_add_test(tc_pubsub_subscribe_rt, SubscribeSingleFieldWithFixedOffsets);
    tcase_add_test(tc_pubsub_subscribe_rt, PublishSubscribeWithWriteCallback);

    Suite *s = suite_create("PubSub RT configuration levels");
    suite_add_tcase(s, tc_pubsub_subscribe_rt);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
