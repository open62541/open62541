/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;
UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3;
#define publishedDataSet1Name "PublishedDataSet 1"
#define publishedDataSet2Name "PublishedDataSet 2"

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    UA_Server_run_startup(server);
    //add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection2);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddWriterGroupWithValidConfiguration){
        UA_StatusCode retVal;
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        UA_NodeId localWriterGroup;
        retVal = UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        UA_Server_setWriterGroupOperational(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 1);
        UA_Server_setWriterGroupDisabled(server, localWriterGroup);
    } END_TEST

START_TEST(AddRemoveAddWriterGroupWithMinimalValidConfiguration){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        UA_NodeId localWriterGroup;
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        UA_Server_setWriterGroupOperational(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_removeWriterGroup(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 0);
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        UA_Server_setWriterGroupOperational(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        writerGroupCount = 0;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 1);
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Server_setWriterGroupOperational(server, localWriterGroup);
        writerGroupCount = 0;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 2);
    } END_TEST

START_TEST(AddWriterGroupWithNullConfig){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        retVal |= UA_Server_addWriterGroup(server, connection1, NULL, NULL);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddWriterGroupWithInvalidConnectionId){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        retVal |= UA_Server_addWriterGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &writerGroupConfig, NULL);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection1)->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_int_eq(writerGroupCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetWriterGroupConfigurationAndCompareValues){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        UA_NodeId localWriterGroup;
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        UA_Server_setWriterGroupOperational(server, localWriterGroup);
        UA_WriterGroupConfig writerGroupConfigCopy;
        retVal |= UA_Server_getWriterGroupConfig(server, localWriterGroup, &writerGroupConfigCopy);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(UA_String_equal(&writerGroupConfig.name, &writerGroupConfigCopy.name), UA_TRUE);
        //todo remove == for floating point compare
        ck_assert(writerGroupConfig.publishingInterval == writerGroupConfig.publishingInterval);
        UA_WriterGroupConfig_clear(&writerGroupConfigCopy);
    } END_TEST

static void setupDataSetWriterTestEnvironment(void){
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    UA_Server_setWriterGroupOperational(server, writerGroup1);
    writerGroupConfig.name = UA_STRING("WriterGroup 2");
    writerGroupConfig.publishingInterval = 50;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup2);
    UA_Server_setWriterGroupOperational(server, writerGroup2);
    writerGroupConfig.name = UA_STRING("WriterGroup 3");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);
    UA_Server_setWriterGroupOperational(server, writerGroup3);
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING(publishedDataSet1Name);
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
    pdsConfig.name = UA_STRING(publishedDataSet2Name);
    UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet2);
}

START_TEST(AddDataSetWriterWithValidConfiguration){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        UA_NodeId localDataSetWriter;
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &localDataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_DataSetWriter *dsw1 = UA_DataSetWriter_findDSWbyId(server, localDataSetWriter);
        ck_assert_ptr_ne(dsw1, NULL);
        UA_WriterGroup *wg1 = UA_WriterGroup_findWGbyId(server, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        ck_assert_int_eq(wg1->writersCount, 1);
    } END_TEST

START_TEST(AddRemoveAddDataSetWriterWithValidConfiguration){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        UA_WriterGroup *wg1 = UA_WriterGroup_findWGbyId(server, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        UA_NodeId dataSetWriter;
        ck_assert_int_eq(wg1->writersCount, 0);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(wg1->writersCount, 1);
        retVal = UA_Server_removeDataSetWriter(server, dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(wg1->writersCount, 0);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(wg1->writersCount, 1);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(wg1->writersCount, 2);

        UA_WriterGroup *wg2 = UA_WriterGroup_findWGbyId(server, writerGroup2);
        ck_assert_ptr_ne(wg2, NULL);
        retVal = UA_Server_addDataSetWriter(server, writerGroup2, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(wg2->writersCount, 1);
    } END_TEST

START_TEST(AddDataSetWriterWithNullConfig){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, NULL, NULL);
        UA_WriterGroup *wg1 = UA_WriterGroup_findWGbyId(server, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        ck_assert_int_eq(wg1->writersCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddDataSetWriterWithInvalidPDSId){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &dataSetWriterConfig, NULL);
        UA_WriterGroup *wg1 = UA_WriterGroup_findWGbyId(server, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        ck_assert_int_eq(wg1->writersCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetDataSetWriterConfigurationAndCompareValues){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        UA_NodeId localDataSetWriter;
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &localDataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_DataSetWriterConfig dataSetWiterConfigCopy;
        retVal |= UA_Server_getDataSetWriterConfig(server, localDataSetWriter, &dataSetWiterConfigCopy);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(UA_String_equal(&dataSetWiterConfigCopy.name, &dataSetWiterConfigCopy.name), UA_TRUE);
        UA_DataSetWriterConfig_clear(&dataSetWiterConfigCopy);
    } END_TEST

START_TEST(AddPDSEmptyName){
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_AddPublishedDataSetResult res = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_BADINVALIDARGUMENT);
    } END_TEST

START_TEST(AddPDSDuplicatedName){
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING(publishedDataSet1Name);
    UA_AddPublishedDataSetResult res = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_GOOD);
    res = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    UA_Server_removePublishedDataSet(server, publishedDataSet1);
    } END_TEST

START_TEST(FindPDS){
        setupDataSetWriterTestEnvironment();
        UA_PublishedDataSet *pdsById = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
        ck_assert_ptr_ne(pdsById, NULL);
        UA_PublishedDataSet *pdsByName = UA_PublishedDataSet_findPDSbyName(server, UA_STRING(publishedDataSet1Name));
        ck_assert_ptr_ne(pdsByName, NULL);
        ck_assert_ptr_eq(pdsById, pdsByName);
        pdsById = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet2);
        ck_assert_ptr_ne(pdsById, NULL);
        pdsByName = UA_PublishedDataSet_findPDSbyName(server, UA_STRING(publishedDataSet2Name));
        ck_assert_ptr_ne(pdsByName, NULL);
        ck_assert_ptr_eq(pdsById, pdsByName);
        UA_Server_removePublishedDataSet(server, publishedDataSet1);
        UA_Server_removePublishedDataSet(server, publishedDataSet2);
    } END_TEST

static void setupDataSetFieldTestEnvironment(void){
    setupDataSetWriterTestEnvironment();
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 2");
    UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter2);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 3");
    UA_Server_addDataSetWriter(server, writerGroup2, publishedDataSet2, &dataSetWriterConfig, &dataSetWriter3);
}

START_TEST(AddDataSetFieldWithValidConfiguration){
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_NodeId localDataSetField;
        UA_PublishedDataSet *pds = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
        ck_assert_ptr_ne(pds, NULL);
        ck_assert_int_eq(pds->fieldSize, 0);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds->fieldSize, 1);
    } END_TEST

START_TEST(AddRemoveAddDataSetFieldWithValidConfiguration){
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_NodeId localDataSetField;
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_int_eq(pds1->fieldSize, 0);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds1->fieldSize, 1);
        retVal = UA_Server_removeDataSetField(server, localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds1->fieldSize, 0);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds1->fieldSize, 1);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds1->fieldSize, 2);
        UA_PublishedDataSet *pds2 = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet2);
        ck_assert_ptr_ne(pds2, NULL);
        retVal = UA_Server_addDataSetField(server, publishedDataSet2, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(pds2->fieldSize, 1);
    } END_TEST

START_TEST(AddDataSetFieldWithNullConfig){
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, NULL, NULL).result;
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_int_eq(pds1->fieldSize, 0);
    } END_TEST

START_TEST(AddDataSetFieldWithInvalidPDSId){
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal = UA_Server_addDataSetField(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &fieldConfig, NULL).result;
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_int_eq(pds1->fieldSize, 0);
    } END_TEST

START_TEST(GetDataSetFieldConfigurationAndCompareValues){
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_NodeId dataSetFieldId;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &dataSetFieldId).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_DataSetFieldConfig fieldConfigCopy;
        retVal |= UA_Server_getDataSetFieldConfig(server, dataSetFieldId, &fieldConfigCopy);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(fieldConfig.dataSetFieldType, fieldConfigCopy.dataSetFieldType);
        ck_assert_int_eq(UA_String_equal(&fieldConfig.field.variable.fieldNameAlias, &fieldConfigCopy.field.variable.fieldNameAlias), UA_TRUE);
        UA_DataSetFieldConfig_clear(&fieldConfigCopy);
    } END_TEST


START_TEST(SinglePublishDataSetField){
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
        UA_Server_setWriterGroupOperational(server, writerGroup1);
        writerGroupConfig.name = UA_STRING("WriterGroup 2");
        writerGroupConfig.publishingInterval = 50;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup2);
        UA_Server_setWriterGroupOperational(server, writerGroup2);
        writerGroupConfig.name = UA_STRING("WriterGroup 3");
        writerGroupConfig.publishingInterval = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);
        UA_Server_setWriterGroupOperational(server, writerGroup3);
        UA_PublishedDataSetConfig pdsConfig;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING(publishedDataSet1Name);
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
        pdsConfig.name = UA_STRING(publishedDataSet2Name);
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet2);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
        UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL);

        UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup1);
        UA_WriterGroup_publishCallback(server, wg);
    } END_TEST

START_TEST(PublishDataSetFieldAsDeltaFrame){
            setupDataSetFieldTestEnvironment();
            UA_DataSetFieldConfig dataSetFieldConfig;
            memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
            dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
            dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
            dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
            dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME);
            dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
            UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL);

            UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup1);
            wg->config.maxEncapsulatedDataSetMessageCount = 3;
            UA_DataSetWriter *dsw = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);
            dsw->config.keyFrameCount = 3;

            UA_WriterGroup_publishCallback(server, wg);
            UA_WriterGroup_publishCallback(server, wg);
            UA_WriterGroup_publishCallback(server, wg);
            UA_WriterGroup_publishCallback(server, wg);
            UA_WriterGroup_publishCallback(server, wg);
        } END_TEST

int main(void) {
    TCase *tc_add_pubsub_writergroup = tcase_create("PubSub WriterGroup items handling");
    tcase_add_checked_fixture(tc_add_pubsub_writergroup, setup, teardown);
    tcase_add_test(tc_add_pubsub_writergroup, AddWriterGroupWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_writergroup, AddRemoveAddWriterGroupWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_writergroup, AddWriterGroupWithNullConfig);
    tcase_add_test(tc_add_pubsub_writergroup, AddWriterGroupWithInvalidConnectionId);
    tcase_add_test(tc_add_pubsub_writergroup, GetWriterGroupConfigurationAndCompareValues);

    TCase *tc_add_pubsub_datasetwriter = tcase_create("PubSub DataSetWriter items handling");
    tcase_add_checked_fixture(tc_add_pubsub_datasetwriter, setup, teardown);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddDataSetWriterWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddRemoveAddDataSetWriterWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddDataSetWriterWithNullConfig);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddDataSetWriterWithInvalidPDSId);
    tcase_add_test(tc_add_pubsub_datasetwriter, GetDataSetWriterConfigurationAndCompareValues);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddPDSEmptyName);
    tcase_add_test(tc_add_pubsub_datasetwriter, AddPDSDuplicatedName);
    tcase_add_test(tc_add_pubsub_datasetwriter, FindPDS);

    TCase *tc_add_pubsub_datasetfields = tcase_create("PubSub DataSetField items handling");
    tcase_add_checked_fixture(tc_add_pubsub_datasetfields, setup, teardown);
    tcase_add_test(tc_add_pubsub_datasetfields, AddDataSetFieldWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_datasetfields, AddRemoveAddDataSetFieldWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_datasetfields, AddDataSetFieldWithNullConfig);
    tcase_add_test(tc_add_pubsub_datasetfields, AddDataSetFieldWithInvalidPDSId);
    tcase_add_test(tc_add_pubsub_datasetfields, GetDataSetFieldConfigurationAndCompareValues);

    TCase *tc_pubsub_publish = tcase_create("PubSub publish DataSetFields");
    tcase_add_checked_fixture(tc_pubsub_publish, setup, teardown);
    tcase_add_test(tc_pubsub_publish, SinglePublishDataSetField);
    tcase_add_test(tc_pubsub_publish, PublishDataSetFieldAsDeltaFrame);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_add_pubsub_writergroup);
    suite_add_tcase(s, tc_add_pubsub_datasetwriter);
    suite_add_tcase(s, tc_add_pubsub_datasetfields);
    suite_add_tcase(s, tc_pubsub_publish);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
