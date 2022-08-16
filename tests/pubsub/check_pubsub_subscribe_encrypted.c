/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types_generated_handling.h>

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>
#include <time.h>

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
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);

    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies,
                                      &config->logger);

    UA_Server_run_startup(server);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Test Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4801/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

// START_TEST(SinglePublishSubscribeDateTime) {
//         /* To check status after running both publisher and subscriber */
//         UA_StatusCode retVal = UA_STATUSCODE_GOOD;
//         UA_PublishedDataSetConfig pdsConfig;
//         UA_NodeId dataSetWriter;
//         UA_NodeId readerIdentifier;
//         UA_NodeId writerGroup;
//         UA_DataSetReaderConfig readerConfig;
//         memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
//         pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
//         pdsConfig.name = UA_STRING("PublishedDataSet Test");
//         UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
//         ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
//         /* Data Set Field */
//         UA_NodeId dataSetFieldId;
//         UA_DataSetFieldConfig dataSetFieldConfig;
//         memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
//         dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
//         dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
//         dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
//         dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME);
//         dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//         UA_Server_addDataSetField(server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldId);
//         /* Writer group */
//         UA_WriterGroupConfig writerGroupConfig;
//         memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
//         writerGroupConfig.name = UA_STRING("WriterGroup Test");
//         writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
//         writerGroupConfig.enabled = UA_FALSE;
//         writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
//         writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
//         retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
//         UA_Server_setWriterGroupOperational(server, writerGroup);
//         /* DataSetWriter */
//         UA_DataSetWriterConfig dataSetWriterConfig;
//         memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
//         dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
//         dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
//         dataSetWriterConfig.keyFrameCount = 10;
//         retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
//         ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
//         /* Reader Group */
//         UA_ReaderGroupConfig readerGroupConfig;
//         memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
//         readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
//         retVal |=  UA_Server_addReaderGroup (server, connection_test, &readerGroupConfig, &readerGroupTest);
//         ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
//         UA_Server_setReaderGroupOperational(server, readerGroupTest);
//         /* Data Set Reader */
//         memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
//         readerConfig.name = UA_STRING ("DataSetReader Test");
//         readerConfig.dataSetWriterId = DATASET_WRITER_ID;
//         /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
//         UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
//         /* FilltestMetadata function in subscriber implementation */
//         UA_DataSetMetaDataType_init (pMetaData);
//         pMetaData->name = UA_STRING ("DataSet Test");
//         /* Static definition of number of fields size to 1 to create one
//         targetVariable */
//         pMetaData->fieldsSize = 1;
//         pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
//                              &UA_TYPES[UA_TYPES_FIELDMETADATA]);
//         /* DateTime DataType */
//         UA_FieldMetaData_init (&pMetaData->fields[0]);
//         UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
//                         &pMetaData->fields[0].dataType);
//         pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
//         pMetaData->fields[0].valueRank = -1; /* scalar */
//
//         /* Add Subscribed Variables */
//         UA_NodeId folderId;
//         UA_NodeId newnodeId;
//         UA_String folderName = readerConfig.dataSetMetaData.name;
//         UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
//         UA_QualifiedName folderBrowseName;
//         if (folderName.length > 0) {
//             oAttr.displayName.locale = UA_STRING ("en-US");
//             oAttr.displayName.text = folderName;
//             folderBrowseName.namespaceIndex = 1;
//             folderBrowseName.name = folderName;
//           }
//         else {
//             oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
//             folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
//         }
//
//         UA_Server_addObjectNode (server, UA_NODEID_NULL,
//                                  UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
//                                  UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
//                                  folderBrowseName, UA_NODEID_NUMERIC (0,
//                                  UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
//
//         /* Variable to subscribe data */
//         UA_VariableAttributes vAttr = UA_VariableAttributes_default;
//         vAttr.description = UA_LOCALIZEDTEXT ("en-US", "DateTime");
//         vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "DateTime");
//         vAttr.dataType    = UA_TYPES[UA_TYPES_DATETIME].typeId;
//         retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
//                                            folderId,
//                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "DateTime"),
//                                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
//         ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
//
//         readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 1;
//         readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables     = (UA_FieldTargetVariable *)
//             UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));
//
//         /* For creating Targetvariable */
//         UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable);
//         readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
//         readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[0].targetVariable.targetNodeId = newnodeId;
//         retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
//                                              &readerIdentifier);
//         ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
//         UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
//         /* run server - publisher and subscriber */
//         UA_Server_run_iterate(server,true);
//         UA_Server_run_iterate(server,true);
//         UA_free(pMetaData->fields);
// }END_TEST


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
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
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
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

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

int main(void) {

    /*Test case to run both publisher and subscriber */
    TCase *tc_pubsub_publish_subscribe = tcase_create("Publisher publishing and Subscriber subscribing");
    tcase_add_checked_fixture(tc_pubsub_publish_subscribe, setup, teardown);
    // tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeDateTime);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
