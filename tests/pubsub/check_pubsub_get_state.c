#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>

#include "ua_pubsub.h"
#include <check.h>
#include <assert.h>

static UA_Server *server = NULL;

/***************************************************************************************************/
static void setup(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "setup");

    server = UA_Server_new();
    assert(server != 0);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
    UA_Server_run_startup(server);
}

/***************************************************************************************************/
static void teardown(void) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "teardown");

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}


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

    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = PublisherId;

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
START_TEST(Test_normal_operation) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_normal_operation");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "configure pubsub");

    /* setup Connection 1: writer */
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

    /* setup Connection 1: reader */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 350.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS1,
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);


    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "check state");
    UA_PubSubState state = UA_PUBSUBSTATE_ERROR;

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_ReaderGroup_getState(server, RGId_Conn1_RG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set groups operational");

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_ReaderGroup_getState(server, RGId_Conn1_RG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "set groups disabled");
    ck_assert(UA_Server_setWriterGroupDisabled(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);

    ck_assert(UA_Server_setReaderGroupDisabled(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_ReaderGroup_getState(server, RGId_Conn1_RG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_DISABLED, state);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_normal_operation");

} END_TEST


/***************************************************************************************************/
START_TEST(Test_corner_cases) {

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "START: Test_corner_cases");

    UA_NodeId id = UA_NODEID_NULL;

    UA_PubSubState state = UA_PUBSUBSTATE_ERROR;
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_WriterGroup_getState(0, id, 0));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_WriterGroup_getState(server, id, 0));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_WriterGroup_getState(0, id, &state));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_WriterGroup_getState(server, id, &state));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "configure pubsub");

    /* setup Connection 1: writer */
    UA_NodeId ConnId_1;
    UA_NodeId_init(&ConnId_1);
    UA_UInt32 PublisherNo_Conn1 = 1;
    AddConnection("Conn1", PublisherNo_Conn1, &ConnId_1);

    UA_NodeId WGId_Conn1_WG1;
    UA_NodeId_init(&WGId_Conn1_WG1);
    UA_UInt32 WGNo_Conn1_WG1 = 1;
    UA_Duration PublishingInterval_Conn1_WG1 = 500.0;
    AddWriterGroup(&ConnId_1, "Conn1_WG1", WGNo_Conn1_WG1, PublishingInterval_Conn1_WG1, &WGId_Conn1_WG1);

    ck_assert(UA_Server_setWriterGroupOperational(server, WGId_Conn1_WG1) == UA_STATUSCODE_GOOD);

    UA_NodeId DsWId_Conn1_WG1_DS1;
    UA_NodeId_init(&DsWId_Conn1_WG1_DS1);
    UA_NodeId VarId_Conn1_WG1_DS1;
    UA_NodeId_init(&VarId_Conn1_WG1_DS1);
    UA_NodeId PDSId_Conn1_WG1_PDS1;
    UA_NodeId_init(&PDSId_Conn1_WG1_PDS1);
    UA_UInt32 DSWNo_Conn1_WG1_DS1 = 1;
    AddPublishedDataSet(&WGId_Conn1_WG1, "Conn1_WG1_PDS1", "Conn1_WG1_DS1", DSWNo_Conn1_WG1_DS1, &PDSId_Conn1_WG1_PDS1,
        &VarId_Conn1_WG1_DS1, &DsWId_Conn1_WG1_DS1);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_WriterGroup_getState(server, WGId_Conn1_WG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);
    /* DataSetWriter is should be operational as well */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetWriter_getState(server, DsWId_Conn1_WG1_DS1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);

    /* setup Connection 1: reader */
    UA_NodeId RGId_Conn1_RG1;
    UA_NodeId_init(&RGId_Conn1_RG1);
    AddReaderGroup(&ConnId_1, "Conn1_RG1", &RGId_Conn1_RG1);

    ck_assert(UA_Server_setReaderGroupOperational(server, RGId_Conn1_RG1) == UA_STATUSCODE_GOOD);

    UA_NodeId DSRId_Conn1_RG1_DSR1;
    UA_NodeId_init(&DSRId_Conn1_RG1_DSR1);
    UA_NodeId VarId_Conn1_RG1_DSR1;
    UA_NodeId_init(&VarId_Conn1_RG1_DSR1);
    UA_Duration MessageReceiveTimeout_Conn1_RG1_DSR1 = 350.0;
    AddDataSetReader(&RGId_Conn1_RG1, "Conn1_RG1_DSR1", PublisherNo_Conn1, WGNo_Conn1_WG1, DSWNo_Conn1_WG1_DS1,
        MessageReceiveTimeout_Conn1_RG1_DSR1, &VarId_Conn1_RG1_DSR1, &DSRId_Conn1_RG1_DSR1);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_ReaderGroup_getState(server, RGId_Conn1_RG1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);
    /* DataSetReader should be operational as well */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_DataSetReader_getState(server, DSRId_Conn1_RG1_DSR1, &state));
    ck_assert_int_eq(UA_PUBSUBSTATE_OPERATIONAL, state);

    /* test wrong nodeIds */
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_DataSetReader_getState(server, RGId_Conn1_RG1, &state));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_DataSetWriter_getState(server, RGId_Conn1_RG1, &state));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_ReaderGroup_getState(server, WGId_Conn1_WG1, &state));
    ck_assert(UA_STATUSCODE_GOOD != UA_Server_WriterGroup_getState(server, RGId_Conn1_RG1, &state));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "END: Test_corner_cases");

} END_TEST

/***************************************************************************************************/
int main(void) {

    TCase *tc_normal_operation = tcase_create("normal_operation");
    tcase_add_checked_fixture(tc_normal_operation, setup, teardown);
    tcase_add_test(tc_normal_operation, Test_normal_operation);

    TCase *tc_corner_cases = tcase_create("corner cases");
    tcase_add_checked_fixture(tc_corner_cases, setup, teardown);
    tcase_add_test(tc_corner_cases, Test_corner_cases);

    Suite *s = suite_create("PubSub getState test suite");
    suite_add_tcase(s, tc_normal_operation);
    suite_add_tcase(s, tc_corner_cases);

    /* TODO: how to provoke and test an error state? */

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
