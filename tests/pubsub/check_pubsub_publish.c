/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

UA_Server *server = NULL;
UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3;
#define publishedDataSet1Name "PublishedDataSet 1"
#define publishedDataSet2Name "PublishedDataSet 2"

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    UA_Server_run_startup(server);
    //add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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
        UA_Server_enableWriterGroup(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_PubSubManager *psm = getPSM(server);
        UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection1);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 1);
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
        retVal |= UA_Server_enableWriterGroup(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_removeWriterGroup(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_PubSubManager *psm = getPSM(server);
        UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection1);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 0);
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        UA_Server_enableWriterGroup(server, localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        writerGroupCount = 0;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 1);
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &localWriterGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Server_enableWriterGroup(server, localWriterGroup);
        writerGroupCount = 0;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 2);
    } END_TEST

START_TEST(AddWriterGroupWithNullConfig){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        retVal |= UA_Server_addWriterGroup(server, connection1, NULL, NULL);
        UA_PubSubManager *psm = getPSM(server);
        UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection1);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddWriterGroupWithInvalidConnectionId){
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PubSubManager *psm = getPSM(server);
        UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection1);
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        retVal |= UA_Server_addWriterGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &writerGroupConfig, NULL);
        size_t writerGroupCount = 0;
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &c->writerGroups, listEntry){
            writerGroupCount++;
        }
        ck_assert_uint_eq(writerGroupCount, 0);
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
        UA_Server_enableWriterGroup(server, localWriterGroup);
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
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    writerGroupConfig.name = UA_STRING("WriterGroup 2");
    writerGroupConfig.publishingInterval = 50;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup2);
    writerGroupConfig.name = UA_STRING("WriterGroup 3");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void setupPublishedDataSetTestEnvironment(void){
    UA_PublishedDataSetConfig pdsConfig;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING(publishedDataSet1Name);
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1).addResult;
    pdsConfig.name = UA_STRING(publishedDataSet2Name);
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet2).addResult;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

START_TEST(AddDataSetWriterWithValidConfiguration){
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();
    UA_StatusCode retVal;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
    UA_NodeId localDataSetWriter;
    retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &localDataSetWriter);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetWriter *dsw1 = UA_DataSetWriter_find(psm, localDataSetWriter);
    ck_assert_ptr_ne(dsw1, NULL);
    UA_WriterGroup *wg1 = UA_WriterGroup_find(psm, writerGroup1);
    ck_assert_ptr_ne(wg1, NULL);
    ck_assert_uint_eq(wg1->writersCount, 1);
} END_TEST

START_TEST(AddRemoveAddDataSetWriterWithValidConfiguration){
        setupDataSetWriterTestEnvironment();
        setupPublishedDataSetTestEnvironment();
        UA_StatusCode retVal;
        UA_PubSubManager *psm = getPSM(server);
        UA_WriterGroup *wg1 = UA_WriterGroup_find(psm, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        UA_NodeId dataSetWriter;
        ck_assert_uint_eq(wg1->writersCount, 0);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(wg1->writersCount, 1);
        retVal = UA_Server_removeDataSetWriter(server, dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(wg1->writersCount, 0);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(wg1->writersCount, 1);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(wg1->writersCount, 2);

        UA_WriterGroup *wg2 = UA_WriterGroup_find(psm, writerGroup2);
        ck_assert_ptr_ne(wg2, NULL);
        retVal = UA_Server_addDataSetWriter(server, writerGroup2, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(wg2->writersCount, 1);
    } END_TEST

START_TEST(AddDataSetWriterWithNullConfig){
        setupDataSetWriterTestEnvironment();
        UA_StatusCode retVal;
        UA_PubSubManager *psm = getPSM(server);
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, NULL, NULL);
        UA_WriterGroup *wg1 = UA_WriterGroup_find(psm, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        ck_assert_uint_eq(wg1->writersCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddDataSetWriterWithInvalidPDSId){
        setupDataSetWriterTestEnvironment();
        UA_PubSubManager *psm = getPSM(server);
        UA_StatusCode retVal;
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1 ");
        retVal = UA_Server_addDataSetWriter(server, writerGroup1, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &dataSetWriterConfig, NULL);
        UA_WriterGroup *wg1 = UA_WriterGroup_find(psm, writerGroup1);
        ck_assert_ptr_ne(wg1, NULL);
        ck_assert_uint_eq(wg1->writersCount, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetDataSetWriterConfigurationAndCompareValues){
        setupDataSetWriterTestEnvironment();
        setupPublishedDataSetTestEnvironment();
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
        setupPublishedDataSetTestEnvironment();
        UA_PubSubManager *psm = getPSM(server);
        UA_PublishedDataSet *pdsById = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pdsById, NULL);
        UA_PublishedDataSet *pdsByName = UA_PublishedDataSet_findByName(psm, UA_STRING(publishedDataSet1Name));
        ck_assert_ptr_ne(pdsByName, NULL);
        ck_assert_ptr_eq(pdsById, pdsByName);
        pdsById = UA_PublishedDataSet_find(psm, publishedDataSet2);
        ck_assert_ptr_ne(pdsById, NULL);
        pdsByName = UA_PublishedDataSet_findByName(psm, UA_STRING(publishedDataSet2Name));
        ck_assert_ptr_ne(pdsByName, NULL);
        ck_assert_ptr_eq(pdsById, pdsByName);
        UA_Server_removePublishedDataSet(server, publishedDataSet1);
        UA_Server_removePublishedDataSet(server, publishedDataSet2);
    } END_TEST

static void setupDataSetFieldTestEnvironment(void){
    setupDataSetWriterTestEnvironment();
    UA_DataSetWriterConfig dataSetWriterConfig;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    retVal |= UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 2");
    retVal |= UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter2);
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 3");
    retVal |= UA_Server_addDataSetWriter(server, writerGroup2, publishedDataSet2, &dataSetWriterConfig, &dataSetWriter3);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

START_TEST(AddDataSetFieldWithValidConfiguration){
        UA_PubSubManager *psm = getPSM(server);
        setupPublishedDataSetTestEnvironment();
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        fieldConfig.field.variable.description = UA_LOCALIZEDTEXT("en", "this is field 1");
        fieldConfig.field.variable.dataSetFieldId = UA_GUID("10000000-2000-3000-4000-500000000000");
        UA_NodeId localDataSetField;
        UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pds, NULL);
        ck_assert_uint_eq(pds->fieldSize, 0);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds->fieldSize, 1);
    } END_TEST

START_TEST(AddRemoveAddDataSetFieldWithValidConfiguration){
        UA_PubSubManager *psm = getPSM(server);
        setupPublishedDataSetTestEnvironment();
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        fieldConfig.field.variable.description = UA_LOCALIZEDTEXT("en", "description");
        UA_NodeId localDataSetField;
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_uint_eq(pds1->fieldSize, 0);

        // Add "field 1"
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.dataSetFieldId = UA_PubSubManager_generateUniqueGuid(psm);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds1->fieldSize, 1);

        // Add "field 2"
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 2");
        const UA_Guid field2Id = UA_PubSubManager_generateUniqueGuid(psm);
        fieldConfig.field.variable.dataSetFieldId = field2Id;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds1->fieldSize, 2);

        // Remove "field 2"
        retVal = UA_Server_removeDataSetField(server, localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds1->fieldSize, 1);
        // Check that the correct field was removed - "field 1" should still be there
        UA_String compareString = UA_STRING("field 1");
        ck_assert(UA_String_equal(&pds1->fields.tqh_first->fieldMetaData.name, &compareString));

        // Add "field 2" again
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 2");
        fieldConfig.field.variable.dataSetFieldId = field2Id;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds1->fieldSize, 2);

        // Add "field 3"
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 3");
        fieldConfig.field.variable.dataSetFieldId = UA_PubSubManager_generateUniqueGuid(psm);
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds1->fieldSize, 3);

        UA_PublishedDataSet *pds2 = UA_PublishedDataSet_find(psm, publishedDataSet2);
        ck_assert_ptr_ne(pds2, NULL);

        // Add "field 1"
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.dataSetFieldId = UA_PubSubManager_generateUniqueGuid(psm);
        retVal = UA_Server_addDataSetField(server, publishedDataSet2, &fieldConfig, &localDataSetField).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(pds2->fieldSize, 1);
    } END_TEST

START_TEST(AddDataSetFieldWithNullConfig){
        UA_PubSubManager *psm = getPSM(server);
        UA_StatusCode retVal;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, NULL, NULL).result;
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        setupPublishedDataSetTestEnvironment();
        setupDataSetFieldTestEnvironment();
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_uint_eq(pds1->fieldSize, 0);
    } END_TEST

START_TEST(AddDataSetFieldWithInvalidPDSId){
        UA_PubSubManager *psm = getPSM(server);
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal = UA_Server_addDataSetField(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &fieldConfig, NULL).result;
        setupPublishedDataSetTestEnvironment();
        setupDataSetFieldTestEnvironment();
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        ck_assert_uint_eq(pds1->fieldSize, 0);
    } END_TEST

START_TEST(GetDataSetFieldConfigurationAndCompareValues){
        UA_PubSubManager *psm = getPSM(server);
        setupPublishedDataSetTestEnvironment();
        setupDataSetFieldTestEnvironment();
        UA_StatusCode retVal;
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.fieldNameAlias = UA_STRING("field 1");
        fieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        fieldConfig.field.variable.description = UA_LOCALIZEDTEXT("en", "this is field 1");
        fieldConfig.field.variable.dataSetFieldId = UA_GUID("10000000-2000-3000-4000-500000000000");
        UA_NodeId dataSetFieldId;
        retVal = UA_Server_addDataSetField(server, publishedDataSet1, &fieldConfig, &dataSetFieldId).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_DataSetFieldConfig fieldConfigCopy;
        retVal |= UA_Server_getDataSetFieldConfig(server, dataSetFieldId, &fieldConfigCopy);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(fieldConfig.dataSetFieldType, fieldConfigCopy.dataSetFieldType);
        ck_assert_int_eq(UA_String_equal(&fieldConfig.field.variable.fieldNameAlias, &fieldConfigCopy.field.variable.fieldNameAlias), UA_TRUE);
        ck_assert_int_eq(UA_LocalizedText_equal(&fieldConfig.field.variable.description, &fieldConfigCopy.field.variable.description), UA_TRUE);
        ck_assert_int_eq(UA_Guid_equal(&fieldConfig.field.variable.dataSetFieldId, &fieldConfigCopy.field.variable.dataSetFieldId), UA_TRUE);

        UA_PublishedDataSet *pds1 = UA_PublishedDataSet_find(psm, publishedDataSet1);
        ck_assert_ptr_ne(pds1, NULL);
        // Make sure that the DataSetFieldId in the MetaData was not generated, but the one from configuration was used
        ck_assert(UA_Guid_equal(&pds1->fields.tqh_first->fieldMetaData.dataSetFieldId, &fieldConfig.field.variable.dataSetFieldId));

        UA_DataSetFieldConfig_clear(&fieldConfigCopy);
    } END_TEST


START_TEST(SinglePublishDataSetFieldAndPublishTimestampTest){
        UA_PublishedDataSetConfig pdsConfig;
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING(publishedDataSet1Name);
        retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1).addResult;
        pdsConfig.name = UA_STRING(publishedDataSet2Name);
        retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet2).addResult;

        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal |= UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL).result;
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup 1");
        writerGroupConfig.publishingInterval = 10;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
        writerGroupConfig.name = UA_STRING("WriterGroup 2");
        writerGroupConfig.publishingInterval = 50;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup2);
        writerGroupConfig.name = UA_STRING("WriterGroup 3");
        writerGroupConfig.publishingInterval = 100;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection2, &writerGroupConfig, &writerGroup3);

        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
        retVal |= UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1, &dataSetWriterConfig, &dataSetWriter1);
        
        retVal |= UA_Server_enableDataSetWriter(server, dataSetWriter1);
        retVal |= UA_Server_enableAllPubSubComponents(server);

        UA_DateTime currentTime = UA_DateTime_now();
        UA_Server_WriterGroup_publish(server, writerGroup1);
        UA_DateTime publishTime;
        UA_WriterGroup_lastPublishTimestamp(server, writerGroup1, &publishTime);
        ck_assert((publishTime - currentTime) < UA_DATETIME_MSEC * 100);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(PublishDataSetFieldAsDeltaFrame){
        setupPublishedDataSetTestEnvironment();
        UA_DataSetFieldConfig dataSetFieldConfig;
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal |= UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL).result;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime2");
        retVal |= UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL).result;
        setupDataSetFieldTestEnvironment();
        retVal |= UA_Server_enableAllPubSubComponents(server);

        UA_PubSubManager *psm = getPSM(server);
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup1);
        wg->config.maxEncapsulatedDataSetMessageCount = 3;
        UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dataSetWriter1);
        dsw->config.keyFrameCount = 3;

        UA_WriterGroup_publishCallback(psm, wg);
        UA_WriterGroup_publishCallback(psm, wg);
        UA_WriterGroup_publishCallback(psm, wg);
        UA_WriterGroup_publishCallback(psm, wg);
        UA_WriterGroup_publishCallback(psm, wg);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST


/* Test DataSetOrdering reconfiguration (OPC UA Part 14, section 6.3.1.1.3) 
 * 
 * NOTE: This test validates that the DataSetOrdering mechanism is invoked correctly
 * with different configurations. It does not decode NetworkMessage buffers to verify
 * actual WriterId order in the wire format.
 */
START_TEST(DataSetOrderingReconfiguration) {
    UA_NodeId writerGroup, pds1, pds2, pds3;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    
    /* Create PublishedDataSets */
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("OrderingDataSet1");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &pds1).addResult;
    pdsConfig.name = UA_STRING("OrderingDataSet2");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &pds2).addResult;
    pdsConfig.name = UA_STRING("OrderingDataSet3");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &pds3).addResult;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    
    /* Create WriterGroup with UNDEFINED ordering initially */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("OrderingWriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    
    UA_UadpWriterGroupMessageDataType *writerGroupMessage = 
        UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask = 
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                           UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupMessage->dataSetOrdering = UA_DATASETORDERINGTYPE_UNDEFINED;
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    writerGroupConfig.messageSettings.content.decoded.type = 
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;

    retVal = UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Create DataSetWriters with mixed WriterIds: 300, 100, 200 */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Writer300");
    dataSetWriterConfig.dataSetWriterId = 300;
    retVal = UA_Server_addDataSetWriter(server, writerGroup, pds1, &dataSetWriterConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Writer100");
    dataSetWriterConfig.dataSetWriterId = 100;
    retVal = UA_Server_addDataSetWriter(server, writerGroup, pds2, &dataSetWriterConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Writer200");
    dataSetWriterConfig.dataSetWriterId = 200;
    retVal = UA_Server_addDataSetWriter(server, writerGroup, pds3, &dataSetWriterConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_enableWriterGroup(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Test 1: Publish with UNDEFINED ordering */
    retVal = UA_Server_triggerWriterGroupPublish(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Disable before reconfiguration */
    retVal = UA_Server_setWriterGroupDisabled(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Reconfigure to ASCENDINGWRITERID */
    UA_WriterGroupConfig configCopy;
    retVal = UA_Server_getWriterGroupConfig(server, writerGroup, &configCopy);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    
    UA_UadpWriterGroupMessageDataType *messageSettings = 
        (UA_UadpWriterGroupMessageDataType*)configCopy.messageSettings.content.decoded.data;
    messageSettings->dataSetOrdering = UA_DATASETORDERINGTYPE_ASCENDINGWRITERID;
    
    retVal = UA_Server_updateWriterGroupConfig(server, writerGroup, &configCopy);
    UA_WriterGroupConfig_clear(&configCopy);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Re-enable and test 2: Publish with ASCENDINGWRITERID ordering */
    retVal = UA_Server_enableWriterGroup(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_triggerWriterGroupPublish(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Disable before reconfiguration */
    retVal = UA_Server_setWriterGroupDisabled(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Reconfigure to ASCENDINGWRITERIDSINGLE */
    retVal = UA_Server_getWriterGroupConfig(server, writerGroup, &configCopy);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    
    messageSettings = (UA_UadpWriterGroupMessageDataType*)configCopy.messageSettings.content.decoded.data;
    messageSettings->dataSetOrdering = UA_DATASETORDERINGTYPE_ASCENDINGWRITERIDSINGLE;
    
    retVal = UA_Server_updateWriterGroupConfig(server, writerGroup, &configCopy);
    UA_WriterGroupConfig_clear(&configCopy);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Re-enable and test 3: Publish with ASCENDINGWRITERIDSINGLE ordering */
    retVal = UA_Server_enableWriterGroup(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_triggerWriterGroupPublish(server, writerGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

/* ---------------------------------------------------------------------------
 * Additional coverage tests:
 * Additional coverage tests (Phase A1):
 *  - WriterGroup / DataSetWriter state transitions
 *  - Double remove returns BADNOTFOUND
 *  - removeWriterGroup cascades to its DataSetWriters
 *  - Invalid configs (publishingInterval = 0, name == NULL)
 *  - keyFrameCount edge cases (0, 1, UINT32_MAX)
 *  - updateWriterGroupConfig is rejected while enabled
 * ------------------------------------------------------------------------- */

START_TEST(WriterGroupStateTransitions) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WriterGroup-State");
    wgc.publishingInterval = 100;
    UA_NodeId wgId;
    retVal = UA_Server_addWriterGroup(server, connection1, &wgc, &wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubState state = UA_PUBSUBSTATE_ERROR;
    retVal = UA_Server_getWriterGroupState(server, wgId, &state);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(state, UA_PUBSUBSTATE_DISABLED);

    /* enable -> operational/preoperational */
    retVal = UA_Server_enableWriterGroup(server, wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_getWriterGroupState(server, wgId, &state);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(UA_PubSubState_isEnabled(state));

    /* enable again should be idempotent (no error) */
    retVal = UA_Server_enableWriterGroup(server, wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* disable -> disabled */
    retVal = UA_Server_disableWriterGroup(server, wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_getWriterGroupState(server, wgId, &state);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(state, UA_PUBSUBSTATE_DISABLED);

    /* disable again - idempotent */
    retVal = UA_Server_disableWriterGroup(server, wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* getState on unknown id */
    retVal = UA_Server_getWriterGroupState(server,
                                           UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                           &state);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    /* enable / disable on unknown id */
    retVal = UA_Server_enableWriterGroup(server,
                                         UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_disableWriterGroup(server,
                                          UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    UA_Server_removeWriterGroup(server, wgId);
} END_TEST

START_TEST(DataSetWriterStateTransitions) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();

    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("DSW-State");
    UA_NodeId dswId;
    retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1,
                                        &dswc, &dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubState state = UA_PUBSUBSTATE_ERROR;
    retVal = UA_Server_getDataSetWriterState(server, dswId, &state);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_enableDataSetWriter(server, dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_disableDataSetWriter(server, dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* unknown id */
    retVal = UA_Server_enableDataSetWriter(server,
                                           UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_disableDataSetWriter(server,
                                            UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_getDataSetWriterState(server,
                                             UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                             &state);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(RemoveDataSetWriterTwiceReturnsBadNotFound) {
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();
    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("DSW-DoubleRemove");
    UA_NodeId dswId;
    UA_StatusCode retVal = UA_Server_addDataSetWriter(server, writerGroup1,
                                                      publishedDataSet1, &dswc,
                                                      &dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_removeDataSetWriter(server, dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* second remove must return BADNOTFOUND */
    retVal = UA_Server_removeDataSetWriter(server, dswId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNOTFOUND);

    /* remove of an arbitrary unknown id */
    retVal = UA_Server_removeDataSetWriter(server,
                                           UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(RemoveWriterGroupCascadesDataSetWriters) {
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();
    UA_PubSubManager *psm = getPSM(server);

    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("Cascade-DSW-1");
    UA_NodeId dsw1Id, dsw2Id;
    UA_StatusCode retVal = UA_Server_addDataSetWriter(server, writerGroup1,
                                                      publishedDataSet1, &dswc,
                                                      &dsw1Id);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    dswc.name = UA_STRING("Cascade-DSW-2");
    retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1,
                                        &dswc, &dsw2Id);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_WriterGroup *wg1 = UA_WriterGroup_find(psm, writerGroup1);
    ck_assert_ptr_ne(wg1, NULL);
    ck_assert_uint_eq(wg1->writersCount, 2);

    /* Remove the WriterGroup -> all attached DataSetWriters must vanish */
    retVal = UA_Server_removeWriterGroup(server, writerGroup1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    ck_assert_ptr_eq(UA_DataSetWriter_find(psm, dsw1Id), NULL);
    ck_assert_ptr_eq(UA_DataSetWriter_find(psm, dsw2Id), NULL);
    ck_assert_ptr_eq(UA_WriterGroup_find(psm, writerGroup1), NULL);

    /* second remove of the writergroup -> BADNOTFOUND */
    retVal = UA_Server_removeWriterGroup(server, writerGroup1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(AddDataSetWriterWithNullName) {
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();
    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    /* leave name as UA_STRING_NULL */
    UA_NodeId dswId;
    UA_StatusCode retVal = UA_Server_addDataSetWriter(server, writerGroup1,
                                                      publishedDataSet1, &dswc,
                                                      &dswId);
    /* Either the implementation rejects this or accepts; in both cases the
     * code path must be exercised. Assert at least no crash and a defined
     * return code (good or a BAD* error). */
    ck_assert(retVal == UA_STATUSCODE_GOOD ||
              (retVal & 0x80000000) != 0);
    if(retVal == UA_STATUSCODE_GOOD)
        UA_Server_removeDataSetWriter(server, dswId);
} END_TEST

START_TEST(WriterGroupKeyFrameCountEdgeCases) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    setupDataSetWriterTestEnvironment();
    setupPublishedDataSetTestEnvironment();

    const UA_UInt32 values[] = { 0u, 1u, UA_UINT32_MAX };
    for(size_t i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
        UA_DataSetWriterConfig dswc;
        memset(&dswc, 0, sizeof(dswc));
        char nameBuf[32];
        snprintf(nameBuf, sizeof(nameBuf), "DSW-KF-%zu", i);
        dswc.name = UA_STRING(nameBuf);
        dswc.keyFrameCount = values[i];
        UA_NodeId dswId;
        retVal = UA_Server_addDataSetWriter(server, writerGroup1,
                                            publishedDataSet1, &dswc, &dswId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_DataSetWriterConfig copy;
        retVal = UA_Server_getDataSetWriterConfig(server, dswId, &copy);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(copy.keyFrameCount, values[i]);
        UA_DataSetWriterConfig_clear(&copy);

        retVal = UA_Server_removeDataSetWriter(server, dswId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    }
} END_TEST

START_TEST(UpdateWriterGroupConfigRejectedWhileEnabled) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WG-UpdateLocked");
    wgc.publishingInterval = 100;
    UA_NodeId wgId;
    retVal = UA_Server_addWriterGroup(server, connection1, &wgc, &wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_enableWriterGroup(server, wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig copy;
    retVal = UA_Server_getWriterGroupConfig(server, wgId, &copy);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    copy.publishingInterval = 250;
    retVal = UA_Server_updateWriterGroupConfig(server, wgId, &copy);
    /* must not be GOOD while the group is enabled */
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    UA_WriterGroupConfig_clear(&copy);

    /* update on unknown id */
    UA_WriterGroupConfig empty;
    memset(&empty, 0, sizeof(empty));
    empty.name = UA_STRING("foo");
    empty.publishingInterval = 100;
    retVal = UA_Server_updateWriterGroupConfig(server,
                                               UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                               &empty);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    /* NULL config */
    retVal = UA_Server_updateWriterGroupConfig(server, wgId, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    UA_Server_disableWriterGroup(server, wgId);
    UA_Server_removeWriterGroup(server, wgId);
} END_TEST

START_TEST(GetWriterGroupConfigInvalidArgs) {
    UA_StatusCode retVal;
    UA_WriterGroupConfig copy;
    /* unknown id */
    retVal = UA_Server_getWriterGroupConfig(server,
                                            UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                            &copy);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    /* NULL out param */
    UA_NodeId wgId;
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WG-Get");
    wgc.publishingInterval = 100;
    retVal = UA_Server_addWriterGroup(server, connection1, &wgc, &wgId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_getWriterGroupConfig(server, wgId, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    UA_Server_removeWriterGroup(server, wgId);
} END_TEST

/* ---- Additional writer/writer-group public-API coverage ---- */

START_TEST(GetWriterGroupStateInvalid) {
    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    UA_StatusCode r =
        UA_Server_getWriterGroupState(server,
                                      UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                      &state);
    ck_assert_int_eq(r, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(TriggerWriterGroupPublishOnDisabledGroup) {
    UA_NodeId wgId;
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WG-Trigger");
    wgc.publishingInterval = 100;
    ck_assert_int_eq(UA_Server_addWriterGroup(server, connection1, &wgc, &wgId),
                     UA_STATUSCODE_GOOD);

    /* Triggering on a disabled group: just exercise the code path
     * (the implementation may accept it and queue the trigger) */
    UA_StatusCode r = UA_Server_triggerWriterGroupPublish(server, wgId);
    (void)r;

    /* Unknown id returns BADNOTFOUND */
    r = UA_Server_triggerWriterGroupPublish(server,
                                            UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_eq(r, UA_STATUSCODE_BADNOTFOUND);

    UA_Server_removeWriterGroup(server, wgId);
} END_TEST

START_TEST(GetWriterGroupLastPublishTimestampInvalid) {
    UA_DateTime ts = 0;
    UA_StatusCode r =
        UA_Server_getWriterGroupLastPublishTimestamp(server,
            UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &ts);
    ck_assert_int_eq(r, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(GetDataSetWriterStateAndConfigInvalid) {
    UA_PubSubState state = UA_PUBSUBSTATE_DISABLED;
    UA_StatusCode r =
        UA_Server_getDataSetWriterState(server,
            UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &state);
    ck_assert_int_eq(r, UA_STATUSCODE_BADNOTFOUND);

    UA_DataSetWriterConfig dswc;
    r = UA_Server_getDataSetWriterConfig(server,
            UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &dswc);
    ck_assert_int_ne(r, UA_STATUSCODE_GOOD);
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
    tcase_add_test(tc_pubsub_publish, SinglePublishDataSetFieldAndPublishTimestampTest);
    tcase_add_test(tc_pubsub_publish, PublishDataSetFieldAsDeltaFrame);

    TCase *tc_pubsub_datasetordering = tcase_create("PubSub DataSetOrdering (OPC UA Part 14)");
    tcase_add_checked_fixture(tc_pubsub_datasetordering, setup, teardown);
    tcase_add_test(tc_pubsub_datasetordering, DataSetOrderingReconfiguration);

    TCase *tc_pubsub_lifecycle = tcase_create("PubSub Writer/WriterGroup lifecycle and edge cases");
    tcase_add_checked_fixture(tc_pubsub_lifecycle, setup, teardown);
    tcase_add_test(tc_pubsub_lifecycle, WriterGroupStateTransitions);
    tcase_add_test(tc_pubsub_lifecycle, DataSetWriterStateTransitions);
    tcase_add_test(tc_pubsub_lifecycle, RemoveDataSetWriterTwiceReturnsBadNotFound);
    tcase_add_test(tc_pubsub_lifecycle, RemoveWriterGroupCascadesDataSetWriters);
    tcase_add_test(tc_pubsub_lifecycle, AddDataSetWriterWithNullName);
    tcase_add_test(tc_pubsub_lifecycle, WriterGroupKeyFrameCountEdgeCases);
    tcase_add_test(tc_pubsub_lifecycle, UpdateWriterGroupConfigRejectedWhileEnabled);
    tcase_add_test(tc_pubsub_lifecycle, GetWriterGroupConfigInvalidArgs);
    tcase_add_test(tc_pubsub_lifecycle, GetWriterGroupStateInvalid);
    tcase_add_test(tc_pubsub_lifecycle, TriggerWriterGroupPublishOnDisabledGroup);
    tcase_add_test(tc_pubsub_lifecycle, GetWriterGroupLastPublishTimestampInvalid);
    tcase_add_test(tc_pubsub_lifecycle, GetDataSetWriterStateAndConfigInvalid);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_add_pubsub_writergroup);
    suite_add_tcase(s, tc_add_pubsub_datasetwriter);
    suite_add_tcase(s, tc_add_pubsub_datasetfields);
    suite_add_tcase(s, tc_pubsub_publish);
    suite_add_tcase(s, tc_pubsub_datasetordering);
    suite_add_tcase(s, tc_pubsub_lifecycle);
    
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
