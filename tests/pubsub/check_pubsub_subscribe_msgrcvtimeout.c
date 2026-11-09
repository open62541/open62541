#include <open62541/plugin/log.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_clock.h"
#include "test_helpers.h"
#include "ua_server_internal.h"
#include "pubsub_test_helpers.h"
#include "ua_pubsub_internal.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;

/* global variables for fast-path configuration */
static UA_DataValue *pFastPathPublisherValue = 0;
static UA_DataValue *pFastPathSubscriberValue = 0;

static UA_Boolean runtime;

static void setup(void) {
    runtime = true;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "setup");

    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_StatusCode res = UA_Server_run_startup(server);
    ck_assert(UA_STATUSCODE_GOOD == res);
}

static void teardown(void) {
    runtime = false;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->pubSubConfig.stateChangeCallback = NULL;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "teardown");
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_run_shutdown(server));
    UA_Server_delete(server);
}

/* Utility functions to setup the PubSub configuration */

static void
AddConnection(char *pName, UA_UInt32 PublisherId, UA_NodeId *opConnectionId) {
    ck_assert(pName != 0);
    ck_assert(opConnectionId != 0);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(pName);
    connectionConfig.enabled = UA_TRUE;
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.id.uint32 = PublisherId;

    ck_assert(UA_Server_addPubSubConnection(server, &connectionConfig,
                                            opConnectionId) == UA_STATUSCODE_GOOD);
}

static void
AddWriterGroup(UA_NodeId *pConnectionId, char *pName, UA_UInt32 WriterGroupId,
               UA_Duration PublishingInterval, UA_NodeId *opWriterGroupId) {
    ck_assert(pConnectionId != 0);
    ck_assert(pName != 0);
    ck_assert(opWriterGroupId != 0);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING(pName);
    writerGroupConfig.publishingInterval = PublishingInterval;
    writerGroupConfig.writerGroupId = (UA_UInt16) WriterGroupId;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage =
        UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;

    ck_assert(UA_Server_addWriterGroup(server, *pConnectionId,
                                       &writerGroupConfig, opWriterGroupId) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

static void
AddPublishedDataSet(UA_NodeId *pWriterGroupId, char *pPublishedDataSetName,
                    char *pDataSetWriterName, UA_UInt32 DataSetWriterId,
                    UA_NodeId *opPublishedDataSetId, UA_NodeId *opPublishedVarId,
                    UA_NodeId *opDataSetWriterId) {
    ck_assert(pWriterGroupId != 0);
    ck_assert(pPublishedDataSetName != 0);
    ck_assert(pDataSetWriterName != 0);
    ck_assert(opPublishedDataSetId != 0);
    ck_assert(opPublishedVarId != 0);
    ck_assert(opDataSetWriterId != 0);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING(pPublishedDataSetName);
    UA_AddPublishedDataSetResult result =
        UA_Server_addPublishedDataSet(server, &pdsConfig, opPublishedDataSetId);
    ck_assert(result.addResult == UA_STATUSCODE_GOOD);

    /* Create variable to publish integer data */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData     = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "Published Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        attr, NULL, opPublishedVarId) == UA_STATUSCODE_GOOD);

    UA_NodeId dataSetFieldId;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Int32 Publish var");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = *opPublishedVarId;
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldResult PdsFieldResult =
        UA_Server_addDataSetField(server, *opPublishedDataSetId,
                                  &dataSetFieldConfig, &dataSetFieldId);
    ck_assert(PdsFieldResult.result == UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING(pDataSetWriterName);
    dataSetWriterConfig.dataSetWriterId = (UA_UInt16) DataSetWriterId;
    dataSetWriterConfig.keyFrameCount = 10;
    ck_assert(UA_Server_addDataSetWriter(server, *pWriterGroupId, *opPublishedDataSetId,
                                         &dataSetWriterConfig, opDataSetWriterId) == UA_STATUSCODE_GOOD);
}

static void
AddReaderGroup(UA_NodeId *pConnectionId, char *pName, UA_NodeId *opReaderGroupId) {
    ck_assert(pConnectionId != 0);
    ck_assert(pName != 0);
    ck_assert(opReaderGroupId != 0);

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING(pName);
    ck_assert(UA_Server_addReaderGroup(server, *pConnectionId, &readerGroupConfig,
                                       opReaderGroupId) == UA_STATUSCODE_GOOD);
}

static void
AddDataSetReader(UA_NodeId *pReaderGroupId, char *pName, UA_UInt32 PublisherId,
                 UA_UInt32 WriterGroupId, UA_UInt32 DataSetWriterId,
                 UA_Duration MessageReceiveTimeout, UA_NodeId *opSubscriberVarId,
                 UA_NodeId *opDataSetReaderId) {
    ck_assert(pReaderGroupId != 0);
    ck_assert(pName != 0);
    ck_assert(opSubscriberVarId != 0);
    ck_assert(opDataSetReaderId != 0);

    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING(pName);
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    readerConfig.publisherId.id.uint32 = PublisherId;
    readerConfig.writerGroupId    = (UA_UInt16) WriterGroupId;
    readerConfig.dataSetWriterId  = (UA_UInt16) DataSetWriterId;
    readerConfig.messageReceiveTimeout = MessageReceiveTimeout;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dsReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dsReaderMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dsReaderMessage;

    UA_DataSetMetaDataType_init(&readerConfig.dataSetMetaData);
    UA_DataSetMetaDataType *pDataSetMetaData = &readerConfig.dataSetMetaData;
    pDataSetMetaData->name = UA_STRING (pName);
    pDataSetMetaData->fieldsSize = 1;
    pDataSetMetaData->fields = (UA_FieldMetaData*) UA_Array_new (pDataSetMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    UA_FieldMetaData_init (&pDataSetMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                    &pDataSetMetaData->fields[0].dataType);
    pDataSetMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pDataSetMetaData->fields[0].name =  UA_STRING ("Int32 Var");
    pDataSetMetaData->fields[0].valueRank = -1;
    ck_assert(UA_Server_addDataSetReader(server, *pReaderGroupId, &readerConfig,
                                         opDataSetReaderId) == UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dsReaderMessage);
    dsReaderMessage = 0;

    /* Variable to subscribe data */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    attr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    attr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 SubscriberData = 0;
    UA_Variant_setScalar(&attr.value, &SubscriberData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                        UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        attr, NULL, opSubscriberVarId) == UA_STATUSCODE_GOOD);

    UA_FieldTargetDataType *pTargetVariables =  (UA_FieldTargetDataType *)
        UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetDataType));
    ck_assert(pTargetVariables != 0);

    pTargetVariables->attributeId  = UA_ATTRIBUTEID_VALUE;
    pTargetVariables->targetNodeId = *opSubscriberVarId;

    ck_assert(UA_Server_DataSetReader_createTargetVariables(server, *opDataSetReaderId,
        readerConfig.dataSetMetaData.fieldsSize, pTargetVariables) == UA_STATUSCODE_GOOD);

    UA_free(pTargetVariables);
    pTargetVariables = 0;

    UA_free(pDataSetMetaData->fields);
    pDataSetMetaData->fields = 0;
}

/* Utility function to trigger server process loop and wait until callbacks are
* executed. Use at least publishing interval for Sleep_ms. */
static void
ServerDoProcess(const char *pMessage, const UA_UInt32 Sleep_ms,
                const UA_UInt32 NoOfRunIterateCycles) {
    ck_assert(pMessage != 0);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ServerDoProcess() sleep : %s", pMessage);
    UA_Server_run_iterate(server, true);
    for (UA_UInt32 i = 0; i < NoOfRunIterateCycles; i++) {
        UA_fakeSleep(Sleep_ms);
        UA_Server_run_iterate(server, true);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ServerDoProcess() wakeup : %s", pMessage);
}

/* utility function to check working pubsub operation */
static void
ValidatePublishSubscribe(UA_NodeId PublishedVarId, UA_NodeId SubscribedVarId,
                         UA_Int32 TestValue, UA_UInt32 Sleep_ms,
                         UA_UInt32 NoOfRunIterateCycles) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ValidatePublishSubscribe(): set variable to publish");

    /* set variable value to publish */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Variant writeValue;
    UA_Variant_setScalar(&writeValue, &TestValue, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_writeValue(server, PublishedVarId, writeValue) == UA_STATUSCODE_GOOD);

    ServerDoProcess("ValidatePublishSubscribe()", Sleep_ms, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ValidatePublishSubscribe(): read subscribed variable");
    UA_Variant SubscribedNodeData;
    UA_Variant_init(&SubscribedNodeData);
    retVal = UA_Server_readValue(server, SubscribedVarId, &SubscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(SubscribedNodeData.data != 0);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ValidatePublishSubscribe(): check value: %i vs. %i",
                TestValue, *(UA_Int32 *)SubscribedNodeData.data);
    ck_assert_int_eq(TestValue, *(UA_Int32 *)SubscribedNodeData.data);
    UA_Variant_clear(&SubscribedNodeData);
}

/* Utility function to check working pubsub operation with fast-path
 * ExternalValue backend impl */
static void
ValidatePublishSubscribe_fast_path(UA_Int32 TestValue, UA_UInt32 Sleep_ms,
                                   UA_UInt32 NoOfRunIterateCycles) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ValidatePublishSubscribe_fast_path(): set variable to publish");

    ck_assert(pFastPathPublisherValue != 0);

    /* set variable value to publish */
    *(UA_Int32 *) pFastPathPublisherValue->value.data = TestValue;

    ServerDoProcess("ValidatePublishSubscribe_fast_path()", Sleep_ms, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "ValidatePublishSubscribe(): read subscribed variable");
    ck_assert(pFastPathSubscriberValue != 0);

    ck_assert_int_eq(TestValue, *(UA_Int32 *) pFastPathSubscriberValue->value.data);
}

/* simple test with 2 connections: 1 DataSetWriter and 1 DataSetReader */
START_TEST(Test_basic) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "\n\nSTART: Test_basic");

    /* Connection 1: Writer 1  --> Connection 2: Reader 1 */

    /* setup Connection 1: 1 writergroup, 1 writer */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", 1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_Duration PublishingInterval_Conn1WG1 = 100.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", 1, PublishingInterval_Conn1WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1;
    UA_NodeId_init(&VarId_Conn1_WG1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1",
                        1, &PDSId_Conn1_WG1_PDS1, &VarId_Conn1_WG1, &DsWId_Conn1_WG1_DS1);

    /* setup Connection 2: corresponding readergroup and reader for Connection 1 */

    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    AddConnection("Conn2", 2, &ConnId_2);

    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);
    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    UA_NodeId VarId_Conn2_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG1_DSR1);
    UA_Duration MessageReceiveTimeout = 400.0;

    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", 1, 1, 1,
                     MessageReceiveTimeout, &VarId_Conn2_RG1_DSR1,
                     &DSRId_Conn2_RG1_DSR1);


    UA_Server_enableAllPubSubComponents(server);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 10, 100, 3);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 33, 100, 3);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 44, 100, 3);

    UA_PubSubState state;
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* now we disable the publisher WriterGroup and check if a MessageReceiveTimeout occurs at Subscriber */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable writergroup");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_PAUSED);

    ServerDoProcess("1", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "check state of datasetreader");

    /* state of ReaderGroup should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* now we disable the reader */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable readergroup. writergroup is still working");

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    ServerDoProcess("4", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    /* then we disable the writer -> no timeout shall occur, because the reader is disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable writergroup");

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for writer group and dataset */
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_PAUSED);

    ServerDoProcess("5", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "END: Test_basic\n\n");
} END_TEST

/* Test different message receive timeouts */

START_TEST(Test_different_timeouts) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "\n\nSTART: Test_different_timeouts");

    /*
        Connection 1: WG1 : DSW1 (pub interval = 20)    --> Connection 1: RG1 : DSR1 (msgrcvtimeout = 100)
                                                        --> Connection 1: RG1 : DSR2 (msgrcvtimeout = 200)
                                                        --> Connection 2: RG1 : DSR1 (msgrcvtimeout = 300)
    */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "prepare configuration");

    /* setup Connection 1 */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 20.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WGNo_Conn1_WG1, PublishingInterval_Conn1_WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1_DS1;
    UA_NodeId_init(&VarId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    UA_UInt32 DSWNo_Conn1_WG1 = 1;

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1",
                        DSWNo_Conn1_WG1, &PDSId_Conn1_WG1_PDS1,
                        &VarId_Conn1_WG1_DS1, &DsWId_Conn1_WG1_DS1);

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 100.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1,
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn1_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Conn1_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    /* setup Connection 2 */
    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    UA_UInt32 PublisherNo_Conn2 = 2;
    AddConnection("Conn2", PublisherNo_Conn2, &ConnId_2);

    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);

    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    UA_NodeId VarId_Conn2_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn2_RG1_DSR1 = 300.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", PublisherNo_Conn1,
                     WGNo_Conn1_WG1, DSWNo_Conn1_WG1, MessageReceiveTimeout_Conn2_RG1_DSR1,
                     &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn2_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Conn2_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "check normal pubsub operation");

    UA_Server_enableAllPubSubComponents(server);

    ServerDoProcess("1", (UA_UInt32) (PublishingInterval_Conn1_WG1+1), 2);

    /* check that all dataset writers- and readers are operational */
    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check that publish/subscribe works (for all readers) -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 10, PublishingInterval_Conn1_WG1, 20);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 5, PublishingInterval_Conn1_WG1, 20);

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, PublishingInterval_Conn1_WG1, 20);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 49, PublishingInterval_Conn1_WG1, 20);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable writergroup");

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "check order and number of different message receive timeouts");

    ServerDoProcess("2", (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "there should not be any additional timeouts");

    ServerDoProcess("3", (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "END: Test_different_timeouts\n\n");
} END_TEST

START_TEST(Test_wrong_timeout) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "\n\nSTART: Test_wrong_timeout");

    /*
      Connection 1: WG1 : DSW1    --> Connection 1: RG1 : DSR1
    */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "prepare configuration");

    /* setup Connection 1 */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 500.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WGNo_Conn1_WG1,
                   PublishingInterval_Conn1_WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1_DS1;
    UA_NodeId_init(&VarId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    UA_UInt32 DSWNo_Conn1_WG1 = 1;

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1",
                        DSWNo_Conn1_WG1, &PDSId_Conn1_WG1_PDS1,
                        &VarId_Conn1_WG1_DS1, &DsWId_Conn1_WG1_DS1);

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 200.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1,
                     WGNo_Conn1_WG1, DSWNo_Conn1_WG1, MessageReceiveTimeout_Conn1_RG1_DSR1,
                     &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "set writer and reader to operational");

    UA_Server_enableAllPubSubComponents(server);

    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_PREOPERATIONAL);

    ServerDoProcess("1", PublishingInterval_Conn1_WG1, 1);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ServerDoProcess("1", PublishingInterval_Conn1_WG1, 5);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ck_assert_uint_eq(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1),
                      UA_STATUSCODE_GOOD);
    ServerDoProcess("timeout", 201, 1);
    ck_assert_uint_eq(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1,
                                                       &state), UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    ck_assert_uint_eq(UA_Server_enableWriterGroup(server, WGId_Conn1_WG1),
                      UA_STATUSCODE_GOOD);
    ServerDoProcess("recover", PublishingInterval_Conn1_WG1, 1);
    ck_assert_uint_eq(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1,
                                                       &state), UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "END: Test_wrong_timeout\n\n");
} END_TEST

START_TEST(Test_update_config) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "\n\nSTART: Test_update_config");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "prepare configuration");

    /* Connection 1: Writer 1  --> Reader 1 */

    /* setup Connection 1: 1 writergroup, 1 writer */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", 1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_Duration PublishingInterval_Conn1WG1 = 100.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", 1, PublishingInterval_Conn1WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1;
    UA_NodeId_init(&VarId_Conn1_WG1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1",
                        1, &PDSId_Conn1_WG1_PDS1, &VarId_Conn1_WG1, &DsWId_Conn1_WG1_DS1);

    /* setup corresponding readergroup and reader for Connection 1 */

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout = 200.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout,
                     &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    const UA_UInt32 SleepTime = 50;
    const UA_UInt32 NoOfRunIterateCycles = 6;

    UA_Server_enableAllPubSubComponents(server);

    ServerDoProcess("1", SleepTime, NoOfRunIterateCycles);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 10, SleepTime, NoOfRunIterateCycles);
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 33, SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable writer group");

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("2", SleepTime, NoOfRunIterateCycles);

    /* The reader timed out, but retains the last sample with BadTimeout. */
    UA_PubSubState state;
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = VarId_Conn1_RG1_DSR1;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue received = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(received.hasValue);
    ck_assert(received.hasStatus);
    ck_assert_uint_eq(received.status, UA_STATUSCODE_BADTIMEOUT);
    ck_assert_int_eq(*(UA_Int32*)received.value.data, 33);
    UA_DataValue_clear(&received);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "enable writer group");
    ck_assert(UA_Server_enableWriterGroup(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("4", SleepTime, NoOfRunIterateCycles);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 50, SleepTime, NoOfRunIterateCycles);
    received = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(received.hasValue);
    ck_assert(!received.hasStatus || received.status == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)received.value.data, 50);
    UA_DataValue_clear(&received);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "disable writer group");

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("5", SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "END: Test_update_config\n\n");
} END_TEST

static UA_StatusCode
customReaderStateMachine(UA_Server *s, UA_NodeId id, void *context,
                          UA_PubSubState *state, UA_PubSubState targetState) {
    *state = targetState;
    return UA_STATUSCODE_GOOD;
}

START_TEST(Test_timeout_recovery_validation) {
    UA_NodeId connection, group, readerId, target;
    AddConnection("Recovery", 1, &connection);
    AddReaderGroup(&connection, "Recovery", &group);
    AddDataSetReader(&group, "Recovery", 1, 1, 1, 200, &target, &readerId);

    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *reader = UA_DataSetReader_find(psm, readerId);
    ck_assert_ptr_ne(reader, NULL);
    UA_ReaderGroup *rg = reader->linkedReaderGroup;
    rg->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    rg->hasReceived = true;
    reader->head.state = UA_PUBSUBSTATE_OPERATIONAL;

    UA_Int32 value = 42;
    UA_DataValue field;
    UA_DataValue_init(&field);
    field.hasValue = true;
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    UA_DataSetMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.dataSetMessageValid = true;
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    msg.header.dataSetMessageSequenceNrEnabled = true;
    msg.header.dataSetMessageSequenceNr = 100;
    msg.fieldCount = 1;
    msg.data.keyFrameFields = &field;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(nm));
    nm.groupHeaderEnabled = true;
    nm.groupHeader.writerGroupIdEnabled = true;
    nm.groupHeader.writerGroupId = 1;
    nm.groupHeader.sequenceNumberEnabled = true;
    nm.groupHeader.sequenceNumber = 100;
    nm.payloadHeaderEnabled = true;
    nm.dataSetWriterIds[0] = 1;
    nm.messageCount = 1;
    nm.payload.dataSetMessages = &msg;

    lockServer(server);
    ck_assert(UA_ReaderGroup_process(psm, rg, &nm));
    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADTIMEOUT), UA_STATUSCODE_GOOD);
    ck_assert(UA_DataSetReader_canRecover(reader));
    ck_assert_uint_eq(reader->msgRcvTimeoutTimerId, 0);
    UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);
    ck_assert(UA_DataSetReader_canRecover(reader));

    /* A publisher restart resets counters; history expires after two timeouts. */
    UA_fakeSleep(401);
    nm.groupHeader.sequenceNumber = 1;
    msg.header.dataSetMessageSequenceNr = 1;
    msg.header.dataSetMessageValid = false;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.header.dataSetMessageValid = true;
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_KEEPALIVE;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    msg.fieldCount = 2;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.fieldCount = 0;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.fieldCount = 1;
    field.hasValue = false;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    field.hasValue = true;
    UA_Variant_init(&field.value);
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    UA_Variant_setScalar(&field.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    field.hasStatus = true;
    field.status = UA_STATUSCODE_BADNODATAAVAILABLE;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    field.hasStatus = false;
    reader->config.dataSetMetaData.configurationVersion.majorVersion = 2;
    msg.header.configVersionMajorVersionEnabled = true;
    msg.header.configVersionMajorVersion = 1;
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    msg.header.configVersionMajorVersion = 2;

    UA_FieldTargetDataType *tv =
        &reader->config.subscribedDataSet.target.targetVariables[0];
    UA_NodeId savedTarget = tv->targetNodeId;
    tv->targetNodeId = UA_NODEID_NUMERIC(1, 999999);
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    tv->targetNodeId = savedTarget;
    ck_assert(UA_DataSetReader_canRecover(reader));
    ck_assert_uint_eq(reader->msgRcvTimeoutTimerId, 0);

    value = 77;
    ck_assert(UA_ReaderGroup_process(psm, rg, &nm));
    ck_assert_int_eq(reader->head.state, UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(!reader->msgRcvTimedOut);
    ck_assert(reader->msgRcvTimeoutTimerId != 0);
    ck_assert(reader->receivedKeyFrame);
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm)); /* Duplicate */

    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADTIMEOUT), UA_STATUSCODE_GOOD);
    ck_assert(UA_DataSetReader_canRecover(reader));
    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADINTERNALERROR), UA_STATUSCODE_GOOD);
    msg.header.dataSetMessageSequenceNr = 2;
    nm.groupHeader.sequenceNumber = 2;
    ck_assert(!UA_DataSetReader_canRecover(reader));
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    ck_assert(!UA_DataSetReader_process(psm, reader, &msg));

    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_DISABLED, UA_STATUSCODE_GOOD), UA_STATUSCODE_GOOD);
    ck_assert(!UA_DataSetReader_canRecover(reader));
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    ck_assert(!UA_DataSetReader_process(psm, reader, &msg));

    reader->config.customStateMachine = customReaderStateMachine;
    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADTIMEOUT), UA_STATUSCODE_GOOD);
    ck_assert(!UA_DataSetReader_canRecover(reader));
    ck_assert(!UA_ReaderGroup_process(psm, rg, &nm));
    ck_assert(!UA_DataSetReader_process(psm, reader, &msg));
    reader->config.customStateMachine = NULL;
    unlockServer(server);
} END_TEST

START_TEST(Test_add_remove) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "\n\nSTART: Test_add_remove");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "prepare configuration");

    /* Connection 1: Reader 1 */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", 1, &ConnId_1);

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout = 200.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", 1, 1, 1,
                     MessageReceiveTimeout, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    /* check for memory leaks */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_removeDataSetReader(server, DSRId_Conn1_RG1_DSR1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "END: Test_add_remove\n\n");
} END_TEST

static const UA_OverrideValueHandling fallbackModes[] = {
    UA_OVERRIDEVALUEHANDLING_DISABLED,
    UA_OVERRIDEVALUEHANDLING_LASTUSABLEVALUE,
    UA_OVERRIDEVALUEHANDLING_OVERRIDEVALUE
};

static void
assertTargetArray(UA_NodeId target, const UA_Int32 *expected, UA_StatusCode status) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = target;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(value.hasValue);
    ck_assert_uint_eq(value.hasStatus ? value.status : UA_STATUSCODE_GOOD, status);
    ck_assert_ptr_eq(value.value.type, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(value.value.arrayLength, 4);
    ck_assert_mem_eq(value.value.data, expected, 4 * sizeof(UA_Int32));
    UA_DataValue_clear(&value);
}

static UA_StatusCode
writeRangeTarget(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                  const UA_NumericRange *range, const UA_DataValue *value) {
    size_t *writes = (size_t*)nodeContext;
    ck_assert_ptr_ne(range, NULL);
    ck_assert_uint_eq(range->dimensionsSize, 1);
    ck_assert_uint_eq(range->dimensions[0].min, 1);
    ck_assert_uint_eq(range->dimensions[0].max, 2);
    ck_assert(value->hasValue);
    ck_assert_uint_eq(value->value.arrayLength, 2);
    (*writes)++;
    return UA_STATUSCODE_GOOD;
}

START_TEST(Test_timeout_range_recovery) {
    UA_NodeId connection, group, readerId, unusedTarget, target;
    AddConnection("RangeRecovery", 1, &connection);
    AddReaderGroup(&connection, "RangeRecovery", &group);
    AddDataSetReader(&group, "RangeRecovery", 1, 1, 1, 200,
                     &unusedTarget, &readerId);

    UA_Int32 initial[] = {10, 20, 30, 40};
    UA_UInt32 dimensions = 4;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    attr.arrayDimensionsSize = 1;
    attr.arrayDimensions = &dimensions;
    UA_Variant_setArray(&attr.value, initial, 4, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Array"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &target), UA_STATUSCODE_GOOD);

    UA_Int32 override[] = {90, 91};
    UA_FieldTargetDataType tv;
    UA_FieldTargetDataType_init(&tv);
    tv.targetNodeId = target;
    tv.attributeId = UA_ATTRIBUTEID_VALUE;
    tv.receiverIndexRange = UA_STRING("1:2");
    tv.writeIndexRange = UA_STRING("1:2");
    tv.overrideValueHandling = fallbackModes[_i];
    UA_Variant_setArray(&tv.overrideValue, override, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_setDataSetReaderTargetVariables(server, readerId, 1, &tv),
                      UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *reader = UA_DataSetReader_find(psm, readerId);
    ck_assert_ptr_ne(reader, NULL);
    UA_Int32 input[] = {500, 60, 70, 800};
    UA_DataValue field;
    UA_DataValue_init(&field);
    field.hasValue = true;
    UA_Variant_setArray(&field.value, input, 4, &UA_TYPES[UA_TYPES_INT32]);
    UA_DataSetMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.dataSetMessageValid = true;
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    msg.fieldCount = 1;
    msg.data.keyFrameFields = &field;

    lockServer(server);
    reader->config.dataSetMetaData.fields[0].valueRank = UA_VALUERANK_ONE_DIMENSION;
    reader->linkedReaderGroup->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    reader->linkedReaderGroup->hasReceived = true;
    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_OPERATIONAL, UA_STATUSCODE_GOOD), UA_STATUSCODE_GOOD);
    ck_assert(UA_DataSetReader_canProcess(reader));
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    UA_Int32 expected[] = {10, 60, 70, 40};
    assertTargetArray(target, expected, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADTIMEOUT), UA_STATUSCODE_GOOD);
    ck_assert(UA_DataSetReader_canProcess(reader));
    ck_assert(UA_DataSetReader_canRecover(reader));
    ck_assert_uint_eq(reader->msgRcvTimeoutTimerId, 0);
    UA_StatusCode status = UA_STATUSCODE_BADTIMEOUT;
    if(fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_LASTUSABLEVALUE)
        status = UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE;
    else if(fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_OVERRIDEVALUE) {
        status = UA_STATUSCODE_GOODLOCALOVERRIDE;
        expected[1] = 90;
        expected[2] = 91;
    }
    assertTargetArray(target, expected, status);

    UA_FieldTargetDataType *storedTarget =
        &reader->config.subscribedDataSet.target.targetVariables[0];
    UA_String savedRange = storedTarget->writeIndexRange;
    storedTarget->writeIndexRange = UA_STRING("10:11");
    ck_assert(!UA_DataSetReader_process(psm, reader, &msg));
    ck_assert(UA_DataSetReader_canRecover(reader));
    assertTargetArray(target, expected, status);
    storedTarget->writeIndexRange = savedRange;

    input[1] = 110;
    input[2] = 120;
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    ck_assert_int_eq(reader->head.state, UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(!reader->msgRcvTimedOut);
    ck_assert(reader->msgRcvTimeoutTimerId != 0);
    expected[1] = 110;
    expected[2] = 120;
    assertTargetArray(target, expected, UA_STATUSCODE_GOOD);

    /* A received BadTimeout uses the same fallback without entering Error. */
    input[1] = 999;
    field.hasStatus = true;
    field.status = UA_STATUSCODE_BADTIMEOUT;
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    ck_assert_int_eq(reader->head.state, UA_PUBSUBSTATE_OPERATIONAL);
    if(fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_OVERRIDEVALUE) {
        expected[1] = 90;
        expected[2] = 91;
    }
    assertTargetArray(target, expected, status);
    ck_assert_uint_eq(reader->lastUsableValues[0].value.arrayLength, 2);
    ck_assert_int_eq(((UA_Int32*)reader->lastUsableValues[0].value.data)[0], 110);

    field.hasStatus = false;
    input[1] = 130;
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    expected[1] = 130;
    expected[2] = 120;
    assertTargetArray(target, expected, UA_STATUSCODE_GOOD);

    /* Successful ranged writes must not require a readable callback backend. */
    size_t writes = 0;
    UA_CallbackValueSource callbacks = {NULL, writeRangeTarget};
    ck_assert_uint_eq(UA_Server_setNodeContext(server, target, &writes),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, target,
        callbacks), UA_STATUSCODE_GOOD);
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    ck_assert_uint_eq(writes, 1);
    UA_DataValue restored;
    UA_DataValue_init(&restored);
    restored.hasValue = true;
    UA_Variant_setArray(&restored.value, expected, 4, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_setVariableNode_internalValueSource(server, target,
        &restored, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setNodeContext(server, target, NULL), UA_STATUSCODE_GOOD);
    unlockServer(server);
} END_TEST

static UA_StatusCode
customReaderWithFallback(UA_Server *s, UA_NodeId id, void *context,
                          UA_PubSubState *state, UA_PubSubState targetState) {
    *state = targetState;
    return UA_Server_updateDataSetReaderTargetValues(s, id, *(UA_StatusCode*)context);
}

START_TEST(Test_custom_target_fallback) {
    UA_NodeId connection, group, readerId, target;
    AddConnection("CustomFallback", 1, &connection);
    AddReaderGroup(&connection, "CustomFallback", &group);
    AddDataSetReader(&group, "CustomFallback", 1, 1, 1, 0, &target, &readerId);

    UA_Int32 override = 99;
    UA_FieldTargetDataType tv;
    UA_FieldTargetDataType_init(&tv);
    tv.targetNodeId = target;
    tv.attributeId = UA_ATTRIBUTEID_VALUE;
    tv.overrideValueHandling = fallbackModes[_i];
    UA_Variant_setScalar(&tv.overrideValue, &override, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_setDataSetReaderTargetVariables(server, readerId, 1, &tv),
                      UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *reader = UA_DataSetReader_find(psm, readerId);
    ck_assert_ptr_ne(reader, NULL);
    UA_Int32 input = 42;
    UA_DataValue field;
    UA_DataValue_init(&field);
    field.hasValue = true;
    UA_Variant_setScalar(&field.value, &input, &UA_TYPES[UA_TYPES_INT32]);
    UA_DataSetMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.dataSetMessageValid = true;
    msg.header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    msg.fieldCount = 1;
    msg.data.keyFrameFields = &field;

    lockServer(server);
    reader->head.state = UA_PUBSUBSTATE_OPERATIONAL;
    ck_assert(UA_DataSetReader_process(psm, reader, &msg));
    UA_StatusCode reason = UA_STATUSCODE_BADTIMEOUT;
    reader->config.customStateMachine = customReaderWithFallback;
    reader->config.context = &reason;
    ck_assert_uint_eq(UA_DataSetReader_setPubSubState(psm, reader,
        UA_PUBSUBSTATE_ERROR, reason), UA_STATUSCODE_GOOD);
    ck_assert(!UA_DataSetReader_canRecover(reader));
    ck_assert(!UA_DataSetReader_canProcess(reader));
    ck_assert(!UA_DataSetReader_process(psm, reader, &msg));
    ck_assert_uint_eq(reader->msgRcvTimeoutTimerId, 0);
    reader->config.customStateMachine = NULL;
    reader->config.context = NULL;
    unlockServer(server);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = target;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(value.hasValue);
    ck_assert_int_eq(*(UA_Int32*)value.value.data,
        fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_OVERRIDEVALUE ? 99 : 42);
    UA_StatusCode expectedStatus =
        fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_DISABLED ? UA_STATUSCODE_BADTIMEOUT :
        (fallbackModes[_i] == UA_OVERRIDEVALUEHANDLING_LASTUSABLEVALUE ?
         UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE : UA_STATUSCODE_GOODLOCALOVERRIDE);
    ck_assert_uint_eq(value.status, expectedStatus);
    UA_DataValue_clear(&value);

    ck_assert_uint_eq(UA_Server_updateDataSetReaderTargetValues(NULL, readerId, reason),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(UA_Server_updateDataSetReaderTargetValues(server,
        UA_NODEID_NUMERIC(1, 999999), reason), UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(UA_Server_updateDataSetReaderTargetValues(server,
        readerId, UA_STATUSCODE_GOOD), UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_FieldTargetDataType targets[2];
    UA_FieldTargetDataType_init(&targets[0]);
    UA_FieldTargetDataType_init(&targets[1]);
    targets[0].targetNodeId = UA_NODEID_NUMERIC(1, 999999);
    targets[1].targetNodeId = target;
    targets[0].attributeId = targets[1].attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_setDataSetReaderTargetVariables(server, readerId, 2, targets),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_updateDataSetReaderTargetValues(server, readerId, reason),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_int_eq(reader->head.state, UA_PUBSUBSTATE_ERROR);
    ck_assert_uint_eq(reader->msgRcvTimeoutTimerId, 0);
    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(value.hasValue);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADTIMEOUT);
    UA_DataValue_clear(&value);

    ck_assert_uint_eq(UA_Server_updateDataSetReaderTargetValues(server, readerId,
        UA_STATUSCODE_BADNOCOMMUNICATION), UA_STATUSCODE_BADNODEIDUNKNOWN);
    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(UA_Variant_isEmpty(&value.value));
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADNOCOMMUNICATION);
    UA_DataValue_clear(&value);
} END_TEST

int main(void) {
    TCase *tc_basic = tcase_create("Message Receive Timeout");
    tcase_add_checked_fixture(tc_basic, setup, teardown);

    /* test case description:
        - check normal pubsub operation (2 connections)
        - 1 Connection with 1 DataSetWriter, 1 Connection with counterpart DataSetReader
        - enable/disable writer- and readergroup multiple times
        - check message receive timeout
    */
    tcase_add_test(tc_basic, Test_basic);

    /* test case description:
        - 1 DataSetWriter
        - multiple DataSetReaders with different timeout settings
        - check order and no of message receive timeouts for the different DataSetReaders
    */
    tcase_add_test(tc_basic, Test_different_timeouts);

    /* test case description:
        - 1 Connection, 1 DataSetWriter, 1 DataSetReader
        - reader with wrong timeout setting (timeout is smaller than publishing interval)
    */
    tcase_add_test(tc_basic, Test_wrong_timeout);

    /* test case description:
        - configure a connection with writer and reader
        - try different updated configurations for the DataSetReader (different MessageReceiveTimeouts)
    */
    tcase_add_test(tc_basic, Test_update_config);
    tcase_add_test(tc_basic, Test_timeout_recovery_validation);
    tcase_add_loop_test(tc_basic, Test_timeout_range_recovery, 0, 3);
    tcase_add_loop_test(tc_basic, Test_custom_target_fallback, 0, 3);

    /* test case description:
        - add and remove a reader without any operation -> check for memory leaks
    */
    tcase_add_test(tc_basic, Test_add_remove);

    Suite *s = suite_create("PubSub timeout test suite: message receive timeout");
    suite_add_tcase(s, tc_basic);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
