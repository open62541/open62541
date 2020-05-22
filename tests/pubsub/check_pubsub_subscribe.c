/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>
#include <time.h>

#include "open62541/types_generated_encoding_binary.h"
#include "ua_pubsub.h"
#include "ua_server_internal.h"

#define UA_SUBSCRIBER_PORT       4801    /* Port for Subscriber*/
#define PUBLISH_INTERVAL         5       /* Publish interval*/
#define PUBLISHER_ID             2234    /* Publisher Id*/
#define DATASET_WRITER_ID        62541   /* DataSet Writer Id*/
#define WRITER_GROUP_ID          100     /* Writer group Id  */
#define PUBLISHER_DATA           42      /* Published data */
#define PUBLISHVARIABLE_NODEID   1000    /* Published data nodeId */
#define SUBSCRIBEOBJECT_NODEID   1001    /* Object nodeId */
#define SUBSCRIBEVARIABLE_NODEID 1002    /* Subscribed data nodeId */
#define READERGROUP_COUNT        2       /* Value to add readergroup to connection */
#define CHECK_READERGROUP_COUNT  3       /* Value to check readergroup count */

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_NodeId connection_test;
UA_NodeId readerGroupTest;

UA_NodeId publishedDataSetTest;

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);
    UA_Server_run_startup(server);
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_clean(config);
    }

    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;

    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Test Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4801/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.numeric = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    UA_PubSubConnection_regist(server, &connection_test);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddReaderGroupWithValidConfiguration) {
        /* To test if ReaderGroup has been added to the connection with valid configuration */
        UA_StatusCode retVal;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        UA_NodeId localreaderGroup;
        retVal =  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry){
            readerGroupCount++;
        }
        /* Check readerGroup count */
        ck_assert_int_eq(readerGroupCount, 1);
        /* To Do: RemoveReaderGroup operation should be carried out when UA_Server_delete has been called */
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST

START_TEST(AddReaderGroupWithNullConfig) {
        /* Check the status of adding ReaderGroup when NULL configuration is given */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        retVal |=  UA_Server_addReaderGroup(server, connection_test, NULL, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry){
            readerGroupCount++;
        }
        /* Check readerGroup count */
        ck_assert_int_eq(readerGroupCount, 0);
    } END_TEST

START_TEST(AddReaderGroupWithInvalidConnectionId) {
        /* Check status of adding ReaderGroup with invalid connection identifier */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &readerGroupConfig, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry){
            readerGroupCount++;
        }
        /* Check readerGroup count */
        ck_assert_int_eq(readerGroupCount, 0);
    } END_TEST

START_TEST(RemoveReaderGroupWithInvalidIdentifier) {
        /* Check status of removing ReaderGroup when giving invalid ReaderGroup identifier */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Delete the added readerGroup */
        retVal |= UA_Server_removeReaderGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry){
            readerGroupCount++;
        }
        /* Check readerGroup count */
        ck_assert_int_eq(readerGroupCount, 1);
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST

START_TEST(AddRemoveMultipleAddReaderGroupWithValidConfiguration) {
        UA_StatusCode retVal   = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup 1");
        UA_NodeId localReaderGroup;
        /* Add ReaderGroup */
        retVal |= UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localReaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Remove added ReaderGroup */
        retVal |= UA_Server_removeReaderGroup(server, localReaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry) {
            readerGroupCount++;
        }

        /* Check ReaderGroup Count */
        ck_assert_int_eq(readerGroupCount, 0);
        /* Add Multiple ReaderGroups */
        for (int iterator = 0; iterator <= READERGROUP_COUNT; iterator++) {
            retVal |= UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localReaderGroup);
            ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        }

        readerGroupCount = 0;
        /* Find ReaderGroup count */
        LIST_FOREACH(readerGroup, &UA_PubSubConnection_findConnectionbyId(server, connection_test)->readerGroups, listEntry) {
            readerGroupCount++;
        }
        /* Check ReaderGroup Count */
        ck_assert_int_eq(readerGroupCount, CHECK_READERGROUP_COUNT);
    } END_TEST

      /* Check status of updating ReaderGroup with invalid identifier */
      /*
START_TEST(UpdateReaderGroupWithInvalidIdentifier) {
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |=  UA_Server_ReaderGroup_updateConfig(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST
    */

START_TEST(GetReaderGroupConfigWithInvalidConfig) {
        /* Check status of getting ReaderGroup configuration with invalid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_ReaderGroup_getConfig(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST

START_TEST(GetReaderGroupConfigWithInvalidIdentifier) {
        /* Check status of getting ReaderGroup configuration with invlaid identifier */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_ReaderGroup_getConfig(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &readerGroupConfig);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST

START_TEST(GetReaderGroupConfigWithValidConfig) {
        /* Check status of getting ReaderGroup configuration with valid parameters */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_NodeId localreaderGroup;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_ReaderGroup_getConfig(server, localreaderGroup, &readerGroupConfig);
        /* To Do: DeleteMembers of readergroup config has to be a separate function */
        UA_String_deleteMembers (&readerGroupConfig.name);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddDataSetReaderWithValidConfiguration) {
        /* Check status of adding DataSetReader to ReaderGroup with valid parameters*/
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localDataSetreader;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetreader Test");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddDataSetReaderWithNullConfig) {
        /* Check status of adding DataSetReader to ReaderGroup with invalid parameters */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetreader Test ");
        /* Remove the added DataSetReader */
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, NULL, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(RemoveDataSetReaderWithValidConfiguration) {
        /* Check status of adding DataSetReader to ReaderGroup with valid parameters */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localDataSetreader;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test ");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_removeDataSetReader(server, localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(RemoveDataSetReaderWithInvalidIdentifier) {
        /* Check status of removing DataSetReader from ReaderGroup with invalid parameters */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test ");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, NULL, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
        /* Remove the added DataSetReader */
        retVal |= UA_Server_removeDataSetReader(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddMultipleDataSetReaderWithValidConfiguration) {
        UA_StatusCode retVal    = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name  = UA_STRING("ReaderGroup 1");
        UA_NodeId localReaderGroup;
        UA_NodeId localReaderGroup2;
        /* DataSetReader configuration */
        UA_DataSetReaderConfig readerConfig;
        memset (&readerConfig, 0, sizeof(readerConfig));
        readerConfig.name       = UA_STRING("DataSet Reader 1");
        UA_NodeId dataSetReader;
        /* Add ReaderGroup */
        retVal |= UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localReaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localReaderGroup2);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_ReaderGroup *readerGroupIdent1 = UA_ReaderGroup_findRGbyId(server, localReaderGroup);
        UA_ReaderGroup *readerGroupIdent2 = UA_ReaderGroup_findRGbyId(server, localReaderGroup2);
        /* Add DataSetReaders to first ReaderGroup */
        retVal = UA_Server_addDataSetReader(server, localReaderGroup, &readerConfig, &dataSetReader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(readerGroupIdent1->readersCount, 1);
        retVal = UA_Server_addDataSetReader(server, localReaderGroup, &readerConfig, &dataSetReader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(readerGroupIdent1->readersCount, 2);
        /* Add DataSetReaders to second ReaderGroup */
        retVal = UA_Server_addDataSetReader(server, localReaderGroup2, &readerConfig, &dataSetReader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(readerGroupIdent2->readersCount, 1);
        retVal = UA_Server_addDataSetReader(server, localReaderGroup2, &readerConfig, &dataSetReader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(readerGroupIdent2->readersCount, 2);
    } END_TEST

START_TEST(UpdateDataSetReaderConfigWithInvalidId) {
        /* Check status of updatting DataSetReader with invalid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup,
                                                        &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |=  UA_Server_DataSetReader_updateConfig(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                                      localreaderGroup, &dataSetreaderConfig );
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetDataSetReaderConfigWithValidConfiguration) {
        /* Check status of getting DataSetReader with Valid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_DataSetReader_getConfig(server, localDataSetreader, &dataSetreaderConfig);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetDataSetReaderConfigWithInvalidConfiguration) {
        /* Check status of getting DataSetReader with Invalid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_DataSetReader_getConfig(server, localDataSetreader, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(GetDataSetReaderConfigWithInvalidIdentifier) {
        /* Check status of getting DataSetReader with Invalid Identifier */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_DataSetReader_getConfig(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &dataSetreaderConfig);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(CreateTargetVariableWithInvalidConfiguration) {
        /* Check status to create target variable with invalid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_TargetVariablesDataType localTargetVariable;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &localTargetVariable);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddTargetVariableWithInvalidConfiguration) {
        /* Check status to create target variable with invalid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |=  UA_Server_DataSetReader_addTargetVariables(NULL, &localreaderGroup, localDataSetreader, UA_PUBSUB_SDS_TARGET);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddTargetVariableWithValidConfiguration) {
        /* Check status after creating target variables with Valid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetReaderConfig;
        UA_NodeId folderId;
        UA_QualifiedName folderBrowseName;
        UA_NodeId localreaderGroupIdentifier;
        UA_NodeId localDataSetreaderIdentifier;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &localreaderGroupIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetReaderConfig, 0, sizeof(UA_DataSetReaderConfig));
        UA_DataSetMetaDataType *pMetaData = &dataSetReaderConfig.dataSetMetaData;
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 2 to create targetVariables
         * with DateTime and ByteString datatype */
        pMetaData->fieldsSize = 2;
        pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                             &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* DateTime DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
        pMetaData->fields[0].valueRank = -1; /* scalar */

        /* ByteString DataType */
        UA_FieldMetaData_init (&pMetaData->fields[1]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_BYTESTRING].typeId,
                        &pMetaData->fields[1].dataType);
        pMetaData->fields[1].builtInType = UA_NS0ID_BYTESTRING;
        pMetaData->fields[1].valueRank = -1; /* scalar */

        retVal |= UA_Server_addDataSetReader(server, localreaderGroupIdentifier, &dataSetReaderConfig, &localDataSetreaderIdentifier);

        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                 folderBrowseName, UA_NODEID_NUMERIC (0,
                                 UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        retVal |=  UA_Server_DataSetReader_addTargetVariables(server, &folderId, localDataSetreaderIdentifier, UA_PUBSUB_SDS_TARGET);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);
    } END_TEST

START_TEST(SinglePublishSubscribeDateTime) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Data Set Field */
        UA_NodeId dataSetFieldId;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField(server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldId);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup (server, connection_test, &readerGroupConfig, &readerGroupTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name = UA_STRING ("DataSetReader Test");
        readerConfig.dataSetWriterId = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader for DateTime DataType */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name = UA_STRING ("DataSet Test");
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
        pMetaData->fields[0].valueRank = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader (server, readerGroupTest, &readerConfig,
                                                          &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_String folderName = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale = UA_STRING ("en-US");
            oAttr.displayName.text = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name = folderName;
          }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                 folderBrowseName, UA_NODEID_NUMERIC (0,
                                 UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        retVal |=  UA_Server_DataSetReader_addTargetVariables (server, &folderId,
                                                             readerIdentifier, UA_PUBSUB_SDS_TARGET);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);
        UA_Server_run_iterate(server,true);
        UA_free(pMetaData->fields);

   }END_TEST

START_TEST(SinglePublishSubscribeInt32) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Int32"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Int32");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &readerGroupTest);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
           targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                 &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_NodeId newnodeId;
        UA_String folderName      = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale        = UA_STRING ("en-US");
            oAttr.displayName.text          = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name           = folderName;
        }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        retVal = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         folderBrowseName, UA_NODEID_NUMERIC(0,
                                         UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType targetVars;
        targetVars.targetVariablesSize = 1;
        targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                          UA_calloc(targetVars.targetVariablesSize,
                                          sizeof(UA_FieldTargetDataType));
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
        targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[0].targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                &targetVars);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType_deleteMembers(&targetVars);
        UA_free(pMetaData->fields);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);

        /* Read data sent by the Publisher */
        UA_Variant *publishedNodeData = UA_Variant_new();
        retVal                        = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal                         = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert_int_eq(*(UA_Int32 *)publishedNodeData->data, *(UA_Int32 *)subscribedNodeData->data);
        UA_Variant_delete(subscribedNodeData);
        UA_Variant_delete(publishedNodeData);
    } END_TEST

START_TEST(SinglePublishSubscribeInt64) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.dataType              = UA_TYPES[UA_TYPES_INT64].typeId;
        UA_Int64 publisherData     = 64;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT64]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Int64"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Int64");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &readerGroupTest);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
           targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                 &UA_TYPES[UA_TYPES_FIELDMETADATA]);
                /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT64].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT64;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_NodeId newnodeId;
        UA_String folderName      = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale        = UA_STRING ("en-US");
            oAttr.displayName.text          = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name           = folderName;
        }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        retVal = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         folderBrowseName, UA_NODEID_NUMERIC(0,
                                         UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT64].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int64"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType targetVars;
        targetVars.targetVariablesSize = 1;
        targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                          UA_calloc(targetVars.targetVariablesSize,
                                          sizeof(UA_FieldTargetDataType));
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
        targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[0].targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                &targetVars);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType_deleteMembers(&targetVars);
        UA_free(pMetaData->fields);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);

        /* Read data sent by the Publisher */
        UA_Variant *publishedNodeData = UA_Variant_new();
        retVal                        = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal                         = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert_int_eq(*(UA_Int64 *)publishedNodeData->data, *(UA_Int64 *)subscribedNodeData->data);
        UA_Variant_delete(subscribedNodeData);
        UA_Variant_delete(publishedNodeData);
    } END_TEST

START_TEST(SinglePublishSubscribeBool) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish boolean data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Bool");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Bool");
        attr.dataType              = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
        UA_Boolean publisherData   = UA_FALSE;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Bool"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Bool");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &readerGroupTest);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
           targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                 &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Boolean DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_BOOLEAN;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_NodeId newnodeId;
        UA_String folderName      = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale        = UA_STRING ("en-US");
            oAttr.displayName.text          = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name           = folderName;
        }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        retVal = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         folderBrowseName, UA_NODEID_NUMERIC(0,
                                         UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Bool");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Bool");
        vAttr.dataType    = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Bool"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType targetVars;
        targetVars.targetVariablesSize = 1;
        targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                          UA_calloc(targetVars.targetVariablesSize,
                                          sizeof(UA_FieldTargetDataType));
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
        targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[0].targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                &targetVars);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType_deleteMembers(&targetVars);
        UA_free(pMetaData->fields);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);

        /* Read data sent by the Publisher */
        UA_Variant *publishedNodeData = UA_Variant_new();
        retVal                        = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal                         = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert_int_eq(*(UA_Boolean *)publishedNodeData->data, *(UA_Boolean *)subscribedNodeData->data);
        UA_Variant_delete(subscribedNodeData);
        UA_Variant_delete(publishedNodeData);
    } END_TEST

START_TEST(SinglePublishSubscribewithValidIdentifiers) {
        /* To check status after running both publisher and subscriber */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_PublishedDataSetConfig pdsConfig;
        UA_NodeId dataSetWriter;
        UA_NodeId readerIdentifier;
        UA_NodeId writerGroup;
        UA_DataSetReaderConfig readerConfig;

        /* Published DataSet */
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet Test");
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetTest);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Integer");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Integer");
        attr.dataType              = UA_TYPES[UA_TYPES_UINT32].typeId;
        attr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_UInt32 publisherData    = PUBLISHER_DATA;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_UINT32]);
        retVal                     = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                               UA_QUALIFIEDNAME(1, "Published Integer"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                               attr, NULL, &publisherNode);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Data Set Field */
        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType              = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Published Integer");
        dataSetFieldConfig.field.variable.promotedField  = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = publisherNode;
        dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField (server, publishedDataSetTest, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.enabled            = UA_FALSE;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                  (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connection_test, &writerGroupConfig, &writerGroup);
        UA_Server_setWriterGroupOperational(server, writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetTest, &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connection_test, &readerGroupConfig, &readerGroupTest);
        UA_Server_setReaderGroupOperational(server, readerGroupTest);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
        readerConfig.publisherId.data = &publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
        targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                 &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupTest, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Add Subscribed Variables */
        UA_NodeId folderId;
        UA_NodeId newnodeId;
        UA_String folderName      = readerConfig.dataSetMetaData.name;
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        UA_QualifiedName folderBrowseName;
        if (folderName.length > 0) {
            oAttr.displayName.locale        = UA_STRING ("en-US");
            oAttr.displayName.text          = folderName;
            folderBrowseName.namespaceIndex = 1;
            folderBrowseName.name           = folderName;
        }
        else {
            oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
            folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
        }

        retVal = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         folderBrowseName, UA_NODEID_NUMERIC(0,
                                         UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.displayName.locale    = UA_STRING ("en-US");
        vAttr.displayName.text      = UA_STRING ("Subscribed Integer");
        vAttr.valueRank             = -1;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID),
                                           UA_NODEID_NUMERIC(1, SUBSCRIBEOBJECT_NODEID),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Integer"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType targetVars;
        targetVars.targetVariablesSize = 1;
        targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                          UA_calloc(targetVars.targetVariablesSize,
                                          sizeof(UA_FieldTargetDataType));
        /* For creating Targetvariable */
        UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
        targetVars.targetVariables[0].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[0].targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                &targetVars);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_TargetVariablesDataType_deleteMembers(&targetVars);
        UA_free(pMetaData->fields);
        /* run server - publisher and subscriber */
        UA_Server_run_iterate(server,true);

        /* Read data sent by the Publisher */
        UA_Variant *publishedNodeData = UA_Variant_new();
        retVal                        = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID), publishedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Read data received by the Subscriber */
        UA_Variant *subscribedNodeData = UA_Variant_new();
        retVal                         = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), subscribedNodeData);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert_int_eq(*(UA_UInt32 *)publishedNodeData->data, *(UA_UInt32 *)subscribedNodeData->data);
        UA_Variant_delete(subscribedNodeData);
        UA_Variant_delete(publishedNodeData);
    } END_TEST

int main(void) {
    TCase *tc_add_pubsub_readergroup = tcase_create("PubSub readerGroup items handling");
    tcase_add_checked_fixture(tc_add_pubsub_readergroup, setup, teardown);

    /* Test cases for ReaderGroup functionality */
    tcase_add_test(tc_add_pubsub_readergroup, AddReaderGroupWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, AddReaderGroupWithNullConfig);
    tcase_add_test(tc_add_pubsub_readergroup, AddReaderGroupWithInvalidConnectionId);
    tcase_add_test(tc_add_pubsub_readergroup, RemoveReaderGroupWithInvalidIdentifier);
    tcase_add_test(tc_add_pubsub_readergroup, AddRemoveMultipleAddReaderGroupWithValidConfiguration);
    /* tcase_add_test(tc_add_pubsub_readergroup, UpdateReaderGroupWithInvalidIdentifier); */
    tcase_add_test(tc_add_pubsub_readergroup, GetReaderGroupConfigWithInvalidConfig);
    tcase_add_test(tc_add_pubsub_readergroup, GetReaderGroupConfigWithInvalidIdentifier);
    tcase_add_test(tc_add_pubsub_readergroup, GetReaderGroupConfigWithValidConfig);

    /* Test cases for DataSetReader functionality */
    tcase_add_test(tc_add_pubsub_readergroup, AddDataSetReaderWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, AddDataSetReaderWithNullConfig);
    tcase_add_test(tc_add_pubsub_readergroup, RemoveDataSetReaderWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, RemoveDataSetReaderWithInvalidIdentifier);
    tcase_add_test(tc_add_pubsub_readergroup, AddMultipleDataSetReaderWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, UpdateDataSetReaderConfigWithInvalidId);
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithInvalidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithInvalidIdentifier);
    tcase_add_test(tc_add_pubsub_readergroup, CreateTargetVariableWithInvalidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, AddTargetVariableWithInvalidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, AddTargetVariableWithValidConfiguration);

    /*Test case to run both publisher and subscriber */
    TCase *tc_pubsub_publish_subscribe = tcase_create("Publisher publishing and Subscriber subscribing");
    tcase_add_checked_fixture(tc_pubsub_publish_subscribe, setup, teardown);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeDateTime);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt64);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeBool);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribewithValidIdentifiers);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_add_pubsub_readergroup);
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
