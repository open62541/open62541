/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "ua_server_internal.h"

#include <check.h>

#define TEST_MQTT_SERVER "opc.mqtt://localhost:1883"
//#define TEST_MQTT_SERVER "opc.mqtt://test.mosquitto.org:1883"

#define MQTT_CLIENT_ID               "TESTCLIENTPUBSUBMQTT"
#define CONNECTIONOPTION_NAME        "mqttClientId"
#define SUBSCRIBE_TOPIC              "customTopic"
#define SUBSCRIBE_INTERVAL             500

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

UA_NodeId connectionIdent;
UA_NodeId subscribedDataSetIdent;
UA_NodeId readerGroupIdent;

UA_NodeId publishedDataSetIdent;
UA_NodeId writerGroupIdent;

UA_DataSetReaderConfig readerConfig;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);

    //add connection
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    connectionConfig.enabled = UA_TRUE;

    /* configure address of the mqtt broker (local on default port) */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    /* Changed to static publisherId from random generation to identify
     * the publisher on Subscriber side */
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;

    /* configure options, set mqtt client id */
    const int connectionOptionsCount = 1;

    UA_KeyValuePair connectionOptions[connectionOptionsCount];

    size_t connectionOptionIndex = 0;
    connectionOptions[connectionOptionIndex].key = UA_QUALIFIEDNAME(0, CONNECTIONOPTION_NAME);
    UA_String mqttClientId = UA_STRING(MQTT_CLIENT_ID);
    UA_Variant_setScalar(&connectionOptions[connectionOptionIndex++].value, &mqttClientId, &UA_TYPES[UA_TYPES_STRING]);

    connectionConfig.connectionProperties.map = connectionOptions;
    connectionConfig.connectionProperties.mapSize = connectionOptionIndex;

    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* DateTime DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].name =  UA_STRING ("DateTime");
    pMetaData->fields[0].valueRank = -1; /* scalar */

    /* Int32 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                   &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[1].name =  UA_STRING ("Int32");
    pMetaData->fields[1].valueRank = -1; /* scalar */

    /* Int64 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                   &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
    pMetaData->fields[2].name =  UA_STRING ("Int64");
    pMetaData->fields[2].valueRank = -1; /* scalar */

    /* Boolean DataType */
    UA_FieldMetaData_init (&pMetaData->fields[3]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[3].name =  UA_STRING ("BoolToggle");
    pMetaData->fields[3].valueRank = -1; /* scalar */
}

START_TEST(SinglePublishSubscribeDateTime){
        UA_StatusCode retval = UA_STATUSCODE_GOOD;

        // add PublishedDataSet
        UA_PublishedDataSetConfig pdsConfig;
        memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
        pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        pdsConfig.name = UA_STRING("PublishedDataSet 1");
        UA_AddPublishedDataSetResult result =
            UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSetIdent);
        ck_assert_int_eq(result.addResult, UA_STATUSCODE_GOOD);

        /* Add a field to the previous created PublishedDataSet */
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
        dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
        dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataSetFieldResult dsFieldResult =
            UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, NULL);
        ck_assert_int_eq(dsFieldResult.result, UA_STATUSCODE_GOOD);

        // add writer group
        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("Demo WriterGroup");
        writerGroupConfig.publishingInterval = SUBSCRIBE_INTERVAL;
        writerGroupConfig.enabled = UA_FALSE;
        writerGroupConfig.writerGroupId = 100;
        UA_UadpWriterGroupMessageDataType *writerGroupMessage;

        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
        writerGroupMessage->networkMessageContentMask =
            (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                               (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                               (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                               (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;

        /* configure the mqtt publish topic */
        UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));
        brokerTransportSettings.queueName = UA_STRING(SUBSCRIBE_TOPIC);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        writerGroupConfig.transportSettings = transportSettings;
        retval = UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        retval = UA_Server_enableWriterGroup(server, writerGroupIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);

        // add DataSetWriter
        UA_NodeId dataSetWriterIdent;
        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
        dataSetWriterConfig.dataSetWriterId = 62541;
        dataSetWriterConfig.keyFrameCount = 10;
        dataSetWriterConfig.transportSettings = transportSettings;

        retval = UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                                            &dataSetWriterConfig, &dataSetWriterIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        retval = UA_Server_enableWriterGroup(server, writerGroupIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroupIdent);
        ck_assert(wg != 0);

        while(wg->state != UA_PUBSUBSTATE_OPERATIONAL)
            UA_Server_run_iterate(server, false);

        UA_WriterGroup_publishCallback(server, wg);

        /*---------------------------------------------------------------------*/

        // add reader group
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup1");

        /* configure the mqtt publish topic */
        UA_BrokerWriterGroupTransportDataType brokerTransportSettingsSubscriber;
        memset(&brokerTransportSettingsSubscriber, 0, sizeof(UA_BrokerWriterGroupTransportDataType));

        brokerTransportSettingsSubscriber.queueName = UA_STRING(SUBSCRIBE_TOPIC);
        brokerTransportSettingsSubscriber.resourceUri = UA_STRING_NULL;
        brokerTransportSettingsSubscriber.authenticationProfileUri = UA_STRING_NULL;

        brokerTransportSettingsSubscriber.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        UA_ExtensionObject transportSettingsSubscriber;
        memset(&transportSettingsSubscriber, 0, sizeof(UA_ExtensionObject));
        transportSettingsSubscriber.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettingsSubscriber.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE];
        transportSettingsSubscriber.content.decoded.data = &brokerTransportSettingsSubscriber;

        readerGroupConfig.transportSettings = transportSettingsSubscriber;

        retval = UA_Server_addReaderGroup(server, connectionIdent, &readerGroupConfig,
                                          &readerGroupIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
        retval = UA_Server_enableReaderGroup(server, readerGroupIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        // add DataSetReader
        memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
        readerConfig.name = UA_STRING("DataSet Reader 1");
        UA_UInt16 publisherIdentifier = 2234;
        readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
        readerConfig.publisherId.id.uint16 = publisherIdentifier;
        readerConfig.writerGroupId    = 100;
        readerConfig.dataSetWriterId  = 62541;

        fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

        retval = UA_Server_addDataSetReader(server, readerGroupIdent, &readerConfig,
                                            &subscribedDataSetIdent);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);


        // add SubscribedVariables
        UA_NodeId folderId;
        UA_String folderName = readerConfig.dataSetMetaData.name;
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

        retval = UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                          UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                          UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                          folderBrowseName, UA_NODEID_NUMERIC (0,
                                                                               UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        /* Create the TargetVariables with respect to DataSetMetaData fields */
        UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
        for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
            /* Variable to subscribe data */
            UA_VariableAttributes vAttr = UA_VariableAttributes_default;
            UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                                  &vAttr.description);
            vAttr.displayName.locale = UA_STRING("en-US");
            vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
            vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

            UA_NodeId newNode;
            retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                                folderId,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vAttr, NULL, &newNode);
            ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

            /* For creating Targetvariables */
            UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
            targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
            targetVars[i].targetVariable.targetNodeId = newNode;
        }

        retval = UA_Server_DataSetReader_createTargetVariables(server, subscribedDataSetIdent,
                                                               readerConfig.dataSetMetaData.fieldsSize, targetVars);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

        for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
            UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

        UA_free(targetVars);
        UA_free(readerConfig.dataSetMetaData.fields);

    } END_TEST

int main(void) {
    TCase *tc_pubsub_subscribe_mqtt = tcase_create("PubSub subscribe mqtt");
    tcase_add_checked_fixture(tc_pubsub_subscribe_mqtt, setup, teardown);
    tcase_add_test(tc_pubsub_subscribe_mqtt, SinglePublishSubscribeDateTime);

    Suite *s = suite_create("PubSub subscribe via mqtt");
    suite_add_tcase(s, tc_pubsub_subscribe_mqtt);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
