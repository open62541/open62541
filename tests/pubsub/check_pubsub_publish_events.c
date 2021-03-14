/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Stefan Joachim Hahn, Technische Hochschule Mittelhessen
 * Copyright (c) 2021 Florian Fischer, Technische Hochschule Mittelhessen
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "open62541/types_generated_encoding_binary.h"

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;
UA_NodeId eventNodeId, eventType,
    connection, writerGroup1, dataSetWriter1, publishedDataSet1, dataSetField1;
UA_WriterGroupConfig writerGroupConfig;

static UA_StatusCode addNewEventType(UA_Server *s);

static void receiveSingleMessage(UA_ByteString buffer, UA_PubSubConnection *conn, UA_NetworkMessage *networkMessage) {
    if (UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        ck_abort_msg("Message buffer allocation failed!");
    }
    UA_StatusCode retval =
        conn->channel->receive(conn->channel, &buffer, NULL, 10000);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        buffer.length = 512;
        UA_ByteString_clear(&buffer);
        ck_abort_msg("Expected message not received!");
    }
    memset(networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, networkMessage);
    for(int i = 0; i < networkMessage->payloadHeader.dataSetPayloadHeader.count; ++i) {
        UA_Byte * rawContent = (UA_Byte *) UA_malloc(networkMessage->payload.dataSetPayload.dataSetMessages[i].data.keyFrameData.rawFields.length);
        memcpy(rawContent,
               networkMessage->payload.dataSetPayload.dataSetMessages[i].data.keyFrameData.rawFields.data,
               networkMessage->payload.dataSetPayload.dataSetMessages[i].data.keyFrameData.rawFields.length);
        networkMessage->payload.dataSetPayload.dataSetMessages[i].data.keyFrameData.rawFields.data = rawContent;
    }
    UA_ByteString_clear(&buffer);
}

static UA_StatusCode
setUpEvent(UA_NodeId *outId, UA_Server *s) {
    UA_Server_createEvent(s, eventType, outId);

    // Set the Event Attributes
    // Setting the Time is required or else the event will not show up in UAExpert!
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(s, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_UInt16 eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(s, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Server_writeObjectProperty_scalar(s, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_String eventSourceName = UA_STRING("Server");
    UA_Server_writeObjectProperty_scalar(s, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addNewEventType(UA_Server *s) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ExampleEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "The simple event type we created");
    return UA_Server_addObjectTypeNode(
        s, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                       UA_QUALIFIEDNAME(0, "PublishEventsExampleEventType"),
                                       attr, NULL, &eventType);
}

static void
addPubSubConnection(UA_Server *s, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("Demo Connection PubSub Events");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    /* Changed to static publisherId from random generation to identify
     * the publisher on Subscriber side */
    connectionConfig.publisherId.numeric = 2234;
    UA_Server_addPubSubConnection(s, &connectionConfig, &connection);
}

static void
addPublishedDataSet(UA_Server *s) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS PubSub Events");
    publishedDataSetConfig.config.event.eventNotifier = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    publishedDataSetConfig.config.event.selectedFieldsSize = 2;
    UA_SimpleAttributeOperand *selectedFields = (UA_SimpleAttributeOperand *)UA_calloc(2, sizeof(UA_SimpleAttributeOperand));
    selectedFields[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectedFields[0].browsePathSize = 1;
    selectedFields[0].browsePath = (UA_QualifiedName*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectedFields[0].browsePath) {
        UA_SimpleAttributeOperand_delete(&selectedFields[0]);
    }
    selectedFields[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectedFields[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");

    selectedFields[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectedFields[1].browsePathSize = 1;
    selectedFields[1].browsePath = (UA_QualifiedName*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(!selectedFields[1].browsePath) {
        UA_SimpleAttributeOperand_delete(&selectedFields[1]);
    }
    selectedFields[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectedFields[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    publishedDataSetConfig.config.event.selectedFields = selectedFields;
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(s, &publishedDataSetConfig, &publishedDataSet1);
}

static void
addDataSetField(UA_Server *s) {
    //Add a field to the previous created PublishedDataSet
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_EVENT;
    dataSetFieldConfig.field.events.fieldNameAlias = UA_STRING("Message");
    UA_SimpleAttributeOperand *sf = UA_SimpleAttributeOperand_new();
    UA_SimpleAttributeOperand_init(sf);
    sf->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    sf->browsePathSize = 1;
    sf->browsePath = (UA_QualifiedName *)
        UA_Array_new(sf->browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    sf->attributeId = UA_ATTRIBUTEID_VALUE;
    sf->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    dataSetFieldConfig.field.events.selectedField = *sf;

    UA_Server_addDataSetField(s, publishedDataSet1,
                              &dataSetFieldConfig, &dataSetField1);
}

static void
addWriterGroup(UA_Server *s) {
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup PubSub Events");
    writerGroupConfig.publishingInterval = 1000;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                                                (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    UA_Server_addWriterGroup(s, connection, &writerGroupConfig, &writerGroup1);
    UA_Server_setWriterGroupOperational(s, writerGroup1);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

static void
addDataSetWriter(UA_Server *s) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DSW PubSub Events");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(s, writerGroup1, publishedDataSet1,
                               &dataSetWriterConfig, &dataSetWriter1);
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)
        UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    //add 2 connections
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    addNewEventType(server);
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo");
    UA_Server_addObjectNode(
        server, UA_NODEID_NUMERIC(1, 50042),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Demo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);


    addPubSubConnection(server, &connectionConfig.transportProfileUri, &networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/*************************************************************
 *         Adding of Values to Queue in DataSetWriter
 *************************************************************/
START_TEST(IrrelevantEventsAreNotAdded){
    UA_DataSetWriter *dsw = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(1, 50042),
                           NULL, UA_TRUE);
    ck_assert_int_eq(dsw->eventQueueEntries,1);
    } END_TEST

START_TEST(AddSelectedFieldsOfTriggeredEventToDataSetWriter){
    UA_DataSetWriter *dsw = UA_DataSetWriter_findDSWbyId(server, dataSetWriter1);
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    ck_assert_int_eq(dsw->eventQueueEntries,1);
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    ck_assert_int_eq(dsw->eventQueueEntries,2);
    } END_TEST

/**************************************
 *         Publishing of Events
 **************************************/
START_TEST(PublishingOfOneDataSetField){
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    writerGroupConfig.publishingInterval = 10000;
    UA_Server_updateWriterGroupConfig(server, writerGroup1, &writerGroupConfig);
    UA_ByteString buffer = UA_BYTESTRING("");
    UA_NetworkMessage networkMessage;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connection);
    receiveSingleMessage(buffer, conn, &networkMessage);
    ck_assert_int_eq(networkMessage.payloadHeader.dataSetPayloadHeader.count,1);
    ck_assert(networkMessage.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME);
    ck_assert_int_eq(networkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount,1);
    ck_assert(UA_NodeId_equal(&networkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.dataSetFields[0].value.type->typeId,&UA_TYPES[UA_TYPES_LOCALIZEDTEXT].typeId));
    } END_TEST

START_TEST(PublishingOfTwoTriggeredEvents){
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    setUpEvent(&eventNodeId, server);
    UA_Server_triggerEvent(server, eventNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                           NULL, UA_TRUE);
    writerGroupConfig.publishingInterval = 10000;
    UA_Server_updateWriterGroupConfig(server, writerGroup1, &writerGroupConfig);
    UA_ByteString buffer = UA_BYTESTRING("");
    UA_NetworkMessage networkMessage;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connection);
    receiveSingleMessage(buffer, conn, &networkMessage);
    ck_assert_int_eq(networkMessage.payloadHeader.dataSetPayloadHeader.count,1);
    ck_assert(networkMessage.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME);
    ck_assert_int_eq(networkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount,2);
    }END_TEST

START_TEST(MessageShouldBeEmptyIfNoEventWasTriggered){
    writerGroupConfig.publishingInterval = 10000;
    UA_Server_updateWriterGroupConfig(server, writerGroup1, &writerGroupConfig);
    UA_ByteString buffer = UA_BYTESTRING("");
    UA_NetworkMessage networkMessage;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connection);
    receiveSingleMessage(buffer, conn, &networkMessage);
    ck_assert_int_eq(networkMessage.payloadHeader.dataSetPayloadHeader.count,1);
    ck_assert(networkMessage.payload.dataSetPayload.dataSetMessages[0].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME);
    ck_assert_int_eq(networkMessage.payload.dataSetPayload.dataSetMessages[0].data.keyFrameData.fieldCount,0);
    }END_TEST

int main(void) {
    TCase *tc_adding_to_queue = tcase_create("Adding of Values to Queue in DataSetWriter");
    tcase_add_checked_fixture(tc_adding_to_queue, setup, teardown);
    tcase_add_test(tc_adding_to_queue, AddSelectedFieldsOfTriggeredEventToDataSetWriter);
    tcase_add_test(tc_adding_to_queue, IrrelevantEventsAreNotAdded);

    TCase *tc_pubsub_events = tcase_create("Publishing of events");
    tcase_add_checked_fixture(tc_pubsub_events, setup, teardown);
    tcase_add_test(tc_pubsub_events, PublishingOfOneDataSetField);
    tcase_add_test(tc_pubsub_events, PublishingOfTwoTriggeredEvents);
    tcase_add_test(tc_pubsub_events, MessageShouldBeEmptyIfNoEventWasTriggered);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_adding_to_queue);
    suite_add_tcase(s, tc_pubsub_events);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
