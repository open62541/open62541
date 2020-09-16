#include <check.h>
#include <assert.h>

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "ua_pubsub.h"
#include "open62541/server.h"
#include "server/ua_server_internal.h"

#include "ua_timer.h"

static UA_Server *server = NULL;

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


#define NO_OF_TIMERS 1000
static UA_UInt64 sTimerIds[NO_OF_TIMERS];


/***************************************************************************************************/
/***************************************************************************************************/

/***************************************************************************************************/
/***************************************************************************************************/
static void setup(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nsetup\n\n");

    memset(sTimerIds, 0, NO_OF_TIMERS * sizeof(sTimerIds[0]));

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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\n\nValidatePublishSubscribe(): set variable to publish");

    /* set variable value to publish */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Variant writeValue;
    UA_Variant_setScalar(&writeValue, &TestValue, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Server_writeValue(server, PublishedVarId, writeValue) == UA_STATUSCODE_GOOD);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Variable to publish: %i", TestValue);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): sleep");
    UA_sleep_ms(Sleep_ms);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): wakeup");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): read subscribed variable");
    UA_Variant SubscribedNodeData;
    UA_Variant_init(&SubscribedNodeData);
    retVal = UA_Server_readValue(server, SubscribedVarId, &SubscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(SubscribedNodeData.data != 0);
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ValidatePublishSubscribe(): received value: %i \n\n\n\n", *(UA_Int32 *)SubscribedNodeData.data);

    ck_assert_int_eq(TestValue, *(UA_Int32 *)SubscribedNodeData.data);
    UA_Variant_clear(&SubscribedNodeData);

}

/***************************************************************************************************/
/***************************************************************************************************/


/***************************************************************************************************/
/* simple test with 2 connections: 1 DataSetWriter and 1 DataSetReader */
START_TEST(Test_basic) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_basic");

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
    UA_Duration MessageReceiveTimeout = 1000.0;
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", 1, 1, 1, MessageReceiveTimeout, &VarId_Conn2_RG1_DSR1, &DSRId_Conn2_RG1_DSR1);

    /* set WriterGroup operational */
    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    /* set ReaderGroup operational */
    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 10, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 33, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));
    ValidatePublishSubscribe(VarId_Conn1_WG1, VarId_Conn2_RG1_DSR1, 40, (UA_UInt32) (PublishingInterval_Conn1WG1 * 3));

    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_basic\n\n");
} END_TEST


/***************************************************************************************************/
START_TEST(Test_multiple_readers) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_multiple_readers");

    /* Connection 1: Writer 1  --> Connection 1: Reader 1 
                               --> Connection 1: Reader 2
                               --> Connection 2: Reader 1
    
    */
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

    // add the same reader on the same connection again -> but with different message receive timeout
    UA_NodeId DSRId_Conn1_RG1_DSR2;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR2);
    UA_NodeId VarId_Conn1_RG1_DSR2;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR2);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR2 = 130.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR2", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, 
        MessageReceiveTimeout_Conn1_RG1_DSR2, &VarId_Conn1_RG1_DSR2, &DSRId_Conn1_RG1_DSR2);

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


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check normal pubsub operation");

    /* set all writer- and readergroups to operational */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1));

    /* check that publish/subscribe works (for all readers) -> set some test values */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\ncheck Connection 1 -> DataSetReader 2 "
        "-> Check this reader first, because it works \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 22, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 44, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\ncheck Connection 2 -> DataSetReader 1 "
        "-> Check this reader first, because it works \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 49, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Connection 1 -> DataSetReader 1 \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 10, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 5, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));



    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_multiple_readers");
} END_TEST



/***************************************************************************************************/
START_TEST(Test_multiple_readers_2) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_multiple_readers -> 2");

    /* Connection 1: Writer 1  --> Connection 1: Reader 1 
                               --> Connection 1: Reader 2
                               --> Connection 1: Reader 3
                               --> Connection 2: Reader 1
    
    */
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

    // add the same reader on the same connection again -> but with different message receive timeout
    UA_NodeId DSRId_Conn1_RG1_DSR2;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR2);
    UA_NodeId VarId_Conn1_RG1_DSR2;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR2);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR2 = 130.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR2", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, 
        MessageReceiveTimeout_Conn1_RG1_DSR2, &VarId_Conn1_RG1_DSR2, &DSRId_Conn1_RG1_DSR2);

    // add the same reader on the same connection again -> but with different message receive timeout
    UA_NodeId DSRId_Conn1_RG1_DSR3;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR3);
    UA_NodeId VarId_Conn1_RG1_DSR3;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR3);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR3 = 130.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR3", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1, 
        MessageReceiveTimeout_Conn1_RG1_DSR3, &VarId_Conn1_RG1_DSR3, &DSRId_Conn1_RG1_DSR3);

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


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check normal pubsub operation");

    /* set all writer- and readergroups to operational */
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1));
    ck_assert(UA_STATUSCODE_GOOD == UA_Server_setReaderGroupOperational(server, RGId_Conn2_RG1));

    /* check that publish/subscribe works (for all readers) -> set some test values */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\ncheck Connection 2 -> DataSetReader 1 "
        "-> Check this reader first, because it works \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 47, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn2_RG1_DSR1, 49, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\ncheck Connection 1 -> DataSetReader 3 "
        "-> Check this reader first, because it works \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR3, 22, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR3, 44, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Connection 1 -> DataSetReader 2 \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 10, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR2, 5, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check Connection 1 -> DataSetReader 1 \n\n");

    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 24, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));
    ValidatePublishSubscribe(VarId_Conn1_WG1_DS1, VarId_Conn1_RG1_DSR1, 13, (UA_UInt32) (5 * PublishingInterval_Conn1_WG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_multiple_readers -> 2");
} END_TEST


/***************************************************************************************************/
START_TEST(Test_many_components) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_many_components");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "prepare configuration");

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


    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn2_WG2) == UA_STATUSCODE_GOOD);

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG1) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn2_RG2) == UA_STATUSCODE_GOOD);
    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn3_RG1) == UA_STATUSCODE_GOOD);


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_many_components\n\n");

} END_TEST


// /***************************************************************************************************/
// static void EmptyCallback(void *application, void *data) {
//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "EmptyCallback()");
// }


// /***************************************************************************************************/
// START_TEST(Test_timer) {

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_timer");

//     const size_t cNoOfTestRuns = 5;
//     for (size_t runs = 0; runs < cNoOfTestRuns; runs++) {

//         for (size_t i = 0; i < NO_OF_TIMERS; i++) {
//             ck_assert(UA_STATUSCODE_GOOD == UA_Timer_addRepeatedCallback(&server->timer, (UA_ApplicationCallback) EmptyCallback, 
//                 server, 0, 60000, &(sTimerIds[i])));

//             UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "start timer: Id = '%u'", (UA_UInt32) sTimerIds[i]);
//         }

//         for (size_t i = 0; i < NO_OF_TIMERS; i++) {
                            
//             UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "stop timer: Id = '%u'", (UA_UInt32) sTimerIds[i]);

//             UA_Timer_removeCallback(&server->timer, sTimerIds[i]);
//         }
//     }

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_timer\n\n");

// } END_TEST



// /***************************************************************************************************/
// static void RemoveCallback(void *application, void *data) {

//     assert(data != 0);

//     UA_UInt64 * pTimerId = (UA_UInt64 *) data;

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "RemoveCallback(): begin - Id = '%u'", 
//         (UA_UInt32) *pTimerId);

//     UA_Timer_removeCallback(&server->timer, *pTimerId);

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "RemoveCallback(): begin");

// }

// /***************************************************************************************************/
// START_TEST(Test_timer_remove) {

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n\nSTART: Test_timer_remove");

//     const size_t cNoOfTestRuns = 5;
//     for (size_t runs = 0; runs < cNoOfTestRuns; runs++) {
//         for (size_t i = 0; i < NO_OF_TIMERS; i++) {
//             ck_assert(UA_STATUSCODE_GOOD == 
//                 UA_Timer_addRepeatedCallback(&server->timer, (UA_ApplicationCallback) RemoveCallback, server, &(sTimerIds[i]), 30000, &(sTimerIds[i])));
            
//             UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "start timer: Id = '%u'", (UA_UInt32) sTimerIds[i]);
//         }

//         UA_comboSleep(70000);
//     }

//     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_timer_remove\n\n");

// } END_TEST



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

    TCase *tc_many_components = tcase_create("many_components");
    tcase_add_checked_fixture(tc_many_components, setup, teardown);
    tcase_add_test(tc_many_components, Test_many_components);

    TCase *tc_multiple_readers = tcase_create("multiple_readers");
    tcase_add_checked_fixture(tc_multiple_readers, setup, teardown);
    tcase_add_test(tc_multiple_readers, Test_multiple_readers);

    TCase *tc_multiple_readers_2 = tcase_create("multiple_readers 2");
    tcase_add_checked_fixture(tc_multiple_readers_2, setup, teardown);
    tcase_add_test(tc_multiple_readers_2, Test_multiple_readers_2);


    // TCase *tc_timer_empty = tcase_create("timer_empty");
    // tcase_add_checked_fixture(tc_timer_empty, setup, teardown);
    // tcase_add_test(tc_timer_empty, Test_timer); 

    // TCase *tc_timer_remove = tcase_create("timer_remove");
    // tcase_add_checked_fixture(tc_timer_remove, setup, teardown);
    // tcase_add_test(tc_timer_remove, Test_timer_remove); 


    Suite *s = suite_create("PubSub timeout test suite: message receive timeout"); 
    suite_add_tcase(s, tc_basic);
    suite_add_tcase(s, tc_many_components);
    suite_add_tcase(s, tc_multiple_readers);
    suite_add_tcase(s, tc_multiple_readers_2);
    // suite_add_tcase(s, tc_timer_empty);
    // suite_add_tcase(s, tc_timer_remove);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
