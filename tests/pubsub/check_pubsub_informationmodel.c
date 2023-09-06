/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include "ua_server_internal.h"
#include "test_helpers.h"

#include <math.h>
#include <string.h>

#include "check.h"

UA_Server *server = NULL;

UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3,
        dataSetWriter4, readerGroup1, dataSetReader1;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_StatusCode retVal = UA_Server_run_startup(server);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void addPublishedDataSet(UA_String pdsName, UA_NodeId *assignedId){
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = pdsName;
    ck_assert_int_eq(UA_Server_addPublishedDataSet(server, &pdsConfig, assignedId).addResult, UA_STATUSCODE_GOOD);
}

static void addDataSetField(UA_NodeId publishedDataSetIdent) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_int_eq(UA_Server_addDataSetField(server, publishedDataSetIdent,
                              &dataSetFieldConfig, &dataSetFieldIdent).result, UA_STATUSCODE_GOOD);
}

static void addPubSubConnection(UA_String connectionName, UA_String addressUrl, UA_NodeId *assignedId){
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = connectionName;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, addressUrl};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    ck_assert_int_eq(UA_Server_addPubSubConnection(server, &connectionConfig, assignedId), UA_STATUSCODE_GOOD);
}

static void addWriterGroup(UA_NodeId parentConnection, UA_String name, UA_Duration interval, UA_NodeId *assignedId){
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = name;
    writerGroupConfig.publishingInterval = interval;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    ck_assert_int_eq(UA_Server_addWriterGroup(server, parentConnection, &writerGroupConfig, assignedId), UA_STATUSCODE_GOOD);
}

static void addDataSetWriter(UA_NodeId parentWriterGroup, UA_NodeId connectedPDS, UA_String name, UA_NodeId *assignedId){
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = name;
    ck_assert_int_eq(UA_Server_addDataSetWriter(server, parentWriterGroup, connectedPDS, &dataSetWriterConfig, assignedId), UA_STATUSCODE_GOOD);
}

static void addReaderGroup(UA_NodeId parentConnection, UA_String name, UA_NodeId *assignedId){
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = name;
    ck_assert_int_eq(UA_Server_addReaderGroup(server, parentConnection, &readerGroupConfig, assignedId), UA_STATUSCODE_GOOD);
}

static void addDataSetReader(UA_NodeId parentReaderGroup, UA_String name, UA_NodeId *assignedId){
    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(readerConfig));
    readerConfig.name = name;
    ck_assert_int_eq(UA_Server_addDataSetReader(server, parentReaderGroup, &readerConfig, assignedId), UA_STATUSCODE_GOOD);
}

static UA_Boolean doubleEqual(UA_Double a, UA_Double b, UA_Double maxAbsDelta){
    return fabs(a-b) < maxAbsDelta;
}

static UA_NodeId
findSingleChildNode(UA_Server *server_, UA_QualifiedName targetName, UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceTypeId;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr =
            UA_Server_translateBrowsePathToNodeIds(server_, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

static void setupBasicPubSubConfiguration(void){
    addPubSubConnection(UA_STRING("Connection 1"), UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    addPubSubConnection(UA_STRING("Connection 2"), UA_STRING("opc.udp://224.0.0.22:4840/"), &connection2);
    addPublishedDataSet(UA_STRING("PublishedDataSet 1"), &publishedDataSet1);
    addPublishedDataSet(UA_STRING("PublishedDataSet 2"), &publishedDataSet2);
    addWriterGroup(connection1, UA_STRING("WriterGroup 1"), 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    addWriterGroup(connection1, UA_STRING("WriterGroup 2"), 100, &writerGroup2);
    UA_Server_enableWriterGroup(server, writerGroup2);
    addWriterGroup(connection2, UA_STRING("WriterGroup 3"), 1000, &writerGroup3);
    UA_Server_enableWriterGroup(server, writerGroup3);
    addDataSetWriter(writerGroup1, publishedDataSet1, UA_STRING("DataSetWriter 1"), &dataSetWriter1);
    addDataSetWriter(writerGroup1, publishedDataSet2, UA_STRING("DataSetWriter 2"), &dataSetWriter2);
    addDataSetWriter(writerGroup2, publishedDataSet2, UA_STRING("DataSetWriter 3"), &dataSetWriter3);
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    ck_assert_int_eq(UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet2,
                               &dataSetWriterConfig, &dataSetWriter4), UA_STATUSCODE_GOOD);
    addReaderGroup(connection1, UA_STRING("ReaderGroup 1"), &readerGroup1);
    addDataSetReader(readerGroup1, UA_STRING("DataSetReader 1"), &dataSetReader1);
}

START_TEST(AddSignlePubSubConnectionAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_readBrowseName(server, connection1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &connectionName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddRemoveAddSignlePubSubConnectionAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal;
    ck_assert_int_eq(UA_Server_removePubSubConnection(server, connection1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, connection1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    retVal = UA_Server_readBrowseName(server, connection1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &connectionName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddSinglePublishedDataSetAndCheckInformationModelRepresentation){
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    addDataSetField(publishedDataSet1);
    UA_QualifiedName browseName;
    UA_NodeId publishedDataNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), publishedDataSet1);
    UA_NodeId dataSetMetaDataNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), publishedDataSet1);
    UA_NodeId configurationVersionNodeId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), publishedDataSet1);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_int_eq(UA_Server_readValue(server, publishedDataNodeId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]));
    UA_NodeId dsf1 = ((UA_PublishedVariableDataType *)value.data)[0].publishedVariable;
    UA_NodeId serverCurrentTimeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    ck_assert(UA_NodeId_equal(&dsf1, &serverCurrentTimeId));
    ck_assert_uint_eq(value.arrayLength, 1);
    UA_Variant_clear(&value);
    UA_Variant_init(&value);
    ck_assert_int_eq(UA_Server_readValue(server, dataSetMetaDataNodeId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]));
    ck_assert_uint_eq(((UA_DataSetMetaDataType *)value.data)->fieldsSize, 1);
    UA_Variant_clear(&value);
    UA_Variant_init(&value);
    ck_assert_int_eq(UA_Server_readValue(server, configurationVersionNodeId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]));
    UA_Variant_clear(&value);
    ck_assert_int_eq(UA_Server_readBrowseName(server, publishedDataSet1, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &pdsName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddRemoveAddSinglePublishedDataSetAndCheckInformationModelRepresentation){
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal;
    ck_assert_int_eq(UA_Server_removePublishedDataSet(server, publishedDataSet1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, publishedDataSet1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addPublishedDataSet(pdsName, &publishedDataSet1);
    retVal = UA_Server_readBrowseName(server, publishedDataSet1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &pdsName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddSingleWriterGroupAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    UA_String wgName = UA_STRING("WriterGroup 1");
    addWriterGroup(connection1, wgName, 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    UA_QualifiedName browseName;
    ck_assert_int_eq(UA_Server_readBrowseName(server, writerGroup1, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &wgName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_NodeId  statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              writerGroup1);

    UA_NodeId stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    ck_assert_int_eq(UA_Server_readValue(server, stateIdNode, &value), UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    } END_TEST

START_TEST(AddRemoveAddSingleWriterGroupAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    UA_String wgName = UA_STRING("WriterGroup 1");
    addWriterGroup(connection1, wgName, 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal;
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, writerGroup1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, writerGroup1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addWriterGroup(connection1, wgName, 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    retVal = UA_Server_readBrowseName(server, writerGroup1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &wgName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddSingleDataSetWriterAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    UA_String wgName = UA_STRING("WriterGroup 1");
    addWriterGroup(connection1, wgName, 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    UA_String dswName = UA_STRING("DataSetWriter 1");
    addDataSetWriter(writerGroup1, publishedDataSet1, dswName, &dataSetWriter1);
    UA_QualifiedName browseName;
    ck_assert_int_eq(UA_Server_readBrowseName(server, dataSetWriter1, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &dswName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_NodeId  statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              dataSetWriter1);

    UA_NodeId stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    ck_assert_int_eq(UA_Server_readValue(server, stateIdNode, &value), UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    } END_TEST

START_TEST(AddRemoveAddSingleDataSetWriterAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String pdsName = UA_STRING("PDS 1");
    addPublishedDataSet(pdsName, &publishedDataSet1);
    UA_String wgName = UA_STRING("WriterGroup 1");
    addWriterGroup(connection1, wgName, 10, &writerGroup1);
    UA_Server_enableWriterGroup(server, writerGroup1);
    UA_String dswName = UA_STRING("DataSetWriter 1");
    addDataSetWriter(writerGroup1, publishedDataSet1, dswName, &dataSetWriter1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal;
    ck_assert_int_eq(UA_Server_removeDataSetWriter(server, dataSetWriter1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, dataSetWriter1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addDataSetWriter(writerGroup1, publishedDataSet1, dswName, &dataSetWriter1);
    retVal = UA_Server_readBrowseName(server, dataSetWriter1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &dswName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(ReadPublishIntervalAndCompareWithInternalValue){
    setupBasicPubSubConfiguration();
    UA_NodeId publishIntervalId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), writerGroup1);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_int_eq(UA_Server_readValue(server, publishIntervalId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DURATION]));
    ck_assert(doubleEqual((UA_Double) *((UA_Duration *) value.data), 10, 0.05));
    UA_Variant_clear(&value);
    } END_TEST

START_TEST(WritePublishIntervalAndCompareWithInternalValue){
        setupBasicPubSubConfiguration();
        UA_NodeId publishIntervalId = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), writerGroup1);
        UA_Variant value;
        UA_Variant_init(&value);
        UA_Duration interval = 100;
        UA_Variant_setScalar(&value, &interval, &UA_TYPES[UA_TYPES_DURATION]);
        ck_assert_int_eq(UA_Server_writeValue(server, publishIntervalId, value), UA_STATUSCODE_GOOD);

        ck_assert_int_eq(UA_Server_readValue(server, publishIntervalId, &value), UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DURATION]));
        ck_assert(doubleEqual((UA_Double) *((UA_Duration *) value.data), 100, 0.05));
        UA_Variant_clear(&value);
    } END_TEST

START_TEST(ReadAddressAndCompareWithInternalValue){
        setupBasicPubSubConfiguration();
        UA_NodeId address = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Address"),
                                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), connection1);
        UA_NodeId url = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Url"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), address);
        UA_NodeId networkInterface = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkInterface"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), address);
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(connectionConfig));
        UA_Server_getPubSubConnectionConfig(server, connection1, &connectionConfig);
        UA_Variant value;
        UA_Variant_init(&value);
        ck_assert_int_eq(UA_Server_readValue(server, url, &value), UA_STATUSCODE_GOOD);
        UA_NetworkAddressUrlDataType *networkAddressUrlDataType = (UA_NetworkAddressUrlDataType *)connectionConfig.address.data;
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING]));
        ck_assert(UA_String_equal(((UA_String *) value.data), &networkAddressUrlDataType->url));
        UA_Variant_clear(&value);
        ck_assert_int_eq(UA_Server_readValue(server, networkInterface, &value), UA_STATUSCODE_GOOD);
        ck_assert(UA_String_equal(((UA_String *) value.data), &networkAddressUrlDataType->networkInterface));
        UA_PubSubConnectionConfig_clear(&connectionConfig);
        UA_Variant_clear(&value);
    } END_TEST

START_TEST(AddSingleDataSetReaderAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String rgName = UA_STRING("ReaderGroup 1");
    addReaderGroup(connection1, rgName, &readerGroup1);
    UA_String dsrName = UA_STRING("DataSetReader 1");
    addDataSetReader(readerGroup1, dsrName, &dataSetReader1);
    UA_QualifiedName browseName;
    ck_assert_int_eq(UA_Server_readBrowseName(server, dataSetReader1, &browseName), UA_STATUSCODE_GOOD);

    ck_assert_int_eq(UA_String_equal(&browseName.name, &dsrName), UA_TRUE);
    
    UA_QualifiedName_clear(&browseName);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_NodeId  statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              dataSetReader1);

    UA_NodeId stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    ck_assert_int_eq(UA_Server_readValue(server, stateIdNode, &value), UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    } END_TEST

START_TEST(AddRemoveAddSingleDataSetReaderAndCheckInformationModelRepresentation){
    UA_StatusCode retVal;
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String rgName = UA_STRING("ReaderGroup 1");
    addReaderGroup(connection1,  rgName, &readerGroup1);
    UA_String dsrName = UA_STRING("DataSetReader 1");
    addDataSetReader(readerGroup1, dsrName, &dataSetReader1);
    UA_QualifiedName browseName;
    ck_assert_int_eq(UA_Server_removeDataSetReader(server, dataSetReader1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, dataSetReader1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addDataSetReader(readerGroup1, dsrName, &dataSetReader1);
    retVal = UA_Server_readBrowseName(server, dataSetReader1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &dsrName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(AddSingleReaderGroupAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String rgName = UA_STRING("ReaderGroup 1");
    addReaderGroup(connection1, rgName, &readerGroup1);
    UA_QualifiedName browseName;
    ck_assert_int_eq(UA_Server_readBrowseName(server, readerGroup1, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &rgName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_NodeId  statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              readerGroup1);

    UA_NodeId stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    ck_assert_int_eq(UA_Server_readValue(server, stateIdNode, &value), UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    } END_TEST

START_TEST(AddRemoveAddSingleReaderGroupAndCheckInformationModelRepresentation){
    UA_String connectionName = UA_STRING("Connection 1");
    addPubSubConnection(connectionName, UA_STRING("opc.udp://224.0.0.22:4840/"), &connection1);
    UA_String rgName = UA_STRING("ReaderGroup 1");
    addReaderGroup(connection1, rgName, &readerGroup1);
    UA_QualifiedName browseName;
    UA_StatusCode retVal;
    ck_assert_int_eq(UA_Server_removeReaderGroup(server, readerGroup1), UA_STATUSCODE_GOOD);
    retVal = UA_Server_readBrowseName(server, readerGroup1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);
    addReaderGroup(connection1, rgName, &readerGroup1);
    retVal = UA_Server_readBrowseName(server, readerGroup1, &browseName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_String_equal(&browseName.name, &rgName), UA_TRUE);
    UA_QualifiedName_clear(&browseName);
    } END_TEST

START_TEST(ReadWriterGroupIdAndCompareWithInternalValue){
    setupBasicPubSubConfiguration();
    UA_NodeId writerGroupIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), dataSetReader1);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_UInt16 writerGroupId = 100;
    UA_Variant_setScalar(&value, &writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_int_eq(UA_Server_writeValue(server, writerGroupIdNode, value), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_readValue(server, writerGroupIdNode, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert(doubleEqual((UA_Double) *((UA_UInt16 *) value.data), 100, 0.05));
    UA_Variant_clear(&value);
    } END_TEST

START_TEST(ReadDataSetWriterIdAndCompareWithInternalValue){
    setupBasicPubSubConfiguration();
    UA_NodeId dataSetwriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), dataSetReader1);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_UInt16 dataSetwriterId = 62541;
    UA_Variant_setScalar(&value, &dataSetwriterId, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_int_eq(UA_Server_writeValue(server, dataSetwriterIdNode, value), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_readValue(server, dataSetwriterIdNode, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert(doubleEqual((UA_Double) *((UA_UInt16 *) value.data), 62541, 0.05));
    UA_Variant_clear(&value);
    } END_TEST

START_TEST(ReadDataSetWriterIdWGAndCompareWithInternalValue){
    setupBasicPubSubConfiguration();
    UA_NodeId dataSetWriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), dataSetWriter4);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_int_eq(UA_Server_readValue(server, dataSetWriterIdNode, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_int_eq(*((UA_UInt16 *) value.data), 62541);
    UA_Variant_clear(&value);
    } END_TEST

int main(void) {
    TCase *tc_add_pubsub_informationmodel = tcase_create("PubSub add single elements and check information model representation");
    tcase_add_checked_fixture(tc_add_pubsub_informationmodel, setup, teardown);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSignlePubSubConnectionAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSignlePubSubConnectionAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSinglePublishedDataSetAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSinglePublishedDataSetAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSingleWriterGroupAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSingleWriterGroupAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSingleDataSetWriterAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSingleDataSetWriterAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSingleReaderGroupAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddSingleDataSetReaderAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSingleDataSetReaderAndCheckInformationModelRepresentation);
    tcase_add_test(tc_add_pubsub_informationmodel, AddRemoveAddSingleReaderGroupAndCheckInformationModelRepresentation);

    TCase *tc_add_pubsub_writergroupelements = tcase_create("PubSub WriterGroup check properties");
    tcase_add_checked_fixture(tc_add_pubsub_writergroupelements, setup, teardown);
    tcase_add_test(tc_add_pubsub_writergroupelements, ReadPublishIntervalAndCompareWithInternalValue);
    tcase_add_test(tc_add_pubsub_writergroupelements, WritePublishIntervalAndCompareWithInternalValue);

    TCase *tc_add_pubsub_pubsubconnectionelements = tcase_create("PubSub Connection check properties");
    tcase_add_checked_fixture(tc_add_pubsub_pubsubconnectionelements, setup, teardown);
    tcase_add_test(tc_add_pubsub_pubsubconnectionelements, ReadAddressAndCompareWithInternalValue);

    TCase *tc_add_pubsub_dataSetReaderElements = tcase_create("PubSub DataSetReader check properties");
    tcase_add_checked_fixture(tc_add_pubsub_dataSetReaderElements, setup, teardown);
    tcase_add_test(tc_add_pubsub_dataSetReaderElements, ReadWriterGroupIdAndCompareWithInternalValue);
    tcase_add_test(tc_add_pubsub_dataSetReaderElements, ReadDataSetWriterIdAndCompareWithInternalValue);

    TCase *tc_add_pubsub_dataSetWriterElements = tcase_create("PubSub DataSetWriter check properties");
    tcase_add_checked_fixture(tc_add_pubsub_dataSetWriterElements, setup, teardown);
    tcase_add_test(tc_add_pubsub_dataSetWriterElements, ReadDataSetWriterIdWGAndCompareWithInternalValue);

    Suite *s = suite_create("PubSub WriterGroups/DataSetReader/Fields handling and publishing");
    suite_add_tcase(s, tc_add_pubsub_informationmodel);
    suite_add_tcase(s, tc_add_pubsub_writergroupelements);
    suite_add_tcase(s, tc_add_pubsub_pubsubconnectionelements);
    suite_add_tcase(s, tc_add_pubsub_dataSetReaderElements);
    suite_add_tcase(s, tc_add_pubsub_dataSetWriterElements);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
