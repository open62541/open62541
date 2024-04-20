/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "test_helpers.h"
#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"
#include <server/ua_server_internal.h>

#include "testing_clock.h"

#include <check.h>
#include <stdio.h>

UA_EventLoop *rtEventLoop = NULL;
UA_Server *server = NULL;
UA_NodeId connectionIdentifier, publishedDataSetIdent, writerGroupIdent, dataSetWriterIdent, dataSetFieldIdent;

UA_DataValue *staticSource1, *staticSource2;

#define PUBLISH_INTERVAL         10       /* Publish interval*/

static UA_StatusCode
addMinimalPubSubConfiguration(void){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    /* Add one PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    connectionConfig.eventLoop = rtEventLoop;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.id.uint32 = UA_UInt32_random();
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;
    /* Add one PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Add one DataSetField to the PDS */
    UA_AddPublishedDataSetResult addResult = UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
    return addResult.addResult;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);

    rtEventLoop = UA_EventLoop_new_POSIX(server->config.logging);

    /* Add the TCP connection manager */
    UA_ConnectionManager *tcpCM =
        UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcp connection manager"));
    rtEventLoop->registerEventSource(rtEventLoop, (UA_EventSource *)tcpCM);

    /* Add the UDP connection manager */
    UA_ConnectionManager *udpCM =
        UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp connection manager"));
    rtEventLoop->registerEventSource(rtEventLoop, (UA_EventSource *)udpCM);

    rtEventLoop->start(rtEventLoop);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);

    if(rtEventLoop->state != UA_EVENTLOOPSTATE_FRESH &&
       rtEventLoop->state != UA_EVENTLOOPSTATE_STOPPED) {
        rtEventLoop->stop(rtEventLoop);
        while(rtEventLoop->state != UA_EVENTLOOPSTATE_STOPPED) {
            rtEventLoop->run(rtEventLoop, 100);
        }
    }

    UA_Server_delete(server);
    rtEventLoop->logger = NULL; /* Don't access the logger that was removed with
                                   the server */
    rtEventLoop->free(rtEventLoop);

    rtEventLoop = NULL;
    server = NULL;

    /* Clean these up only after the server is done */
    if(staticSource1) {
        UA_DataValue_delete(staticSource1);
        staticSource1 = NULL;
    }
    if(staticSource2) {
        UA_DataValue_delete(staticSource2);
        staticSource2 = NULL;
    }
}

START_TEST(PublishSingleFieldWithStaticValueSource) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));

        /* Create Variant and configure as DataSetField source */
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = 1000;
        staticSource1 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource1->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource1;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

        /* DataSetWriter muste be added AFTER addDataSetField otherwize lastSamples will be uninitialized */
        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(PublishSingleFieldWithDifferentBinarySizes) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);

        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Test WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_StatusCode res = UA_Server_addWriterGroup(server, connectionIdentifier,
                                                     &writerGroupConfig, &writerGroupIdent);
        ck_assert(res == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        /* Create Variant and configure as DataSetField source */
        UA_String stringValue = UA_STRING("12345");
        staticSource1 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource1->value, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
        staticSource1->value.storageType = UA_VARIANT_DATA_NODELETE;
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource1;
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        UA_Server_run_iterate(server, false);
    } END_TEST

START_TEST(SetupInvalidPubSubConfigWithStaticValueSource) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Test WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;

        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField(server, publishedDataSetIdent,
                                  &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_BADCONFIGURATIONERROR);
    } END_TEST

START_TEST(PublishSingleFieldWithFixedOffsets) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        // Create Variant and configure as DataSetField source
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = (UA_UInt32) 1000;
        staticSource1 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource1->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource1;
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server, true);

        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        /* run server - publisher and subscriber */
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        rtEventLoop->run(rtEventLoop, 100);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        rtEventLoop->run(rtEventLoop, 100);


        UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(PublishPDSWithMultipleFieldsAndFixedOffset) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        // Create Variant and configure as DataSetField source
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = (UA_UInt32) 1000;
        staticSource1 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource1->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.rtInformationModelNode = UA_FALSE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource1;
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, NULL).result == UA_STATUSCODE_GOOD);
        UA_UInt32 *intValue2 = UA_UInt32_new();
        *intValue2 = (UA_UInt32) 2000;
        staticSource2 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource2->value, intValue2, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource2;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, NULL).result == UA_STATUSCODE_GOOD);

        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server, true);
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(PublishSingleFieldInCustomCallback) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("Test DataSetWriter");
        dataSetWriterConfig.dataSetWriterId = 62541;
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        // Create Variant and configure as DataSetField source
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = (UA_UInt32) 1000;
        staticSource1 = UA_DataValue_new();
        UA_Variant_setScalar(&staticSource1->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticSource1;
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent).result == UA_STATUSCODE_GOOD);

        ck_assert(UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent) == UA_STATUSCODE_GOOD);
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server, true);
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        /* run server - publisher and subscriber */
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        rtEventLoop->run(rtEventLoop, 100);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        rtEventLoop->run(rtEventLoop, 100);


        UA_Server_run_iterate(server, false);
} END_TEST

static UA_StatusCode
simpleNotificationRead(UA_Server *srv, const UA_NodeId *sessionId,
                       void *sessionContext, const UA_NodeId *nodeid,
                       void *nodeContext, const UA_NumericRange *range){
    //allow read without any preparation
    return UA_STATUSCODE_GOOD;
}

static UA_NodeId *nodes[3];
static UA_NodeId *dsf[3];
static UA_UInt32 *values[3];
static UA_NodeId variableNodeId;
static UA_UInt32 *integerRTValue;

static UA_StatusCode
externalDataWriteCallback(UA_Server *s, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, const UA_NumericRange *range,
                          const UA_DataValue *data){
    if(UA_NodeId_equal(nodeId, nodes[0])){
        memcpy(values[0], data->value.data, sizeof(UA_UInt32));
    } else if(UA_NodeId_equal(nodeId, nodes[1])){
        memcpy(values[1], data->value.data, sizeof(UA_UInt32));
    } else if(UA_NodeId_equal(nodeId, nodes[2])){
        memcpy(values[2], data->value.data, sizeof(UA_UInt32));
    } else if(UA_NodeId_equal(nodeId, &variableNodeId)){
        memcpy(integerRTValue, data->value.data, sizeof(UA_UInt32));
    }
    return UA_STATUSCODE_GOOD;
}

START_TEST(PubSubConfigWithInformationModelRTVariable) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        //add a new variable to the information model
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_UInt32 myInteger = 42;
        UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_UINT32]);
        attr.description = UA_LOCALIZEDTEXT("en-US","test node");
        attr.displayName = UA_LOCALIZEDTEXT("en-US","test node");
        attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "test node");
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        ck_assert_int_eq(UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                  parentReferenceNodeId, myIntegerName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &variableNodeId), UA_STATUSCODE_GOOD);
        ck_assert(!UA_NodeId_isNull(&variableNodeId));
        UA_Variant variant;
        UA_Variant_init(&variant);
        UA_Server_readValue(server, variableNodeId, &variant);
        ck_assert(*((UA_UInt32 *)variant.data) == 42);
        //use the added var in rt config
        integerRTValue = UA_UInt32_new();
        *integerRTValue = 42;
        UA_DataValue *externalValueSourceDataValue = UA_DataValue_new();
        UA_Variant_setScalar(&externalValueSourceDataValue->value, integerRTValue, &UA_TYPES[UA_TYPES_UINT32]);
        UA_ValueBackend valueBackend;
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &externalValueSourceDataValue;
        valueBackend.backend.external.callback.notificationRead = simpleNotificationRead;
        valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
        UA_Server_setVariableNode_valueBackend(server, variableNodeId, valueBackend);
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        dsfConfig.field.variable.publishParameters.publishedVariable = variableNodeId;
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
        UA_NodeId dsfNodeId;
        ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dsfNodeId).result == UA_STATUSCODE_GOOD);
        //read and update static memory directly and read new value over the information model
        UA_Variant_clear(&variant);
        UA_Variant_init(&variant);
        ck_assert(UA_Server_readValue(server, variableNodeId, &variant) == UA_STATUSCODE_GOOD);
        ck_assert(*((UA_UInt32 *)variant.data) == 42);
        *integerRTValue = *integerRTValue + 1;
        UA_Variant_clear(&variant);
        UA_Variant_init(&variant);
        UA_Server_readValue(server, variableNodeId, &variant);
        ck_assert(*((UA_UInt32 *)variant.data) == 43);
        UA_Server_removeDataSetField(server, dsfNodeId);
        UA_Variant_clear(&variant);
        UA_Variant_init(&variant);
        UA_Server_readValue(server, variableNodeId, &variant);
        ck_assert(*((UA_UInt32 *)variant.data) == 43);
        UA_DataValue_delete(externalValueSourceDataValue);
        UA_Variant_clear(&variant);
    } END_TEST

START_TEST(PubSubConfigWithMultipleInformationModelRTVariables) {
        ck_assert(addMinimalPubSubConfiguration() == UA_STATUSCODE_GOOD);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        wgm->networkMessageContentMask = UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = wgm;
        writerGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        ck_assert(UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_enableWriterGroup(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(wgm);

        UA_DataValue *externalValueSources[3];
        for (size_t i = 0; i < 3; ++i) {
            nodes[i] = UA_NodeId_new();
            dsf[i] = UA_NodeId_new();
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            UA_UInt32 myInteger = (UA_UInt32) i;
            UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_UINT32]);
            attr.description = UA_LOCALIZEDTEXT("en-US","test node");
            attr.displayName = UA_LOCALIZEDTEXT("en-US","test node");
            attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
            attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
            UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "test node");
            UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
            UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
            ck_assert(UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                                parentReferenceNodeId, myIntegerName,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, nodes[i]) == UA_STATUSCODE_GOOD);
            //use the added var in rt config
            values[i] = UA_UInt32_new();
            *values[i] = (UA_UInt32) i;
            externalValueSources[i] = UA_DataValue_new();
            externalValueSources[i]->hasValue = UA_TRUE;
            UA_Variant_setScalar(&externalValueSources[i]->value, values[i], &UA_TYPES[UA_TYPES_UINT32]);

            UA_ValueBackend valueBackend;
            valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
            valueBackend.backend.external.value = &externalValueSources[i];
            valueBackend.backend.external.callback.notificationRead = simpleNotificationRead;
            valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
            UA_Server_setVariableNode_valueBackend(server, *nodes[i], valueBackend);

            UA_DataSetFieldConfig dsfConfig;
            memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
            dsfConfig.field.variable.publishParameters.publishedVariable = *nodes[i];
            //dsfConfig.field.variable.rtFieldSourceEnabled = UA_TRUE;
            //dsfConfig.field.variable.staticValueSource.value = variantRT;
            ck_assert(UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, dsf[i]).result == UA_STATUSCODE_GOOD);
        }
        ck_assert(UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);

        ck_assert(UA_Server_setWriterGroupDisabled(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        ck_assert(UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent) == UA_STATUSCODE_GOOD);
        for (size_t j = 0; j < 3; ++j) {
            UA_Variant variant;
            UA_Variant_init(&variant);
            ck_assert(UA_Server_readValue(server, *nodes[j], &variant) == UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(j, *((UA_UInt32 *)variant.data));
            UA_Server_removeDataSetField(server, *dsf[j]);
            UA_NodeId_delete(dsf[j]);
            UA_Variant_clear(&variant);
            UA_Variant_init(&variant);
            ck_assert(UA_Server_readValue(server, *nodes[j], &variant) == UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(j, *((UA_UInt32 *)variant.data));
            UA_Variant_clear(&variant);
            UA_NodeId_delete(nodes[j]);
            UA_DataValue_delete(externalValueSources[j]);
        }
    } END_TEST

int main(void) {
    TCase *tc_pubsub_rt_static_value_source = tcase_create("PubSub RT publish with static value sources");
    tcase_add_checked_fixture(tc_pubsub_rt_static_value_source, setup, teardown);
    tcase_add_test(tc_pubsub_rt_static_value_source, PublishSingleFieldWithStaticValueSource);
    tcase_add_test(tc_pubsub_rt_static_value_source, PublishSingleFieldWithDifferentBinarySizes);
    tcase_add_test(tc_pubsub_rt_static_value_source, SetupInvalidPubSubConfigWithStaticValueSource);
    tcase_add_test(tc_pubsub_rt_static_value_source, PubSubConfigWithInformationModelRTVariable);
    tcase_add_test(tc_pubsub_rt_static_value_source, PubSubConfigWithMultipleInformationModelRTVariables);

    TCase *tc_pubsub_rt_fixed_offsets = tcase_create("PubSub RT publish with fixed offsets");
    tcase_add_checked_fixture(tc_pubsub_rt_fixed_offsets, setup, teardown);
    tcase_add_test(tc_pubsub_rt_fixed_offsets, PublishSingleFieldWithFixedOffsets);
    tcase_add_test(tc_pubsub_rt_fixed_offsets, PublishPDSWithMultipleFieldsAndFixedOffset);
    tcase_add_test(tc_pubsub_rt_fixed_offsets, PublishSingleFieldInCustomCallback);

    Suite *s = suite_create("PubSub RT configuration levels");
    suite_add_tcase(s, tc_pubsub_rt_static_value_source);
    suite_add_tcase(s, tc_pubsub_rt_fixed_offsets);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
