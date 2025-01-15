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

#include <stdio.h>
#include <check.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

UA_Server *server;
UA_DataSetReaderConfig readerConfig;
static UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;
static UA_NodeId readerGroupIdentifier, readerIdentifier;

static UA_NetworkAddressUrlDataType networkAddressUrl =
    {{0, NULL}, UA_STRING_STATIC("opc.udp://224.0.0.22:4840/")};
static UA_String transportProfile =
    UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

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

    /* Add DataSetFields with static value source to PDS */
    UA_DataSetFieldConfig dsfConfig;
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        /* TODO: Point to a variable in the information model */
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
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

    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);

    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);

    /* Print the Offset Table */
    UA_PubSubOffsetTable ot;
    UA_Server_computeWriterGroupOffsetTable(server, writerGroupIdent, &ot);
    for(size_t i = 0; i < ot.offsetsSize; i++) {
        UA_String out = UA_STRING_NULL;
        UA_NodeId_print(&ot.offsets[i].component, &out);
        printf("%u:\tOffset %u\tOffsetType %u\tComponent %.*s\n",
               (unsigned)i, (unsigned)ot.offsets[i].offset,
               (unsigned)ot.offsets[i].offsetType,
               (int)out.length, out.data);
        UA_String_clear(&out);
    }

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
        /* UInt32 DataType */
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                       &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[i].name =  UA_STRING ("UInt32 varibale");
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
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
        // Initialize the values at first to create the buffered NetworkMessage
        // with correct size and offsets
        UA_Variant value;
        UA_Variant_init(&value);
        UA_UInt32 intValue = 0;
        UA_Variant_setScalar(&value, &intValue, &UA_TYPES[UA_TYPES_UINT32]);
        vAttr.value = value;
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                  folderId, UA_NS0ID(HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &newnodeId);

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
    readerConfig.expectedEncoding = UA_PUBSUB_RT_RAW;
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
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig, &readerIdentifier);

    UA_free(readerConfig.subscribedDataSet.target.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}

START_TEST(SubscriberOffsets) {
    addPubSubConnection(server);
    addReaderGroup(server);
    addDataSetReader(server);

    /* Print the Offset Table */
    UA_PubSubOffsetTable ot;
    UA_Server_computeReaderGroupOffsetTable(server, readerGroupIdentifier, &ot);
    for(size_t i = 0; i < ot.offsetsSize; i++) {
        UA_String out = UA_STRING_NULL;
        UA_NodeId_print(&ot.offsets[i].component, &out);
        printf("%u:\tOffset %u\tOffsetType %u\tComponent %.*s\n",
               (unsigned)i, (unsigned)ot.offsets[i].offset,
               (unsigned)ot.offsets[i].offsetType,
               (int)out.length, out.data);
        UA_String_clear(&out);
    }

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

