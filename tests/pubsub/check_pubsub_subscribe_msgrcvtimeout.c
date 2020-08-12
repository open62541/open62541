#include <check.h>
#include <assert.h>

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "ua_pubsub.h"

static UA_Server *server = NULL;

/* global variables to check timeout callback order of execution and component Ids */
static UA_NodeId ExpectedCallbackComponentNodeId;
static UA_UInt32 MsgRcvTimeoutCallbackTriggerCnt = 0;

static UA_UInt32 ExpectedNoOfTimeouts = 0;
static UA_NodeId *pExpectedTimeoutIds = 0;


/***************************************************************************************************/
/* Note: unit test-plugins can't be used because PubSub does not work with testing_networklayers, 
that's why we have copied the implementation for (UA_comboSleep) here */
#ifdef _WIN32
#include <time.h>
void UA_comboSleep(unsigned long duration) {
    Sleep(duration);
}
#else
#define NANO_SECOND_MULTIPLIER 1000000

/* Note: defining '_POSIX_C_SOURCE >= 199309L' within c code here does not work
that's why we have added it to the CMakeLists.txt */
#include <time.h>

void UA_comboSleep(unsigned long duration) {
    unsigned long sec = duration / 1000;
    unsigned long ns = (duration % 1000) * NANO_SECOND_MULTIPLIER;
    struct timespec sleepValue;
    sleepValue.tv_sec = sec;
    sleepValue.tv_nsec = ns;
    nanosleep(&sleepValue, NULL);
}
#endif 
/***************************************************************************************************/

/***************************************************************************************************/
/* TODO: we should use thread_wrapper.h but 
    usage of the testing-plugins does not work for this test */

#ifndef WIN32
#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define THREAD_CREATE(handle, callback) pthread_create(&handle, NULL, callback, NULL)
#define THREAD_JOIN(handle) pthread_join(handle, NULL)
#define THREAD_CALLBACK(name) static void * name(void *_)
#else
#include <windows.h>
#define THREAD_HANDLE HANDLE
#define THREAD_CREATE(handle, callback) { handle = CreateThread( NULL, 0, callback, NULL, 0, NULL); }
#define THREAD_JOIN(handle) WaitForSingleObject(handle, INFINITE)
#define THREAD_CALLBACK(name) static DWORD WINAPI name( LPVOID lpParam )
#endif

/* server thread  */
THREAD_HANDLE ServerThread = 0;
static volatile UA_Boolean ServerRunning = true;

THREAD_CALLBACK(ServerLoop) {
    while (ServerRunning)
        UA_Server_run_iterate(server, true);
    return 0;
}

/***************************************************************************************************/
/***************************************************************************************************/

/***************************************************************************************************/
/***************************************************************************************************/
static void setup(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nsetup\n\n");

    UA_NodeId_init(&ExpectedCallbackComponentNodeId);
    MsgRcvTimeoutCallbackTriggerCnt = 0;
    ExpectedNoOfTimeouts = 0;
    pExpectedTimeoutIds = 0;

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
        UA_malloc(sizeof(UA_PubSubTransportLayer));
    assert(config->pubsubTransportLayers != 0);
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;

    UA_Server_run_startup(server);
    ServerRunning = UA_TRUE;
    ServerThread = 0;
    THREAD_CREATE(ServerThread, ServerLoop);
    assert(ServerThread != 0);
}

/***************************************************************************************************/
static void teardown(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nteardown\n\n");

    ServerRunning = false;
    THREAD_JOIN(ServerThread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    UA_NodeId_clear(&ExpectedCallbackComponentNodeId);

    if (pExpectedTimeoutIds != 0) {
        UA_free(pExpectedTimeoutIds);
        pExpectedTimeoutIds = 0;
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

    assert(pName != 0);
    assert(opConnectionId != 0);

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

    assert(pConnectionId != 0);
    assert(pName != 0);
    assert(opWriterGroupId != 0);

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

    assert(pWriterGroupId != 0);
    assert(pPublishedDataSetName != 0);
    assert(pDataSetWriterName != 0);
    assert(opPublishedDataSetId != 0);
    assert(opPublishedVarId != 0);
    assert(opDataSetWriterId != 0);

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

    assert(pConnectionId != 0);
    assert(pName != 0);
    assert(opReaderGroupId != 0);

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING(pName);
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

    assert(pReaderGroupId != 0);
    assert(pName != 0);
    assert(opSubscriberVarId != 0);
    assert(opDataSetReaderId != 0);

    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING(pName);
    UA_Variant_setScalar(&readerConfig.publisherId, (UA_UInt16*) &PublisherId, &UA_TYPES[UA_TYPES_UINT16]);
    readerConfig.writerGroupId    = (UA_UInt16) WriterGroupId;
    readerConfig.dataSetWriterId  = (UA_UInt16) DataSetWriterId;
    readerConfig.messageReceiveTimeout = MessageReceiveTimeout;

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

    UA_FieldTargetVariable *pTargetVariables =  (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    assert(pTargetVariables != 0);
    
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
/* utility function to check working pubsub operation */
static void ValidatePublishSubscribe(
    UA_NodeId PublishedVarId,
    UA_NodeId SubscribedVarId,
    UA_Int32 TestValue,
    UA_UInt32 Sleep_ms) /* use a sleep time to ensure that at least one publish callback can be 
    executed -> as we are asynchronous to the publish callback it's recommended to use
    2 times the configured publishing interval */
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): set variable to publish");

    /* set variable value to publish */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Variant writeValue;
    UA_Variant_setScalar(&writeValue, &TestValue, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_writeValue(server, PublishedVarId, writeValue) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): sleep");
    UA_sleep_ms(Sleep_ms);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): wakeup");

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
/***************************************************************************************************/

/***************************************************************************************************/
/* Custom PubSub statechange callback:
    we only check for a MessageReceiveTimeout callback for a specific DataSetReader 
    --> a MessageReceiveTimeout callback sets 
            the DataSetReader state to error with status BadTimeout*/
static void PubSubStateChangeCallback_basic (UA_NodeId *pubsubComponentId,
                                UA_PubSubState state,
                                UA_StatusCode status) {
    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    if ((UA_NodeId_equal(pubsubComponentId, &ExpectedCallbackComponentNodeId) == UA_TRUE) && 
        (state == UA_PUBSUBSTATE_ERROR) &&
        (status == UA_STATUSCODE_BADTIMEOUT)) {
        
        MsgRcvTimeoutCallbackTriggerCnt++;
    }
}

/***************************************************************************************************/
/* simple test with 2 connections: 1 DataSetWriter and 1 DataSetReader */
START_TEST(Test_basic) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_basic");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubsubConfiguration.pubsubStateChangeCallback = PubSubStateChangeCallback_basic;

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
    UA_Duration MessageReceiveTimeout = 150.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    UA_PubSubState state;    
    /* check WriterGroup and DataSetWriter state */
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* set WriterGroup operational */
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 0");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1WG1 + MessageReceiveTimeout));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 0");

    /* there should not be a MessageReceiveTimeout, writers are running, readers are still disabled  */
    ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 0);

    /* check ReaderGroup and DataSetReader state */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    /* set ReaderGroup operational */
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 10, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 33, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));
    
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* there should not be a callback notification for MessageReceiveTimeout */
    ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 0);

    /* now we disable the publisher WriterGroup and check if a MessageReceiveTimeout occurs at Subscriber */
    /* we can check 
        the state of the DataSetReader, which should be ERROR and 
        if the MessageReceiveTimeout callback has been called */
    UA_NodeId_copy(&DSRId_Conn2_RG1_DSR1, &ExpectedCallbackComponentNodeId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 1");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1WG1 + MessageReceiveTimeout));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 1");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreader");

    /* state of ReaderGroup should still be ok */
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
     /* but DataSetReader state shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* check that PubSubStateChange callback has been called for the specific DataSetReader */
    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);
    
    /* enable the publisher WriterGroup again */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 2");
    UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 2");

    /* DataSetReader state shall be back to operational, after receiving a new message */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 3");
    UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 3");
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    /* now we disable the reader */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable readergroup. writergroup is still working");
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 4");
    // UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 4");

    // /* then we disable the writer -> no timeout shall occur, because the reader is disabled */
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    // ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    // ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_DISABLED);
    // ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 5");
    // UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 5");

    // /* enable readergroup -> nothing happens, because there's no writer, timeout is only checked after first received message */
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable readergroup");
    // ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    // ck_assert(UA_Server_ReaderGroup_getState(server, RGId_Conn2_RG1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 6");
    // UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 6");

    // /* reader state shall still be operational */
    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 7");
    // UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 7");

    // /* PubSubStateChange callback must not have been triggered */
    // ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 1);

    // /* enable writergroup again -> now normal MessageReceiveTimeout check shall start again */
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup again");
    // ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 8");
    // UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1WG1 * 4));
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 8");

    // ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    // ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    // /* PubSubStateChange callback must not have been triggered */
    // ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 1);

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_basic\n\n");
} END_TEST

/***************************************************************************************************/
/***************************************************************************************************/
/* Test different message receive timeouts */

/***************************************************************************************************/
/* Custom PubSub statechange callback:
    check order of execution for datasetreader components 
*/
static void PubSubStateChangeCallback_different_timeouts (
    UA_NodeId *pubsubComponentId,
    UA_PubSubState state,
    UA_StatusCode status) {

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    ck_assert(MsgRcvTimeoutCallbackTriggerCnt <= ExpectedNoOfTimeouts);

    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(status == UA_STATUSCODE_BADTIMEOUT);

    assert(pExpectedTimeoutIds != 0);
    ck_assert(UA_NodeId_equal(pubsubComponentId, &pExpectedTimeoutIds[MsgRcvTimeoutCallbackTriggerCnt]) == UA_TRUE);
    MsgRcvTimeoutCallbackTriggerCnt++;
}


/* TODO: test does not work if we add the same reader on the same connection
    maybe only 1 reader per connection receives the data ... ??
    or the second reader overwrites the first? */

START_TEST(Test_different_timeouts) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_different_timeouts");

    /* 
        Connection 1: WG1 : DSW1 (pub interval = 100)   --> Connection 1: RG1 : DSR1 (msgrcvtimeout = 120)
                                                        --> Connection 1: RG1 : DSR2 (msgrcvtimeout = 130)
                                                        --> Connection 2: RG1 : DSR1 (msgrcvtimeout = 150)
    */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubsubConfiguration.pubsubStateChangeCallback = PubSubStateChangeCallback_different_timeouts;

    /* setup Connection 1 */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 100.0;
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
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 120.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, 
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    // // add the same reader on the same connection again -> but with different message receive timeout
    // UA_NodeId DSRId_Conn1_RG1_DSR2;
    // UA_NodeId_init(&DSRId_Conn1_RG1_DSR2);
    // UA_NodeId VarId_Conn1_RG1_DSR2;
    // UA_NodeId_init(&VarId_Conn1_RG1_DSR2);
    // UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR2 = 130.0;
    // AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR2", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, 
    //     MessageReceiveTimeout_Conn1_RG1_DSR2, &VarId_Conn1_RG1_DSR2, &DSRId_Conn1_RG1_DSR2);

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
    UA_Duration MessageReceiveTimeout_Conn2_RG1_DSR1 = 150.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, MessageReceiveTimeout_Conn2_RG1_DSR1, 
        &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    /* prepare expected order of pubsub component timeouts: */
    ExpectedNoOfTimeouts = 2;
    pExpectedTimeoutIds = (UA_NodeId*) UA_calloc(ExpectedNoOfTimeouts, sizeof(UA_NodeId));
    assert(pExpectedTimeoutIds != 0);
    pExpectedTimeoutIds[0] = DSRId_Conn1_RG1_DSR1;
    pExpectedTimeoutIds[1] = DSRId_Conn2_RG1_DSR1;
    /* TODO: 3rd datasetreader */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check normal pubsub operation");

    /* set all writer- and readergroups to operational */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1));

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
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 10, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 5, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));

    // ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 22, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));
    // ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 44, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 49, (UA_UInt32) (3 * PublishingInterval_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup");
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check order and number of different message receive timeouts");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 1");
    UA_sleep_ms((UA_UInt32) (3 * PublishingInterval_Conn1_WG1 + MessageReceiveTimeout_Conn2_RG1_DSR1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 1");

    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 2");
    UA_sleep_ms((UA_UInt32) (5 * PublishingInterval_Conn1_WG1 + 3 * MessageReceiveTimeout_Conn2_RG1_DSR1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 2");

    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_different_timeouts\n\n");
} END_TEST


/***************************************************************************************************/
/***************************************************************************************************/
/* Test wrong message receive timeout setting (receive timeout is smaller than publishing interval)*/

/***************************************************************************************************/
/* Custom PubSub statechange callback:
    count no of message receive timeouts
*/
static void PubSubStateChangeCallback_wrong_timeout (
    UA_NodeId *pubsubComponentId,
    UA_PubSubState state,
    UA_StatusCode status) {

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    if ((UA_NodeId_equal(pubsubComponentId, &ExpectedCallbackComponentNodeId) == UA_TRUE) && 
        (state == UA_PUBSUBSTATE_ERROR) &&
        (status == UA_STATUSCODE_BADTIMEOUT)) {
        
        MsgRcvTimeoutCallbackTriggerCnt++;
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
    config->pubsubConfiguration.pubsubStateChangeCallback = PubSubStateChangeCallback_wrong_timeout;

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

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set writer and reader to operational");

    /* set all writer- and readergroups to operational */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));

    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    ck_assert(UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* timeout should happen immediately after receipt of first message 
        depending on the async start of the Publisher there can be 1 or 2 timeouts, 
        and the state of the DataSetReader could be operational again, 
            e.g. after receiving another message, which should trigger another timeout */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 1");
    UA_sleep_ms((UA_UInt32) 300);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 1");

    ck_assert_int_eq(MsgRcvTimeoutCallbackTriggerCnt, 1);
    ExpectedNoOfTimeouts = MsgRcvTimeoutCallbackTriggerCnt;

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 2");
    UA_sleep_ms((UA_UInt32) 300);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 2");

    /* now the reader should have received something and the state changes to operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 3");
    UA_sleep_ms((UA_UInt32) 300);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 3");

    /* there should have happened another timeout */
    ck_assert_int_eq(MsgRcvTimeoutCallbackTriggerCnt, 2);

    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_wrong_timeout\n\n");
} END_TEST



/***************************************************************************************************/
/***************************************************************************************************/
/* Test a huge configuration (many pubsub components) */


/***************************************************************************************************/
/* Custom PubSub statechange callback:
    count no of message receive timeouts
*/
static void PubSubStateChangeCallback_many_components (
    UA_NodeId *pubsubComponentId,
    UA_PubSubState state,
    UA_StatusCode status) {

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): MsgRcvTimeoutCallbackTriggerCnt = %u (before increment)", MsgRcvTimeoutCallbackTriggerCnt);

    ck_assert(MsgRcvTimeoutCallbackTriggerCnt <= ExpectedNoOfTimeouts);

    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(status == UA_STATUSCODE_BADTIMEOUT);

    assert(pExpectedTimeoutIds != 0);
    ck_assert(UA_NodeId_equal(pubsubComponentId, &pExpectedTimeoutIds[MsgRcvTimeoutCallbackTriggerCnt]) == UA_TRUE);
    MsgRcvTimeoutCallbackTriggerCnt++;
}

/***************************************************************************************************/
START_TEST(Test_many_components) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_many_components");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubsubConfiguration.pubsubStateChangeCallback = PubSubStateChangeCallback_many_components;

    /* setup Connection 1: writers */
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
    UA_Duration PublishingInterval_Conn2_WG1 = 250.0;
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
    UA_Duration PublishingInterval_Conn2_WG2 = 100.0;
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
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 350.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn2, WGNo_Conn2_WG1, DSWNo_Conn2_WG1_DS1, 
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

     /* setup Connection 2: readers */
    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);

    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    UA_NodeId VarId_Conn2_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn2_RG1_DSR1 = 750.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS1, 
        MessageReceiveTimeout_Conn2_RG1_DSR1, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    UA_NodeId RGId_Conn2_RG2;
    UA_NodeId_init(&RGId_Conn2_RG2);
    AddReaderGroup(&ConnId_2, "Conn2_RG2", &RGId_Conn2_RG2);

    UA_NodeId DSRId_Conn2_RG2_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG2_DSR1);
    UA_NodeId VarId_Conn2_RG2_DSR1;
    UA_NodeId_init(&VarId_Conn2_RG2_DSR1);
    UA_Duration MessageReceiveTimeout_Conn2_RG2_DSR1 = 850.0;
    AddDataSetReader(&RGId_Conn2_RG2, "Conn2_RG2_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS2, 
        MessageReceiveTimeout_Conn2_RG2_DSR1, &VarId_Conn2_RG2_DSR1, &DSRId_Conn2_RG2_DSR1);

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
    UA_Duration MessageReceiveTimeout_Conn3_RG1_DSR1 = 150.0;
    AddDataSetReader(&RGId_Conn3_RG1, "Conn3_RG1_DSR1", PublisherNo_Conn2, WGNo_Conn2_WG2, DSWNo_Conn2_WG2_DS1, 
        MessageReceiveTimeout_Conn3_RG1_DSR1, &VarId_Conn3_RG1_DSR1, &DSRId_Conn3_RG1_DSR1);

    /* prepare expected PubSub component timeouts */
    ExpectedNoOfTimeouts = 2;
    pExpectedTimeoutIds = (UA_NodeId*) UA_calloc(ExpectedNoOfTimeouts, sizeof(UA_NodeId));
    assert(pExpectedTimeoutIds != 0);
    pExpectedTimeoutIds[0] = DSRId_Conn2_RG1_DSR1;
    pExpectedTimeoutIds[1] = DSRId_Conn2_RG2_DSR1;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set everything operational");

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);

    /* check that publish/subscribe works -> set some test values */

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 10, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 33, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS2, VarId_Conn2_RG2_DSR1, 99, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS2, VarId_Conn2_RG2_DSR1, 100, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));

    ValidatePublishSubscribe(VarId_Conn2_WG1_DS1, VarId_Conn1_RG1_DSR1, 40, (UA_UInt32) (PublishingInterval_Conn2_WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn2_WG1_DS1, VarId_Conn1_RG1_DSR1, 50, (UA_UInt32) (PublishingInterval_Conn2_WG1 * 3));

    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 123, (UA_UInt32) (PublishingInterval_Conn2_WG2 * 3));
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 124, (UA_UInt32) (PublishingInterval_Conn2_WG2 * 3));

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST A");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup: Conn 1 - WG 1");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_PubSubState state = UA_PUBSUBSTATE_OPERATIONAL;
    ck_assert(UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_DISABLED);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep A 1");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1_WG1 + MessageReceiveTimeout_Conn2_RG2_DSR1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup A 1");

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
    ValidatePublishSubscribe(VarId_Conn2_WG1_DS1, VarId_Conn1_RG1_DSR1, 99, (UA_UInt32) (PublishingInterval_Conn2_WG1 * 3));
    /* Conn 3: RG 1 DatasetReader1 state shall still be operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 118, (UA_UInt32) (PublishingInterval_Conn2_WG2 * 3));

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);
    
    /* enable the publisher WriterGroup again */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup Conn 1 - WG 1");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep A 2");
    UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn1_WG1 * 4));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup A 2");

    /* DataSetReader state shall be back to operational, after receiving a new message */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* PubSubStateChange callback must not have been triggered again */
    ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 2);

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST B");

    /* prepare expected pubsub component timeouts */
    ExpectedNoOfTimeouts = 1;
    pExpectedTimeoutIds[0] = DSRId_Conn1_RG1_DSR1;
    MsgRcvTimeoutCallbackTriggerCnt = 0;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writergroup: Conn 2 - WG 1");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep B 1");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn2_WG1 + MessageReceiveTimeout_Conn1_RG1_DSR1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup B 1");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreaders");

    /* DatasetReader Conn1: RG 1 shall be error */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* other DataSetReaders shall be operational */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS2, VarId_Conn2_RG2_DSR1, 48, (UA_UInt32) (PublishingInterval_Conn1_WG1 * 3));
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);
    ValidatePublishSubscribe(VarId_Conn2_WG2_DS1, VarId_Conn3_RG1_DSR1, 119, (UA_UInt32) (PublishingInterval_Conn2_WG2 * 3));
    
    /* check number of timeouts */
    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);
    
    /* enable the publisher WriterGroup again */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writergroup");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep B 2");
    UA_sleep_ms((UA_UInt32) (PublishingInterval_Conn2_WG1 * 4));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup B 2");

    /* DataSetReader state shall be back to operational, after receiving a new message */
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* PubSubStateChange callback must not have been triggered again */
    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);

    /***************************************************************************************************/
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "TEST C");

    /* prepare expected pubsub component timeouts */
    UA_free(pExpectedTimeoutIds);

    ExpectedNoOfTimeouts = 4;
    pExpectedTimeoutIds = (UA_NodeId*) UA_calloc(ExpectedNoOfTimeouts, sizeof(UA_NodeId));
    assert(pExpectedTimeoutIds != 0);
    pExpectedTimeoutIds[0] = DSRId_Conn3_RG1_DSR1;
    pExpectedTimeoutIds[1] = DSRId_Conn1_RG1_DSR1;
    pExpectedTimeoutIds[2] = DSRId_Conn2_RG1_DSR1;
    pExpectedTimeoutIds[3] = DSRId_Conn2_RG2_DSR1;

    MsgRcvTimeoutCallbackTriggerCnt = 0;

    /* realign all publishers, so we can determine the order of timeouts */
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep C 0");
    UA_sleep_ms((UA_UInt32) (1000));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup C 0");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable all writers");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep C 1");
    UA_sleep_ms((UA_UInt32) (2000));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup C 1");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state of datasetreaders");

    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn2_RG2_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn3_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_ERROR);

    /* check number of timeouts */
    ck_assert_int_eq(ExpectedNoOfTimeouts, MsgRcvTimeoutCallbackTriggerCnt);

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_many_components\n\n");

} END_TEST


/***************************************************************************************************/
/***************************************************************************************************/
/* Test update DataSetReader configuration */


/***************************************************************************************************/
/* Custom PubSub statechange callback:
    count no of message receive timeouts
*/
static void PubSubStateChangeCallback_update_config (
    UA_NodeId *pubsubComponentId,
    UA_PubSubState state,
    UA_StatusCode status) {

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSubStateChangeCallback(): "
        "Component Id = %.*s, state = %i, status = 0x%08x %s", (UA_Int32) strId.length, strId.data, state, status, UA_StatusCode_name(status));
    UA_String_clear(&strId);

    MsgRcvTimeoutCallbackTriggerCnt++;
}

/***************************************************************************************************/
START_TEST(Test_update_config) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_update_config");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* set custom callback triggered for specific PubSub state changes */
    config->pubsubConfiguration.pubsubStateChangeCallback = PubSubStateChangeCallback_update_config;
    MsgRcvTimeoutCallbackTriggerCnt = 0;

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

    UA_PubSubState state;    
    /* set WriterGroup operational */
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 0");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1WG1 + MessageReceiveTimeout));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 0");

    /* set ReaderGroup operational */
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 10, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 33, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));
    
    ck_assert(UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state) == UA_STATUSCODE_GOOD);
    ck_assert(state == UA_PUBSUBSTATE_OPERATIONAL);

    /* there should not be a callback notification for MessageReceiveTimeout */
    ck_assert(MsgRcvTimeoutCallbackTriggerCnt == 0);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writer group");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 0");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1WG1 + MessageReceiveTimeout));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 0");

    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "enable writer group");
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 1");
    UA_sleep_ms((UA_UInt32) (3 * PublishingInterval_Conn1WG1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 1");

    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "update reader config");
    UA_DataSetReaderConfig ReaderConfig;
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_DataSetReader_getConfig(server, DSRId_Conn1_RG1_DSR1, &ReaderConfig));
    ReaderConfig.messageReceiveTimeout = 1000;
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_DataSetReader_updateConfig(server, DSRId_Conn1_RG1_DSR1, RGId_Conn1_RG1, &ReaderConfig));
    UA_DataSetReaderConfig_clear(&ReaderConfig);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 2");
    UA_sleep_ms((UA_UInt32) (3 * PublishingInterval_Conn1WG1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 2");

    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn1_RG1_DSR1, 50, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));

    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable writer group");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 3");
    UA_sleep_ms((UA_UInt32) (2 * PublishingInterval_Conn1WG1));
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 3");

    /* Message ReceiveTimeout is higher, so there should not have been a timeout yet */
    ck_assert_int_eq(1, MsgRcvTimeoutCallbackTriggerCnt);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "sleep 3");
    UA_sleep_ms((UA_UInt32) 1000);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "wakeup 3");

    ck_assert_int_eq(2, MsgRcvTimeoutCallbackTriggerCnt);

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_update_config\n\n");

} END_TEST


/***************************************************************************************************/
int main(void) {

    /* test case description: 
        - check normal pubsub operation (2 connections)
        - 1 Connection with 1 DataSetWriter, 1 Connection with counterpart DataSetReader
        - enable/disable writer- and readergroup multiple times
        - check message receive timeout
    */
    TCase *tc_basic = tcase_create("basic");
    tcase_add_checked_fixture(tc_basic, setup, teardown);
    tcase_add_test(tc_basic, Test_basic);

    /* test case description: 
        - 1 DataSetWriter
        - multiple DataSetReaders with different timeout settings
        - check order and no of message receive timeouts for the different DataSetReaders
    */
    TCase *tc_different_timeouts = tcase_create("different timeouts");
    tcase_add_checked_fixture(tc_different_timeouts, setup, teardown);
    tcase_add_test(tc_different_timeouts, Test_different_timeouts);

    /* test case description: 
        - 1 Connection, 1 DataSetWriter, 1 DataSetReader
        - reader with wrong timeout setting (timeout is smaller than publishing interval)
    */
    TCase *tc_wrong_timeout = tcase_create("wrong timeout setting");
    tcase_add_checked_fixture(tc_wrong_timeout, setup, teardown);
    tcase_add_test(tc_wrong_timeout, Test_wrong_timeout);

    /* test case description:
        - configure multiple connections with multiple readers and writers
        - disable/enable and check for correct timeouts 
    */
    TCase *tc_many_components = tcase_create("many components");
    tcase_add_checked_fixture(tc_many_components, setup, teardown);
    tcase_add_test(tc_many_components, Test_many_components);

    /* test case description:
        - configure a connection with writer and reader
        - try different updated configurations for the DataSetReader (different MessageReceiveTimeouts)
    */
    TCase *tc_update_config = tcase_create("update config");
    tcase_add_checked_fixture(tc_update_config, setup, teardown);
    tcase_add_test(tc_update_config, Test_update_config);

    Suite *s = suite_create("PubSub timeout test suite: message receive timeout");
    suite_add_tcase(s, tc_basic);
    suite_add_tcase(s, tc_different_timeouts);
    suite_add_tcase(s, tc_wrong_timeout);
    suite_add_tcase(s, tc_many_components);
    suite_add_tcase(s, tc_update_config);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

