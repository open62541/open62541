/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>
#include <time.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#define MULTICAST_URL "opc.udp://224.0.0.22:4801/"
//#define MULTICAST_URL "opc.udp://[ff01::100]:4801/"

#define UA_SUBSCRIBER_PORT       4801    /* Port for Subscriber*/
#define PUBLISH_INTERVAL         5       /* Publish interval*/
#define PUBLISHER_ID             2234    /* Publisher Id*/
#define DATASET_WRITER_ID        62541   /* DataSet Writer Id*/
#define WRITER_GROUP_ID          100     /* Writer group Id  */
#define PUBLISHER_DATA           42      /* Published data */
#define PUBLISHVARIABLE_NODEID   1000    /* Published data nodeId */
#define SUBSCRIBEVARIABLE_NODEID 1002    /* Subscribed data nodeId */
#define SUBSCRIBEVARIABLE2_NODEID 1003   /* Subscribed data nodeId */
#define READERGROUP_COUNT        2       /* Value to add readergroup to connection */
#define CHECK_READERGROUP_COUNT  3       /* Value to check readergroup count */
#define VALID_DATASETMESSAGE_SIZE 128    /* Valid DataSetMessage configuredSize */
#define INVALID_DATASETMESSAGE_SIZE 1    /* Invalid DataSetMessage configuredSize */

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_NodeId connectionId;
UA_NodeId readerGroupId;
UA_NodeId publishedDataSetId;

/* Nodes in the information model */
UA_NodeId folderId;
UA_NodeId nodeId32;
UA_NodeId nodeId64;
UA_NodeId nodeIdDateTime;

static void addVariables(void) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
    folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                folderBrowseName,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                oAttr, NULL, &folderId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Variable to subscribe data */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName.locale = UA_STRING("en-US");
    vAttr.displayName.text = UA_STRING("UInt32 Variable");
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    res = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)0 + 50000), folderId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "UInt32 Variable"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &nodeId32);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    vAttr.displayName.locale = UA_STRING("en-US");
    vAttr.displayName.text = UA_STRING("UInt64 Variable");
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    res = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)1 + 50001), folderId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "UInt64 Variable"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &nodeId64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "DateTime");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "DateTime");
    vAttr.dataType    = UA_TYPES[UA_TYPES_DATETIME].typeId;
    res = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)1 + 50002), folderId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "DateTime"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &nodeIdDateTime);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);
    UA_Server_run_startup(server);

    addVariables();

    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Test Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(MULTICAST_URL)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void checkReceived(void) {
    while(true) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
        /* Read data sent by the Publisher */
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.attributeId = UA_ATTRIBUTEID_VALUE;
        rvi.nodeId = UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID);
        UA_DataValue publishedNodeData = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        
        /* Read data received by the Subscriber */
        rvi.nodeId = UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID);
        UA_DataValue subscribedNodeData = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        
        /* Check if data sent from Publisher is being received by Subscriber */
        ck_assert(publishedNodeData.value.type == subscribedNodeData.value.type);
        UA_Boolean eq = 
            (UA_order(&publishedNodeData, &subscribedNodeData,
                      &UA_TYPES[UA_TYPES_DATAVALUE]) == UA_ORDER_EQ);
        UA_DataValue_clear(&subscribedNodeData);
        UA_DataValue_clear(&publishedNodeData);
        if(eq)
            break;
    }
}

START_TEST(AddReaderGroupWithValidConfiguration) {
    /* To test if ReaderGroup has been added to the connection with valid configuration */
    UA_StatusCode retVal;
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup Test");
    UA_NodeId localreaderGroup;
    retVal =  UA_Server_addReaderGroup(server, connectionId,
                                       &readerGroupConfig, &localreaderGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    size_t readerGroupCount = 0;
    UA_ReaderGroup *readerGroup;
    UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connectionId);
    LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
        readerGroupCount++;
    }
    /* Check readerGroup count */
    ck_assert_uint_eq(readerGroupCount, 1);
    /* To Do: RemoveReaderGroup operation should be carried out when UA_Server_delete has been called */
    UA_Server_removeReaderGroup(server, localreaderGroup);
} END_TEST

START_TEST(AddReaderGroupWithNullConfig) {
    /* Check the status of adding ReaderGroup when NULL configuration is given */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |=  UA_Server_addReaderGroup(server, connectionId, NULL, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    size_t readerGroupCount = 0;
    UA_ReaderGroup *readerGroup;
    UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connectionId);
    LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
        readerGroupCount++;
    }
    /* Check readerGroup count */
    ck_assert_uint_eq(readerGroupCount, 0);
} END_TEST

START_TEST(AddReaderGroupWithInvalidConnectionId) {
    /* Check status of adding ReaderGroup with invalid connection identifier */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup Test");
    retVal |=  UA_Server_addReaderGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX), &readerGroupConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    size_t readerGroupCount = 0;
    UA_ReaderGroup *readerGroup;
    UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connectionId);
    LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
        readerGroupCount++;
    }
    /* Check readerGroup count */
    ck_assert_uint_eq(readerGroupCount, 0);
} END_TEST

START_TEST(RemoveReaderGroupWithInvalidIdentifier) {
        /* Check status of removing ReaderGroup when giving invalid ReaderGroup identifier */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Delete the added readerGroup */
        retVal |= UA_Server_removeReaderGroup(server, UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);

        UA_PubSubManager *psm = getPSM(server);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connectionId);
        LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
            readerGroupCount++;
        }
        /* Check readerGroup count */
        ck_assert_uint_eq(readerGroupCount, 1);
        UA_Server_removeReaderGroup(server, localreaderGroup);
    } END_TEST

START_TEST(AddRemoveMultipleAddReaderGroupWithValidConfiguration) {
        UA_StatusCode retVal   = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup 1");
        UA_NodeId localReaderGroup;
        /* Add ReaderGroup */
        retVal |= UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localReaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Remove added ReaderGroup */
        retVal |= UA_Server_removeReaderGroup(server, localReaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_PubSubManager *psm = getPSM(server);
        size_t readerGroupCount = 0;
        UA_ReaderGroup *readerGroup;
        UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connectionId);
        LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
            readerGroupCount++;
        }

        /* Check ReaderGroup Count */
        ck_assert_uint_eq(readerGroupCount, 0);
        /* Add Multiple ReaderGroups */
        for (int iterator = 0; iterator <= READERGROUP_COUNT; iterator++) {
            retVal |= UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localReaderGroup);
            ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        }

        readerGroupCount = 0;
        /* Find ReaderGroup count */
        LIST_FOREACH(readerGroup, &conn->readerGroups, listEntry) {
            readerGroupCount++;
        }
        /* Check ReaderGroup Count */
        ck_assert_uint_eq(readerGroupCount, CHECK_READERGROUP_COUNT);
    } END_TEST

      /* Check status of updating ReaderGroup with invalid identifier */
      /*
START_TEST(UpdateReaderGroupWithInvalidIdentifier) {
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_ReaderGroupConfig readerGroupConfig;
        UA_NodeId localreaderGroup;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal |= UA_Server_ReaderGroup_getConfig(server, localreaderGroup, &readerGroupConfig);
        /* To Do: DeleteMembers of readergroup config has to be a separate function */
        UA_String_clear (&readerGroupConfig.name);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetreader Test");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup,
                                             &dataSetreaderConfig, &localDataSetreader);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &localreaderGroup);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test ");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup,
                                             &dataSetreaderConfig, &localDataSetreader);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &localreaderGroup);
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
    UA_PubSubManager *psm = getPSM(server);
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
    retVal |= UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localReaderGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localReaderGroup2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_ReaderGroup *readerGroupIdent1 = UA_ReaderGroup_find(psm, localReaderGroup);
    UA_ReaderGroup *readerGroupIdent2 = UA_ReaderGroup_find(psm, localReaderGroup2);
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

START_TEST(GetDataSetReaderConfigWithValidConfiguration) {
        /* Check status of getting DataSetReader with Valid configuration */
        UA_StatusCode retVal = UA_STATUSCODE_GOOD;
        UA_DataSetReaderConfig dataSetreaderConfig;
        UA_NodeId localreaderGroup;
        UA_NodeId localDataSetreader;
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        retVal |= UA_Server_DataSetReader_getConfig(server, localDataSetreader, &dataSetreaderConfig);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String_clear(&dataSetreaderConfig.name);
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test");
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
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test");
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
        UA_ReaderGroupConfig readerGroupConfig;
        memset(&readerGroupConfig, 0, sizeof(readerGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &localreaderGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        memset(&dataSetreaderConfig, 0, sizeof(dataSetreaderConfig));
        dataSetreaderConfig.name = UA_STRING("DataSetReader Test");
        retVal |= UA_Server_addDataSetReader(server, localreaderGroup, &dataSetreaderConfig, &localDataSetreader);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        retVal |= UA_Server_DataSetReader_createTargetVariables(server,
                                                                UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                                                0, NULL);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Data Set Field */
        UA_NodeId dataSetFieldId;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal = UA_Server_addDataSetField(server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldId).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup (server, connectionId, &readerGroupConfig, &readerGroupId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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

        readerConfig.subscribedDataSet.target.targetVariablesSize = 1;
        readerConfig.subscribedDataSet.target.targetVariables = (UA_FieldTargetDataType *)
            UA_calloc(1, sizeof(UA_FieldTargetDataType));

        /* For creating Targetvariable */
        readerConfig.subscribedDataSet.target.targetVariables->attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.target.targetVariables->targetNodeId = nodeIdDateTime;
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig, &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(readerConfig.subscribedDataSet.target.targetVariables);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));
        UA_free(pMetaData->fields);
}END_TEST

START_TEST(SinglePublishSubscribeDateTimeRaw) {
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Data Set Field */
        UA_NodeId dataSetFieldId;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        retVal = UA_Server_addDataSetField(server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldId).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount = 10;
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup (server, connectionId, &readerGroupConfig, &readerGroupId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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

        readerConfig.subscribedDataSet.target.targetVariablesSize = 1;
        readerConfig.subscribedDataSet.target.targetVariables = (UA_FieldTargetDataType*)
            UA_calloc(1, sizeof(UA_FieldTargetDataType));

        /* For creating Targetvariable */
        readerConfig.subscribedDataSet.target.targetVariables->attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.target.targetVariables->targetNodeId = nodeIdDateTime;
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig, &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* run server - publisher and subscriber */
        UA_free(readerConfig.subscribedDataSet.target.targetVariables);
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        retVal = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(
            server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, "Subscribed Int32"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));
        
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
} END_TEST

START_TEST(SinglePublishSubscribeInt32StatusCode) {
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_DataSetFieldResult retval = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);

        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_STATUSCODE;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);

        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
        readerConfig.writerGroupId    = WRITER_GROUP_ID;
        readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
        readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_STATUSCODE;
        /* Setting up Meta data configuration in DataSetReader */
        UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
        /* FilltestMetadata function in subscriber implementation */
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name       = UA_STRING ("DataSet Test");
        /* Static definition of number of fields size to 1 to create one
           targetVariable */
        pMetaData->fieldsSize = 1;
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* Write the value back - but with a status code */
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.attributeId = UA_ATTRIBUTEID_VALUE;
        rvi.nodeId = publisherNode;

        UA_WriteValue wv;
        UA_WriteValue_init(&wv);
        wv.value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        wv.value.hasStatus = true;
        wv.value.status = UA_STATUSCODE_BADINTERNALERROR;
        wv.attributeId = UA_ATTRIBUTEID_VALUE;
        wv.nodeId = publisherNode;
        UA_Server_write(server, &wv);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        /* run server - publisher and subscriber */
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);

        /* Check that the status code was received */
        checkReceived();

        /* Unset the status code */
        wv.value.hasStatus = false;
        UA_Server_write(server, &wv);
        UA_WriteValue_clear(&wv);
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.dataType              = UA_TYPES[UA_TYPES_INT64].typeId;
        UA_Int64 publisherData     = 64;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT64]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_DataSetFieldResult retval = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT64].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT64;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT64].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, "Subscribed Int64"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        /* run server - publisher and subscriber */
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish boolean data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Bool");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Bool");
        attr.dataType              = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
        UA_Boolean publisherData   = UA_FALSE;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_DataSetFieldResult retval = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* Boolean DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_BOOLEAN;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Bool");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Bool");
        vAttr.dataType    = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, "Subscribed Bool"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        /* run server - publisher and subscriber */
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
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
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataSetFieldResult retval = UA_Server_addDataSetField(server, publishedDataSetId,
                                  &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask          =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.displayName.locale    = UA_STRING ("en-US");
        vAttr.displayName.text      = UA_STRING ("Subscribed Integer");
        vAttr.valueRank             = -1;
        vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, "Subscribed Integer"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
} END_TEST

START_TEST(SinglePublishSubscribeHeartbeat) {
    UA_PubSubManager *psm = getPSM(server);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId dataSetWriter;
    UA_NodeId readerIdentifier;
    UA_NodeId writerGroup;
    UA_DataSetReaderConfig readerConfig;

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name               = UA_STRING("WriterGroup Test");
    writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
    writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);

    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
    dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount   = 1;
    retVal |= UA_Server_addDataSetWriter(server, writerGroup, UA_NODEID_NULL,
                                         &dataSetWriterConfig, &dataSetWriter);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);

    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name             = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = PUBLISHER_ID;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = WRITER_GROUP_ID;
    readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name       = UA_STRING ("DataSet Test 2");
    /* Static definition of number of fields size to 1 to create one targetVariable */
    pMetaData->fieldsSize = 0;
    pMetaData->fields     = NULL;
    retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_free(pMetaData->fields);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* run server - publisher and subscriber */
    UA_fakeSleep(50 + 1);
    UA_Server_run_iterate(server,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(server,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(server,true);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, readerIdentifier);
    ck_assert_ptr_ne(dsr, NULL);

    UA_fakeSleep(100);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(SinglePublishSubscribeWithoutPayloadHeader) {
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_DataSetFieldResult retval = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);
        ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        /* run server - publisher and subscriber */
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
} END_TEST

/* Have two readers listening for the same writer */
START_TEST(MultiPublishSubscribeInt32) {
    /* To check status after running both publisher and subscriber */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    UA_NodeId dataSetWriter;
    UA_NodeId readerIdentifier;
    UA_NodeId writerGroup;

    /* Published DataSet */
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet Test");
    retVal =UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Create variable to publish integer data */
    UA_NodeId publisherNode;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
    attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 publisherData     = 42;
    UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
    UA_DataSetFieldResult retval = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);
    ck_assert_int_eq(retval.result, UA_STATUSCODE_GOOD);
    /* Writer group */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name               = UA_STRING("WriterGroup Test");
    writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
    writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    /* Message settings in WriterGroup to include necessary headers */
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* DataSetWriter */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
    dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount   = 10;
    retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                         &dataSetWriterConfig, &dataSetWriter);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);

    /* Data Set Reader */
    /* Parameters to filter received NetworkMessage */
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name             = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = PUBLISHER_ID;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = WRITER_GROUP_ID;
    readerConfig.dataSetWriterId  = DATASET_WRITER_ID;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING ("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize = 1;
    pMetaData->fields     = (UA_FieldMetaData*)
        UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* Unsigned Integer DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[0].valueRank   = -1; /* scalar */
    retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Add Subscribed Variables */
    /* Variable to subscribe data */
    UA_NodeId newnodeId;
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
    vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       vAttr, NULL, &newnodeId);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* For creating Targetvariable */
    UA_FieldTargetDataType targetVar;
    UA_FieldTargetDataType_init(&targetVar);
    targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVar.targetNodeId = newnodeId;
    retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier, 1, &targetVar);

    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Create a second reader for the same writer in the same readergroup */
    UA_NodeId reader2Id;
    retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig, &reader2Id);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Variable to subscribe data */
    UA_NodeId newnodeId2;
    retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE2_NODEID), folderId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Subscribed Int32 - 2"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       vAttr, NULL, &newnodeId2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Create Targetvariable */
    targetVar.targetNodeId = newnodeId2;
    retVal |= UA_Server_DataSetReader_createTargetVariables(server, reader2Id, 1, &targetVar);

    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_FieldTargetDataType_clear(&targetVar);
    UA_free(pMetaData->fields);

    /* run server - publisher and subscriber */
    ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

    /* run server - publisher and subscriber */
    UA_fakeSleep(50 + 1);
    UA_Server_run_iterate(server,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(server,true);
    UA_fakeSleep(PUBLISH_INTERVAL + 1);
    UA_Server_run_iterate(server,true);
    checkReceived();

    /* Check the received value for the second reader */
    UA_Variant publishedNodeData;
    UA_Variant_init(&publishedNodeData);
    retVal = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
                                 &publishedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Read data received by the Subscriber */
    UA_Variant subscribedNodeData;
    UA_Variant_init(&subscribedNodeData);
    retVal = UA_Server_readValue(server, newnodeId2, &subscribedNodeData);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Check if data sent from Publisher is being received by Subscriber */
    ck_assert_int_eq(*(UA_UInt32 *)publishedNodeData.data,
                     *(UA_UInt32 *)subscribedNodeData.data);
    UA_Variant_clear(&subscribedNodeData);
    UA_Variant_clear(&publishedNodeData);
} END_TEST

static void
addTargetVariable(void) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_DateTime dateTime = 0;
    UA_Variant_setScalar(&attr.value, &dateTime, &UA_TYPES[UA_TYPES_DATETIME]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "subscribedTargetVar");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "subscribedTargetVar");
    attr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "demoVar");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "subscribedTargetVar");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    retVal = UA_Server_addVariableNode(
        server, myIntegerNodeId, parentNodeId, parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

START_TEST(ValidDataSetConfigurationAddAndRemove) {
    UA_PubSubManager *psm = getPSM(server);
    addTargetVariable();
    UA_SubscribedDataSetConfig cfg;
    UA_NodeId ret;
    memset(&cfg, 0, sizeof(UA_SubscribedDataSetConfig));

    /* Fill the SSDS MetaData */
    UA_DataSetMetaDataType_init(&cfg.dataSetMetaData);
    cfg.dataSetMetaData.name = UA_STRING("DemoStandaloneSDS");
    cfg.dataSetMetaData.fieldsSize = 1;
    UA_FieldMetaData fieldMetaData;
    cfg.dataSetMetaData.fields = &fieldMetaData;

    /* DateTime DataType */
    UA_FieldMetaData_init(&cfg.dataSetMetaData.fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &cfg.dataSetMetaData.fields[0].dataType);
    cfg.dataSetMetaData.fields[0].builtInType = UA_NS0ID_DATETIME;
    cfg.dataSetMetaData.fields[0].name = UA_STRING("subscribedTargetVar");
    cfg.dataSetMetaData.fields[0].valueRank = -1; /* scalar */

    cfg.name = UA_STRING("DemoStandaloneSDS");
    cfg.subscribedDataSet.target.targetVariablesSize = 1;
    UA_FieldTargetDataType fieldTargetDataType;
    cfg.subscribedDataSet.target.targetVariables = &fieldTargetDataType;

    UA_FieldTargetDataType_init(&cfg.subscribedDataSet.target.targetVariables[0]);
    cfg.subscribedDataSet.target.targetVariables[0].attributeId = UA_ATTRIBUTEID_VALUE;
    cfg.subscribedDataSet.target.targetVariables[0].targetNodeId =
    UA_NODEID_STRING(1, "demoVar");
    UA_StatusCode retVal = UA_Server_addSubscribedDataSet(server, &cfg, &ret);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_SubscribedDataSet *ssds = NULL;
    ssds = UA_SubscribedDataSet_find(psm, ret);
    ck_assert_ptr_ne(ssds, NULL);
    ssds = NULL;
    ssds = UA_SubscribedDataSet_findByName(psm, UA_STRING("DemoStandaloneSDS"));
    ck_assert_ptr_ne(ssds, NULL);
    UA_SubscribedDataSetConfig ssds_config;
    memset(&ssds_config, 0, sizeof(UA_SubscribedDataSetConfig));
    retVal = UA_SubscribedDataSetConfig_copy(&ssds->config, &ssds_config);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_SubscribedDataSetConfig_clear(&ssds_config);
    retVal = UA_Server_removeSubscribedDataSet(server, ret);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ssds = UA_SubscribedDataSet_findByName(psm, UA_STRING("DemoStandaloneSDS"));
    ck_assert_ptr_eq(ssds, NULL);
    ssds = UA_SubscribedDataSet_find(psm, ret);
    ck_assert_ptr_eq(ssds, NULL);

} END_TEST

START_TEST(AddAndRemoveReaderUsingDataSet) {
    UA_PubSubManager *psm = getPSM(server);
    addTargetVariable();
    UA_SubscribedDataSetConfig cfg;
    UA_NodeId ret;
    memset(&cfg, 0, sizeof(UA_SubscribedDataSetConfig));

    /* Fill the SSDS MetaData */
    UA_DataSetMetaDataType_init(&cfg.dataSetMetaData);
    cfg.dataSetMetaData.name = UA_STRING("DemoStandaloneSDS");
    cfg.dataSetMetaData.fieldsSize = 1;
    UA_FieldMetaData fieldMetaData;
    cfg.dataSetMetaData.fields = &fieldMetaData;

    /* DateTime DataType */
    UA_FieldMetaData_init(&cfg.dataSetMetaData.fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &cfg.dataSetMetaData.fields[0].dataType);
    cfg.dataSetMetaData.fields[0].builtInType = UA_NS0ID_DATETIME;
    cfg.dataSetMetaData.fields[0].name = UA_STRING("subscribedTargetVar");
    cfg.dataSetMetaData.fields[0].valueRank = -1; /* scalar */

    cfg.name = UA_STRING("DemoStandaloneSDS");
    cfg.subscribedDataSet.target.targetVariablesSize = 1;
    UA_FieldTargetDataType fieldTargetDataType;
    cfg.subscribedDataSet.target.targetVariables = &fieldTargetDataType;

    UA_FieldTargetDataType_init(&cfg.subscribedDataSet.target.targetVariables[0]);
    cfg.subscribedDataSet.target.targetVariables[0].attributeId = UA_ATTRIBUTEID_VALUE;
    cfg.subscribedDataSet.target.targetVariables[0].targetNodeId =
    UA_NODEID_STRING(1, "demoVar");
    UA_StatusCode retVal = UA_Server_addSubscribedDataSet(server, &cfg, &ret);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_SubscribedDataSet *ssds = NULL;
    ssds = UA_SubscribedDataSet_find(psm, ret);
    ck_assert_ptr_ne(ssds, NULL);
    /* Reader Group */
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
    retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);

    /* Data Set Reader */
    /* Parameters to filter received NetworkMessage */
    UA_DataSetReaderConfig readerConfig;
    memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
    readerConfig.name             = UA_STRING ("DataSetReader Test");
    UA_UInt16 publisherIdentifier = PUBLISHER_ID;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = WRITER_GROUP_ID;
    readerConfig.dataSetWriterId  = DATASET_WRITER_ID;
    readerConfig.linkedStandaloneSubscribedDataSetName = cfg.name;
    /* DataSetMetaData already contained in Standalone SDS no need to set up */
    UA_NodeId readerIdentifier;
    retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                         &readerIdentifier);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    /* SSDS is now connected to this reader -> isConnected shall be true */
    ck_assert(ssds->connectedReader != NULL);

    UA_Server_removeDataSetReader(server, readerIdentifier);
    /* SSDS no longer connected, isConnected has to reset(false) */
    ck_assert(ssds->connectedReader == NULL);
} END_TEST

START_TEST(SinglePublishOnDemand) {
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
        retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId).addResult;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int64");
        attr.dataType              = UA_TYPES[UA_TYPES_INT64].typeId;
        UA_Int64 publisherData     = 64;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT64]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        retVal = UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent).result;
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId,
                                            &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT64].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT64;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int64");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT64].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, "Subscribed Int64"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_FieldTargetDataType_clear(&targetVar);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));

        /* run server - publisher and subscriber */
        UA_fakeSleep(50 + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        UA_fakeSleep(PUBLISH_INTERVAL + 1);
        UA_Server_run_iterate(server,true);
        checkReceived();
        retVal = UA_Server_WriterGroup_publish(server, writerGroup);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        checkReceived();

    } END_TEST

START_TEST(ValidConfiguredSizPublishSubscribe) {
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
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        /* messageSettings to specify the padding for the dataSetMessage */
        dataSetWriterConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        dataSetWriterConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE];
        UA_UadpDataSetWriterMessageDataType *messageSettings = UA_UadpDataSetWriterMessageDataType_new();
        /* set the configuredSize */
        messageSettings->configuredSize = VALID_DATASETMESSAGE_SIZE;
        dataSetWriterConfig.messageSettings.content.decoded.data = messageSettings;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        UA_UadpDataSetWriterMessageDataType_delete(messageSettings);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));
        UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(InvalidConfiguredSizPublishSubscribe) {
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
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Create variable to publish integer data */
        UA_NodeId publisherNode;
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.displayName           = UA_LOCALIZEDTEXT("en-US","Published Int32");
        attr.dataType              = UA_TYPES[UA_TYPES_INT32].typeId;
        UA_Int32 publisherData     = 42;
        UA_Variant_setScalar(&attr.value, &publisherData, &UA_TYPES[UA_TYPES_INT32]);
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, PUBLISHVARIABLE_NODEID),
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
        UA_Server_addDataSetField (server, publishedDataSetId, &dataSetFieldConfig, &dataSetFieldIdent);

        /* Writer group */
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
        writerGroupConfig.name               = UA_STRING("WriterGroup Test");
        writerGroupConfig.publishingInterval = PUBLISH_INTERVAL;
        writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
        writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
        /* Message settings in WriterGroup to include necessary headers */
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
            (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER;
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        retVal |= UA_Server_addWriterGroup(server, connectionId, &writerGroupConfig, &writerGroup);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* DataSetWriter */
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name            = UA_STRING("DataSetWriter Test");
        dataSetWriterConfig.dataSetWriterId = DATASET_WRITER_ID;
        dataSetWriterConfig.keyFrameCount   = 10;
        /* messageSettings to specify the padding for the dataSetMessage */
        dataSetWriterConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        dataSetWriterConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE];
        UA_UadpDataSetWriterMessageDataType *messageSettings = UA_UadpDataSetWriterMessageDataType_new();
        /* set the configuredSize */
        messageSettings->configuredSize = INVALID_DATASETMESSAGE_SIZE;
        dataSetWriterConfig.messageSettings.content.decoded.data = messageSettings;
        retVal |= UA_Server_addDataSetWriter(server, writerGroup, publishedDataSetId,
                                             &dataSetWriterConfig, &dataSetWriter);
        UA_UadpDataSetWriterMessageDataType_delete(messageSettings);

        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Reader Group */
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof (UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING ("ReaderGroup Test");
        retVal |=  UA_Server_addReaderGroup(server, connectionId, &readerGroupConfig, &readerGroupId);
        /* Data Set Reader */
        /* Parameters to filter received NetworkMessage */
        memset (&readerConfig, 0, sizeof (UA_DataSetReaderConfig));
        readerConfig.name             = UA_STRING ("DataSetReader Test");
        UA_UInt16 publisherIdentifier = PUBLISHER_ID;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
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
        pMetaData->fields     = (UA_FieldMetaData*)
            UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
        /* Unsigned Integer DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT32].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[0].valueRank   = -1; /* scalar */
        retVal |= UA_Server_addDataSetReader(server, readerGroupId, &readerConfig,
                                             &readerIdentifier);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Add Subscribed Variables */
        /* Variable to subscribe data */
        UA_NodeId newnodeId;
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Int32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_INT32].typeId;
        retVal = UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, SUBSCRIBEVARIABLE_NODEID), folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),  UA_QUALIFIEDNAME(1, "Subscribed Int32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType targetVar;
        UA_FieldTargetDataType_init(&targetVar);
        targetVar.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVar.targetNodeId = newnodeId;
        retVal |= UA_Server_DataSetReader_createTargetVariables(server, readerIdentifier,
                                                                1, &targetVar);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_free(pMetaData->fields);

        /* run server - publisher and subscriber */
        ck_assert_int_eq(UA_STATUSCODE_GOOD, UA_Server_enableAllPubSubComponents(server));
        UA_Server_run_iterate(server, false);
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
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithValidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithInvalidConfiguration);
    tcase_add_test(tc_add_pubsub_readergroup, GetDataSetReaderConfigWithInvalidIdentifier);
    tcase_add_test(tc_add_pubsub_readergroup, CreateTargetVariableWithInvalidConfiguration);

    /*Test case to run both publisher and subscriber */
    TCase *tc_pubsub_publish_subscribe = tcase_create("Publisher publishing and Subscriber subscribing");
    tcase_add_checked_fixture(tc_pubsub_publish_subscribe, setup, teardown);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeDateTime);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeDateTimeRaw);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt32StatusCode);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeInt64);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeBool);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribewithValidIdentifiers);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeHeartbeat);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishSubscribeWithoutPayloadHeader);
    tcase_add_test(tc_pubsub_publish_subscribe, MultiPublishSubscribeInt32);
    tcase_add_test(tc_pubsub_publish_subscribe, SinglePublishOnDemand);

    /*Test cases for the subscribed datasets */
    TCase *tc_pubsub_datasets = tcase_create("Subscriber using subscribed datasets");
    tcase_add_test(tc_pubsub_publish_subscribe, ValidDataSetConfigurationAddAndRemove);
    tcase_add_test(tc_pubsub_publish_subscribe, AddAndRemoveReaderUsingDataSet);

    TCase *tc_dataSetMessage_padding = tcase_create("PubSub DataSetMessage padding");
    tcase_add_checked_fixture(tc_dataSetMessage_padding, setup, teardown);

    tcase_add_test(tc_dataSetMessage_padding, ValidConfiguredSizPublishSubscribe);
    tcase_add_test(tc_dataSetMessage_padding, InvalidConfiguredSizPublishSubscribe);

    Suite *suite = suite_create("PubSub readerGroups/reader/Fields handling and publishing");
    suite_add_tcase(suite, tc_add_pubsub_readergroup);
    suite_add_tcase(suite, tc_pubsub_publish_subscribe);
    suite_add_tcase(suite, tc_pubsub_datasets);
    suite_add_tcase(suite, tc_dataSetMessage_padding);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
