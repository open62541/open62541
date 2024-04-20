/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>
#include <time.h>

#include "../../deps/mp_printf.h"
#include "testing_clock.h"
#include "open62541/types_generated_handling.h"
#include "ua_pubsub.h"
#include "ua_server_internal.h"

#define STR_BUFSIZE             1024

#define UA_SUBSCRIBER_PORT       4801    /* Port for Subscriber*/
#define UA_PUBLISHER_PORT        4801    /* Port for Publisher*/
#define PUBLISH_INTERVAL         5       /* Publish interval*/
#define PUBLISHER_ID             2234    /* Publisher Id*/
#define DATASET_WRITER_ID        62541   /* DataSet Writer Id*/
#define WRITER_GROUP_ID          100     /* Writer group Id  */
#define PUBLISHVARIABLE_NODEID   1000    /* Published data nodeId */
#define SUBSCRIBEVARIABLE_NODEID 1002    /* Subscribed data nodeId */

/* Global declaration for test cases  */
UA_Server *serverPublisher = NULL;
UA_Server *serverSubscriber = NULL;

UA_ServerConfig *configPublisher = NULL;
UA_ServerConfig *configSubscriber = NULL;

// #define UA_TYPES_DATAGRAMWRITERGROUPTRANSPORT2DATATYPE 264
// typedef struct UA_DatagramWriterGroupTransport2DataType {
//     UA_Variant address;
// } UA_DatagramWriterGroupTransport2DataType;

// UA_NodeId publisherConnectionId;
// UA_NodeId subscriberConnectionId;
//
// UA_NodeId readerGroupId;
static void
setupFolder(UA_Server *server, UA_NodeId *folderId) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed Variables");
    folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                folderBrowseName,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                oAttr, NULL, folderId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}


static void
setupPubSubServer(UA_Server **server, UA_ServerConfig **config, UA_UInt16 portNumber,
                  UA_EventLoop *el) {
    UA_ServerConfig stack_config;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&stack_config, 0, sizeof(UA_ServerConfig));
    if(el) {
        stack_config.eventLoop = el;
        stack_config.externalEventLoop = true;
    }
    retVal |= UA_ServerConfig_setMinimal(&stack_config, portNumber, NULL);
    *server = UA_Server_newWithConfig(&stack_config);
    UA_ServerConfig *sc = UA_Server_getConfig(*server);
    sc->tcpReuseAddr = true;
    *config = sc;

    retVal |= UA_Server_run_startup(*server);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void
addUDPConnection(UA_Server *server, const char *host, UA_Int16 portNumber,
                 UA_NodeId *outConnectionId) {
    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Test Connection");
    char address[STR_BUFSIZE];
    memset(&address, 0, sizeof(address));
    mp_snprintf(address, STR_BUFSIZE, "opc.udp://%s:%d", host, portNumber);
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(address)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    /* also set the same publisher Id for the subscriber connection as it does not matter */
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    ck_assert_int_eq(UA_Server_addPubSubConnection(server, &connectionConfig,
                                                   outConnectionId), UA_STATUSCODE_GOOD);
}

static void
setupPublishedDataInt32(UA_Server *server, UA_UInt32 publishVariableNodeId,
                        UA_NodeId *outPublishedDataSetId) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet Test");
    UA_AddPublishedDataSetResult result = UA_Server_addPublishedDataSet(server, &pdsConfig, outPublishedDataSetId);
    ck_assert_int_eq(result.addResult, UA_STATUSCODE_GOOD);

    /* Create variable to publish integer data */
    UA_NodeId publisherNode;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData     = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, publishVariableNodeId),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, "Published Int32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       attr, NULL, &publisherNode);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Data Set Field */
    UA_NodeId dataSetFieldId;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Int32");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_int_eq(UA_Server_addDataSetField(server, *outPublishedDataSetId, &dataSetFieldConfig, &dataSetFieldId).result, UA_STATUSCODE_GOOD);
}

static void
setupWrittenData(UA_Server *server, UA_NodeId connectionId, UA_NodeId publishedDataSetId,
                 const char *dstHost, UA_UInt16 dstPort) {
    char dstAddress[STR_BUFSIZE];
    memset(&dstAddress, 0, sizeof(dstAddress));
    mp_snprintf(dstAddress, STR_BUFSIZE, "opc.udp://%s:%d", dstHost, dstPort);

    UA_NodeId writerGroup;
    UA_NodeId dataSetWriter;
    /* Writer group */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup Test");
    writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    /* Message settings in WriterGroup to include necessary headers */
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;

    /* udpTransportSettings. */
    UA_DatagramWriterGroupTransport2DataType udpTransportSettings;
    memset(&udpTransportSettings, 0, sizeof(UA_DatagramWriterGroupTransport2DataType));
    UA_NetworkAddressUrlDataType url =
        {UA_STRING_NULL, UA_STRING(dstAddress)};
    UA_ExtensionObject_setValue(&udpTransportSettings.address, &url,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    /* Encapsulate config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORT2DATATYPE];
    transportSettings.content.decoded.data = &udpTransportSettings;

    writerGroupConfig.transportSettings = transportSettings;

    UA_StatusCode retVal = UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
    retVal |= UA_Server_enableWriterGroup(server, writerGroup);

    /* DataSetWriter */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
    dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount = 10;
    retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                         &dataSetWriterConfig, &dataSetWriter);
    UA_free(writerGroupMessage);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void
setupPublishingUnicast(UA_Server *server, UA_NodeId connectionId, const char *dstHost,
                       UA_UInt16 dstPort, UA_UInt32 publishVariableNodeId) {
    UA_NodeId outPublishedDataSetId;
    setupPublishedDataInt32(serverPublisher, publishVariableNodeId, &outPublishedDataSetId);
    setupWrittenData(server, connectionId, outPublishedDataSetId, dstHost, dstPort);
}

static void
setupSubscribing(UA_Server *server, UA_NodeId connectionId,
                 UA_NodeId targetNodeId, UA_UInt32 subscribeVariableNodeId,
                 UA_NodeId *outReaderGroupId) {
    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");

    UA_StatusCode retVal =  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, outReaderGroupId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_enableReaderGroup(server, *outReaderGroupId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Data Set Reader */
    /* Parameters to filter received NetworkMessage */
    UA_DataSetReaderConfig readerConfig;
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
    pMetaData->fields     = (UA_FieldMetaData*)
        UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* Unsigned Integer DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    retVal = UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                    &pMetaData->fields[0].dataType);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[0].valueRank   = -1; /* scalar */

    UA_NodeId readerIdentifier;
    retVal = UA_Server_addDataSetReader(server, *outReaderGroupId, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);


    UA_FieldTargetVariable targetVar;
    memset(&targetVar, 0, sizeof(UA_FieldTargetVariable));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&targetVar.targetVariable);
    targetVar.targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVar.targetVariable.targetNodeId = targetNodeId;
    retVal = UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                            1, &targetVar);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_FieldTargetDataType_clear(&targetVar.targetVariable);
    UA_free(pMetaData->fields);
}

UA_NodeId publisherConnectionId;
UA_NodeId subscriberConnectionId;
UA_NodeId readerGroupId;
UA_NodeId outVariableNodeId;

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    setupPubSubServer(&serverPublisher, &configPublisher, UA_PUBLISHER_PORT, NULL);
    addUDPConnection(serverPublisher, "localhost", UA_PUBLISHER_PORT, &publisherConnectionId);
    setupPublishingUnicast(serverPublisher, publisherConnectionId, "127.0.0.1",
                           UA_SUBSCRIBER_PORT, PUBLISHVARIABLE_NODEID);

    setupPubSubServer(&serverSubscriber, &configSubscriber,
                      UA_SUBSCRIBER_PORT, NULL);

    UA_NodeId folderId;
    setupFolder(serverSubscriber, &folderId);
    addUDPConnection(serverSubscriber, "localhost", UA_SUBSCRIBER_PORT, &subscriberConnectionId);

    /* Add subscribed Variables */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_StatusCode retVal =
        UA_Server_addVariableNode(serverSubscriber, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                  folderId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &outVariableNodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    setupSubscribing(serverSubscriber, subscriberConnectionId, outVariableNodeId,
                     SUBSCRIBEVARIABLE_NODEID, &readerGroupId);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    UA_Server_run_shutdown(serverPublisher);
    UA_Server_run_shutdown(serverSubscriber);

    /* Call server delete functions */
    UA_Server_delete(serverSubscriber);
    UA_Server_delete(serverPublisher);
}

static void
checkReceived(UA_Server *publisher, UA_UInt32 publishVariableNodeId,
              UA_Server *subscriber, UA_UInt32 subscribeVariableNodeId) {
    /* Read data sent by the Publisher */
    UA_Variant publishedNodeData;
    UA_Variant_init(&publishedNodeData);
    UA_StatusCode retVal =
        UA_Server_readValue(publisher, UA_NODEID_NUMERIC(1, publishVariableNodeId),
                            &publishedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Read data received by the Subscriber */
    UA_Variant subscribedNodeData;
    UA_Variant_init(&subscribedNodeData);
    retVal = UA_Server_readValue(subscriber, UA_NODEID_NUMERIC(1, subscribeVariableNodeId),
                                 &subscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Check if data sent from Publisher is being received by Subscriber */
    ck_assert(publishedNodeData.type == subscribedNodeData.type);
    ck_assert(UA_order(publishedNodeData.data,
                       subscribedNodeData.data,
                       subscribedNodeData.type) == UA_ORDER_EQ);
    UA_Variant_clear(&subscribedNodeData);
    UA_Variant_clear(&publishedNodeData);
}

START_TEST(SinglePublishSubscribeInt32) {
    /* run server - publisher and subscriber */

    UA_fakeSleep(15);
    UA_Server_run_iterate(serverPublisher,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(serverPublisher,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(serverSubscriber,true);

    checkReceived(serverPublisher, PUBLISHVARIABLE_NODEID,
                  serverSubscriber, SUBSCRIBEVARIABLE_NODEID);
} END_TEST

START_TEST(RemoveAndAddReaderGroup) {
        UA_NodeId readerGroupId2;
        setupSubscribing(serverSubscriber, subscriberConnectionId, outVariableNodeId,
                         SUBSCRIBEVARIABLE_NODEID, &readerGroupId2);
        setupSubscribing(serverSubscriber, subscriberConnectionId,
                         outVariableNodeId, SUBSCRIBEVARIABLE_NODEID,
                         &readerGroupId);
} END_TEST

int main(void) {

    /*Test case to run both publisher and subscriber */
    TCase *tc_pubsub_publish_subscribe = tcase_create("Publisher publishing and Subscriber subscribing");
    tcase_add_checked_fixture(tc_pubsub_publish_subscribe, setup, teardown);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32);
    tcase_add_test(tc_pubsub_publish_subscribe, RemoveAndAddReaderGroup);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
