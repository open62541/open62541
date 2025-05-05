#include <open62541/plugin/log.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
#include "util/ua_util_internal.h"
#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;
UA_Logger logger;

/* global variables for test configuration */
static UA_Boolean UseRawEncoding = UA_FALSE;

static void setup(void) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "setup");
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config != 0);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_run_startup(server));
}

static void teardown(void) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "teardown");
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_run_shutdown(server));
    UA_Server_delete(server);
}

static void
AddConnection(char *pName, UA_PublisherId publisherId, UA_NodeId *opConnectionId) {
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(pName);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    /* Deep copy is not needed (not even for string) because
     * UA_Server_addPubSubConnection performs deep copy */
    connectionConfig.publisherId = publisherId;
    ck_assert_int_eq(UA_STATUSCODE_GOOD,
                     UA_Server_addPubSubConnection(server, &connectionConfig, opConnectionId));
}

static void
AddWriterGroup(UA_NodeId *pConnectionId, char *pName,
               UA_UInt32 WriterGroupId, UA_NodeId *opWriterGroupId) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING(pName);
    writerGroupConfig.publishingInterval = 50.0;
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
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_addWriterGroup(server, *pConnectionId, &writerGroupConfig, opWriterGroupId));
    
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

/***************************************************************************************************/
static void
AddPublishedDataSet(UA_NodeId *pWriterGroupId, char *pPublishedDataSetName,
                    char *pDataSetWriterName, UA_UInt32 DataSetWriterId,
                    UA_NodeId *opPublishedDataSetId, UA_NodeId *opPublishedVarId,
                    UA_DataValue **oppFastPathPublisherDataValue, UA_NodeId *opDataSetWriterId) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING(pPublishedDataSetName);
    UA_AddPublishedDataSetResult result =
        UA_Server_addPublishedDataSet(server, &pdsConfig, opPublishedDataSetId);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, result.addResult);

    /* Create variable to publish integer data */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData     = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "Published Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        attr, NULL, opPublishedVarId));

    UA_NodeId dataSetFieldId;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Int32 Publish var");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = *opPublishedVarId;
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldResult PdsFieldResult =
        UA_Server_addDataSetField(server, *opPublishedDataSetId, &dataSetFieldConfig, &dataSetFieldId);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, PdsFieldResult.result);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING(pDataSetWriterName);
    dataSetWriterConfig.dataSetWriterId = (UA_UInt16) DataSetWriterId;
    dataSetWriterConfig.keyFrameCount = 10;
    if (UseRawEncoding) {
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
    } else {
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    }
    ck_assert_int_eq(UA_STATUSCODE_GOOD,
                     UA_Server_addDataSetWriter(server, *pWriterGroupId,
                                                *opPublishedDataSetId, &dataSetWriterConfig,
                                                opDataSetWriterId));
}

static void
AddReaderGroup(UA_NodeId *pConnectionId, char *pName,
               UA_NodeId *opReaderGroupId) {
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING(pName);
    ck_assert_int_eq(UA_STATUSCODE_GOOD,
        UA_Server_addReaderGroup(server, *pConnectionId, &readerGroupConfig, opReaderGroupId));
}

static void
AddDataSetReader(UA_NodeId *pReaderGroupId, char *pName,
                 UA_PublisherId publisherId, UA_UInt32 WriterGroupId,
                 UA_UInt32 DataSetWriterId, UA_NodeId *opSubscriberVarId,
                 UA_DataValue **oppFastPathSubscriberDataValue, UA_NodeId *opDataSetReaderId) {
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING(pName);
    readerConfig.publisherId = publisherId;
    readerConfig.writerGroupId    = (UA_UInt16) WriterGroupId;
    readerConfig.dataSetWriterId  = (UA_UInt16) DataSetWriterId;
    readerConfig.messageReceiveTimeout = 0.0;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dsReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dsReaderMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                    (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dsReaderMessage;
    if (UseRawEncoding) {
        readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
        readerConfig.expectedEncoding = UA_PUBSUB_RT_RAW;
    } else {
        readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    }

    UA_DataSetMetaDataType_init(&readerConfig.dataSetMetaData);
    UA_DataSetMetaDataType *pDataSetMetaData = &readerConfig.dataSetMetaData;
    pDataSetMetaData->name = UA_STRING (pName);
    pDataSetMetaData->fieldsSize = 1;
    pDataSetMetaData->fields = (UA_FieldMetaData*) UA_Array_new (pDataSetMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    UA_FieldMetaData_init(pDataSetMetaData->fields);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pDataSetMetaData->fields->dataType);
    pDataSetMetaData->fields->builtInType = UA_NS0ID_INT32;
    pDataSetMetaData->fields->name =  UA_STRING ("Int32 Var");
    pDataSetMetaData->fields->valueRank = -1;
    ck_assert(UA_Server_addDataSetReader(server, *pReaderGroupId, &readerConfig,
                                         opDataSetReaderId) == UA_STATUSCODE_GOOD);
    UA_UadpDataSetReaderMessageDataType_delete(dsReaderMessage);
    dsReaderMessage = 0;

    /* Variable to subscribe data */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    attr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    attr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 subscriberData = 0;
    UA_Variant_setScalar(&attr.value, &subscriberData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(
        UA_STATUSCODE_GOOD,
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, opSubscriberVarId));

    UA_FieldTargetDataType targetVariable;
    UA_FieldTargetDataType_init(&targetVariable);
    targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVariable.targetNodeId = *opSubscriberVarId;

    ck_assert_int_eq(UA_STATUSCODE_GOOD,
                     UA_Server_DataSetReader_createTargetVariables(server, *opDataSetReaderId,
                                                                   1, &targetVariable));

    UA_free(pDataSetMetaData->fields);
    pDataSetMetaData->fields = NULL;
}

static void
ValidatePublishSubscribe(
    const UA_UInt32 NoOfTestVars, UA_NodeId *publisherVarIds, UA_NodeId *subscriberVarIds,
    UA_DataValue **fastPathPublisherValues,  /* fast-path publisher DataValue */
    UA_DataValue **fastPathSubscriberValues, /* fast-path subscriber DataValue */
    const UA_Int32 TestValue,
    const UA_UInt32 Sleep_ms /* use at least publishing interval */) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "ValidatePublishSubscribe(): set variable to publish");

    /* set variable value to publish */
    UA_Int32 tmpValue = TestValue;
    for (UA_UInt32 i = 0; i < NoOfTestVars; i++) {
        tmpValue = TestValue + (UA_Int32) i;
        UA_Variant writeValue;
        UA_Variant_init(&writeValue);
        UA_Variant_setScalarCopy(&writeValue, &tmpValue, &UA_TYPES[UA_TYPES_INT32]);
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_writeValue(server, publisherVarIds[i], writeValue));
        UA_Variant_clear(&writeValue);
    }

    UA_Boolean done = false;
    while(!done) {
        UA_fakeSleep(Sleep_ms);
        UA_Server_run_iterate(server, true);
        done = true;
        UA_UInt32 i = 0;
        for(i = 0; i < NoOfTestVars; i++) {
            tmpValue = TestValue + (UA_Int32)i;
            UA_Variant SubscribedNodeData;
            UA_Variant_init(&SubscribedNodeData);
            UA_Server_readValue(server, subscriberVarIds[i], &SubscribedNodeData);
            if(tmpValue != *(UA_Int32 *)SubscribedNodeData.data)
                done = false;
            UA_Variant_clear(&SubscribedNodeData);
        }
    }
}

static void DoTest_1_Connection(UA_PublisherId publisherId) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_1_Connection() begin");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "raw encoding  = %s", (UseRawEncoding) ? "enabled" : "disabled");

#define DOTEST_1_CONNECTION_MAX_VARS 1
    UA_NodeId publisherVarIds[DOTEST_1_CONNECTION_MAX_VARS];
    UA_NodeId subscriberVarIds[DOTEST_1_CONNECTION_MAX_VARS];

    /* Attention: Publisher and corresponding Subscriber DataValue must have the same index */
    UA_DataValue *fastPathPublisherDataValues[DOTEST_1_CONNECTION_MAX_VARS];
    UA_DataValue *fastPathSubscriberDataValues[DOTEST_1_CONNECTION_MAX_VARS];
    for (UA_UInt32 i = 0; i < DOTEST_1_CONNECTION_MAX_VARS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    /* setup Connection 1: */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    AddConnection("Conn1", publisherId, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    AddWriterGroup(&ConnId_1, "Conn1_WG1", 1, &WGId_Conn1_WG1);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);

    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", 1,
                        &PDSId_Conn1_WG1_PDS1, publisherVarIds,
                        fastPathPublisherDataValues, &DsWId_Conn1_WG1_DS1);

    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", publisherId, 1, 1,
                     subscriberVarIds, fastPathSubscriberDataValues,
                     &DSRId_Conn1_RG1_DSR1);

    /* set groups operational */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(DOTEST_1_CONNECTION_MAX_VARS, publisherVarIds, subscriberVarIds,
        fastPathPublisherDataValues, fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(DOTEST_1_CONNECTION_MAX_VARS, publisherVarIds, subscriberVarIds,
        fastPathPublisherDataValues, fastPathSubscriberDataValues, 33, 100);

    ValidatePublishSubscribe(DOTEST_1_CONNECTION_MAX_VARS, publisherVarIds, subscriberVarIds,
        fastPathPublisherDataValues, fastPathSubscriberDataValues, 44, 100);

    /* set groups to disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable groups");

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1));
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove Connection");
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePubSubConnection(server, ConnId_1));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove PDS");
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, PDSId_Conn1_WG1_PDS1));

    /* Iterate so the connections are actually deleted */
    UA_Server_run_iterate(server, false);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_1_Connection() end");
}

/***************************************************************************************************/
/* simple test with 1 connection */
START_TEST(Test_1_connection) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_1_connection");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Test PublisherId BYTE with all combinations");

    UA_PublisherId publisherId;
    publisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    publisherId.id.byte = 2;

    UseRawEncoding = UA_FALSE;
    DoTest_1_Connection(publisherId);

    UseRawEncoding = UA_TRUE;
    DoTest_1_Connection(publisherId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Test PublisherId UINT16 with all combinations");

    publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    publisherId.id.uint16 = 3;

    UseRawEncoding = UA_FALSE;
    DoTest_1_Connection(publisherId);

    UseRawEncoding = UA_TRUE;
    DoTest_1_Connection(publisherId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Test PublisherId UINT32 with all combinations");

    publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    publisherId.id.uint32 = 5;

    UseRawEncoding = UA_FALSE;
    DoTest_1_Connection(publisherId);

    UseRawEncoding = UA_TRUE;
    DoTest_1_Connection(publisherId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Test PublisherId UINT64 with all combinations");

    publisherId.idType = UA_PUBLISHERIDTYPE_UINT64;
    publisherId.id.uint64 = 6;

    UseRawEncoding = UA_FALSE;
    DoTest_1_Connection(publisherId);

    UseRawEncoding = UA_TRUE;
    DoTest_1_Connection(publisherId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Test PublisherId STRING with all combinations");

    publisherId.idType = UA_PUBLISHERIDTYPE_STRING;
    publisherId.id.string = UA_STRING("My PublisherId");

    UseRawEncoding = UA_FALSE;
    DoTest_1_Connection(publisherId);

    UseRawEncoding = UA_TRUE;
    DoTest_1_Connection(publisherId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_1_connection");
} END_TEST


/***************************************************************************************************/
static void DoTest_multiple_Connections(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_Connections() begin");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "raw encoding  = %s", (UseRawEncoding) ? "enabled" : "disabled");

    /*  Writers                             -> Readers
        ----------------------------------------------------------------------------------
        Conn1 BYTE   Id, WG 1, DSW 1        -> Conn2, RG 1, DSR 1
        Conn2 BYTE   Id, WG 1, DSW 1        -> Conn1, RG 1, DSR 1
        Conn3 UINT16 Id, WG 1, DSW 1        -> Conn4, RG 1, DSR 1
        Conn4 UINT32 Id, WG 1, DSW 1        -> Conn3, RG 1, DSR 1
        Conn5 UINT64 Id, WG 1, DSW 1        -> Conn6, RG 1, DSR 1
        Conn6 UINT16 Id, WG 1, DSW 1        -> Conn5, RG 1, DSR 1
    */

    /* note: PublisherId BYTE 1 is different than UINT16 1 */

    /* every connection has exactly 1 Writer- and ReaderGroup, 1 DataSetWriter and -reader, and 1 publish/subscribe variable */
#define DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS 6

    UA_NodeId ConnectionIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    UA_NodeId WriterGroupIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    UA_NodeId PublishedDataSetIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    UA_NodeId ReaderGroupIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];

    /* Attention: Publisher and corresponding Subscriber NodeId and DataValue must have the same index
        e.g. publisherVarIds[0] value is set and subscriberVarIds[0] value is checked at ValidatePublishSubscribe() function */
    UA_NodeId publisherVarIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    UA_NodeId subscriberVarIds[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];

    UA_DataValue *fastPathPublisherDataValues[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    UA_DataValue *fastPathSubscriberDataValues[DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS];
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    const UA_UInt32 WG_Id = 1;
    const UA_UInt32 DSW_Id = 1;

    /* setup all Publishers */

    /* setup Connection 1: */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_PublisherId Conn1_PublisherId;
    Conn1_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn1_PublisherId.id.byte = 1;
    AddConnection("Conn1", Conn1_PublisherId, &ConnId_1);
    ConnectionIds[0] = ConnId_1;

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WG_Id, &WGId_Conn1_WG1);
    WriterGroupIds[0] = WGId_Conn1_WG1;

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1",
                        DSW_Id, &PDSId_Conn1_WG1_PDS1, publisherVarIds,
                        fastPathPublisherDataValues, &DsWId_Conn1_WG1_DS1);
    PublishedDataSetIds[0] = PDSId_Conn1_WG1_PDS1;

    /* setup Connection 2: */
    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    UA_PublisherId Conn2_PublisherId;
    Conn2_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn2_PublisherId.id.byte = 2;
    AddConnection("Conn2", Conn2_PublisherId, &ConnId_2);
    ConnectionIds[1] = ConnId_2;

    UA_NodeId WGId_Conn2_WG1;
    UA_NodeId_init(&WGId_Conn2_WG1);
    AddWriterGroup(&ConnId_2, "Conn2_WG1", WG_Id, &WGId_Conn2_WG1);
    WriterGroupIds[1] = WGId_Conn2_WG1;

    UA_NodeId DsWId_Conn2_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG1_DS1);
    UA_NodeId PDSId_Conn2_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS1", "Conn2_WG1_DS1",
                        DSW_Id, &PDSId_Conn2_WG1_PDS1, &publisherVarIds[1],
                        &fastPathPublisherDataValues[1], &DsWId_Conn2_WG1_DS1);
    PublishedDataSetIds[1] = PDSId_Conn2_WG1_PDS1;

    /* setup Connection 3: */
    UA_NodeId ConnId_3;
    UA_NodeId_init(&ConnId_3);
    UA_PublisherId Conn3_PublisherId;
    Conn3_PublisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    Conn3_PublisherId.id.uint16 = 1;
    AddConnection("Conn3", Conn3_PublisherId, &ConnId_3);
    ConnectionIds[2] = ConnId_3;

    UA_NodeId WGId_Conn3_WG1;
    UA_NodeId_init(&WGId_Conn3_WG1);
    AddWriterGroup(&ConnId_3, "Conn3_WG1", WG_Id, &WGId_Conn3_WG1);
    WriterGroupIds[2] = WGId_Conn3_WG1;

    UA_NodeId DsWId_Conn3_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn3_WG1_DS1);
    UA_NodeId PDSId_Conn3_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn3_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn3_WG1, "Conn3_WG1_PDS1", "Conn3_WG1_DS1",
                        DSW_Id, &PDSId_Conn3_WG1_PDS1, &publisherVarIds[2],
                        &fastPathPublisherDataValues[2], &DsWId_Conn3_WG1_DS1);
    PublishedDataSetIds[2] = PDSId_Conn3_WG1_PDS1;

    /* setup Connection 4 */
    UA_NodeId ConnId_4;
    UA_NodeId_init(&ConnId_4);
    UA_PublisherId Conn4_PublisherId;
    Conn4_PublisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    Conn4_PublisherId.id.uint32 = 15;
    AddConnection("Conn4", Conn4_PublisherId, &ConnId_4);
    ConnectionIds[3] = ConnId_4;

    UA_NodeId WGId_Conn4_WG1;
    UA_NodeId_init(&WGId_Conn4_WG1);
    AddWriterGroup(&ConnId_4, "Conn4_WG1", WG_Id, &WGId_Conn4_WG1);
    WriterGroupIds[3] = WGId_Conn4_WG1;

    UA_NodeId DsWId_Conn4_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn4_WG1_DS1);
    UA_NodeId PDSId_Conn4_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn4_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn4_WG1, "Conn4_WG1_PDS1", "Conn4_WG1_DS1",
                        DSW_Id, &PDSId_Conn4_WG1_PDS1, &publisherVarIds[3],
                        &fastPathPublisherDataValues[3], &DsWId_Conn4_WG1_DS1);
    PublishedDataSetIds[3] = PDSId_Conn4_WG1_PDS1;

    /* setup Connection 5 */
    UA_NodeId ConnId_5;
    UA_NodeId_init(&ConnId_5);
    UA_PublisherId Conn5_PublisherId;
    Conn5_PublisherId.idType = UA_PUBLISHERIDTYPE_UINT64;
    Conn5_PublisherId.id.uint64 = 33;
    AddConnection("Conn5", Conn5_PublisherId, &ConnId_5);
    ConnectionIds[4] = ConnId_5;

    UA_NodeId WGId_Conn5_WG1;
    UA_NodeId_init(&WGId_Conn5_WG1);
    AddWriterGroup(&ConnId_5, "Conn5_WG1", WG_Id, &WGId_Conn5_WG1);
    WriterGroupIds[4] = WGId_Conn5_WG1;

    UA_NodeId DsWId_Conn5_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn5_WG1_DS1);
    UA_NodeId PDSId_Conn5_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn5_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn5_WG1, "Conn5_WG1_PDS1", "Conn5_WG1_DS1",
                        DSW_Id, &PDSId_Conn5_WG1_PDS1, &publisherVarIds[4],
                        &fastPathPublisherDataValues[4], &DsWId_Conn5_WG1_DS1);
    PublishedDataSetIds[4] = PDSId_Conn5_WG1_PDS1;

    /* setup Connection 6: */
    UA_NodeId ConnId_6;
    UA_NodeId_init(&ConnId_6);
    UA_PublisherId Conn6_PublisherId;
    Conn6_PublisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    Conn6_PublisherId.id.uint16 = 2;
    AddConnection("Conn6", Conn6_PublisherId, &ConnId_6);
    ConnectionIds[5] = ConnId_6;

    UA_NodeId WGId_Conn6_WG1;
    UA_NodeId_init(&WGId_Conn6_WG1);
    AddWriterGroup(&ConnId_6, "Conn6_WG1", WG_Id, &WGId_Conn6_WG1);
    WriterGroupIds[5] = WGId_Conn6_WG1;

    UA_NodeId DsWId_Conn6_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn6_WG1_DS1);
    UA_NodeId PDSId_Conn6_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn6_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn6_WG1, "Conn6_WG1_PDS1", "Conn6_WG1_DS1",
                        DSW_Id, &PDSId_Conn6_WG1_PDS1, &publisherVarIds[5],
                        &fastPathPublisherDataValues[5], &DsWId_Conn6_WG1_DS1);
    PublishedDataSetIds[5] = PDSId_Conn6_WG1_PDS1;

    /* setup all Subscribers */

    /* setup Connection 1: */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1",
                     Conn2_PublisherId, WG_Id, DSW_Id,
                     &subscriberVarIds[1], &fastPathSubscriberDataValues[1],
                     &DSRId_Conn1_RG1_DSR1);
    ReaderGroupIds[0] = RGId_Conn1_RG1;

    /* setup Connection 2: */
    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);
    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1",
                     Conn1_PublisherId, WG_Id, DSW_Id, subscriberVarIds,
                     fastPathSubscriberDataValues, &DSRId_Conn2_RG1_DSR1);
    ReaderGroupIds[1] = RGId_Conn2_RG1;

    /* setup Connection 3: */
    UA_NodeId RGId_Conn3_RG1;
    UA_NodeId_init(&RGId_Conn3_RG1);
    AddReaderGroup(&ConnId_3, "Conn3_RG1", &RGId_Conn3_RG1);
    UA_NodeId DSRId_Conn3_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn3_RG1_DSR1);
    AddDataSetReader(&RGId_Conn3_RG1, "Conn3_RG1_DSR1",
                     Conn4_PublisherId, WG_Id, DSW_Id, &subscriberVarIds[3],
                     &fastPathSubscriberDataValues[3], &DSRId_Conn3_RG1_DSR1);
    ReaderGroupIds[2] = RGId_Conn3_RG1;

    /* setup Connection 4: */
    UA_NodeId RGId_Conn4_RG1;
    UA_NodeId_init(&RGId_Conn4_RG1);
    AddReaderGroup(&ConnId_4, "Conn4_RG1", &RGId_Conn4_RG1);
    UA_NodeId DSRId_Conn4_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn4_RG1_DSR1);
    AddDataSetReader(&RGId_Conn4_RG1, "Conn4_RG1_DSR1",
                     Conn3_PublisherId, WG_Id, DSW_Id, &subscriberVarIds[2],
                     &fastPathSubscriberDataValues[2], &DSRId_Conn4_RG1_DSR1);
    ReaderGroupIds[3] = RGId_Conn4_RG1;

    /* setup Connection 5: */
    UA_NodeId RGId_Conn5_RG1;
    UA_NodeId_init(&RGId_Conn5_RG1);
    AddReaderGroup(&ConnId_5, "Conn5_RG1", &RGId_Conn5_RG1);
    UA_NodeId DSRId_Conn5_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn5_RG1_DSR1);
    AddDataSetReader(&RGId_Conn5_RG1, "Conn5_RG1_DSR1",
                     Conn6_PublisherId, WG_Id, DSW_Id, &subscriberVarIds[5],
                     &fastPathSubscriberDataValues[5], &DSRId_Conn5_RG1_DSR1);
    ReaderGroupIds[4] = RGId_Conn5_RG1;

    /* setup Connection 6: */
    UA_NodeId RGId_Conn6_RG1;
    UA_NodeId_init(&RGId_Conn6_RG1);
    AddReaderGroup(&ConnId_6, "Conn6_RG1", &RGId_Conn6_RG1);
    UA_NodeId DSRId_Conn6_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn6_RG1_DSR1);
    AddDataSetReader(&RGId_Conn6_RG1, "Conn6_RG1_DSR1",
                     Conn5_PublisherId, WG_Id, DSW_Id, &subscriberVarIds[4],
                     &fastPathSubscriberDataValues[4], &DSRId_Conn6_RG1_DSR1);
    ReaderGroupIds[5] = RGId_Conn6_RG1;

    /* set groups operational */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 50, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 100, 100);

    /* set groups to disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable groups");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD,
                         UA_Server_setWriterGroupDisabled(server, WriterGroupIds[i]));
    }
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD,
                         UA_Server_setReaderGroupDisabled(server, ReaderGroupIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove Connection");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD,
                         UA_Server_removePubSubConnection(server, ConnectionIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove PublishedDataSets");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_CONNECTIONS_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD,
                         UA_Server_removePublishedDataSet(server,
                                                          PublishedDataSetIds[i]));
    }

    /* Iterate so the connections are actually deleted */
    UA_Server_run_iterate(server, false);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_Connections() end");
}

/***************************************************************************************************/
/* test with multiple connections */
START_TEST(Test_multiple_connections) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_multiple_connections");

    /* note: fast-path does not support
        - multiple groups and/or DataSets yet, therefore we only test multiple connections
        - STRING publisherIds */

    UseRawEncoding = UA_FALSE;
    DoTest_multiple_Connections();

    UseRawEncoding = UA_TRUE;
    DoTest_multiple_Connections();

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_multiple_connections");
} END_TEST

/***************************************************************************************************/

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL

 /* Read PublisherId value of PubSub information model -> check that string read works correctly */
static void
Test_string_PublisherId_InformationModel(const UA_NodeId connectionId,
                                         const UA_String expectedStringIdValue) {
    /* Get PublisherId node */
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);;
    rpe.isInverse = false;
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "PublisherId");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = connectionId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr;
    UA_BrowsePathResult_init(&bpr);
    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, bpr.statusCode);
    ck_assert_uint_eq(1, bpr.targetsSize);
    UA_NodeId PublisherIdNode = bpr.targets[0].targetId.nodeId;

    /* Read value */
    UA_Variant PublisherIdValue;
    UA_Variant_init(&PublisherIdValue);
    UA_StatusCode res = UA_Server_readValue(server, PublisherIdNode, &PublisherIdValue);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    ck_assert(true == UA_Variant_hasScalarType(&PublisherIdValue, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert(true == UA_String_equal(&expectedStringIdValue, (UA_String*) PublisherIdValue.data));
    UA_Variant_clear(&PublisherIdValue);

    UA_BrowsePathResult_clear(&bpr);
}

#endif

/***************************************************************************************************/
static void DoTest_string_PublisherId(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_string_PublisherId() begin");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "raw encoding  = %s", (UseRawEncoding) ? "enabled" : "disabled");

    /*  Writers                                     -> Readers
        ----------------------------------------------------------------------------------
        Conn1 STRING Id "H"       WG 1 DSW 1        -> Conn2, RG 1, DSR 1
        Conn2 STRING Id "h"       WG 1 DSW 1        -> Conn1, RG 1, DSR 1
        Conn3 STRING Id "hallo"   WG 1 DSW 1        -> Conn5, RG 1, DSR 1
        Conn4 STRING Id "hallo1"  WG 1 DSW 1        -> Conn3, RG 1, DSR 1
        Conn5 STRING Id "Hallo"   WG 1 DSW 1        -> Conn4, RG 1, DSR 1
    */

    /* every connection has exactly 1 Writer- and ReaderGroup, 1 DataSetWriter and -reader, and 1 publish/subscribe variable */
#define DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS 5
    UA_NodeId ConnectionIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    UA_NodeId WriterGroupIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    UA_NodeId PublishedDataSetIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    UA_NodeId ReaderGroupIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];

    /* Attention: Publisher and corresponding Subscriber NodeId and DataValue must have the same index
        e.g. publisherVarIds[0] value is set and subscriberVarIds[0] value is checked at ValidatePublishSubscribe() function */
    UA_NodeId publisherVarIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    UA_NodeId subscriberVarIds[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];

    UA_DataValue *fastPathPublisherDataValues[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    UA_DataValue *fastPathSubscriberDataValues[DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS];
    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    const UA_UInt32 WG_Id = 1;
    const UA_UInt32 DSW_Id = 1;

    /* setup all Publishers */

{   /* create a local scope and allocate string PublisherIds on stack to check that
        there are no memory issues with strings (deep copy of strings is done etc.) */

        /* setup Connection 1: */
        UA_NodeId ConnId_1;
        UA_NodeId_init(&ConnId_1);
        UA_PublisherId Conn1_PublisherId;
        Conn1_PublisherId.idType = UA_PUBLISHERIDTYPE_STRING;
        Conn1_PublisherId.id.string = UA_STRING("H");  // allocate string on stack
        AddConnection("Conn1", Conn1_PublisherId, &ConnId_1);
        ConnectionIds[0] = ConnId_1;

        UA_NodeId WGId_Conn1_WG1;
        UA_NodeId_init(&WGId_Conn1_WG1);
        AddWriterGroup(&ConnId_1, "Conn1_WG1", WG_Id, &WGId_Conn1_WG1);
        WriterGroupIds[0] = WGId_Conn1_WG1;

        UA_NodeId DsWId_Conn1_WG1_DS1;
        UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
        UA_NodeId PDSId_Conn1_WG1_PDS1;
        UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
        AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSW_Id, &PDSId_Conn1_WG1_PDS1,
            publisherVarIds, fastPathPublisherDataValues, &DsWId_Conn1_WG1_DS1);
        PublishedDataSetIds[0] = PDSId_Conn1_WG1_PDS1;

        /* setup Connection 2: */
        UA_NodeId ConnId_2;
        UA_NodeId_init(&ConnId_2);
        UA_PublisherId Conn2_PublisherId;
        Conn2_PublisherId.idType = UA_PUBLISHERIDTYPE_STRING;
        Conn2_PublisherId.id.string = UA_STRING("h");
        AddConnection("Conn2", Conn2_PublisherId, &ConnId_2);
        ConnectionIds[1] = ConnId_2;

        UA_NodeId WGId_Conn2_WG1;
        UA_NodeId_init(&WGId_Conn2_WG1);
        AddWriterGroup(&ConnId_2, "Conn2_WG1", WG_Id, &WGId_Conn2_WG1);
        WriterGroupIds[1] = WGId_Conn2_WG1;

        UA_NodeId DsWId_Conn2_WG1_DS1;
        UA_NodeId_init(&DsWId_Conn2_WG1_DS1);
        UA_NodeId PDSId_Conn2_WG1_PDS1;
        UA_NodeId_init(&PDSId_Conn2_WG1_PDS1);
        AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS1", "Conn2_WG1_DS1", DSW_Id, &PDSId_Conn2_WG1_PDS1,
            &publisherVarIds[1], &fastPathPublisherDataValues[1], &DsWId_Conn2_WG1_DS1);
        PublishedDataSetIds[1] = PDSId_Conn2_WG1_PDS1;

        /* setup Connection 3: */
        UA_NodeId ConnId_3;
        UA_NodeId_init(&ConnId_3);
        UA_PublisherId Conn3_PublisherId;
        Conn3_PublisherId.idType = UA_PUBLISHERIDTYPE_STRING;
        Conn3_PublisherId.id.string = UA_STRING("hallo");
        AddConnection("Conn3", Conn3_PublisherId, &ConnId_3);
        ConnectionIds[2] = ConnId_3;

        UA_NodeId WGId_Conn3_WG1;
        UA_NodeId_init(&WGId_Conn3_WG1);
        AddWriterGroup(&ConnId_3, "Conn3_WG1", WG_Id, &WGId_Conn3_WG1);
        WriterGroupIds[2] = WGId_Conn3_WG1;

        UA_NodeId DsWId_Conn3_WG1_DS1;
        UA_NodeId_init(&DsWId_Conn3_WG1_DS1);
        UA_NodeId PDSId_Conn3_WG1_PDS1;
        UA_NodeId_init(&PDSId_Conn3_WG1_PDS1);
        AddPublishedDataSet(&WGId_Conn3_WG1, "Conn3_WG1_PDS1", "Conn3_WG1_DS1", DSW_Id, &PDSId_Conn3_WG1_PDS1,
            &publisherVarIds[2], &fastPathPublisherDataValues[2], &DsWId_Conn3_WG1_DS1);
        PublishedDataSetIds[2] = PDSId_Conn3_WG1_PDS1;

        /* setup Connection 4 */
        UA_NodeId ConnId_4;
        UA_NodeId_init(&ConnId_4);
        UA_PublisherId Conn4_PublisherId;
        Conn4_PublisherId.idType = UA_PUBLISHERIDTYPE_STRING;
        Conn4_PublisherId.id.string = UA_STRING("hallo1");
        AddConnection("Conn4", Conn4_PublisherId, &ConnId_4);
        ConnectionIds[3] = ConnId_4;

        UA_NodeId WGId_Conn4_WG1;
        UA_NodeId_init(&WGId_Conn4_WG1);
        AddWriterGroup(&ConnId_4, "Conn4_WG1", WG_Id, &WGId_Conn4_WG1);
        WriterGroupIds[3] = WGId_Conn4_WG1;

        UA_NodeId DsWId_Conn4_WG1_DS1;
        UA_NodeId_init(&DsWId_Conn4_WG1_DS1);
        UA_NodeId PDSId_Conn4_WG1_PDS1;
        UA_NodeId_init(&PDSId_Conn4_WG1_PDS1);
        AddPublishedDataSet(&WGId_Conn4_WG1, "Conn4_WG1_PDS1", "Conn4_WG1_DS1", DSW_Id, &PDSId_Conn4_WG1_PDS1,
            &publisherVarIds[3], &fastPathPublisherDataValues[3], &DsWId_Conn4_WG1_DS1);
        PublishedDataSetIds[3] = PDSId_Conn4_WG1_PDS1;

        /* setup Connection 5 */
        UA_NodeId ConnId_5;
        UA_NodeId_init(&ConnId_5);
        UA_PublisherId Conn5_PublisherId;
        Conn5_PublisherId.idType = UA_PUBLISHERIDTYPE_STRING;
        Conn5_PublisherId.id.string = UA_STRING("Hallo");
        AddConnection("Conn5", Conn5_PublisherId, &ConnId_5);
        ConnectionIds[4] = ConnId_5;

        UA_NodeId WGId_Conn5_WG1;
        UA_NodeId_init(&WGId_Conn5_WG1);
        AddWriterGroup(&ConnId_5, "Conn5_WG1", WG_Id, &WGId_Conn5_WG1);
        WriterGroupIds[4] = WGId_Conn5_WG1;

        UA_NodeId DsWId_Conn5_WG1_DS1;
        UA_NodeId_init(&DsWId_Conn5_WG1_DS1);
        UA_NodeId PDSId_Conn5_WG1_PDS1;
        UA_NodeId_init(&PDSId_Conn5_WG1_PDS1);
        AddPublishedDataSet(&WGId_Conn5_WG1, "Conn5_WG1_PDS1", "Conn5_WG1_DS1", DSW_Id, &PDSId_Conn5_WG1_PDS1,
            &publisherVarIds[4], &fastPathPublisherDataValues[4], &DsWId_Conn5_WG1_DS1);
        PublishedDataSetIds[4] = PDSId_Conn5_WG1_PDS1;

        /* setup all Subscribers */

        /* setup Connection 1: */
        UA_NodeId RGId_Conn1_RG1;
        UA_NodeId_init(&RGId_Conn1_RG1);
        AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
        UA_NodeId DSRId_Conn1_RG1_DSR1;
        UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
        AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1",
                         Conn2_PublisherId, WG_Id, DSW_Id,
            &subscriberVarIds[1], &fastPathSubscriberDataValues[1], &DSRId_Conn1_RG1_DSR1);
        ReaderGroupIds[0] = RGId_Conn1_RG1;

        /* setup Connection 2: */
        UA_NodeId RGId_Conn2_RG1;
        UA_NodeId_init(&RGId_Conn2_RG1);
        AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);
        UA_NodeId DSRId_Conn2_RG1_DSR1;
        UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
        AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1",
                         Conn1_PublisherId, WG_Id, DSW_Id,
            subscriberVarIds, fastPathSubscriberDataValues, &DSRId_Conn2_RG1_DSR1);
        ReaderGroupIds[1] = RGId_Conn2_RG1;

        /* setup Connection 3: */
        UA_NodeId RGId_Conn3_RG1;
        UA_NodeId_init(&RGId_Conn3_RG1);
        AddReaderGroup(&ConnId_3, "Conn3_RG1", &RGId_Conn3_RG1);
        UA_NodeId DSRId_Conn3_RG1_DSR1;
        UA_NodeId_init(&DSRId_Conn3_RG1_DSR1);
        AddDataSetReader(&RGId_Conn3_RG1, "Conn3_RG1_DSR1",
                         Conn5_PublisherId, WG_Id, DSW_Id,
            &subscriberVarIds[4], &fastPathSubscriberDataValues[4], &DSRId_Conn3_RG1_DSR1);
        ReaderGroupIds[2] = RGId_Conn3_RG1;

        /* setup Connection 4: */
        UA_NodeId RGId_Conn4_RG1;
        UA_NodeId_init(&RGId_Conn4_RG1);
        AddReaderGroup(&ConnId_4, "Conn4_RG1", &RGId_Conn4_RG1);
        UA_NodeId DSRId_Conn4_RG1_DSR1;
        UA_NodeId_init(&DSRId_Conn4_RG1_DSR1);
        AddDataSetReader(&RGId_Conn4_RG1, "Conn4_RG1_DSR1",
                         Conn3_PublisherId, WG_Id, DSW_Id,
            &subscriberVarIds[2], &fastPathSubscriberDataValues[2], &DSRId_Conn4_RG1_DSR1);
        ReaderGroupIds[3] = RGId_Conn4_RG1;

        /* setup Connection 5: */
        UA_NodeId RGId_Conn5_RG1;
        UA_NodeId_init(&RGId_Conn5_RG1);
        AddReaderGroup(&ConnId_5, "Conn5_RG1", &RGId_Conn5_RG1);
        UA_NodeId DSRId_Conn5_RG1_DSR1;
        UA_NodeId_init(&DSRId_Conn5_RG1_DSR1);
        AddDataSetReader(&RGId_Conn5_RG1, "Conn5_RG1_DSR1",
                         Conn4_PublisherId, WG_Id, DSW_Id,
            &subscriberVarIds[3], &fastPathSubscriberDataValues[3], &DSRId_Conn5_RG1_DSR1);
        ReaderGroupIds[4] = RGId_Conn5_RG1;

    }   /* end of configuration scope -> string PublisherIds are deallocated from stack
            deep copy of string PublisherId is done by UA_Server_addPubSubConnection() */

    /* set groups operational */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 50, 100);

    ValidatePublishSubscribe(DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 100, 100);

    /* check PubSub information model - string Ids */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    char *expectedPublisherIds[] = {
        "H",
        "h",
        "hallo",
        "hallo1",
        "Hallo"
    };

    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        Test_string_PublisherId_InformationModel(ConnectionIds[i], UA_STRING(expectedPublisherIds[i]));
    }
#endif

    /* set groups to disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable groups");
    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setWriterGroupDisabled(server, WriterGroupIds[i]));
    }
    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setReaderGroupDisabled(server, ReaderGroupIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove Connection");
    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePubSubConnection(server, ConnectionIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove PublishedDataSets");
    for (UA_UInt32 i = 0; i < DOTEST_STRING_PUBLISHERID_MAX_COMPONENTS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, PublishedDataSetIds[i]));
    }

    /* Iterate so the connections are actually deleted */
    UA_Server_run_iterate(server, false);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_string_PublisherId() end");
}


/***************************************************************************************************/
/* test string PublisherId */
START_TEST(Test_string_publisherId) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_string_publisherId");

    UseRawEncoding = UA_FALSE;
    DoTest_string_PublisherId();

    UseRawEncoding = UA_TRUE;
    DoTest_string_PublisherId();

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_string_publisherId");
} END_TEST

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

/***************************************************************************************************/
START_TEST(Test_string_publisherId_file_config) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_string_publisherId_file_config");

    UseRawEncoding = UA_FALSE;

#define STRING_PUBLISHERID_FILE_MAX_COMPONENTS 1
    /* Attention: Publisher and corresponding Subscriber NodeId and DataValue must have the same index
        e.g. publisherVarIds[0] value is set and subscriberVarIds[0] value is checked at ValidatePublishSubscribe() function */
    UA_NodeId publisherVarIds[STRING_PUBLISHERID_FILE_MAX_COMPONENTS];
    UA_NodeId subscriberVarIds[STRING_PUBLISHERID_FILE_MAX_COMPONENTS];

    UA_DataValue *fastPathPublisherDataValues[STRING_PUBLISHERID_FILE_MAX_COMPONENTS];
    UA_DataValue *fastPathSubscriberDataValues[STRING_PUBLISHERID_FILE_MAX_COMPONENTS];
    for (UA_UInt32 i = 0; i < STRING_PUBLISHERID_FILE_MAX_COMPONENTS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    /* we do not use a file, but setup PubSubConfigurationDataType structure and encode it to a bytestring
        to simulate file read */
    UA_ByteString encodedConfigDataBuffer;
    UA_ByteString_init(&encodedConfigDataBuffer);

    /* constants for configuration */
#define DS_MESSAGECONTENT_MASK (UA_UADPDATASETMESSAGECONTENTMASK_STATUS | \
                                UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP)
#define MAX_NETWORKMESSAGE_SIZE 1400
#define UADP_NETWORKMESSAGE_CONTENT_MASK \
    (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID | UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER | \
    UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID | UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER)

{   /* we use a local scope to make sure that string PublisherId configuration works correctly
        -> e.g. deep copy of Id */
    UA_PubSubConfigurationDataType config;
    UA_PubSubConfigurationDataType_init(&config);

    /* PublishedDataSet config */
    config.publishedDataSetsSize = 1;
    config.publishedDataSets = UA_PublishedDataSetDataType_new();
    ck_assert(config.publishedDataSets != 0);
    UA_PublishedDataSetDataType *pds = config.publishedDataSets;
    UA_PublishedDataSetDataType_init(pds);
    pds->name = UA_STRING_ALLOC("PublishedDataSet 1");
    UA_DataSetMetaDataType dataSetMetaData;
    UA_DataSetMetaDataType_init(&dataSetMetaData);
    dataSetMetaData.name = UA_STRING_ALLOC("PublishedDataSet 1");
    dataSetMetaData.fieldsSize = 1;
    dataSetMetaData.fields = UA_FieldMetaData_new();
    ck_assert(dataSetMetaData.fields != 0);
    UA_FieldMetaData_init(dataSetMetaData.fields);
    dataSetMetaData.fields->name = UA_STRING_ALLOC("Field 0");
    dataSetMetaData.fields->fieldFlags = UA_DATASETFIELDFLAGS_NONE;
    dataSetMetaData.fields->builtInType = UA_TYPES_INT32;
    dataSetMetaData.fields->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    dataSetMetaData.fields->valueRank = UA_VALUERANK_SCALAR;
    pds->dataSetMetaData = dataSetMetaData;
    UA_PublishedDataItemsDataType *pdsDataItems = UA_PublishedDataItemsDataType_new();
    ck_assert(pdsDataItems != 0);
    UA_PublishedDataItemsDataType_init(pdsDataItems);
    pdsDataItems->publishedDataSize = 1;
    pdsDataItems->publishedData = UA_PublishedVariableDataType_new();
    ck_assert(pdsDataItems->publishedData != 0);
    UA_PublishedVariableDataType_init(pdsDataItems->publishedData);
    pdsDataItems->publishedData->attributeId = UA_ATTRIBUTEID_VALUE;
    /* Create variable to publish integer data */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData     = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "Published Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        attr, NULL, publisherVarIds));
    UA_NodeId_copy(publisherVarIds, &pdsDataItems->publishedData->publishedVariable);
    pds->dataSetSource.encoding = UA_EXTENSIONOBJECT_DECODED;
    pds->dataSetSource.content.decoded.type = &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE];
    pds->dataSetSource.content.decoded.data = pdsDataItems;

    /* connection config */
    config.connectionsSize = 1;
    config.connections = UA_PubSubConnectionDataType_new();
    ck_assert(config.connections != 0);
    UA_PubSubConnectionDataType *connection = config.connections;
    UA_PubSubConnectionDataType_init(connection);
    connection->name = UA_STRING_ALLOC("My Connection");
    UA_String publisherIdString = UA_STRING("Publisher 1");
    UA_Variant_setScalarCopy(&connection->publisherId, &publisherIdString, &UA_TYPES[UA_TYPES_STRING]); // TODO?
    connection->transportProfileUri = UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType *addr = UA_NetworkAddressUrlDataType_new();
    ck_assert(addr != 0);
    UA_NetworkAddressUrlDataType_init(addr);
    addr->url = UA_STRING_ALLOC("opc.udp://224.0.0.22:4840/");
    connection->address.encoding = UA_EXTENSIONOBJECT_DECODED;
    connection->address.content.decoded.type = &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    connection->address.content.decoded.data = addr;

    /* WriterGroup config */
    connection->writerGroupsSize = 1;
    connection->writerGroups = UA_WriterGroupDataType_new();
    ck_assert(connection->writerGroups != 0);
    UA_WriterGroupDataType *wg = connection->writerGroups;
    UA_WriterGroupDataType_init(wg);
    wg->name = UA_STRING_ALLOC("WriterGroup 1");
    wg->maxNetworkMessageSize = MAX_NETWORKMESSAGE_SIZE;
    wg->writerGroupId = 1;
    wg->publishingInterval = 50;
    wg->keepAliveTime = 1000.00;
    UA_UadpWriterGroupMessageDataType *wgMessageSettings = UA_UadpWriterGroupMessageDataType_new();
    ck_assert(wgMessageSettings != 0);
    UA_UadpWriterGroupMessageDataType_init(wgMessageSettings);
    wgMessageSettings->networkMessageContentMask = UADP_NETWORKMESSAGE_CONTENT_MASK;
    wg->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    wg->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    wg->messageSettings.content.decoded.data = wgMessageSettings;

    /* DataSetWriter config */
    wg->dataSetWritersSize = 1;
    wg->dataSetWriters = UA_DataSetWriterDataType_new();
    ck_assert(wg->dataSetWriters != 0);
    UA_DataSetWriterDataType *dsw = wg->dataSetWriters;
    dsw->name = UA_STRING_ALLOC("DataSetWriter 1");
    dsw->dataSetWriterId = 1;
    dsw->dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    dsw->keyFrameCount = 1;
    dsw->dataSetName = UA_STRING_ALLOC("PublishedDataSet 1");
    UA_UadpDataSetWriterMessageDataType *dswMessageSettings = UA_UadpDataSetWriterMessageDataType_new();
    ck_assert(dswMessageSettings != 0);
    UA_UadpDataSetWriterMessageDataType_init(dswMessageSettings);
    dswMessageSettings->dataSetMessageContentMask = DS_MESSAGECONTENT_MASK;
    dsw->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    dsw->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE];
    dsw->messageSettings.content.decoded.data = dswMessageSettings;

    // ReaderGroup
    connection->readerGroupsSize = 1;
    connection->readerGroups = UA_ReaderGroupDataType_new();
    ck_assert(connection->readerGroups != 0);
    UA_ReaderGroupDataType *rg = connection->readerGroups;
    UA_ReaderGroupDataType_init(rg);
    rg->name = UA_STRING_ALLOC("ReaderGroup 1");
    rg->maxNetworkMessageSize = MAX_NETWORKMESSAGE_SIZE;

    // DataSetReader
    rg->dataSetReadersSize = 1;
    rg->dataSetReaders = UA_DataSetReaderDataType_new();
    ck_assert(rg->dataSetReaders != 0);
    UA_DataSetReaderDataType *dsr = rg->dataSetReaders;
    UA_DataSetReaderDataType_init(dsr);
    dsr->name = UA_STRING_ALLOC("DataSetReader 1");
    UA_Variant_setScalarCopy(&dsr->publisherId, &publisherIdString, &UA_TYPES[UA_TYPES_STRING]);
    dsr->writerGroupId = 1;
    dsr->dataSetWriterId = 1;

    UA_DataSetMetaDataType dsrDataSetMetaData;
    UA_DataSetMetaDataType_init(&dsrDataSetMetaData);
    dsrDataSetMetaData.name = UA_STRING_ALLOC("PublishedDataSet 1");
    dsrDataSetMetaData.fieldsSize = 1;
    dsrDataSetMetaData.fields = UA_FieldMetaData_new();
    ck_assert(dsrDataSetMetaData.fields != 0);
    dsrDataSetMetaData.fields->name = UA_STRING_ALLOC("Field_0");
    dsrDataSetMetaData.fields->builtInType = UA_TYPES_INT32;
    dsrDataSetMetaData.fields->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    dsrDataSetMetaData.fields->valueRank = UA_VALUERANK_SCALAR;
    dsr->dataSetMetaData = dsrDataSetMetaData;

    UA_TargetVariablesDataType *targetVars = UA_TargetVariablesDataType_new();
    ck_assert(targetVars != 0);
    UA_TargetVariablesDataType_init(targetVars);
    targetVars->targetVariablesSize = 1;
    targetVars->targetVariables = UA_FieldTargetDataType_new();
    ck_assert(targetVars->targetVariables != 0);
    targetVars->targetVariables->attributeId = UA_ATTRIBUTEID_VALUE;
    /* Variable to subscribe data */
    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Subscribed Int32");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed Int32");
    attr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 subscriberData = 0;
    UA_Variant_setScalar(&attr.value, &subscriberData, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, subscriberVarIds));
    UA_NodeId_copy(subscriberVarIds, &targetVars->targetVariables->targetNodeId);
    dsr->subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED;
    dsr->subscribedDataSet.content.decoded.type = &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE];
    dsr->subscribedDataSet.content.decoded.data = targetVars;
    dsr->dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    dsr->messageReceiveTimeout = 400.0;
    dsr->keyFrameCount = 1;
    UA_UadpDataSetReaderMessageDataType *dsrMessageSettings = UA_UadpDataSetReaderMessageDataType_new();
    ck_assert(dsrMessageSettings != 0);
    UA_UadpDataSetReaderMessageDataType_init(dsrMessageSettings);
    dsrMessageSettings->dataSetMessageContentMask = DS_MESSAGECONTENT_MASK;
    dsrMessageSettings->networkMessageContentMask = UADP_NETWORKMESSAGE_CONTENT_MASK;
    dsrMessageSettings->publishingInterval = 50;
    dsr->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    dsr->messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    dsr->messageSettings.content.decoded.data = dsrMessageSettings;

    /* encode to bytestring to simulate PubSub file config read */
    UA_UABinaryFileDataType BinaryFileData;
    UA_UABinaryFileDataType_init(&BinaryFileData);
    UA_Variant_setScalar(&BinaryFileData.body, (void*) &config, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);
    UA_ExtensionObject extObj;
    UA_ExtensionObject_init(&extObj);
    extObj.encoding = UA_EXTENSIONOBJECT_DECODED;
    extObj.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    extObj.content.decoded.data = &BinaryFileData;
    size_t fileSize = UA_ExtensionObject_calcSizeBinary(&extObj);
    encodedConfigDataBuffer.data = (UA_Byte*)UA_calloc(fileSize, sizeof(UA_Byte));
    ck_assert(encodedConfigDataBuffer.data != 0);
    encodedConfigDataBuffer.length = fileSize;
    UA_Byte *bufferPos = encodedConfigDataBuffer.data;
    ck_assert_int_eq(UA_STATUSCODE_GOOD,
        UA_ExtensionObject_encodeBinary(&extObj, &bufferPos, bufferPos + fileSize));
    UA_PubSubConfigurationDataType_clear(&config);
}
    /* load and apply config from ByteString buffer */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_loadPubSubConfigFromByteString(server, encodedConfigDataBuffer));

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(STRING_PUBLISHERID_FILE_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(STRING_PUBLISHERID_FILE_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 33, 100);

    ValidatePublishSubscribe(STRING_PUBLISHERID_FILE_MAX_COMPONENTS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 44, 100);

    UA_ByteString_clear(&encodedConfigDataBuffer);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_string_publisherId_file_config");
} END_TEST

#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */

/***************************************************************************************************/
static void DoTest_multiple_Groups(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_Groups() begin");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "raw encoding  = %s", (UseRawEncoding) ? "enabled" : "disabled");

    /*  Writers                             -> Readers              -> Var Index
        ----------------------------------------------------------------------------------
        Conn1 BYTE   Id  WG 1, DSW 1        -> Conn2, RG 2, DSR 1      0
                         WG 2, DSW 1        -> Conn2, RG 1, DSR 1      1
                         WG 3, DSW 1        -> Conn3, RG 1, DSR 1      2
                         WG 4, DSW 1        -> Conn1, RG 1, DSR 1      3
        Conn2 BYTE   Id  WG 1, DSW 1        -> Conn1, RG 3, DSR 1      4
                         WG 2, DSW 1        -> Conn3, RG 2, DSR 1      5
        Conn3 UINT16 Id  WG 1, DSW 1        -> Conn1, RG 2, DSR 1      6
                         WG 2, DSW 1        -> Conn1, RG 4, DSR 1      7
    */

#define DOTEST_MULTIPLE_GROUPS_MAX_CONNECTIONS 3
    UA_NodeId ConnectionIds[DOTEST_MULTIPLE_GROUPS_MAX_CONNECTIONS];

#define DOTEST_MULTIPLE_GROUPS_MAX_WRITERGROUPS 8
    UA_NodeId WriterGroupIds[DOTEST_MULTIPLE_GROUPS_MAX_WRITERGROUPS];

#define DOTEST_MULTIPLE_GROUPS_MAX_PDS 8
    UA_NodeId PublishedDataSetIds[DOTEST_MULTIPLE_GROUPS_MAX_PDS];

#define DOTEST_MULTIPLE_GROUPS_MAX_READERGROUPS 8
    UA_NodeId ReaderGroupIds[DOTEST_MULTIPLE_GROUPS_MAX_READERGROUPS];

    /* Attention: Publisher and corresponding Subscriber NodeId and DataValue must have the same index
       e.g. publisherVarIds[0] value is set and subscriberVarIds[0] value is checked at ValidatePublishSubscribe() function
       see table "Var Index" entry above */
#define DOTEST_MULTIPLE_GROUPS_MAX_VARS 8
    UA_NodeId publisherVarIds[DOTEST_MULTIPLE_GROUPS_MAX_VARS];
    UA_NodeId subscriberVarIds[DOTEST_MULTIPLE_GROUPS_MAX_VARS];

    UA_DataValue *fastPathPublisherDataValues[DOTEST_MULTIPLE_GROUPS_MAX_VARS];
    UA_DataValue *fastPathSubscriberDataValues[DOTEST_MULTIPLE_GROUPS_MAX_VARS];
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_GROUPS_MAX_VARS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    const UA_UInt32 DSW_Id = 1;

    /* setup all Publishers */

    /* setup Connection 1: */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_PublisherId Conn1_PublisherId;
    Conn1_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn1_PublisherId.id.byte = 1;
    AddConnection("Conn1", Conn1_PublisherId, &ConnId_1);
    ConnectionIds[0] = ConnId_1;

    /* WriterGroup 1 */
    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    const UA_UInt32 Conn1_WG1_Id = 1;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", Conn1_WG1_Id, &WGId_Conn1_WG1);
    WriterGroupIds[0] = WGId_Conn1_WG1;

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSW_Id,
                        &PDSId_Conn1_WG1_PDS1, publisherVarIds,
                        fastPathPublisherDataValues, &DsWId_Conn1_WG1_DS1);
    PublishedDataSetIds[0] = PDSId_Conn1_WG1_PDS1;

    /* WriterGroup 2 */
    UA_NodeId WGId_Conn1_WG2;
    UA_NodeId_init(&WGId_Conn1_WG2);
    const UA_UInt32 Conn1_WG2_Id = 2;
    AddWriterGroup(&ConnId_1, "Conn1_WG2", Conn1_WG2_Id, &WGId_Conn1_WG2);
    WriterGroupIds[1] = WGId_Conn1_WG2;

    UA_NodeId DsWId_Conn1_WG2_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG2_DS1);
    UA_NodeId PDSId_Conn1_WG2_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG2_PDS1);
    AddPublishedDataSet(&WGId_Conn1_WG2, "Conn1_WG2_PDS1", "Conn1_WG2_DS1", DSW_Id,
                        &PDSId_Conn1_WG2_PDS1, &publisherVarIds[1],
                        &fastPathPublisherDataValues[1], &DsWId_Conn1_WG2_DS1);
    PublishedDataSetIds[1] = PDSId_Conn1_WG2_PDS1;

    /* WriterGroup 3 */
    UA_NodeId WGId_Conn1_WG3;
    UA_NodeId_init(&WGId_Conn1_WG3);
    const UA_UInt32 Conn1_WG3_Id = 3;
    AddWriterGroup(&ConnId_1, "Conn1_WG3", Conn1_WG3_Id, &WGId_Conn1_WG3);
    WriterGroupIds[2] = WGId_Conn1_WG3;

    UA_NodeId DsWId_Conn1_WG3_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG3_DS1);
    UA_NodeId PDSId_Conn1_WG3_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG3_PDS1);
    AddPublishedDataSet(&WGId_Conn1_WG3, "Conn1_WG3_PDS1", "Conn1_WG3_DS1", DSW_Id,
                        &PDSId_Conn1_WG3_PDS1, &publisherVarIds[2],
                        &fastPathPublisherDataValues[2], &DsWId_Conn1_WG3_DS1);
    PublishedDataSetIds[2] = PDSId_Conn1_WG3_PDS1;

    /* WriterGroup 4 */
    UA_NodeId WGId_Conn1_WG4;
    UA_NodeId_init(&WGId_Conn1_WG4);
    const UA_UInt32 Conn1_WG4_Id = 4;
    AddWriterGroup(&ConnId_1, "Conn1_WG4", Conn1_WG4_Id, &WGId_Conn1_WG4);
    WriterGroupIds[3] = WGId_Conn1_WG4;

    UA_NodeId DsWId_Conn1_WG4_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG4_DS1);
    UA_NodeId PDSId_Conn1_WG4_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG4_PDS1);
    AddPublishedDataSet(&WGId_Conn1_WG4, "Conn1_WG4_PDS1", "Conn1_WG4_DS1", DSW_Id,
                        &PDSId_Conn1_WG4_PDS1, &publisherVarIds[3],
                        &fastPathPublisherDataValues[3], &DsWId_Conn1_WG4_DS1);
    PublishedDataSetIds[3] = PDSId_Conn1_WG4_PDS1;

    /* setup Connection 2: */
    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    UA_PublisherId Conn2_PublisherId;
    Conn2_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn2_PublisherId.id.byte = 2;
    AddConnection("Conn2", Conn2_PublisherId, &ConnId_2);
    ConnectionIds[1] = ConnId_2;

    /* WriterGroup 1 */
    UA_NodeId WGId_Conn2_WG1;
    UA_NodeId_init(&WGId_Conn2_WG1);
    const UA_UInt32 Conn2_WG1_Id = 1;
    AddWriterGroup(&ConnId_2, "Conn2_WG1", Conn2_WG1_Id, &WGId_Conn2_WG1);
    WriterGroupIds[4] = WGId_Conn2_WG1;

    UA_NodeId DsWId_Conn2_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG1_DS1);
    UA_NodeId PDSId_Conn2_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS1", "Conn2_WG1_DS1", DSW_Id,
                        &PDSId_Conn2_WG1_PDS1, &publisherVarIds[4],
                        &fastPathPublisherDataValues[4], &DsWId_Conn2_WG1_DS1);
    PublishedDataSetIds[4] = PDSId_Conn2_WG1_PDS1;

    /* WriterGroup 2 */
    UA_NodeId WGId_Conn2_WG2;
    UA_NodeId_init(&WGId_Conn2_WG2);
    const UA_UInt32 Conn2_WG2_Id = 2;
    AddWriterGroup(&ConnId_2, "Conn2_WG2", Conn2_WG2_Id, &WGId_Conn2_WG2);
    WriterGroupIds[5] = WGId_Conn2_WG2;

    UA_NodeId DsWId_Conn2_WG2_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG2_DS1);
    UA_NodeId PDSId_Conn2_WG2_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG2_PDS1);
    AddPublishedDataSet(&WGId_Conn2_WG2, "Conn2_WG2_PDS1", "Conn2_WG2_DS1", DSW_Id,
                        &PDSId_Conn2_WG2_PDS1, &publisherVarIds[5],
                        &fastPathPublisherDataValues[5], &DsWId_Conn2_WG2_DS1);
    PublishedDataSetIds[5] = PDSId_Conn2_WG2_PDS1;

    /* setup Connection 3: */
    UA_NodeId ConnId_3;
    UA_NodeId_init(&ConnId_3);
    UA_PublisherId Conn3_PublisherId;
    Conn3_PublisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    Conn3_PublisherId.id.uint16 = 1;
    AddConnection("Conn3", Conn3_PublisherId, &ConnId_3);
    ConnectionIds[2] = ConnId_3;

    /* WriterGroup 1 */
    UA_NodeId WGId_Conn3_WG1;
    UA_NodeId_init(&WGId_Conn3_WG1);
    const UA_UInt32 Conn3_WG1_Id = 1;
    AddWriterGroup(&ConnId_3, "Conn3_WG1", Conn3_WG1_Id, &WGId_Conn3_WG1);
    WriterGroupIds[6] = WGId_Conn3_WG1;

    UA_NodeId DsWId_Conn3_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn3_WG1_DS1);
    UA_NodeId PDSId_Conn3_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn3_WG1_PDS1);
    AddPublishedDataSet(&WGId_Conn3_WG1, "Conn3_WG1_PDS1", "Conn3_WG1_DS1", DSW_Id,
                        &PDSId_Conn3_WG1_PDS1, &publisherVarIds[6],
                        &fastPathPublisherDataValues[6], &DsWId_Conn3_WG1_DS1);
    PublishedDataSetIds[6] = PDSId_Conn3_WG1_PDS1;

    /* WriterGroup 2 */
    UA_NodeId WGId_Conn3_WG2;
    UA_NodeId_init(&WGId_Conn3_WG2);
    const UA_UInt32 Conn3_WG2_Id = 2;
    AddWriterGroup(&ConnId_3, "Conn3_WG2", Conn3_WG2_Id, &WGId_Conn3_WG2);
    WriterGroupIds[7] = WGId_Conn3_WG2;

    UA_NodeId DsWId_Conn3_WG2_DS1;
    UA_NodeId_init(&DsWId_Conn3_WG2_DS1);
    UA_NodeId PDSId_Conn3_WG2_PDS1;
    UA_NodeId_init(&PDSId_Conn3_WG2_PDS1);
    AddPublishedDataSet(&WGId_Conn3_WG2, "Conn3_WG2_PDS1", "Conn3_WG2_DS1", DSW_Id,
                        &PDSId_Conn3_WG2_PDS1, &publisherVarIds[7],
                        &fastPathPublisherDataValues[7], &DsWId_Conn3_WG2_DS1);
    PublishedDataSetIds[7] = PDSId_Conn3_WG2_PDS1;

    /* setup all Subscribers */

    /* setup Connection 1: */

    /* ReaderGroup 1 */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", Conn1_PublisherId, Conn1_WG4_Id,
                     DSW_Id, &subscriberVarIds[3],
                     &fastPathSubscriberDataValues[3], &DSRId_Conn1_RG1_DSR1);
    ReaderGroupIds[0] = RGId_Conn1_RG1;

    /* ReaderGroup 2 */
    UA_NodeId RGId_Conn1_RG2;
    UA_NodeId_init(&RGId_Conn1_RG2);
    AddReaderGroup(&ConnId_1, "Conn1_RG2", &RGId_Conn1_RG2);
    UA_NodeId DSRId_Conn1_RG2_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG2_DSR1);
    AddDataSetReader(&RGId_Conn1_RG2, "Conn1_RG2_DSR1", Conn3_PublisherId, Conn3_WG1_Id,
                     DSW_Id, &subscriberVarIds[6], &fastPathSubscriberDataValues[6],
                     &DSRId_Conn1_RG2_DSR1);
    ReaderGroupIds[1] = RGId_Conn1_RG2;

    /* ReaderGroup 3 */
    UA_NodeId RGId_Conn1_RG3;
    UA_NodeId_init(&RGId_Conn1_RG3);
    AddReaderGroup(&ConnId_1, "Conn1_RG3", &RGId_Conn1_RG3);
    UA_NodeId DSRId_Conn1_RG3_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG3_DSR1);
    AddDataSetReader(&RGId_Conn1_RG3, "Conn1_RG3_DSR1", Conn2_PublisherId, Conn2_WG1_Id,
                     DSW_Id, &subscriberVarIds[4],
                     &fastPathSubscriberDataValues[4], &DSRId_Conn1_RG3_DSR1);
    ReaderGroupIds[2] = RGId_Conn1_RG3;

    /* ReaderGroup 4 */
    UA_NodeId RGId_Conn1_RG4;
    UA_NodeId_init(&RGId_Conn1_RG4);
    AddReaderGroup(&ConnId_1, "Conn1_RG4", &RGId_Conn1_RG4);
    UA_NodeId DSRId_Conn1_RG4_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG4_DSR1);
    AddDataSetReader(&RGId_Conn1_RG4, "Conn1_RG4_DSR1", Conn3_PublisherId, Conn3_WG2_Id,
                     DSW_Id, &subscriberVarIds[7], &fastPathSubscriberDataValues[7],
                     &DSRId_Conn1_RG4_DSR1);
    ReaderGroupIds[3] = RGId_Conn1_RG4;

    /* setup Connection 2: */

    /* ReaderGroup 1 */
    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);
    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1", Conn1_PublisherId, Conn1_WG2_Id,
                     DSW_Id, &subscriberVarIds[1], &fastPathSubscriberDataValues[1],
                     &DSRId_Conn2_RG1_DSR1);
    ReaderGroupIds[4] = RGId_Conn2_RG1;

    /* ReaderGroup 2 */
    UA_NodeId RGId_Conn2_RG2;
    UA_NodeId_init(&RGId_Conn2_RG2);
    AddReaderGroup(&ConnId_2, "Conn2_RG2", &RGId_Conn2_RG2);
    UA_NodeId DSRId_Conn2_RG2_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG2_DSR1);
    AddDataSetReader(&RGId_Conn2_RG2, "Conn2_RG2_DSR1", Conn1_PublisherId, Conn1_WG1_Id,
                     DSW_Id, subscriberVarIds, fastPathSubscriberDataValues,
                     &DSRId_Conn2_RG2_DSR1);
    ReaderGroupIds[5] = RGId_Conn2_RG2;

    /* setup Connection 3: */

    /* ReaderGroup 1 */
    UA_NodeId RGId_Conn3_RG1;
    UA_NodeId_init(&RGId_Conn3_RG1);
    AddReaderGroup(&ConnId_3, "Conn3_RG1", &RGId_Conn3_RG1);
    UA_NodeId DSRId_Conn3_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn3_RG1_DSR1);
    AddDataSetReader(&RGId_Conn3_RG1, "Conn3_RG1_DSR1", Conn1_PublisherId, Conn1_WG3_Id,
                     DSW_Id, &subscriberVarIds[2], &fastPathSubscriberDataValues[2],
                     &DSRId_Conn3_RG1_DSR1);
    ReaderGroupIds[6] = RGId_Conn3_RG1;

    /* ReaderGroup 2 */
    UA_NodeId RGId_Conn3_RG2;
    UA_NodeId_init(&RGId_Conn3_RG2);
    AddReaderGroup(&ConnId_3, "Conn3_RG2", &RGId_Conn3_RG2);
    UA_NodeId DSRId_Conn3_RG2_DSR1;
    UA_NodeId_init(&DSRId_Conn3_RG2_DSR1);
    AddDataSetReader(&RGId_Conn3_RG2, "Conn3_RG2_DSR1", Conn2_PublisherId, Conn2_WG2_Id,
                     DSW_Id, &subscriberVarIds[5], &fastPathSubscriberDataValues[5],
                     &DSRId_Conn3_RG2_DSR1);
    ReaderGroupIds[7] = RGId_Conn3_RG2;

    /* set groups operational */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(DOTEST_MULTIPLE_GROUPS_MAX_VARS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_GROUPS_MAX_VARS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 50, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_GROUPS_MAX_VARS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 100, 100);

    /* set groups to disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable groups");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_GROUPS_MAX_WRITERGROUPS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setWriterGroupDisabled(server, WriterGroupIds[i]));
    }
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_GROUPS_MAX_READERGROUPS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setReaderGroupDisabled(server, ReaderGroupIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove Connections");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_GROUPS_MAX_CONNECTIONS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePubSubConnection(server, ConnectionIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove PublishedDataSets");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_GROUPS_MAX_PDS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, PublishedDataSetIds[i]));
    }

    /* Iterate so the connections are actually deleted */
    UA_Server_run_iterate(server, false);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_Groups() end");
}

/***************************************************************************************************/
START_TEST(Test_multiple_groups) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_multiple_groups");

    UseRawEncoding = UA_FALSE;
    DoTest_multiple_Groups();

    UseRawEncoding = UA_TRUE;
    DoTest_multiple_Groups();

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_multiple_groups");
} END_TEST


/***************************************************************************************************/
static void DoTest_multiple_DataSets(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_DataSets() begin");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "raw encoding  = %s", (UseRawEncoding) ? "enabled" : "disabled");

    /*  Writers                             -> Readers              -> Var Index
        ----------------------------------------------------------------------------------
        Conn1 BYTE   Id  WG 1, DSW 1        -> Conn2, RG 1, DSR 1      0
                         WG 1, DSW 2        -> Conn2, RG 1, DSR 3      1
                         WG 1, DSW 3        -> Conn2, RG 1, DSR 2      2
                         WG 1, DSW 4        -> Conn1, RG 1, DSR 1      3
        Conn2 UINT16 Id  WG 1, DSW 1        -> Conn1, RG 1, DSR 2      4
                         WG 1, DSW 2        -> Conn2, RG 1, DSR 4      5
    */

#define DOTEST_MULTIPLE_DATASETS_MAX_CONNECTIONS 2
    UA_NodeId ConnectionIds[DOTEST_MULTIPLE_DATASETS_MAX_CONNECTIONS];

#define DOTEST_MULTIPLE_DATASETS_MAX_WRITERGROUPS 2
    UA_NodeId WriterGroupIds[DOTEST_MULTIPLE_DATASETS_MAX_WRITERGROUPS];

#define DOTEST_MULTIPLE_DATASETS_MAX_PDS 6
    UA_NodeId PublishedDataSetIds[DOTEST_MULTIPLE_DATASETS_MAX_PDS];

#define DOTEST_MULTIPLE_DATASETS_MAX_READERGROUPS 2
    UA_NodeId ReaderGroupIds[DOTEST_MULTIPLE_DATASETS_MAX_READERGROUPS];

    /* Attention: Publisher and corresponding Subscriber NodeId and DataValue must have the same index
       e.g. publisherVarIds[0] value is set and subscriberVarIds[0] value is checked at ValidatePublishSubscribe() function
       see table "Var Index" entry above */
#define DOTEST_MULTIPLE_DATASETS_MAX_VARS 6
    UA_NodeId publisherVarIds[DOTEST_MULTIPLE_DATASETS_MAX_VARS];
    UA_NodeId subscriberVarIds[DOTEST_MULTIPLE_DATASETS_MAX_VARS];

    UA_DataValue *fastPathPublisherDataValues[DOTEST_MULTIPLE_DATASETS_MAX_VARS];
    UA_DataValue *fastPathSubscriberDataValues[DOTEST_MULTIPLE_DATASETS_MAX_VARS];
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_DATASETS_MAX_VARS; i++) {
        fastPathPublisherDataValues[i] = 0;
        fastPathSubscriberDataValues[i] = 0;
    }

    const UA_UInt32 WG_Id = 1;

    /* setup all Publishers */

    /* setup Connection 1: */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_PublisherId Conn1_PublisherId;
    Conn1_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn1_PublisherId.id.byte = 1;
    AddConnection("Conn1", Conn1_PublisherId, &ConnId_1);
    ConnectionIds[0] = ConnId_1;

    /* WriterGroup 1 */
    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WG_Id, &WGId_Conn1_WG1);
    WriterGroupIds[0] = WGId_Conn1_WG1;

    /* DataSetWriter 1 */
    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    const UA_UInt32 Conn1_WG1_DSW1_Id = 1;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", Conn1_WG1_DSW1_Id, &PDSId_Conn1_WG1_PDS1,
        publisherVarIds, fastPathPublisherDataValues, &DsWId_Conn1_WG1_DS1);
    PublishedDataSetIds[0] = PDSId_Conn1_WG1_PDS1;

    /* DataSetWriter 2 */
    UA_NodeId DsWId_Conn1_WG1_DS2;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS2);
    UA_NodeId PDSId_Conn1_WG1_PDS2;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS2);
    const UA_UInt32 Conn1_WG1_DSW2_Id = 2;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG2_PDS1", "Conn1_WG2_DS1", Conn1_WG1_DSW2_Id, &PDSId_Conn1_WG1_PDS2,
        &publisherVarIds[1], &fastPathPublisherDataValues[1], &DsWId_Conn1_WG1_DS2);
    PublishedDataSetIds[1] = PDSId_Conn1_WG1_PDS2;

    /* DataSetWriter 3 */
    UA_NodeId DsWId_Conn1_WG1_DS3;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS3);
    UA_NodeId PDSId_Conn1_WG1_PDS3;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS3);
    const UA_UInt32 Conn1_WG1_DSW3_Id = 3;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG2_PDS3", "Conn1_WG2_DS3", Conn1_WG1_DSW3_Id, &PDSId_Conn1_WG1_PDS3,
        &publisherVarIds[2], &fastPathPublisherDataValues[2], &DsWId_Conn1_WG1_DS3);
    PublishedDataSetIds[2] = PDSId_Conn1_WG1_PDS3;

    /* DataSetWriter 4 */
    UA_NodeId DsWId_Conn1_WG1_DS4;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS4);
    UA_NodeId PDSId_Conn1_WG1_PDS4;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS4);
    const UA_UInt32 Conn1_WG1_DSW4_Id = 4;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG2_PDS4", "Conn1_WG2_DS4", Conn1_WG1_DSW4_Id, &PDSId_Conn1_WG1_PDS4,
        &publisherVarIds[3], &fastPathPublisherDataValues[3], &DsWId_Conn1_WG1_DS4);
    PublishedDataSetIds[3] = PDSId_Conn1_WG1_PDS4;

    /* setup Connection 2: */
    UA_NodeId ConnId_2;
    UA_NodeId_init(&ConnId_2);
    UA_PublisherId Conn2_PublisherId;
    Conn2_PublisherId.idType = UA_PUBLISHERIDTYPE_BYTE;
    Conn2_PublisherId.id.byte = 2;
    AddConnection("Conn2", Conn2_PublisherId, &ConnId_2);
    ConnectionIds[1] = ConnId_2;

    /* WriterGroup 1 */
    UA_NodeId WGId_Conn2_WG1;
    UA_NodeId_init(&WGId_Conn2_WG1);
    AddWriterGroup(&ConnId_2, "Conn2_WG1", WG_Id, &WGId_Conn2_WG1);
    WriterGroupIds[1] = WGId_Conn2_WG1;

    /* DataSetWriter 1 */
    UA_NodeId DsWId_Conn2_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn2_WG1_DS1);
    UA_NodeId PDSId_Conn2_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn2_WG1_PDS1);
    const UA_UInt32 Conn2_WG1_DSW1_Id = 1;
    AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS1", "Conn2_WG1_DS1", Conn2_WG1_DSW1_Id, &PDSId_Conn2_WG1_PDS1,
        &publisherVarIds[4], &fastPathPublisherDataValues[4], &DsWId_Conn2_WG1_DS1);
    PublishedDataSetIds[4] = PDSId_Conn2_WG1_PDS1;

    /* DataSetWriter 2 */
    UA_NodeId DsWId_Conn2_WG1_DS2;
    UA_NodeId_init(&DsWId_Conn2_WG1_DS2);
    UA_NodeId PDSId_Conn2_WG1_PDS2;
    UA_NodeId_init(&PDSId_Conn2_WG1_PDS2);
    const UA_UInt32 Conn2_WG1_DSW2_Id = 2;
    AddPublishedDataSet(&WGId_Conn2_WG1, "Conn2_WG1_PDS2", "Conn2_WG1_DS2", Conn2_WG1_DSW2_Id, &PDSId_Conn2_WG1_PDS2,
        &publisherVarIds[5], &fastPathPublisherDataValues[5], &DsWId_Conn2_WG1_DS2);
    PublishedDataSetIds[5] = PDSId_Conn2_WG1_PDS2;

    /* setup all Subscribers */

    /* setup Connection 1: */

    /* ReaderGroup 1 */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);
    ReaderGroupIds[0] = RGId_Conn1_RG1;

    /* DataSetReader 1 */
    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1",
                     Conn1_PublisherId, WG_Id, Conn1_WG1_DSW4_Id,
        &subscriberVarIds[3], &fastPathSubscriberDataValues[3], &DSRId_Conn1_RG1_DSR1);

    /* DataSetReader 2 */
    UA_NodeId DSRId_Conn1_RG1_DSR2;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR2);
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR2",
                     Conn2_PublisherId, WG_Id, Conn1_WG1_DSW1_Id,
        &subscriberVarIds[4], &fastPathSubscriberDataValues[4], &DSRId_Conn1_RG1_DSR2);

    /* setup Connection 2: */

    /* ReaderGroup 1 */
    UA_NodeId RGId_Conn2_RG1;
    UA_NodeId_init(&RGId_Conn2_RG1);
    AddReaderGroup(&ConnId_2, "Conn2_RG1", &RGId_Conn2_RG1);
    ReaderGroupIds[1] = RGId_Conn2_RG1;

    /* DataSetReader 1 */
    UA_NodeId DSRId_Conn2_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR1);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR1",
                     Conn1_PublisherId, WG_Id, Conn1_WG1_DSW1_Id,
        subscriberVarIds, fastPathSubscriberDataValues, &DSRId_Conn2_RG1_DSR1);

    /* DataSetReader 2 */
    UA_NodeId DSRId_Conn2_RG1_DSR2;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR2);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR2",
                     Conn1_PublisherId, WG_Id, Conn1_WG1_DSW3_Id,
        &subscriberVarIds[2], &fastPathSubscriberDataValues[2], &DSRId_Conn2_RG1_DSR2);

    /* DataSetReader 3 */
    UA_NodeId DSRId_Conn2_RG1_DSR3;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR3);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR3",
                     Conn1_PublisherId, WG_Id, Conn1_WG1_DSW2_Id,
        &subscriberVarIds[1], &fastPathSubscriberDataValues[1], &DSRId_Conn2_RG1_DSR3);

    /* DataSetReader 4 */
    UA_NodeId DSRId_Conn2_RG1_DSR4;
    UA_NodeId_init(&DSRId_Conn2_RG1_DSR4);
    AddDataSetReader(&RGId_Conn2_RG1, "Conn2_RG1_DSR4",
                     Conn2_PublisherId, WG_Id, Conn1_WG1_DSW2_Id,
        &subscriberVarIds[5], &fastPathSubscriberDataValues[5], &DSRId_Conn2_RG1_DSR4);

    /* set groups operational */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* check that publish/subscribe works -> set some test values */
    ValidatePublishSubscribe(DOTEST_MULTIPLE_DATASETS_MAX_VARS, publisherVarIds, subscriberVarIds,
                             fastPathPublisherDataValues, fastPathSubscriberDataValues, 10, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_DATASETS_MAX_VARS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 50, 100);

    ValidatePublishSubscribe(DOTEST_MULTIPLE_DATASETS_MAX_VARS, publisherVarIds,
                             subscriberVarIds, fastPathPublisherDataValues,
                             fastPathSubscriberDataValues, 100, 100);

    /* set groups to disabled */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "disable groups");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_DATASETS_MAX_WRITERGROUPS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setWriterGroupDisabled(server, WriterGroupIds[i]));
    }
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_DATASETS_MAX_READERGROUPS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_setReaderGroupDisabled(server, ReaderGroupIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove Connections");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_DATASETS_MAX_CONNECTIONS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePubSubConnection(server, ConnectionIds[i]));
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "remove PublishedDataSets");
    for (UA_UInt32 i = 0; i < DOTEST_MULTIPLE_DATASETS_MAX_PDS; i++) {
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_removePublishedDataSet(server, PublishedDataSetIds[i]));
    }

    /* Iterate so the connections are actually deleted */
    UA_Server_run_iterate(server, false);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "DoTest_multiple_DataSets() end");
}

/***************************************************************************************************/
START_TEST(Test_multiple_datasets) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_multiple_datasets");

    UseRawEncoding = UA_FALSE;
    DoTest_multiple_DataSets();

    UseRawEncoding = UA_TRUE;
    DoTest_multiple_DataSets();

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_multiple_datasets");
} END_TEST


/***************************************************************************************************/
int main(void) {

    TCase *tc_basic = tcase_create("PublisherId");
    tcase_add_checked_fixture(tc_basic, setup, teardown);

    /* test case description:
        - test 1 connection with 1 WriterGroup, 1 DatasetWriter, 1 ReaderGroup and 1 DataSetReader
        - test all possible publisherId types
        - test with all combinations of fast-path and raw-encoding
    */
    tcase_add_test(tc_basic, Test_1_connection);

    /* test case description:
        - setup a PubSub configuration with multiple Connections
        - all WriterGroup Ids and DataSetWriter Ids are equal, because we want to test the publisherId check
        - use the same publishing interval for all groups to ensure that the Readers receive multiple messages at the same time
        - set different publishing values to ensure that PublisherId check works and every DataSetReader receives the correct message
        - test all different publisherId types
        - test with all combinations of fast-path and raw-encoding
        - TODO: fast-path does not support multiple groups and datasets, therefore we only test multiple connections with fast-path here
        - TODO: fast-path does not support STRING publisherIds
    */
    tcase_add_test(tc_basic, Test_multiple_connections);

    /* test case description:
        - setup a PubSub configuration with multiple Connections
        - all connections have a STRING PublisherId
        - all WriterGroup Ids and DataSetWriter Ids are equal, because we want to test the publisherId check
        - use the same publishing interval for all groups to ensure that the Readers receive multiple messages at the same time
        - set different publishing values to ensure that PublisherId check works and every DataSetReader receives the correct message
        - test with and without raw-encoding
        - TODO: fast-path does not support string PublisherIds at the moment
    */
    tcase_add_test(tc_basic, Test_string_publisherId);

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
    /* test case description:
        - test pubsub file config with string PublisherId
    */
    tcase_add_test(tc_basic, Test_string_publisherId_file_config);
#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */

    /* test case description:
        - setup a PubSub configuration with multiple Groups
        - use the same publishing interval for all groups to ensure that the Readers receive multiple messages at the same time
        - set different publishing values to ensure that PublisherId check works and every DataSetReader receives the correct message
        - test with with and without raw-encoding
    */
    tcase_add_test(tc_basic, Test_multiple_groups);

    /* test case description:
        - setup a PubSub configuration with multiple DataSets
        - use the same publishing interval for all groups to ensure that the Readers receive multiple messages at the same time
        - set different publishing values to ensure that PublisherId check works and every DataSetReader receives the correct message
        - test with with and without raw-encoding
    */
    tcase_add_test(tc_basic, Test_multiple_datasets);

    Suite *s = suite_create("PubSub publisherId tests");
    suite_add_tcase(s, tc_basic);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
