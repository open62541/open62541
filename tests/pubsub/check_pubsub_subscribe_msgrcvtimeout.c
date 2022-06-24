#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_clock.h"
#include "ua_pubsub.h"

#include <check.h>

static UA_Server *server = NULL;

/* global variables to check PubSubStateChangeCallback */
static UA_NodeId ExpectedCallbackComponentNodeId;
static UA_StatusCode ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
static UA_PubSubState ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
static UA_UInt32 CallbackCnt = 0;

static UA_UInt32 ExpectedCallbackCnt = 0;
static UA_NodeId *pExpectedComponentCallbackIds = 0;

/* global variables for fast-path configuration */
static UA_Boolean UseFastPath = UA_FALSE;
static UA_DataValue *pFastPathPublisherValue = 0;
static UA_DataValue *pFastPathSubscriberValue = 0;

static UA_Boolean runtime;

/***************************************************************************************************/
/***************************************************************************************************/
static void setup(void) {

    runtime = true;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nsetup\n\n");

    UA_NodeId_init(&ExpectedCallbackComponentNodeId);
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    CallbackCnt = 0;
    ExpectedCallbackCnt = 0;
    pExpectedComponentCallbackIds = 0;

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    UA_StatusCode res = UA_Server_run_startup(server);
    ck_assert(UA_STATUSCODE_GOOD == res);

    UseFastPath = UA_FALSE;
}

/***************************************************************************************************/
static void teardown(void) {

    runtime = false;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nteardown\n\n");

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_run_shutdown(server));
    UA_Server_delete(server);

    UA_NodeId_clear(&ExpectedCallbackComponentNodeId);

    if (pExpectedComponentCallbackIds != 0) {
        UA_free(pExpectedComponentCallbackIds);
        pExpectedComponentCallbackIds = 0;
    }
}
/***************************************************************************************************/
/***************************************************************************************************/


/***************************************************************************************************/
/***************************************************************************************************/
/* utility functions to setup the PubSub configuration */

/***************************************************************************************************/
static void AddConnection(
    char *pName,
    UA_UInt32 PublisherId,
    UA_NodeId *opConnectionId) {

    ck_assert(pName != 0);
    ck_assert(opConnectionId != 0);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(pName);
    connectionConfig.enabled = UA_TRUE;
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    connectionConfig.publisherIdType = UA_PUBSUB_PUBLISHERID_NUMERIC;
    connectionConfig.publisherId.numeric = PublisherId;

    ck_assert(UA_Server_addPubSubConnection(server, &connectionConfig, opConnectionId) == UA_STATUSCODE_GOOD);
    ck_assert(UA_PubSubConnection_regist(server, opConnectionId) == UA_STATUSCODE_GOOD);
}

/***************************************************************************************************/
static void AddWriterGroup(
    UA_NodeId *pConnectionId,
    char *pName,
    UA_UInt32 WriterGroupId,
    UA_Duration PublishingInterval,
    UA_NodeId *opWriterGroupId) {

    ck_assert(pConnectionId != 0);
    ck_assert(pName != 0);
    ck_assert(opWriterGroupId != 0);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING(pName);
    writerGroupConfig.publishingInterval = PublishingInterval;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = (UA_UInt16) WriterGroupId;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    if (UseFastPath) {
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    }
    ck_assert(UA_Server_addWriterGroup(server, *pConnectionId, &writerGroupConfig, opWriterGroupId) == UA_STATUSCODE_GOOD);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

/***************************************************************************************************/
static void AddPublishedDataSet(
    UA_NodeId *pWriterGroupId,
    char *pPublishedDataSetName,
    char *pDataSetWriterName,
    UA_UInt32 DataSetWriterId,
    UA_NodeId *opPublishedDataSetId,
    UA_NodeId *opPublishedVarId,
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
    UA_AddPublishedDataSetResult result = UA_Server_addPublishedDataSet(server, &pdsConfig, opPublishedDataSetId);
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
    if (UseFastPath) {
        dataSetFieldConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
        pFastPathPublisherValue = UA_DataValue_new();
        ck_assert(pFastPathPublisherValue != 0);
        UA_Int32 *pPublisherData  = UA_Int32_new();
        ck_assert(pPublisherData != 0);
        *pPublisherData = 42;
        UA_Variant_setScalar(&pFastPathPublisherValue->value, pPublisherData, &UA_TYPES[UA_TYPES_INT32]);
        /* add external value backend for fast-path */
        UA_ValueBackend valueBackend;
        memset(&valueBackend, 0, sizeof(valueBackend));
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &pFastPathPublisherValue;
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setVariableNode_valueBackend(server, *opPublishedVarId, valueBackend));
    }
    UA_DataSetFieldResult PdsFieldResult = UA_Server_addDataSetField(server, *opPublishedDataSetId,
                              &dataSetFieldConfig, &dataSetFieldId);
    ck_assert(PdsFieldResult.result == UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING(pDataSetWriterName);
    dataSetWriterConfig.dataSetWriterId = (UA_UInt16) DataSetWriterId;
    dataSetWriterConfig.keyFrameCount = 10;
    ck_assert(UA_Server_addDataSetWriter(server, *pWriterGroupId, *opPublishedDataSetId, &dataSetWriterConfig, opDataSetWriterId) == UA_STATUSCODE_GOOD);
}

/***************************************************************************************************/
static void AddReaderGroup(
    UA_NodeId *pConnectionId,
    char *pName,
    UA_NodeId *opReaderGroupId) {

    ck_assert(pConnectionId != 0);
    ck_assert(pName != 0);
    ck_assert(opReaderGroupId != 0);

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING(pName);
    if (UseFastPath) {
        readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    }
    ck_assert(UA_Server_addReaderGroup(server, *pConnectionId, &readerGroupConfig,
                                       opReaderGroupId) == UA_STATUSCODE_GOOD);
}

/***************************************************************************************************/
static void AddDataSetReader(
    UA_NodeId *pReaderGroupId,
    char *pName,
    UA_UInt32 PublisherId,
    UA_UInt32 WriterGroupId,
    UA_UInt32 DataSetWriterId,
    UA_Duration MessageReceiveTimeout,
    UA_NodeId *opSubscriberVarId,
    UA_NodeId *opDataSetReaderId) {

    ck_assert(pReaderGroupId != 0);
    ck_assert(pName != 0);
    ck_assert(opSubscriberVarId != 0);
    ck_assert(opDataSetReaderId != 0);

    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING(pName);
    UA_Variant_setScalar(&readerConfig.publisherId, (UA_UInt16*) &PublisherId, &UA_TYPES[UA_TYPES_UINT16]);
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
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, opSubscriberVarId) == UA_STATUSCODE_GOOD);

    if (UseFastPath) {
        pFastPathSubscriberValue = UA_DataValue_new();
        ck_assert(pFastPathSubscriberValue != 0);
        UA_Int32 *pSubscriberData  = UA_Int32_new();
        ck_assert(pSubscriberData != 0);
        *pSubscriberData = 0;
        UA_Variant_setScalar(&pFastPathSubscriberValue->value, pSubscriberData, &UA_TYPES[UA_TYPES_INT32]);
        /* add external value backend for fast-path */
        UA_ValueBackend valueBackend;
        memset(&valueBackend, 0, sizeof(valueBackend));
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &pFastPathSubscriberValue;
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setVariableNode_valueBackend(server, *opSubscriberVarId, valueBackend));
    }

    UA_FieldTargetVariable *pTargetVariables =  (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    ck_assert(pTargetVariables != 0);

    UA_FieldTargetDataType_init(&pTargetVariables[0].targetVariable);

    pTargetVariables[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    pTargetVariables[0].targetVariable.targetNodeId = *opSubscriberVarId;

    ck_assert(UA_Server_DataSetReader_createTargetVariables(server, *opDataSetReaderId,
        readerConfig.dataSetMetaData.fieldsSize, pTargetVariables) == UA_STATUSCODE_GOOD);

    UA_FieldTargetDataType_clear(&pTargetVariables[0].targetVariable);
    UA_free(pTargetVariables);
    pTargetVariables = 0;

    UA_free(pDataSetMetaData->fields);
    pDataSetMetaData->fields = 0;
}

/***************************************************************************************************/
/***************************************************************************************************/

/***************************************************************************************************/
/* utility function to trigger server process loop and wait until callbacks are executed */
static void ServerDoProcess(
    const char *pMessage,
    const UA_UInt32 Sleep_ms,             /* use at least publishing interval */
    const UA_UInt32 NoOfRunIterateCycles)
{
    ck_assert(pMessage != 0);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ServerDoProcess() sleep : %s", pMessage);
    UA_Server_run_iterate(server, true);
    for (UA_UInt32 i = 0; i < NoOfRunIterateCycles; i++) {
        UA_fakeSleep(Sleep_ms);
        UA_Server_run_iterate(server, true);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ServerDoProcess() wakeup : %s", pMessage);
}


/***************************************************************************************************/
/* utility function to check working pubsub operation */
static void ValidatePublishSubscribe(
    UA_NodeId PublishedVarId,
    UA_NodeId SubscribedVarId,
    UA_Int32 TestValue,
    UA_UInt32 Sleep_ms, /* use at least publishing interval */
    UA_UInt32 NoOfRunIterateCycles)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): set variable to publish");

    /* set variable value to publish */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Variant writeValue;
    UA_Variant_setScalar(&writeValue, &TestValue, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_writeValue(server, PublishedVarId, writeValue) == UA_STATUSCODE_GOOD);

    ServerDoProcess("ValidatePublishSubscribe()", Sleep_ms, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): read subscribed variable");
    UA_Variant SubscribedNodeData;
    UA_Variant_init(&SubscribedNodeData);
    retVal = UA_Server_readValue(server, SubscribedVarId, &SubscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(SubscribedNodeData.data != 0);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): check value: %i vs. %i",
        TestValue, *(UA_Int32 *)SubscribedNodeData.data);
    ck_assert_int_eq(TestValue, *(UA_Int32 *)SubscribedNodeData.data);
    UA_Variant_clear(&SubscribedNodeData);
}


/***************************************************************************************************/
/* utility function to check working pubsub operation with fast-path ExternalValue backend impl    */
static void ValidatePublishSubscribe_fast_path(
    UA_Int32 TestValue,
    UA_UInt32 Sleep_ms, /* use at least publishing interval */
    UA_UInt32 NoOfRunIterateCycles)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe_fast_path(): set variable to publish");

    ck_assert(pFastPathPublisherValue != 0);

    /* set variable value to publish */
    *(UA_Int32 *) pFastPathPublisherValue->value.data = TestValue;

    ServerDoProcess("ValidatePublishSubscribe_fast_path()", Sleep_ms, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): read subscribed variable");
    ck_assert(pFastPathSubscriberValue != 0);

    ck_assert_int_eq(TestValue, *(UA_Int32 *) pFastPathSubscriberValue->value.data);
}


/***************************************************************************************************/
/***************************************************************************************************/

/***************************************************************************************************/
static void PubSubStateChangeCallback_basic (UA_Server *hostServer,
                                UA_NodeId *pubsubComponentId,
                                UA_PubSubState state,
                                UA_StatusCode status) {
    ck_assert(hostServer == server);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    ck_assert(ExpectedCallbackStateChange == state);
    ck_assert(ExpectedCallbackStatus == status);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Callback Cnt = %u", CallbackCnt);

    ck_assert(CallbackCnt < ExpectedCallbackCnt);
    ck_assert(pExpectedComponentCallbackIds != 0);
    UA_String_init(&strId);
    UA_NodeId_print(&(pExpectedComponentCallbackIds[CallbackCnt]), &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Expected Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    ck_assert(UA_NodeId_equal(pubsubComponentId, &(pExpectedComponentCallbackIds[CallbackCnt])) == UA_TRUE);
    CallbackCnt++;
}

/***************************************************************************************************/
/* simple test with 2 connections: 1 DataSetWriter and 1 DataSetReader */
START_TEST(Test_basic) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_basic");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_basic;

    /* Connection 1: Writer 1  --> Connection 2: Reader 1 */

    /* setup Connection 1: 1 writergroup, 1 writer */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", 1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_Duration PublishingInterval_Conn1WG1 = 300.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", 1, PublishingInterval_Conn1WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1;
    UA_NodeId_init(&VarId_Conn1_WG1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", 1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1, &DsWId_Conn1_WG1_DS1);

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
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    UA_PubSubState state;
    /* check WriterGroup and DataSetWriter state */
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* state change to operational of WriterGroup */
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds = (UA_NodeId*) UA_calloc(ExpectedCallbackCnt, sizeof(UA_NodeId));
    ck_assert(pExpectedComponentCallbackIds != 0);
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn1_WG1;

    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;

    /* set WriterGroup operational */
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for writer group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ServerDoProcess("0", (UA_UInt32) (PublishingInterval_Conn1WG1), 3);

    /* there should not be a MessageReceiveTimeout, writers are running, readers are still disabled  */
    ck_assert(CallbackCnt == 0);

    /* check ReaderGroup and DataSetReader state */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* state change to operational of WriterGroup */
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = RGId_Conn2_RG1;

    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;

    /* set ReaderGroup operational */
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for reader group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 10, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 33, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 44, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* there should not be a callback notification for MessageReceiveTimeout */
    ck_assert(CallbackCnt == 0);

    /* now we disable the publisher WriterGroup and check if a MessageReceiveTimeout occurs at Subscriber */
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn1_WG1;

    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* check that callback has been called for writer group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;

    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;

    ServerDoProcess("1", (UA_UInt32) (PublishingInterval_Conn1WG1), 3);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreader");

    /* state of ReaderGroup should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* check that PubSubStateChange callback has been called for the specific DataSetReader */
    ck_assert_int_eq(1, CallbackCnt);
    CallbackCnt = 0;

    /* enable the publisher WriterGroup again */
    /* DataSetReader state shall be back to operational after receiving a new message */
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn1_WG1;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for writer group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;

    ServerDoProcess("2", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq(1, CallbackCnt);

    ServerDoProcess("3", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(1, CallbackCnt);
    CallbackCnt = 0;

    /* now we disable the reader */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable readergroup. writergroup is still working");

    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = RGId_Conn2_RG1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for reader group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    ServerDoProcess("4", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    /* then we disable the writer -> no timeout shall occur, because the reader is disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");

    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn1_WG1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check that callback has been called for writer group and dataset */
    ck_assert_int_eq(2, CallbackCnt);
    CallbackCnt = 0;

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    ServerDoProcess("5", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    /* Note: when we enable the readergroup it receives old messages ...
        we are not sure if this is correct, or if the socket needs to be flushed, before enabling the reader again ... */

    // /* enable readergroup -> nothing should happen, because there's no writer, timeout is only checked after first received message */
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable readergroup");
    // ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    // ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // ServerDoProcess("6", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    // /* reader state shall still be operational */
    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // ServerDoProcess("7", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    // /* PubSubStateChange callback must not have been triggered */
    // ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 1);

    // /* enable writergroup again -> now normal MessageReceiveTimeout check shall start again */
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup again");
    // ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    // ServerDoProcess("8", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // /* PubSubStateChange callback must not have been triggered */
    // ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 1);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_basic\n\n");

} END_TEST

/* Test different message receive timeouts */

static void
PubSubStateChangeCallback_different_timeouts(UA_Server *hostServer, UA_NodeId *pubsubComponentId,
                                             UA_PubSubState state, UA_StatusCode status) {
    ck_assert(hostServer == server);

    /* Disable some checks during shutdown */
    if(!runtime)
        return;

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    ck_assert(ExpectedCallbackStateChange == state);
    ck_assert(ExpectedCallbackStatus == status);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Callback Cnt = %u", CallbackCnt);

    ck_assert(CallbackCnt < ExpectedCallbackCnt);
    ck_assert(pExpectedComponentCallbackIds != 0);
    UA_String_init(&strId);
    UA_NodeId_print(&(pExpectedComponentCallbackIds[CallbackCnt]), &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Expected Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    ck_assert(UA_NodeId_equal(pubsubComponentId, &(pExpectedComponentCallbackIds[CallbackCnt])) == UA_TRUE);
    CallbackCnt++;
}


/* TODO: test does not work if we add the same reader on the same connection ...
    maybe only 1 reader per connection receives the data ... ??
    or the second reader overwrites the first?
    issue: https://github.com/open62541/open62541/issues/3901 */

START_TEST(Test_different_timeouts) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_different_timeouts");

    /*
        Connection 1: WG1 : DSW1 (pub interval = 20)    --> Connection 1: RG1 : DSR1 (msgrcvtimeout = 100)
                                                        --> Connection 1: RG1 : DSR2 (msgrcvtimeout = 200)
                                                        --> Connection 2: RG1 : DSR1 (msgrcvtimeout = 300)
    */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
   /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_different_timeouts;

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
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSWNo_Conn1_WG1, &PDSId_Conn1_WG1_PDS1,
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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn1_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    // // add the same reader on the same connection again -> but with different message receive timeout
    // UA_NodeId DSRId_Conn1_RG1_DSR2;
    // UA_NodeId_init(&DSRId_Conn1_RG1_DSR2);
    // UA_NodeId VarId_Conn1_RG1_DSR2;
    // UA_NodeId_init(&VarId_Conn1_RG1_DSR2);
    // UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR2 = 200.0;
    // AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR2", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1,
    //     MessageReceiveTimeout_Conn1_RG1_DSR2, &VarId_Conn1_RG1_DSR2, &DSRId_Conn1_RG1_DSR2);
    // UA_String_init(&strId);
    // UA_NodeId_print(&DSRId_Conn1_RG1_DSR2, &strId);
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn1_RG1_DSR2 Id = %.*s", (UA_Int32) strId.length, strId.data);
    // UA_String_clear(&strId);

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
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, MessageReceiveTimeout_Conn2_RG1_DSR1,
        &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn2_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn2_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    /* prepare expected order of pubsub component timeouts: */
    ExpectedCallbackCnt = 6;
    pExpectedComponentCallbackIds = (UA_NodeId*) UA_calloc(ExpectedCallbackCnt, sizeof(UA_NodeId));
    ck_assert(pExpectedComponentCallbackIds != 0);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check normal pubsub operation");

    /* set all writer- and readergroups to operational (this triggers the publish and subscribe callback)
        enable the readers first, because otherwise we receive something immediately and start the
        message receive timeout.
        If we do some other checks before triggering the server_run_iterate function, this could
        cause a timeout. */
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;

    ExpectedCallbackCnt = 6;
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = RGId_Conn1_RG1;
    pExpectedComponentCallbackIds[2] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[3] = RGId_Conn2_RG1;
    /* TODO: 2nd datasetreader */
    pExpectedComponentCallbackIds[4] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[5] = WGId_Conn1_WG1;

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));

    /* check that callback has been called for writer and reader groups and datasets */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /* check that all dataset writers- and readers are operational */
    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR2, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check that publish/subscribe works (for all readers) -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 10, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 5, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);

    // ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 22, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);
    // ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 44, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 49, (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn1_WG1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));

    /* check that callback has been called for writer group and dataset */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check order and number of different message receive timeouts");

    ExpectedCallbackCnt = 2;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = DSRId_Conn2_RG1_DSR1;
    /* TODO: 2nd datasetreader */

    ServerDoProcess("1", (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);
    /* check that callback has been called for reader group and dataset */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "there should not be any additional timeouts");

    ServerDoProcess("2", (UA_UInt32) (PublishingInterval_Conn1_WG1), 20);
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_different_timeouts\n\n");
} END_TEST



/***************************************************************************************************/
/***************************************************************************************************/
/* Test wrong message receive timeout setting (receive timeout is smaller than publishing interval)*/

/***************************************************************************************************/
static void PubSubStateChangeCallback_wrong_timeout (
    UA_Server *hostServer,
    UA_NodeId *pubsubComponentId,
    UA_PubSubState state,
    UA_StatusCode status) {

    ck_assert(hostServer == server);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    if ((UA_NodeId_equal(pubsubComponentId, &ExpectedCallbackComponentNodeId) == UA_TRUE) &&
        (state == ExpectedCallbackStateChange) &&
        (status == ExpectedCallbackStatus)) {

        CallbackCnt++;
    }
}

/***************************************************************************************************/
START_TEST(Test_wrong_timeout) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_wrong_timeout");

    /*
        Connection 1: WG1 : DSW1    --> Connection 1: RG1 : DSR1
    */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_wrong_timeout;

    /* setup Connection 1 */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 500.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WGNo_Conn1_WG1, PublishingInterval_Conn1_WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1_DS1;
    UA_NodeId_init(&VarId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    UA_UInt32 DSWNo_Conn1_WG1 = 1;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSWNo_Conn1_WG1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1_DS1, &DsWId_Conn1_WG1_DS1);

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 200.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1,
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    /* expected order of pubsub component timeouts: */
    UA_NodeId_copy(&DSRId_Conn1_RG1_DSR1, &ExpectedCallbackComponentNodeId);
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set writer and reader to operational");

    /* set all writer- and readergroups to operational */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));

    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* timeout should happen after receipt of first message  */
    ServerDoProcess("1", (UA_UInt32) (MessageReceiveTimeout_Conn1_RG1_DSR1 + 100), 1);

    ck_assert_int_eq(1, CallbackCnt);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    ServerDoProcess("2", (UA_UInt32) PublishingInterval_Conn1_WG1, 1);
    ServerDoProcess("2", (UA_UInt32) 100, 1);

    /* now the reader should have received something and the state changes to operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ServerDoProcess("3", 300, 1);

    /* then there should have happened another timeout */
    ck_assert_int_eq(2, CallbackCnt);

    /* DataSetReader state toggles from error to operational, because it receives messages but always too late */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_wrong_timeout\n\n");
} END_TEST



/***************************************************************************************************/
/***************************************************************************************************/
/* Test a bigger configuration */

/***************************************************************************************************/
static void
PubSubStateChangeCallback_many_components(UA_Server *hostServer, UA_NodeId *pubsubComponentId,
                                          UA_PubSubState state, UA_StatusCode status) {
    ck_assert(hostServer == server);

    if(!runtime)
        return;

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    ck_assert(pExpectedComponentCallbackIds != 0);
    ck_assert(CallbackCnt < ExpectedCallbackCnt);

    UA_String_init(&strId);
    UA_NodeId_print(&(pExpectedComponentCallbackIds[CallbackCnt]), &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Expected Id (on timeout) = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    ck_assert(state == ExpectedCallbackStateChange);
    ck_assert(status == ExpectedCallbackStatus);
    if (ExpectedCallbackStateChange == UA_PUBSUBSTATE_ERROR) {
        /*  On error we want to verify the order of DataSetReader timeouts */
        ck_assert(UA_NodeId_equal(pubsubComponentId, &pExpectedComponentCallbackIds[CallbackCnt]) == UA_TRUE);
    } /* when the state is set back to operational we cannot verify the order of StateChanges, because we
            cannot know which DataSetReader will be operational first */
    CallbackCnt++;
}

/***************************************************************************************************/
START_TEST(Test_many_components) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_many_components");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    /*  Writers            : Interval  -> Readers            : Timeout
        ----------------------------------------------------------------------------------
        Conn 1 WG 1 - DSW1 : 30        -> Conn 2 RG 1 - DSR1 : 40
        Conn 1 WG 1 - DSW2 : 30        -> Conn 2 RG 2 - DSR1 : 45
        Conn 2 WG 1 - DSW1 : 20        -> Conn 1 RG 1 - DSR1 : 25
        Conn 2 WG 2 - DSW1 : 10        -> Conn 3 RG 1 - DSR1 : 25
    */

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_many_components;

    /* setup Connection 1: writers */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 30.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WGNo_Conn1_WG1, PublishingInterval_Conn1_WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1_DS1;
    UA_NodeId_init(&VarId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    UA_UInt32 DSWNo_Conn1_WG1_DS1 = 1;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSWNo_Conn1_WG1_DS1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1_DS1, &DsWId_Conn1_WG1_DS1);

    UA_NodeId DsWId_Conn1_WG1_DS2;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS2);
    UA_NodeId VarId_Conn1_WG1_DS2;
    UA_NodeId_init(&VarId_Conn1_WG1_DS2);
    UA_NodeId PDSId_Conn1_WG1_PDS2;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS2);
    UA_UInt32 DSWNo_Conn1_WG1_DS2 = 2;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS2", "Conn1_WG1_DS2", DSWNo_Conn1_WG1_DS2, &PDSId_Conn1_WG1_PDS2,
        &VarId_Conn1_WG1_DS2, &DsWId_Conn1_WG1_DS2);


    /* setup Connection 2: writers */
    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    UA_UInt32 PublisherNo_Conn2 = 2;
    AddConnection("Conn2", PublisherNo_Conn2, &ConnId_2);

    UA_NodeId WGId_Conn2_WG1;
    UA_NodeId_init(&WGId_Conn2_WG1);
    UA_UInt32 WGNo_Conn2_WG1 = 1;
    UA_Duration PublishingInterval_Conn2_WG1 = 20.0;
    AddWriterGroup(&ConnId_2, "Conn2_WG1", WGNo_Conn2_WG1, PublishingInterval_Conn2_WG1, &WGId_Conn2_WG1);

    UA_NodeId DsWId_Conn2_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG1_DS1);
    UA_NodeId VarId_Conn2_WG1_DS1;
    UA_NodeId_init(&VarId_Conn2_WG1_DS1);
    UA_NodeId PDSId_Conn2_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG1_PDS1);
    UA_UInt32 DSWNo_Conn2_WG1_DS1 = 1;
    AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS1", "Conn2_WG1_DS1", DSWNo_Conn2_WG1_DS1, &PDSId_Conn2_WG1_PDS1,
        &VarId_Conn2_WG1_DS1, &DsWId_Conn2_WG1_DS1);


    UA_NodeId WGId_Conn2_WG2;
    UA_NodeId_init(&WGId_Conn2_WG2);
    UA_UInt32 WGNo_Conn2_WG2 = 2;
    UA_Duration PublishingInterval_Conn2_WG2 = 10.0;
    AddWriterGroup(&ConnId_2, "Conn2_WG2", WGNo_Conn2_WG2, PublishingInterval_Conn2_WG2, &WGId_Conn2_WG2);

    UA_NodeId DsWId_Conn2_WG2_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG2_DS1);
    UA_NodeId VarId_Conn2_WG2_DS1;
    UA_NodeId_init(&VarId_Conn2_WG2_DS1);
    UA_NodeId PDSId_Conn2_WG2_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG2_PDS1);
    UA_UInt32 DSWNo_Conn2_WG2_DS1 = 1;
    AddPublishedDataSet(&WGId_Conn2_WG2, "Conn2_WG2_PDS1", "Conn2_WG2_DS1", DSWNo_Conn2_WG2_DS1, &PDSId_Conn2_WG2_PDS1,
        &VarId_Conn2_WG2_DS1, &DsWId_Conn2_WG2_DS1);

    /* setup Connection 1: readers */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 25.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn2, WGNo_Conn2_WG1, DSWNo_Conn2_WG1_DS1,
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);
    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn1_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn1_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

     /* setup Connection 2: readers */
    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);

    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    UA_NodeId VarId_Conn2_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn2_RG1_DSR1 = 40.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS1,
        MessageReceiveTimeout_Conn2_RG1_DSR1, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn2_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn2_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    UA_NodeId RGId_Conn2_RG2;
    UA_NodeId_init(&RGId_Conn2_RG2);
    AddReaderGroup(&ConnId_2, "Conn2_RG2", &RGId_Conn2_RG2);

    UA_NodeId DSRId_Conn2_RG2_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG2_DSR1);
    UA_NodeId VarId_Conn2_RG2_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG2_DSR1);
    UA_Duration MessageReceiveTimeout_Conn2_RG2_DSR1 = 45.0;
    AddDataSetReader(&RGId_Conn2_RG2, "Conn2_RG2_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS2,
        MessageReceiveTimeout_Conn2_RG2_DSR1, &VarId_Conn2_RG2_DSR1, &DSRId_Conn2_RG2_DSR1);
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn2_RG2_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn2_RG2_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    /* setup Connection 3: readers */
    UA_NodeId ConnId_3;
    UA_NodeId_init(&ConnId_3);
    UA_UInt32 PublisherNo_Conn3 = 3;
    AddConnection("Conn3", PublisherNo_Conn3, &ConnId_3);

    UA_NodeId RGId_Conn3_RG1;
    UA_NodeId_init(&RGId_Conn3_RG1);
    AddReaderGroup(&ConnId_3, "Conn3_RG1", &RGId_Conn3_RG1);

    UA_NodeId DSRId_Conn3_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn3_RG1_DSR1);
    UA_NodeId VarId_Conn3_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn3_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn3_RG1_DSR1 = 25.0;
    AddDataSetReader(&RGId_Conn3_RG1, "Conn3_RG1_DSR1", PublisherNo_Conn2, WGNo_Conn2_WG2, DSWNo_Conn2_WG2_DS1,
        MessageReceiveTimeout_Conn3_RG1_DSR1, &VarId_Conn3_RG1_DSR1, &DSRId_Conn3_RG1_DSR1);
    UA_String_init(&strId);
    UA_NodeId_print(&DSRId_Conn3_RG1_DSR1, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Conn3_RG1_DSR1 Id = %.*s", (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);

    const UA_UInt32 SleepTime = 5;
    const UA_UInt32 NoOfRunIterateCycles = 11;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set everything operational");

    /* check normal operation first -> there should not be any timeouts */
    ExpectedCallbackCnt = 15;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    pExpectedComponentCallbackIds = (UA_NodeId*) UA_calloc(ExpectedCallbackCnt, sizeof(UA_NodeId));
    ck_assert(pExpectedComponentCallbackIds != 0);
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = RGId_Conn1_RG1;
    pExpectedComponentCallbackIds[2] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[3] = RGId_Conn2_RG1;
    pExpectedComponentCallbackIds[4] = DSRId_Conn2_RG2_DSR1;
    pExpectedComponentCallbackIds[5] = RGId_Conn2_RG2;
    pExpectedComponentCallbackIds[6] = DSRId_Conn3_RG1_DSR1;
    pExpectedComponentCallbackIds[7] = RGId_Conn3_RG1;
    pExpectedComponentCallbackIds[8] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[9] = DsWId_Conn1_WG1_DS2;
    pExpectedComponentCallbackIds[10] = WGId_Conn1_WG1;
    pExpectedComponentCallbackIds[11] = DsWId_Conn2_WG1_DS1;
    pExpectedComponentCallbackIds[12] = WGId_Conn2_WG1;
    pExpectedComponentCallbackIds[13] = DsWId_Conn2_WG2_DS1;
    pExpectedComponentCallbackIds[14] = WGId_Conn2_WG2;

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /* check that publish/subscribe works -> set some test values */

    /* use a low enough sleep value to ensure that publishing intervals and message receive timeouts
        are handled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Conn2_RG1_DSR1");
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 10, SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Conn2_RG2_DSR1");
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS2, VarId_Conn2_RG2_DSR1, 99, SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Conn1_RG1_DSR1");
    ValidatePublishSubscribe(VarId_Conn2_WG1_DS1, VarId_Conn1_RG1_DSR1, 40, SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Conn3_RG1_DSR1");
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 123, SleepTime, NoOfRunIterateCycles);

    ck_assert_int_eq(0, CallbackCnt);

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST A");

    /* prepare expected pubsub components with message receive timeout */
    ExpectedCallbackCnt = 3;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = DsWId_Conn1_WG1_DS2;
    pExpectedComponentCallbackIds[2] = WGId_Conn1_WG1;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup: Conn 1 - WG 1");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_PubSubState state = UA_PUBSUBSTATE_OPERATIONAL;
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 2;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = DSRId_Conn2_RG2_DSR1;

    ServerDoProcess("A 1", SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreaders");

    /* state of Conn 2: ReaderGroup 1 should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* state of Conn 2: ReaderGroup 2 should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* Conn 1: RG 1 DatasetReader1 state shall still be operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn2_WG1_DS1, VarId_Conn1_RG1_DSR1, 99, SleepTime, NoOfRunIterateCycles);
    /* Conn 3: RG 1 DatasetReader1 state shall still be operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 118, SleepTime, NoOfRunIterateCycles);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /* enable the publisher WriterGroup again */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup Conn 1 - WG 1");
    CallbackCnt = 0;
    ExpectedCallbackCnt = 3;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = DsWId_Conn1_WG1_DS2;
    pExpectedComponentCallbackIds[2] = WGId_Conn1_WG1;

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 2;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    pExpectedComponentCallbackIds[1] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[2] = DSRId_Conn2_RG2_DSR1;

    ServerDoProcess("A 2", SleepTime, NoOfRunIterateCycles);

    /* DataSetReader state shall be back to operational, after receiving a new message */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST B");

    /* prepare expected pubsub component timeouts */
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn2_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn2_WG1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup: Conn 2 - WG 1");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;

    ServerDoProcess("B 1", (UA_UInt32) SleepTime, NoOfRunIterateCycles);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreaders");

    /* DatasetReader Conn1: RG 1 shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* other DataSetReaders shall be operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, SleepTime, NoOfRunIterateCycles);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 119, SleepTime, NoOfRunIterateCycles);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /* enable the publisher WriterGroup again */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup");
    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DsWId_Conn2_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn2_WG1;

    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;

    ServerDoProcess("B 1", SleepTime, NoOfRunIterateCycles);

    /* DataSetReader state shall be back to operational, after receiving a new message */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST C");

    /* prepare expected pubsub component timeouts */
    ExpectedCallbackCnt = 15;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = DsWId_Conn1_WG1_DS2;
    pExpectedComponentCallbackIds[2] = WGId_Conn1_WG1;
    pExpectedComponentCallbackIds[3] = DsWId_Conn2_WG1_DS1;
    pExpectedComponentCallbackIds[4] = WGId_Conn2_WG1;
    pExpectedComponentCallbackIds[5] = DsWId_Conn2_WG2_DS1;
    pExpectedComponentCallbackIds[6] = WGId_Conn2_WG2;
    pExpectedComponentCallbackIds[7] = DSRId_Conn1_RG1_DSR1;
    pExpectedComponentCallbackIds[8] = RGId_Conn1_RG1;
    pExpectedComponentCallbackIds[9] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[10] = RGId_Conn2_RG1;
    pExpectedComponentCallbackIds[11] = DSRId_Conn2_RG2_DSR1;
    pExpectedComponentCallbackIds[12] = RGId_Conn2_RG2;
    pExpectedComponentCallbackIds[13] = DSRId_Conn3_RG1_DSR1;
    pExpectedComponentCallbackIds[14] = RGId_Conn3_RG1;

    /* realign all publishers, so we can determine the order of timeouts */
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ServerDoProcess("C 1", SleepTime, NoOfRunIterateCycles);

    ExpectedCallbackCnt = 2;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    pExpectedComponentCallbackIds[0] = DsWId_Conn2_WG2_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn2_WG2;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable all writers");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    pExpectedComponentCallbackIds[0] = DSRId_Conn3_RG1_DSR1;

    ServerDoProcess("C 1", SleepTime, 6);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 2;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    pExpectedComponentCallbackIds[0] = DsWId_Conn2_WG1_DS1;
    pExpectedComponentCallbackIds[1] = WGId_Conn2_WG1;

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 1;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    pExpectedComponentCallbackIds[0] = DSRId_Conn1_RG1_DSR1;

    ServerDoProcess("C 1", SleepTime, 6);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 3;
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;
    pExpectedComponentCallbackIds[0] = DsWId_Conn1_WG1_DS1;
    pExpectedComponentCallbackIds[1] = DsWId_Conn1_WG1_DS2;
    pExpectedComponentCallbackIds[2] = WGId_Conn1_WG1;

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    ExpectedCallbackCnt = 2;
    pExpectedComponentCallbackIds[0] = DSRId_Conn2_RG1_DSR1;
    pExpectedComponentCallbackIds[1] = DSRId_Conn2_RG2_DSR1;

    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;

    ServerDoProcess("C 2", SleepTime, NoOfRunIterateCycles);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedCallbackCnt, CallbackCnt);
    CallbackCnt = 0;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreaders");

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_many_components\n\n");

} END_TEST


/***************************************************************************************************/
/***************************************************************************************************/
/* Test update DataSetReader configuration */

/***************************************************************************************************/
/* Custom PubSub statechange callback:
    count no of message receive timeouts
*/
static void
PubSubStateChangeCallback_update_config(UA_Server *hostServer, UA_NodeId *pubsubComponentId,
                                        UA_PubSubState state, UA_StatusCode status) {
    ck_assert(hostServer == server);

    if(!runtime)
        return;

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
                "Component Id = %.*s, state = %i, status = 0x%08x %s",
                (UA_Int32) strId.length, strId.data, state, status,
                UA_StatusCode_name(status));
    UA_String_clear(&strId);

    if (UA_NodeId_equal(pubsubComponentId, &ExpectedCallbackComponentNodeId) == UA_TRUE) {
        ck_assert(ExpectedCallbackStateChange == state);
        ck_assert(ExpectedCallbackStatus == status);
        CallbackCnt++;
    }
}

/***************************************************************************************************/
START_TEST(Test_update_config) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_update_config");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_update_config;
    CallbackCnt = 0;

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

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", 1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1, &DsWId_Conn1_WG1_DS1);

    /* setup corresponding readergroup and reader for Connection 1 */

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout = 200.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    UA_NodeId_copy(&DSRId_Conn1_RG1_DSR1, &ExpectedCallbackComponentNodeId);
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;

    const UA_UInt32 SleepTime = 50;
    const UA_UInt32 NoOfRunIterateCycles = 6;

    UA_PubSubState state;
    /* set WriterGroup operational */
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("1", SleepTime, NoOfRunIterateCycles);

    /* set ReaderGroup operational */
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert(CallbackCnt == 1);
    CallbackCnt = 0;

    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 10, SleepTime, NoOfRunIterateCycles);
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 33, SleepTime, NoOfRunIterateCycles);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* there should not be a callback notification for MessageReceiveTimeout */
    ck_assert(CallbackCnt == 0);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writer group");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("2", SleepTime, NoOfRunIterateCycles);

    /* check number of timeouts */
    ck_assert_int_eq(1, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writer group");
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("3", SleepTime, NoOfRunIterateCycles);

    /* check number of state changes */
    ck_assert_int_eq(2, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "update reader config");
    UA_DataSetReaderConfig ReaderConfig;
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_DataSetReader_getConfig(server, DSRId_Conn1_RG1_DSR1, &ReaderConfig));
    ReaderConfig.messageReceiveTimeout = 1000;
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_DataSetReader_updateConfig(server, DSRId_Conn1_RG1_DSR1, RGId_Conn1_RG1, &ReaderConfig));
    UA_DataSetReaderConfig_clear(&ReaderConfig);

    ServerDoProcess("4", SleepTime, NoOfRunIterateCycles);

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 50, SleepTime, NoOfRunIterateCycles);

    ck_assert_int_eq(2, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writer group");
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("5", SleepTime, NoOfRunIterateCycles);

    /* Message ReceiveTimeout is higher, so there should not have been a timeout yet */
    ck_assert_int_eq(2, CallbackCnt);

    ServerDoProcess("5", SleepTime, NoOfRunIterateCycles * 5);

    ck_assert_int_eq(3, CallbackCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_update_config\n\n");

} END_TEST


/***************************************************************************************************/
START_TEST(Test_add_remove) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_add_remove");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

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
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    /* check for memory leaks */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_removeDataSetReader(server, DSRId_Conn1_RG1_DSR1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_add_remove\n\n");

} END_TEST


/***************************************************************************************************/
static void PubSubStateChangeCallback_fast_path (UA_Server *hostServer, UA_NodeId *pubsubComponentId,
                                UA_PubSubState state,
                                UA_StatusCode status) {
    ck_assert(hostServer == server);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback_fast_path(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    if (UA_NodeId_equal(pubsubComponentId, &ExpectedCallbackComponentNodeId) == UA_TRUE) {
        ck_assert(ExpectedCallbackStateChange == state);
        ck_assert(ExpectedCallbackStatus == status);
        CallbackCnt++;
    }
}

/***************************************************************************************************/
/* simple test with 2 connections: 1 DataSetWriter and 1 DataSetReader */
START_TEST(Test_fast_path) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_fast_path");

    UseFastPath = UA_TRUE;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubSubConfig.stateChangeCallback = PubSubStateChangeCallback_fast_path;

    /* Connection 1: Writer 1  --> Connection 2: Reader 1 */

    /* setup Connection 1: 1 writergroup, 1 writer */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", 1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_Duration PublishingInterval_Conn1WG1 = 300.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", 1, PublishingInterval_Conn1WG1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1;
    UA_NodeId_init(&VarId_Conn1_WG1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", 1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1, &DsWId_Conn1_WG1_DS1);

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
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    UA_PubSubState state;
    /* check WriterGroup and DataSetWriter state */
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* freeze WriterGroup and set it operational */
    ck_assert(UA_Server_freezeWriterGroupConfiguration(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    ServerDoProcess("0", (UA_UInt32) (PublishingInterval_Conn1WG1), 3);

    /* there should not be a MessageReceiveTimeout, writers are running, readers are still disabled  */
    ck_assert(CallbackCnt == 0);

    /* check ReaderGroup and DataSetReader state */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* freeze ReaderGroup and set it operational */
    UA_NodeId_copy(&DSRId_Conn2_RG1_DSR1, &ExpectedCallbackComponentNodeId);
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;

    ck_assert(UA_Server_freezeReaderGroupConfiguration(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    /* check that PubSubStateChange callback has been called for the specific DataSetReader */
    ck_assert_int_eq(1, CallbackCnt);
    CallbackCnt = 0;

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe_fast_path(10, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ValidatePublishSubscribe_fast_path(33, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ValidatePublishSubscribe_fast_path(44, (UA_UInt32) PublishingInterval_Conn1WG1, 3);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* there should not be a callback notification for MessageReceiveTimeout */
    ck_assert(CallbackCnt == 0);

    /* now we disable the publisher WriterGroup and check if a MessageReceiveTimeout occurs at Subscriber */
    UA_NodeId_copy(&DSRId_Conn2_RG1_DSR1, &ExpectedCallbackComponentNodeId);
    ExpectedCallbackStatus = UA_STATUSCODE_BADTIMEOUT;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_ERROR;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    ServerDoProcess("1", (UA_UInt32) (PublishingInterval_Conn1WG1), 3);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreader");

    /* state of ReaderGroup should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* check that PubSubStateChange callback has been called for the specific DataSetReader */
    ck_assert_int_eq(1, CallbackCnt);

    /* enable the publisher WriterGroup again */
    /* DataSetReader state shall be back to operational after receiving a new message */
    ExpectedCallbackStatus = UA_STATUSCODE_GOOD;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_OPERATIONAL;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ServerDoProcess("2", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq(2, CallbackCnt);

    ServerDoProcess("3", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(2, CallbackCnt);

    /* now we disable the reader */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable readergroup. writergroup is still working");
    ExpectedCallbackStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    ExpectedCallbackStateChange = UA_PUBSUBSTATE_DISABLED;

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    /* check number of state changes */
    ck_assert_int_eq(3, CallbackCnt);

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    ServerDoProcess("4", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    /* then we disable the writer -> no timeout shall occur, because the reader is disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    ServerDoProcess("5", (UA_UInt32) (PublishingInterval_Conn1WG1), 4);

    ck_assert(UA_Server_unfreezeReaderGroupConfiguration(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_DataValue_clear(pFastPathPublisherValue);
    UA_DataValue_delete(pFastPathPublisherValue);
    UA_DataValue_clear(pFastPathSubscriberValue);
    UA_DataValue_delete(pFastPathSubscriberValue);

    UseFastPath = UA_FALSE;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_fast_path\n\n");

} END_TEST


/***************************************************************************************************/
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
        - configure multiple connections with multiple readers and writers
        - disable/enable and check for correct timeouts
    */
    tcase_add_test(tc_basic, Test_many_components);

    /* test case description:
        - configure a connection with writer and reader
        - try different updated configurations for the DataSetReader (different MessageReceiveTimeouts)
    */
    tcase_add_test(tc_basic, Test_update_config);

    /* test case description:
        - add and remove a reader without any operation -> check for memory leaks
    */
    tcase_add_test(tc_basic, Test_add_remove);

    /* test case description:
        - test message receive timeout with fast-path
    */
    tcase_add_test(tc_basic, Test_fast_path);

    Suite *s = suite_create("PubSub timeout test suite: message receive timeout");
    suite_add_tcase(s, tc_basic);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

