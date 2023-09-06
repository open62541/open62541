/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Kalycito Infotech Private Limited (Author: Suriya Narayanan)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <time.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(CreateAndLockConfiguration) {
    //create config
    UA_NodeId connection1, readerGroup1, dataSetReader1;
    UA_PubSubConnectionConfig connectionConfig;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup 1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;
    retVal |= UA_Server_addReaderGroup(server, connection1, &readerGroupConfig, &readerGroup1);

    //get internal RG Pointer
    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroup1);
    ck_assert(readerGroup->state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(readerGroup->configurationFrozen == UA_FALSE);

    UA_DataSetReaderConfig dataSetReaderConfig;
    memset(&dataSetReaderConfig, 0, sizeof(dataSetReaderConfig));
    dataSetReaderConfig.name = UA_STRING("DataSetReader 1");
    retVal |= UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader1);
    UA_DataSetReader *dataSetReader = UA_ReaderGroup_findDSRbyId(server, dataSetReader1);
    ck_assert(dataSetReader != NULL);
    ck_assert(dataSetReader->configurationFrozen == UA_FALSE);

    //get internal PubSubConnection Pointer
    UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connection1);
    ck_assert(pubSubConnection != NULL);
    ck_assert(pubSubConnection->configurationFreezeCounter == 0);

    //Lock the reader group and the child pubsub entities
    retVal |= UA_Server_freezeReaderGroupConfiguration(server, readerGroup1);

    ck_assert(readerGroup->configurationFrozen == UA_TRUE);
    ck_assert(dataSetReader->configurationFrozen == UA_TRUE);
    ck_assert(pubSubConnection->configurationFreezeCounter > 0);

    //set state to disabled and implicit unlock the configuration
    retVal |= UA_Server_unfreezeReaderGroupConfiguration(server, readerGroup1);

    ck_assert(readerGroup->configurationFrozen == UA_FALSE);
    ck_assert(dataSetReader->configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->configurationFreezeCounter == 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(CreateAndReleaseMultipleLocks) {
    //create config
    UA_NodeId connection1, readerGroup1, readerGroup2, dataSetReader1, dataSetReader2, dataSetReader3;
    UA_PubSubConnectionConfig connectionConfig;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    //Add two reader groups
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup 1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;
    retVal |= UA_Server_addReaderGroup(server, connection1, &readerGroupConfig, &readerGroup1);
    readerGroupConfig.name = UA_STRING("ReaderGroup 2");
    retVal |= UA_Server_addReaderGroup(server, connection1, &readerGroupConfig, &readerGroup2);

    UA_DataSetReaderConfig dataSetReaderConfig;
    memset(&dataSetReaderConfig, 0, sizeof(dataSetReaderConfig));
    dataSetReaderConfig.name = UA_STRING("DataSetReader 1");
    retVal |= UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader1);
    dataSetReaderConfig.name = UA_STRING("DataSetReader 2");
    retVal |= UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader2);
    dataSetReaderConfig.name = UA_STRING("DataSetReader 3");
    retVal |= UA_Server_addDataSetReader(server, readerGroup2, &dataSetReaderConfig, &dataSetReader3);

    UA_ReaderGroup *readerGroup_1 = UA_ReaderGroup_findRGbyId(server, readerGroup1);
    UA_ReaderGroup *readerGroup_2 = UA_ReaderGroup_findRGbyId(server, readerGroup2);
    UA_DataSetReader *dataSetReader_1 = UA_ReaderGroup_findDSRbyId(server, dataSetReader1);
    UA_DataSetReader *dataSetReader_2 = UA_ReaderGroup_findDSRbyId(server, dataSetReader2);
    UA_DataSetReader *dataSetReader_3 = UA_ReaderGroup_findDSRbyId(server, dataSetReader3);
    UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connection1);
    //freeze configuration of both RG
    ck_assert(readerGroup_1->configurationFrozen == UA_FALSE);
    ck_assert(readerGroup_2->configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->configurationFreezeCounter == 0);

    retVal |= UA_Server_freezeReaderGroupConfiguration(server, readerGroup1);
    retVal |= UA_Server_freezeReaderGroupConfiguration(server, readerGroup2);

    ck_assert(readerGroup_1->configurationFrozen == UA_TRUE);
    ck_assert(readerGroup_2->configurationFrozen == UA_TRUE);
    ck_assert(pubSubConnection->configurationFreezeCounter > 0);

    //unlock one tree, get sure connection still locked
    retVal |= UA_Server_unfreezeReaderGroupConfiguration(server, readerGroup1);
    ck_assert(readerGroup_1->configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->configurationFreezeCounter > 0);
    ck_assert(dataSetReader_1->configurationFrozen == UA_FALSE);
    ck_assert(dataSetReader_2->configurationFrozen == UA_FALSE);
    ck_assert(dataSetReader_3->configurationFrozen == UA_TRUE);

    retVal |= UA_Server_unfreezeReaderGroupConfiguration(server, readerGroup2);
    ck_assert(readerGroup_2->configurationFrozen == UA_FALSE);
    ck_assert(pubSubConnection->configurationFreezeCounter == 0);
    ck_assert(dataSetReader_3->configurationFrozen == UA_FALSE);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(CreateLockAndEditConfiguration) {
    UA_NodeId connection1, readerGroup1, dataSetReader1, dataSetReader2;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup 1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;
    retVal |= UA_Server_addReaderGroup(server, connection1, &readerGroupConfig, &readerGroup1);

    //get internal RG Pointer
    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroup1);
    ck_assert(readerGroup->state == UA_PUBSUBSTATE_DISABLED);
    ck_assert(readerGroup->configurationFrozen == UA_FALSE);

    UA_DataSetReaderConfig dataSetReaderConfig;
    memset(&dataSetReaderConfig, 0, sizeof(dataSetReaderConfig));
    dataSetReaderConfig.name = UA_STRING("DataSetReader 1");
    /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
    UA_DataSetMetaDataType *pMetaData = &dataSetReaderConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
    targetVariable */
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* DateTime DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].name =  UA_STRING ("DateTime");
    pMetaData->fields[0].valueRank = -1; /* scalar */
    retVal |= UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader1);

    UA_NodeId folderId;
    UA_String folderName = dataSetReaderConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    retVal |= UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
        UA_calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < dataSetReaderConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&dataSetReaderConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = dataSetReaderConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = dataSetReaderConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retVal |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                            folderId,
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                            UA_QUALIFIEDNAME(1, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                            vAttr, NULL, &newNode);
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    UA_DataSetReader *dataSetReader_1 = UA_ReaderGroup_findDSRbyId(server, dataSetReader1);
    ck_assert(dataSetReader_1 != NULL);
    ck_assert(dataSetReader_1->configurationFrozen == UA_FALSE);

    //Lock the reader group and the child pubsub entities
    UA_Server_freezeReaderGroupConfiguration(server, readerGroup1);
    //call not allowed configuration methods
    retVal = UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader2);
    ck_assert(retVal == UA_STATUSCODE_BADCONFIGURATIONERROR);
    retVal = UA_Server_removeDataSetReader(server, dataSetReader1);
    ck_assert(retVal == UA_STATUSCODE_BADCONFIGURATIONERROR);
    retVal = UA_Server_DataSetReader_createTargetVariables(server, dataSetReader1,
                                                           dataSetReaderConfig.dataSetMetaData.fieldsSize,
                                                           targetVars);
    ck_assert(retVal == UA_STATUSCODE_BADCONFIGURATIONERROR);

    //unlock the reader group
    UA_Server_unfreezeReaderGroupConfiguration(server, readerGroup1);
    retVal = UA_Server_DataSetReader_createTargetVariables(server, dataSetReader1,
                                                           dataSetReaderConfig.dataSetMetaData.fieldsSize,
                                                           targetVars);
    ck_assert(retVal == UA_STATUSCODE_GOOD);
    retVal = UA_Server_addDataSetReader(server, readerGroup1, &dataSetReaderConfig, &dataSetReader2);
    for(size_t i = 0; i < dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(dataSetReaderConfig.dataSetMetaData.fields);
    ck_assert(retVal == UA_STATUSCODE_GOOD);
    retVal = UA_Server_removeDataSetReader(server, dataSetReader1);
    ck_assert(retVal == UA_STATUSCODE_GOOD);
    } END_TEST

int main(void) {
    TCase *tc_lock_configuration = tcase_create("Create and Lock");
    tcase_add_checked_fixture(tc_lock_configuration, setup, teardown);
    tcase_add_test(tc_lock_configuration, CreateAndLockConfiguration);
    tcase_add_test(tc_lock_configuration, CreateAndReleaseMultipleLocks);
    tcase_add_test(tc_lock_configuration, CreateLockAndEditConfiguration);

    Suite *s = suite_create("PubSub subscriber configuration lock mechanism");
    suite_add_tcase(s, tc_lock_configuration);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
