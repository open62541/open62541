/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 -2021 Kalycito Infotech Private Limited (Author: Keerthivasan)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/pubsub_udp.h>
#include "open62541/server_pubsub.h"
#include <open62541/plugin/securitypolicy_default.h>

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#include <check.h>
#include <stdio.h>

UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, dataSetWriterIdent, dataSetFieldIdent, dataSetFieldIdent1,  readerGroupIdentifier, readerIdentifier;

UA_UInt32    *subValue;
UA_DataValue *subDataValueRT;
UA_UInt32    *subValue1;
UA_DataValue *subDataValueRT1;
UA_NodeId    subNodeId;
UA_NodeId    subNodeId1;
UA_NodeId    pubNodeId;
UA_NodeId    pubNodeId1;
#define             UA_AES128CTR_SIGNING_KEY_LENGTH          32
#define             UA_AES128CTR_KEY_LENGTH                  16
#define             UA_AES128CTR_KEYNONCE_LENGTH             4

UA_Byte signingKeyPub[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKeyPub[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNoncePub[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

typedef struct {
    UA_ByteString *buffer;
} UA_ReceiveContext;

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
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = 2234;
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
    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes128Ctr(&config->pubSubConfig.securityPolicies[0],
                                      &config->logger);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_StatusCode
recvTestFun(UA_PubSubChannel *channel, void *context, const UA_ByteString *buffer) {
    UA_ReceiveContext *ctx = (UA_ReceiveContext*)context;
    memcpy(ctx->buffer->data, buffer->data, buffer->length);
    ctx->buffer->length = buffer->length;
    return UA_STATUSCODE_GOOD;
}

static void receiveSingleMessageRT(UA_PubSubConnection *connection, UA_ReaderGroup *readerGroup) {
    UA_ByteString buffer;
    UA_DataSetReader *dataSetReader = LIST_FIRST(&readerGroup->readers);
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        ck_abort_msg("Message buffer allocation failed!");
    }

    if(!connection->channel) {
        ck_abort_msg("No connection established");
        return;
    }

    UA_ReceiveContext testCtx = {&buffer};
    UA_StatusCode retval = connection->channel->receive(connection->channel, NULL, recvTestFun, &testCtx, 1000000);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        buffer.length = 512;
        UA_ByteString_clear(&buffer);
        ck_abort_msg("Expected message not received!");
    }

    UA_NetworkMessage currentNetworkMessage;
    memset(&currentNetworkMessage, 0, sizeof(UA_NetworkMessage));
    size_t payLoadPosition = 0;
    UA_NetworkMessage_decodeHeaders(&buffer, &payLoadPosition, &currentNetworkMessage);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    verifyAndDecryptNetworkMessage(&config->logger,
                                   &buffer,
                                   &payLoadPosition,
                                   &currentNetworkMessage,
                                   readerGroup);
    UA_NetworkMessage_clear(&currentNetworkMessage);
    size_t currentPosition = 0;
    /* Decode only the necessary offset and update the networkMessage */
    if(UA_NetworkMessage_updateBufferedNwMessage(&dataSetReader->bufferedMessage, &buffer, &currentPosition) != UA_STATUSCODE_GOOD) {
        ck_abort_msg("PubSub receive. Unknown field type!");
    }

    /* Check the decoded message is the expected one */
    if((dataSetReader->bufferedMessage.nm->groupHeader.writerGroupId != dataSetReader->config.writerGroupId) ||
       (*dataSetReader->bufferedMessage.nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds != dataSetReader->config.dataSetWriterId)) {
        ck_abort_msg("PubSub receive. Unknown message received. Will not be processed.");
    }

    UA_ReaderGroup *rg =
        UA_ReaderGroup_findRGbyId(server, dataSetReader->linkedReaderGroup);

    UA_DataSetReader_process(server, rg, dataSetReader,
                             dataSetReader->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages);

    /* Delete the payload value of every dsf's decoded */
     UA_DataSetMessage *dsm = dataSetReader->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages;
     if(dsm->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
         for(UA_UInt16 i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
             UA_Variant_clear(&dsm->data.keyFrameData.dataSetFields[i].value);
         }
     }

    UA_ByteString_clear(&buffer);
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

    if(UA_NodeId_equal(nodeId, &subNodeId1)){
        memcpy(subValue1, data->value.data, sizeof(UA_UInt32));
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

START_TEST(SetupInvalidPubSubConfig) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
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
    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKeyPub};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKeyPub};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNoncePub};
    UA_Server_setWriterGroupEncryptionKeys(server, writerGroupIdent, 1, sk, ek, kn);
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
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    /* UA_Server_addDataSetWriter fails because fields in PDS is not RT capable */
    ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_BADCONFIGURATIONERROR);
    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    // TODO security token not necessary for readergroup (extracted from security-header)
    UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek, kn);
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
    UA_Variant_clear(&variant);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTIMPLEMENTED); // Multiple DSR not supported

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    retVal = UA_Server_removeDataSetReader(server, readerIdentifier2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_BADNOTSUPPORTED); // DateTime not supported
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(PublishAndSubscribeSingleFieldWithFixedOffsets) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
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
    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKeyPub};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKeyPub};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNoncePub};
    UA_Server_setWriterGroupEncryptionKeys(server, writerGroupIdent, 1, sk, ek, kn);
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

    /* Add dataset writer */
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
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    // TODO security token not necessary for readergroup (extracted from security-header)
    UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek, kn);
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
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);

    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    receiveSingleMessageRT(connection, readerGroup);
   /* Read data received by the Subscriber */
    UA_Variant *subscribedNodeData = UA_Variant_new();
    retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 50002), subscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert((*(UA_UInt32 *)subscribedNodeData->data) == 1000);
    UA_Variant_clear(subscribedNodeData);
    UA_free(subscribedNodeData);
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    UA_DataValue_delete(dataValue);
    UA_free(subValue);
    UA_free(subDataValueRT);
} END_TEST


START_TEST(PublishPDSWithMultipleFieldsAndSubscribeFixedSize) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    /* Add Subscribed Variables */
    UA_NodeId folderId1;
    UA_String folderName1 = UA_STRING("PubNodes");
    UA_ObjectAttributes oAttr1 = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName1;
    if (folderName1.length > 0) {
        oAttr1.displayName.locale = UA_STRING ("en-US");
        oAttr1.displayName.text = folderName1;
        folderBrowseName1.namespaceIndex = 1;
        folderBrowseName1.name = folderName1;
      }
    else {
        oAttr1.displayName = UA_LOCALIZEDTEXT ("en-US", "Published Variables");
        folderBrowseName1 = UA_QUALIFIEDNAME (1, "Published Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName1, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr1, NULL, &folderId1);
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 value            = 0;
    vAttr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&vAttr.value, &value, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.displayName           = UA_LOCALIZEDTEXT("en-US", "Published variable");
    vAttr.dataType              = UA_TYPES[UA_TYPES_UINT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 60000),
                                       folderId1,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed DateTime"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &pubNodeId);
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 60001),
                                       folderId1,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed1 DateTime"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &pubNodeId1);
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    UA_UInt32 *intValue = UA_UInt32_new();
    *intValue = 1000;
    UA_DataValue *dataValue = UA_DataValue_new();
    UA_Variant_setScalar(&dataValue->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    dataValue->hasValue = true;
    dsfConfig.field.variable.fieldNameAlias = UA_STRING("Published UInt32");
    dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &dataValue;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, 60000), valueBackend);
    dsfConfig.field.variable.rtValueSource.rtInformationModelNode = true;
    dsfConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, 60000);
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, NULL).result == UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig dsfConfig1;
    memset(&dsfConfig1, 0, sizeof(UA_DataSetFieldConfig));
    UA_UInt32 *intValue1 = UA_UInt32_new();
    *intValue1 = 2000;
    UA_DataValue *dataValue1 = UA_DataValue_new();
    UA_Variant_setScalar(&dataValue1->value, intValue1, &UA_TYPES[UA_TYPES_UINT32]);
    dataValue->hasValue = true;
    dsfConfig1.field.variable.fieldNameAlias = UA_STRING("Published1 UInt32");
    dsfConfig1.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend1;
    valueBackend1.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend1.backend.external.value = &dataValue1;
    valueBackend1.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend1.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, 60001), valueBackend1);
    dsfConfig1.field.variable.rtValueSource.rtInformationModelNode = true;
    dsfConfig1.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, 60001);
    ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig1, NULL).result == UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
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
    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKeyPub};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKeyPub};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNoncePub};
    UA_Server_setWriterGroupEncryptionKeys(server, writerGroupIdent, 1, sk, ek, kn);
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
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
    retVal =  UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    // TODO security token not necessary for readergroup (extracted from security-header)
    UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek, kn);
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
    pMetaData->fieldsSize = 2;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* UInt32 DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                   &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_UINT32;
    pMetaData->fields[0].valueRank = -1; /* scalar */

    UA_FieldMetaData_init(&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                   &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_UINT32;
    pMetaData->fields[1].valueRank = -1; /* scalar */

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

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
    /* Variable to subscribe data */
    vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 60003),
                                       folderId1,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &subNodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    subValue        = UA_UInt32_new();
    subDataValueRT  = UA_DataValue_new();
    subDataValueRT->hasValue = UA_TRUE;
    UA_Variant_setScalar(&subDataValueRT->value, subValue, &UA_TYPES[UA_TYPES_UINT32]);
    /* Set the value backend of the above create node to 'external value source' */
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &subDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, subNodeId, valueBackend);

    vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed1 UInt32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed1 UInt32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 60004),
                                       folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed1 UInt32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &subNodeId1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    subValue1        = UA_UInt32_new();
    subDataValueRT1  = UA_DataValue_new();
    subDataValueRT1->hasValue = UA_TRUE;
    UA_Variant_setScalar(&subDataValueRT1->value, subValue1, &UA_TYPES[UA_TYPES_UINT32]);
    /* Set the value backend of the above create node to 'external value source' */
    valueBackend1.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend1.backend.external.value = &subDataValueRT1;
    valueBackend1.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend1.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, subNodeId1, valueBackend1);

    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 2;
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable*) UA_calloc(2, sizeof(UA_FieldTargetVariable));
    UA_FieldTargetDataType_init(&targetVars[0].targetVariable);
    targetVars[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[0].targetVariable.targetNodeId = UA_NODEID_NUMERIC(1, 60003);
    UA_FieldTargetDataType_init(&targetVars[1].targetVariable);
    targetVars[1].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[1].targetVariable.targetNodeId = UA_NODEID_NUMERIC(1, 60004);
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = targetVars;

    retVal = UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                        &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    UA_FieldTargetDataType_clear(&targetVars[0].targetVariable);
    UA_FieldTargetDataType_clear(&targetVars[1].targetVariable);
    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);

    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    receiveSingleMessageRT(connection, readerGroup);
   /* Read data received by the Subscriber */
    UA_Variant *subscribedNodeData = UA_Variant_new();
    retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 60003), subscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert((*(UA_UInt32 *)subscribedNodeData->data) == 1000);
    UA_Variant_clear(subscribedNodeData);
    UA_free(subscribedNodeData);
   /* Read data received by the Subscriber */
    UA_Variant *subscribedNodeData1 = UA_Variant_new();
    retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 60004), subscribedNodeData1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert((*(UA_UInt32 *)subscribedNodeData1->data) == 2000);
    UA_Variant_clear(subscribedNodeData1);
    UA_free(subscribedNodeData1);
    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier) == UA_STATUSCODE_GOOD);
    UA_Server_deleteNode(server, pubNodeId1, UA_TRUE);
    UA_NodeId_clear(&pubNodeId);
    UA_Server_deleteNode(server, pubNodeId1, UA_TRUE);
    UA_NodeId_clear(&pubNodeId1);
    UA_Server_deleteNode(server, subNodeId, UA_TRUE);
    UA_NodeId_clear(&subNodeId);
    UA_Server_deleteNode(server, subNodeId1, UA_TRUE);
    UA_NodeId_clear(&subNodeId1);
    UA_free(subValue);
    UA_free(subDataValueRT);
    UA_free(subValue1);
    UA_free(subDataValueRT1);
    /* Free external data source */
    UA_free(intValue);
    UA_free(dataValue);
    /* Free external data source */
    UA_free(intValue1);
    UA_free(dataValue1);
} END_TEST

int main(void) {
    TCase *tc_pubsub_encryption_rt = tcase_create("PubSub encryption RT with fixed offsets");
    tcase_add_checked_fixture(tc_pubsub_encryption_rt, setup, teardown);
    tcase_add_test(tc_pubsub_encryption_rt, SetupInvalidPubSubConfig);
    tcase_add_test(tc_pubsub_encryption_rt, PublishAndSubscribeSingleFieldWithFixedOffsets);
    tcase_add_test(tc_pubsub_encryption_rt, PublishPDSWithMultipleFieldsAndSubscribeFixedSize);

    Suite *s = suite_create("PubSub encryption RT configuration levels");
    suite_add_tcase(s, tc_pubsub_encryption_rt);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
