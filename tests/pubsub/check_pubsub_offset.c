/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 4
#define PUBSUB_CONFIG_MAX_STRING_LENGTH 8
#define PUBSUB_SUBSCRIBER_NODEID_BASE 61000

UA_Server *server;
UA_DataSetReaderConfig readerConfig;
static UA_NodeId publishedDataSetIdent, writerGroupIdent, connectionIdentifier;
static UA_NodeId readerGroupIdentifier, readerIdentifier;

static UA_NetworkAddressUrlDataType networkAddressUrl =
    {{0, NULL}, UA_STRING_STATIC("opc.udp://224.0.0.22:4840/")};
static UA_String transportProfile =
    UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

static void
assertFixedStringOffsets(const UA_PubSubOffsetTable *ot,
                         const UA_NodeId components[PUBSUB_CONFIG_FIELD_COUNT],
                         size_t firstFieldSize) {
    const UA_PubSubOffset *rawOffsets[PUBSUB_CONFIG_FIELD_COUNT];
    size_t rawOffsetsSize = 0;
    for(size_t i = 0; i < ot->offsetsSize; i++) {
        if(ot->offsets[i].offsetType != UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW)
            continue;
        ck_assert_uint_lt(rawOffsetsSize, PUBSUB_CONFIG_FIELD_COUNT);
        rawOffsets[rawOffsetsSize++] = &ot->offsets[i];
    }

    ck_assert_uint_eq(rawOffsetsSize, PUBSUB_CONFIG_FIELD_COUNT);
    for(size_t i = 0; i < rawOffsetsSize; i++)
        ck_assert(UA_NodeId_equal(&rawOffsets[i]->component, &components[i]));
    ck_assert_uint_eq(rawOffsets[1]->offset - rawOffsets[0]->offset,
                      firstFieldSize);
    ck_assert_uint_eq(rawOffsets[2]->offset - rawOffsets[1]->offset,
                      4 + PUBSUB_CONFIG_MAX_STRING_LENGTH);
    ck_assert_uint_eq(rawOffsets[3]->offset - rawOffsets[2]->offset,
                      4 + PUBSUB_CONFIG_MAX_STRING_LENGTH);
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(PublisherOffsets) {
    /* Add a PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);

    /* Add a PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);

    /* Add a direct String field between two fixed-size numeric fields */
    UA_NodeId stringNodeId = UA_NODEID_NUMERIC(1, 60000);
    UA_String stringValue = UA_STRING("abc");
    UA_VariableAttributes stringAttr = UA_VariableAttributes_default;
    stringAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    stringAttr.valueRank = UA_VALUERANK_SCALAR;
    stringAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&stringAttr.value, &stringValue, &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode res = UA_Server_addVariableNode(
        server, stringNodeId, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "Offset String"), UA_NS0ID(BASEDATAVARIABLETYPE),
        stringAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId byteStringNodeId = UA_NODEID_NUMERIC(1, 60001);
    UA_ByteString byteStringValue = UA_BYTESTRING("xy");
    UA_VariableAttributes byteStringAttr = UA_VariableAttributes_default;
    byteStringAttr.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
    byteStringAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&byteStringAttr.value, &byteStringValue,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    res = UA_Server_addVariableNode(
        server, byteStringNodeId, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "Offset ByteString"), UA_NS0ID(BASEDATAVARIABLETYPE),
        byteStringAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId publishedVariables[PUBSUB_CONFIG_FIELD_COUNT] = {
        UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME),
        stringNodeId,
        byteStringNodeId,
        UA_NS0ID(SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN)
    };
    UA_NodeId fieldIds[PUBSUB_CONFIG_FIELD_COUNT];
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        UA_DataSetFieldConfig dsfConfig;
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        dsfConfig.field.variable.publishParameters.publishedVariable = publishedVariables[i];
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        if(i == 1 || i == 2)
            dsfConfig.field.variable.maxStringLength =
                PUBSUB_CONFIG_MAX_STRING_LENGTH;
        UA_DataSetFieldResult result =
            UA_Server_addDataSetField(server, publishedDataSetIdent,
                                      &dsfConfig, &fieldIds[i]);
        ck_assert_uint_eq(result.result, UA_STATUSCODE_GOOD);
    }

    /* Add a WriterGroup */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUBSUB_CONFIG_PUBLISH_CYCLE_MS;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    /* Change message settings of writerGroup to send PublisherId, WriterGroupId
     * in GroupHeader and DataSetWriterId in PayloadHeader of NetworkMessage */
    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_UadpWriterGroupMessageDataType_init(&writerGroupMessage);
    writerGroupMessage.networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    UA_ExtensionObject_setValue(&writerGroupConfig.messageSettings, &writerGroupMessage,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);

    UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent);

    /* Add a DataSetWriter to the WriterGroup */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    UA_UadpDataSetWriterMessageDataType uadpDataSetWriterMessageDataType;
    UA_UadpDataSetWriterMessageDataType_init(&uadpDataSetWriterMessageDataType);
    uadpDataSetWriterMessageDataType.dataSetMessageContentMask =
        UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    UA_ExtensionObject_setValue(&dataSetWriterConfig.messageSettings,
                                &uadpDataSetWriterMessageDataType,
                                &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]);

    res = UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                                     &dataSetWriterConfig, &dataSetWriterIdent);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Compute the offsets with a short string */
    UA_PubSubOffsetTable ot;
    res = UA_Server_computeWriterGroupOffsetTable(server, writerGroupIdent, &ot);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertFixedStringOffsets(&ot, fieldIds, sizeof(UA_DateTime));

    /* Cleanup */
    UA_PubSubOffsetTable_clear(&ot);
} END_TEST

/* Define MetaData for TargetVariables */
static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL)
        return;

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to PUBSUB_CONFIG_FIELD_COUNT
     * to create targetVariables */
    pMetaData->fieldsSize = PUBSUB_CONFIG_FIELD_COUNT;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    for(size_t i = 0; i < pMetaData->fieldsSize; i++) {
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        if(i == 1) {
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_STRING].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_STRING;
            pMetaData->fields[i].maxStringLength =
                PUBSUB_CONFIG_MAX_STRING_LENGTH;
            pMetaData->fields[i].name = UA_STRING("String variable");
        } else if(i == 2) {
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_BYTESTRING].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_BYTESTRING;
            pMetaData->fields[i].maxStringLength =
                PUBSUB_CONFIG_MAX_STRING_LENGTH;
            pMetaData->fields[i].name = UA_STRING("ByteString variable");
        } else {
            UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                           &pMetaData->fields[i].dataType);
            pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
            pMetaData->fields[i].name = UA_STRING("UInt32 variable");
        }
        pMetaData->fields[i].valueRank = -1; /* scalar */
    }
}

/* Add new connection to the server */
static void
addPubSubConnection(UA_Server *server) {
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.id.uint32 = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
}

/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                             &readerGroupIdentifier);
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static void
addSubscribedVariables (UA_Server *server) {
    UA_NodeId folderId;
    UA_NodeId newnodeId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), folderBrowseName,
                            UA_NS0ID(BASEOBJECTTYPE), oAttr,
                            NULL, &folderId);

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    readerConfig.subscribedDataSet.target.targetVariablesSize = readerConfig.dataSetMetaData.fieldsSize;
    readerConfig.subscribedDataSet.target.targetVariables = (UA_FieldTargetDataType*)
        UA_calloc(readerConfig.subscribedDataSet.target.targetVariablesSize, sizeof(UA_FieldTargetDataType));

    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.valueRank = UA_VALUERANK_SCALAR;
        // Initialize the values at first to create the buffered NetworkMessage
        // with correct size and offsets
        UA_UInt32 intValue = 0;
        UA_String stringValue = UA_STRING_NULL;
        UA_ByteString byteStringValue = UA_BYTESTRING_NULL;
        if(i == 1) {
            vAttr.description = UA_LOCALIZEDTEXT("en-US", "Subscribed String");
            vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed String");
            vAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
            UA_Variant_setScalar(&vAttr.value, &stringValue,
                                 &UA_TYPES[UA_TYPES_STRING]);
        } else if(i == 2) {
            vAttr.description = UA_LOCALIZEDTEXT("en-US", "Subscribed ByteString");
            vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed ByteString");
            vAttr.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
            UA_Variant_setScalar(&vAttr.value, &byteStringValue,
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
        } else {
            vAttr.description = UA_LOCALIZEDTEXT("en-US", "Subscribed UInt32");
            vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed UInt32");
            vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
            UA_Variant_setScalar(&vAttr.value, &intValue,
                                 &UA_TYPES[UA_TYPES_UINT32]);
        }
        UA_StatusCode res = UA_Server_addVariableNode(
            server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + PUBSUB_SUBSCRIBER_NODEID_BASE),
            folderId, UA_NS0ID(HASCOMPONENT),
            UA_QUALIFIEDNAME(1, "Subscribed Variable"),
            UA_NS0ID(BASEDATAVARIABLETYPE), vAttr, NULL, &newnodeId);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

        UA_FieldTargetDataType *tv = &readerConfig.subscribedDataSet.target.targetVariables[i];
        tv->attributeId  = UA_ATTRIBUTEID_VALUE;
        tv->targetNodeId = newnodeId;
    }
}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID | UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER | UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    dataSetReaderMessage->dataSetMessageContentMask = UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;

    readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

    addSubscribedVariables(server);
    UA_StatusCode res =
        UA_Server_addDataSetReader(server, readerGroupIdentifier,
                                   &readerConfig, &readerIdentifier);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_free(readerConfig.subscribedDataSet.target.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}

START_TEST(SubscriberOffsets) {
    addPubSubConnection(server);
    addReaderGroup(server);
    addDataSetReader(server);

    UA_PubSubOffsetTable ot;
    UA_StatusCode res =
        UA_Server_computeDataSetReaderOffsetTable(server, readerIdentifier, &ot);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId targetNodeIds[PUBSUB_CONFIG_FIELD_COUNT] = {
        UA_NODEID_NUMERIC(1, PUBSUB_SUBSCRIBER_NODEID_BASE),
        UA_NODEID_NUMERIC(1, PUBSUB_SUBSCRIBER_NODEID_BASE + 1),
        UA_NODEID_NUMERIC(1, PUBSUB_SUBSCRIBER_NODEID_BASE + 2),
        UA_NODEID_NUMERIC(1, PUBSUB_SUBSCRIBER_NODEID_BASE + 3)
    };
    assertFixedStringOffsets(&ot, targetNodeIds, sizeof(UA_UInt32));

    UA_PubSubOffsetTable_clear(&ot);
} END_TEST

int main(void) {
    TCase *tc_offset = tcase_create("PubSub Offset");
    tcase_add_checked_fixture(tc_offset, setup, teardown);
    tcase_add_test(tc_offset, PublisherOffsets);
    tcase_add_test(tc_offset, SubscriberOffsets);

    Suite *s = suite_create("PubSub Offsets");
    suite_add_tcase(s, tc_offset);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
