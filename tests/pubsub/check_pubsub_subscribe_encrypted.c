/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types_generated_handling.h>

#include "ua_pubsub.h"
#include "pubsub_test_helpers.h"
#include "ua_server_internal.h"

#include <check.h>
#include <time.h>
#include <stdlib.h>

#define UA_SUBSCRIBER_PORT       4801    /* Port for Subscriber*/
#define PUBLISH_INTERVAL         5       /* Publish interval*/
#define PUBLISHER_ID             2234    /* Publisher Id*/
#define DATASET_WRITER_ID        62541   /* DataSet Writer Id*/
#define WRITER_GROUP_ID          100     /* Writer group Id  */
#define PUBLISHER_DATA           42      /* Published data */
#define PUBLISHVARIABLE_NODEID   1000    /* Published data nodeId */
#define SUBSCRIBEOBJECT_NODEID   1001    /* Object nodeId */
#define SUBSCRIBEVARIABLE_NODEID 1002    /* Subscribed data nodeId */
#define READERGROUP_COUNT        2       /* Value to add readergroup to connection */
#define CHECK_READERGROUP_COUNT  3       /* Value to check readergroup count */

#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_NodeId connection_test;
UA_NodeId readerGroupTest;

UA_NodeId publishedDataSetTest;

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    server = UA_Server_new();
    ck_assert(server != NULL);
    config = UA_Server_getConfig(server);
    retVal |= UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);

    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    retVal |= UA_Server_run_startup(server);

    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Test Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4801);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = PUBLISHER_ID;
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(SinglePublishSubscribeDateTime) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Data Set Field */
        UA_NodeId dataSetFieldId;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataSetFieldResult retval = UA_Server_addDataSetField(server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldId);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup (server, connection_test, &readerGroupConfig, &readerGroupTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name = UA_STRING ("DataSetReader Test");
        readerConfig.dataSetWriterId = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
        targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                             &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* DateTime DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
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
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "DateTime");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "DateTime");
        vAttr.dataType    = UA_TYPES[UA_TYPES_DATETIME].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "DateTime"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables     = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));

        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = newnodeId;
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);
        UA_Server_run_iterate(server,true);
        UA_free(pMetaData->fields);
}END_TEST


START_TEST(SinglePublishSubscribeInt32) {

        /* Common encryption key informaton */
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Int32"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Int32");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        retVal = UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
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

        /* Writer Group Encryption settings */
        writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);


        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_Server_setWriterGroupEncryptionKeys(server, writerGroup, 1, sk, ek, kn);
        /* set the encryption keys for writergroup */

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");

        /* Reader Group Encryption settings */
        readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &readerGroupTest);

        /* Add the encryption key informaton for readergroup */
        // TODO security token not necessary for readergroup (extracted from security-header)
        UA_Server_setReaderGroupEncryptionKeys(server, readerGroupTest, 1, sk, ek, kn);


        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
           targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                 &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_NodeId newnodeId;
        UA_String folderName      = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale        = UA_STRING ("en-US");
            oAttr.displayName.text          = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name           = folderName;
        }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        retVal = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         folderBrowseName, UA_NODEID_NUMERIC(0,
                                         UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_FieldTargetVariable targetVar;
        memset(&targetVar, 0, sizeof(UA_FieldTargetVariable));
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVar.targetVariable);
        targetVar.targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetVariable.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_FieldTargetDataType_clear(&targetVar.targetVariable);
        UA_free(pMetaData->fields);

        /* run callbacks - publisher and subscriber */
        UA_Server_setWriterGroupOperational(server, writerGroup);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);

        UA_Server_run_iterate(server, true);

        /* Read data sent by the Publisher */
        UA_Variant *publishedNodeData = UA_Variant_new();
        retVal                        = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal                         = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert_int_eq(*(UA_Int32 *)publishedNodeData->data, *(UA_Int32 *)subscribedNodeData->data);
        UA_Variant_delete(subscribedNodeData);
        UA_Variant_delete(publishedNodeData);
    } END_TEST

/* Several ReaderGroups with different security settings on one connection. A
 * NetworkMessage must only be delivered to the DataSetReaders of a ReaderGroup
 * whose own security settings (mode and keys) accept this very message. All
 * ReaderGroups use the same PublisherId/WriterGroupId/DataSetWriterId, so the
 * identifiers alone don't tell the groups apart. */

#define VARIABLE_FIRST  3001
#define VARIABLE_SECOND 3002

typedef struct {
    UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH];
    UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH];
    UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH];
} TestKeys;

static void
setTestKeys(TestKeys *keys, UA_Byte fill) {
    memset(keys->signingKey, fill, sizeof(keys->signingKey));
    memset(keys->encryptingKey, fill, sizeof(keys->encryptingKey));
    memset(keys->keyNonce, fill, sizeof(keys->keyNonce));
}

static void
addInt32Variable(UA_UInt32 id) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 initial = 0;
    UA_Variant_setScalar(&attr.value, &initial, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode retVal =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, id),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static UA_Int32
readInt32Variable(UA_UInt32 id) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode retVal =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(1, id), &value);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32]));
    UA_Int32 result = *(UA_Int32*)value.data;
    UA_Variant_clear(&value);
    return result;
}

/* Add a ReaderGroup with one DataSetReader that writes the received Int32 to
 * the variable. The keys are ignored for the security mode None. */
static void
addReaderGroupWithReader(UA_MessageSecurityMode mode, TestKeys *keys,
                         UA_UInt32 variableId, UA_NodeId *readerGroupId) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup Test");
    readerGroupConfig.securityMode = mode;
    if(mode != UA_MESSAGESECURITYMODE_NONE)
        readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
    retVal = UA_Server_addReaderGroup(server, connection_test,
                                      &readerGroupConfig, readerGroupId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    if(mode != UA_MESSAGESECURITYMODE_NONE) {
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, keys->signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, keys->encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keys->keyNonce};
        retVal = UA_Server_setReaderGroupEncryptionKeys(server, *readerGroupId, 1, sk, ek, kn);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    }

    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSetReader Test");
    UA_UInt16 publisherIdentifier = PUBLISHER_ID;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId = WRITER_GROUP_ID;
    readerConfig.dataSetWriterId = DATASET_WRITER_ID;
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DataSet Test");
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData*)
        UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[0].valueRank = -1; /* scalar */
    UA_NodeId readerIdentifier;
    retVal = UA_Server_addDataSetReader(server, *readerGroupId, &readerConfig,
                                        &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_FieldTargetVariable targetVar;
    memset(&targetVar, 0, sizeof(UA_FieldTargetVariable));
    UA_FieldTargetDataType_init(&targetVar.targetVariable);
    targetVar.targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
    targetVar.targetVariable.targetNodeId = UA_NODEID_NUMERIC(1, variableId);
    retVal = UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                           1, &targetVar);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_free(pMetaData->fields);
    retVal = UA_Server_setReaderGroupOperational(server, *readerGroupId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

/* Process a NetworkMessage with a single Int32 field through the same
 * connection dispatch used by the event loop. With the security mode None, the message is
 * neither signed nor encrypted. Otherwise it is secured with the keys. */
static void
sendAndReceive(UA_Int32 value,
               UA_MessageSecurityMode mode, TestKeys *keys) {
    UA_DataValue field;
    UA_DataValue_init(&field);
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    field.hasValue = true;

    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(UA_DataSetMessage));
    dsm.header.dataSetMessageValid = true;
    dsm.header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    dsm.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm.data.keyFrameData.fieldCount = 1;
    dsm.data.keyFrameData.dataSetFields = &field;

    UA_UInt16 dataSetWriterId = DATASET_WRITER_ID;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm.publisherIdEnabled = true;
    nm.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    nm.publisherId.uint16 = PUBLISHER_ID;
    nm.groupHeaderEnabled = true;
    nm.groupHeader.writerGroupIdEnabled = true;
    nm.groupHeader.writerGroupId = WRITER_GROUP_ID;
    nm.payloadHeaderEnabled = true;
    nm.payloadHeader.dataSetPayloadHeader.count = 1;
    nm.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = &dataSetWriterId;
    nm.payload.dataSetPayload.dataSetMessages = &dsm;

    UA_PubSubSecurityPolicy *policy = &config->pubSubConfig.securityPolicies[0];
    UA_Byte nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    void *policyContext = NULL;
    size_t sigSize = 0;
    if(mode != UA_MESSAGESECURITYMODE_NONE) {
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, keys->signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, keys->encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keys->keyNonce};
        UA_StatusCode retVal =
            policy->newContext(policy->policyContext, &sk, &ek, &kn, &policyContext);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        sigSize = policy->symmetricModule.cryptoModule.signatureAlgorithm.
            getLocalSignatureSize(policyContext);

        nm.securityEnabled = true;
        nm.securityHeader.networkMessageSigned = true;
        nm.securityHeader.networkMessageEncrypted =
            (mode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
        nm.securityHeader.securityTokenId = 1;
        memcpy(nm.securityHeader.messageNonce, nonce, sizeof(nonce));
        nm.securityHeader.messageNonceSize = sizeof(nonce);
    }

    UA_ByteString buf;
    UA_StatusCode retVal =
        UA_ByteString_allocBuffer(&buf, UA_NetworkMessage_calcSizeBinary(&nm, NULL) + sigSize);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_Byte *bufPos = buf.data;
    UA_Byte *encryptStart = NULL;
    retVal = UA_NetworkMessage_encodeBinary(&nm, &bufPos, &buf.data[buf.length],
                                            &encryptStart);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    if(mode != UA_MESSAGESECURITYMODE_NONE) {
        retVal = UA_NetworkMessage_signEncrypt(&nm, mode, policy, policyContext,
                                               buf.data, encryptStart, bufPos);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        policy->deleteContext(policyContext);
    }

    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connection_test);
    ck_assert(connection != NULL);
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection_process(server, connection, buf);
    UA_UNLOCK(&server->serviceMutex);
    UA_ByteString_clear(&buf);
}

static void
checkSecurityIsolation(UA_Boolean plainGroupFirst) {
    addInt32Variable(VARIABLE_FIRST);
    addInt32Variable(VARIABLE_SECOND);

    /* The secured group uses the first variable and the group without security
     * the second. The order of adding them is the tested parameter. */
    TestKeys keys, otherKeys;
    setTestKeys(&keys, 0x11);
    setTestKeys(&otherKeys, 0x22);
    UA_NodeId securedGroup, plainGroup;
    if(plainGroupFirst) {
        addReaderGroupWithReader(UA_MESSAGESECURITYMODE_NONE, NULL,
                                 VARIABLE_SECOND, &plainGroup);
        addReaderGroupWithReader(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys,
                                 VARIABLE_FIRST, &securedGroup);
    } else {
        addReaderGroupWithReader(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys,
                                 VARIABLE_FIRST, &securedGroup);
        addReaderGroupWithReader(UA_MESSAGESECURITYMODE_NONE, NULL,
                                 VARIABLE_SECOND, &plainGroup);
    }

    /* An unsigned message is only accepted by the group without security */
    sendAndReceive(11, UA_MESSAGESECURITYMODE_NONE, NULL);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 11);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 0);

    /* A message secured with the keys of the secured group is not delivered to
     * the group without security. A signed message is not what that group
     * expects. */
    sendAndReceive(22, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 22);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 11);

    /* A message secured with some other keys is not accepted by any group */
    sendAndReceive(33, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &otherKeys);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 22);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 11);

    /* Signing without encryption is not sufficient for the secured group */
    sendAndReceive(44, UA_MESSAGESECURITYMODE_SIGN, &keys);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 22);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 11);
}

START_TEST(ReaderGroupsOfDifferentSecurityModeNoneFirst) {
    checkSecurityIsolation(true);
} END_TEST

START_TEST(ReaderGroupsOfDifferentSecurityModeNoneLast) {
    checkSecurityIsolation(false);
} END_TEST

START_TEST(ReaderGroupsWithDifferentKeys) {
    addInt32Variable(VARIABLE_FIRST);
    addInt32Variable(VARIABLE_SECOND);

    TestKeys keysFirst, keysSecond;
    setTestKeys(&keysFirst, 0x11);
    setTestKeys(&keysSecond, 0x22);
    UA_NodeId firstGroup, secondGroup;
    addReaderGroupWithReader(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keysFirst,
                             VARIABLE_FIRST, &firstGroup);
    addReaderGroupWithReader(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keysSecond,
                             VARIABLE_SECOND, &secondGroup);

    sendAndReceive(11, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keysFirst);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 11);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 0);

    sendAndReceive(22, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keysSecond);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 11);
    ck_assert_int_eq(readInt32Variable(VARIABLE_SECOND), 22);
} END_TEST

int main(void) {

    /*Test case to run both publisher and subscriber */
    TCase *tc_pubsub_publish_subscribe = tcase_create("Publisher publishing and Subscriber subscribing");
    tcase_add_checked_fixture(tc_pubsub_publish_subscribe, setup, teardown);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeDateTime);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32);

    /* Several ReaderGroups with different security settings on one connection */
    TCase *tc_pubsub_readergroup_security =
        tcase_create("ReaderGroups with different security settings");
    tcase_add_checked_fixture(tc_pubsub_readergroup_security, setup, teardown);
    tcase_add_test(tc_pubsub_readergroup_security, ReaderGroupsOfDifferentSecurityModeNoneFirst);
    tcase_add_test(tc_pubsub_readergroup_security, ReaderGroupsOfDifferentSecurityModeNoneLast);
    tcase_add_test(tc_pubsub_readergroup_security, ReaderGroupsWithDifferentKeys);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);
    suite_add_tcase(suite, tc_pubsub_readergroup_security);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
