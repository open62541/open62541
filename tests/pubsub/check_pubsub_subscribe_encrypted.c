/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"
#include "testing_clock.h"

#include <check.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

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
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    config = UA_Server_getConfig(server);

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
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
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
        writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
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

        readerConfig.subscribedDataSet.target.targetVariablesSize = 1;
        readerConfig.subscribedDataSet.target.targetVariables     = (UA_FieldTargetDataType *)
            UA_calloc(readerConfig.subscribedDataSet.target.targetVariablesSize, sizeof(UA_FieldTargetDataType));

        /* For creating Targetvariable */
        readerConfig.subscribedDataSet.target.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.target.targetVariables[0].targetNodeId = newnodeId;
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig, &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(readerConfig.subscribedDataSet.target.targetVariables);

        retVal = UA_Server_enableAllPubSubComponents(server);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

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

        /* Iterate to push writer to operational state */
        UA_fakeSleep(15);
        UA_Server_run_iterate(server,true);

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
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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

        UA_FieldTargetDataType targetVar;
        memset(&targetVar, 0, sizeof(UA_FieldTargetDataType));
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier, 1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        retVal = UA_Server_enableAllPubSubComponents(server);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* run server - publisher and subscriber */
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);

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
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
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

    UA_FieldTargetDataType targetVar;
    memset(&targetVar, 0, sizeof(UA_FieldTargetDataType));
    UA_FieldTargetDataType_init(&targetVar);
    targetVar.attributeId = UA_ATTRIBUTEID_VALUE;
    targetVar.targetNodeId = UA_NODEID_NUMERIC(1, variableId);
    retVal = UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                           1, &targetVar);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_free(pMetaData->fields);
    retVal = UA_Server_enableAllPubSubComponents(server);
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
    dsm.fieldCount = 1;
    dsm.data.keyFrameFields = &field;

    UA_UInt16 dataSetWriterId = DATASET_WRITER_ID;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    nm.publisherId.id.uint16 = PUBLISHER_ID;
    nm.groupHeaderEnabled = true;
    nm.groupHeader.writerGroupIdEnabled = true;
    nm.groupHeader.writerGroupId = WRITER_GROUP_ID;
    nm.payloadHeaderEnabled = true;
    nm.messageCount = 1;
    nm.dataSetWriterIds[0] = dataSetWriterId;
    nm.payload.dataSetMessages = &dsm;

    UA_PubSubSecurityPolicy *policy = &config->pubSubConfig.securityPolicies[0];
    UA_Byte nonce[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    void *policyContext = NULL;
    size_t sigSize = 0;
    if(mode != UA_MESSAGESECURITYMODE_NONE) {
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, keys->signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, keys->encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keys->keyNonce};
        UA_StatusCode retVal =
            policy->newGroupContext(policy, &sk, &ek, &kn, &policyContext);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        sigSize = policy->getSignatureSize(policy, policyContext);

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
    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.ctx.pos = buf.data;
    ctx.ctx.end = buf.data + buf.length;
    UA_Byte *encryptStart = NULL;
    retVal = UA_NetworkMessage_encodeBinaryWithEncryptStart(&ctx, &nm, &encryptStart);
    UA_Byte *bufPos = ctx.ctx.pos;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    if(mode != UA_MESSAGESECURITYMODE_NONE) {
        retVal = UA_NetworkMessage_signEncrypt(&nm, mode, policy, policyContext,
                                               buf.data, encryptStart, bufPos);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        policy->deleteGroupContext(policy, policyContext);
    }

    UA_PubSubConnection *connection =
        UA_PubSubConnection_find(getPSM(server), connection_test);
    ck_assert(connection != NULL);
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection_process(getPSM(server), connection, buf);
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

static UA_StatusCode
failDecrypt(const UA_PubSubSecurityPolicy *policy, void *context,
            UA_ByteString *data) {
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

START_TEST(ReaderGroup_failedDecryptPreservesReplayHistory) {
    addInt32Variable(VARIABLE_FIRST);
    TestKeys keys;
    setTestKeys(&keys, 0x11);
    UA_NodeId rgId;
    addReaderGroupWithReader(UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys,
                             VARIABLE_FIRST, &rgId);
    UA_PubSubSecurityPolicy *policy = &config->pubSubConfig.securityPolicies[0];
    UA_StatusCode (*decrypt)(const UA_PubSubSecurityPolicy *, void *, UA_ByteString *) =
        policy->decrypt;
    policy->decrypt = failDecrypt;
    sendAndReceive(11, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys);
    policy->decrypt = decrypt;
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 0);

    /* The helper reuses the nonce. Only successful decryption consumes it. */
    sendAndReceive(22, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 22);
    sendAndReceive(33, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, &keys);
    ck_assert_int_eq(readInt32Variable(VARIABLE_FIRST), 22);
} END_TEST

START_TEST(RejectReplayNonce) {
    UA_ReaderGroup rg;
    memset(&rg, 0, sizeof(rg));
    rg.securityTokenId = 7;

    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.securityEnabled = true;
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    nm.publisherId.id.uint16 = PUBLISHER_ID;
    nm.securityHeader.securityTokenId = 7;
    nm.securityHeader.messageNonceSize = 8;
    nm.securityHeader.messageNonce[4] = 1;

    UA_StatusCode res = UA_ReaderGroup_checkReplay(&rg, &nm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_ReaderGroup_checkReplay(&rg, &nm);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    nm.securityHeader.messageNonce[4] = 2;
    res = UA_ReaderGroup_checkReplay(&rg, &nm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    nm.securityHeader.messageNonce[4] = 1;
    res = UA_ReaderGroup_checkReplay(&rg, &nm);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* Histories are independent for each publisher, but token and nonce
     * validation must apply before a history entry is accepted. */
    nm.publisherId.id.uint16++;
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    nm.securityHeader.securityTokenId++;
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    nm.securityHeader.securityTokenId = rg.securityTokenId;
    nm.securityHeader.messageNonceSize = 7;
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    nm.securityHeader.messageNonceSize = 8;

    /* String publisher identifiers are retained by value, and serial
     * arithmetic permits a counter rollover without accepting stale data. */
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_STRING;
    nm.publisherId.id.string = UA_STRING("publisher");
    memset(&nm.securityHeader.messageNonce[4], 0xff, 4);
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    memset(&nm.securityHeader.messageNonce[4], 0, 4);
    nm.securityHeader.messageNonce[4] = 1;
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    for(size_t i = 0; i < UA_PUBSUB_REPLAY_HISTORY_SIZE; i++)
        UA_PublisherId_clear(&rg.replayHistory[i].publisherId);
} END_TEST

START_TEST(ReaderGroup_failedUpdatePreservesReplayHistory) {
    addInt32Variable(VARIABLE_FIRST);
    UA_NodeId rgId;
    addReaderGroupWithReader(UA_MESSAGESECURITYMODE_NONE, NULL, VARIABLE_FIRST, &rgId);
    ck_assert_uint_eq(UA_Server_disableReaderGroup(server, rgId), UA_STATUSCODE_GOOD);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), rgId);
    ck_assert_ptr_ne(rg, NULL);

    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.securityEnabled = true;
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_STRING;
    nm.publisherId.id.string = UA_STRING("retained-publisher");
    nm.securityHeader.securityTokenId = rg->securityTokenId;
    nm.securityHeader.messageNonceSize = 8;
    nm.securityHeader.messageNonce[4] = 1;
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(rg, &nm), UA_STATUSCODE_GOOD);

    UA_ReaderGroupConfig updated;
    ck_assert_uint_eq(UA_Server_getReaderGroupConfig(server, rgId, &updated),
                      UA_STATUSCODE_GOOD);
    UA_UInt32 settings = 0;
    UA_ExtensionObject_setValueNoDelete(&updated.transportSettings, &settings,
                                       &UA_TYPES[UA_TYPES_UINT32]);
    /* Simulate a transport becoming unavailable during config validation. */
    UA_PubSubConnection *connection = rg->linkedConnection;
    UA_String profile = connection->config.transportProfileUri;
    connection->config.transportProfileUri = UA_STRING("unsupported-profile");
    UA_StatusCode res = UA_Server_updateReaderGroupConfig(server, rgId, &updated);
    connection->config.transportProfileUri = profile;
    UA_ReaderGroupConfig_clear(&updated);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    ck_assert_uint_eq(UA_Server_removeReaderGroup(server, rgId), UA_STATUSCODE_GOOD);
} END_TEST

/* Every PublisherId representation has an independent replay history. */
START_TEST(ReplayHistory_publisherTypes) {
    UA_ReaderGroup rg;
    memset(&rg, 0, sizeof(rg));
    rg.securityTokenId = 7;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    nm.securityEnabled = true;
    nm.securityHeader.securityTokenId = 7;
    nm.securityHeader.messageNonceSize = 8;
    nm.securityHeader.messageNonce[4] = 1;
    /* Absent PublisherIds match each other, independently of enabled IDs. */
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = (UA_PublisherIdType)_i;
    if(_i == UA_PUBLISHERIDTYPE_STRING)
        nm.publisherId.id.string = UA_STRING("publisher");
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    for(size_t i = 0; i < UA_PUBSUB_REPLAY_HISTORY_SIZE; i++)
        UA_PublisherId_clear(&rg.replayHistory[i].publisherId);
} END_TEST

START_TEST(ReplayHistory_evictsOwnedString) {
    UA_ReaderGroup rg;
    memset(&rg, 0, sizeof(rg));
    rg.securityTokenId = 7;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.securityEnabled = true;
    nm.publisherIdEnabled = true;
    nm.publisherId.idType = UA_PUBLISHERIDTYPE_STRING;
    nm.securityHeader.securityTokenId = 7;
    nm.securityHeader.messageNonceSize = 8;
    nm.securityHeader.messageNonce[4] = 1;
    /* Exceed the bounded history and release the oldest owned identifiers. */
    for(size_t i = 0; i < UA_PUBSUB_REPLAY_HISTORY_SIZE + 2; i++) {
        char publisher[32];
        snprintf(publisher, sizeof(publisher), "publisher-%lu", (unsigned long)i);
        nm.publisherId.id.string = UA_STRING(publisher);
        ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_ReaderGroup_checkReplay(&rg, &nm),
                          UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    }
    for(size_t i = 0; i < UA_PUBSUB_REPLAY_HISTORY_SIZE; i++)
        UA_PublisherId_clear(&rg.replayHistory[i].publisherId);
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
    tcase_add_test(tc_pubsub_readergroup_security, ReaderGroup_failedDecryptPreservesReplayHistory);
    tcase_add_test(tc_pubsub_readergroup_security, ReaderGroup_failedUpdatePreservesReplayHistory);

    TCase *tc_replay = tcase_create("Replay protection");
    tcase_add_test(tc_replay, RejectReplayNonce);
    tcase_add_loop_test(tc_replay, ReplayHistory_publisherTypes,
                        UA_PUBLISHERIDTYPE_BYTE, UA_PUBLISHERIDTYPE_STRING + 1);
    tcase_add_test(tc_replay, ReplayHistory_evictsOwnedString);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);
    suite_add_tcase(suite, tc_pubsub_readergroup_security);
    suite_add_tcase(suite, tc_replay);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
