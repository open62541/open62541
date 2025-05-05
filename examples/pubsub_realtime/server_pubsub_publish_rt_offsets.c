/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

static UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;

/* Values in static locations. We cycle the dvPointers double-pointer to the
 * next with atomic operations. */
UA_UInt32 valueStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue dvStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *dvPointers[PUBSUB_CONFIG_FIELD_COUNT];
UA_NodeId publishVariables[PUBSUB_CONFIG_FIELD_COUNT];

int main(void) {
    /* Prepare the values */
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        valueStore[i] = (UA_UInt32) i + 1;
        UA_Variant_setScalar(&dvStore[i].value, &valueStore[i], &UA_TYPES[UA_TYPES_UINT32]);
        dvStore[i].hasValue = true;
        dvPointers[i] = &dvStore[i];
    }

    /* Initialize the server */
    UA_Server *server = UA_Server_new();

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
        /* Create the variable */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                  UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &publishVariables[i]);

        /* Set the value backend of the above create node to 'external value source' */
        UA_ValueBackend valueBackend;
        memset(&valueBackend, 0, sizeof(UA_ValueBackend));
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &dvPointers[i];
        UA_Server_setVariableNode_valueBackend(server, publishVariables[i], valueBackend);

        /* Add the DataSetField */
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        dsfConfig.field.variable.publishParameters.publishedVariable = publishVariables[i];
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
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
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION |
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

    /* Print the Offset Table */
    UA_PubSubOffsetTable ot;
    UA_Server_computeWriterGroupOffsetTable(server, writerGroupIdent, &ot);
    for(size_t i = 0; i < ot.offsetsSize; i++) {
        UA_String out = UA_STRING_NULL;
        if(ot.offsets[i].offsetType >= UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE) {
            /* For writers the component is the NodeId is the DataSetField.
             * Instead print the source node that contains the data */
            UA_DataSetFieldConfig dsfc;
            UA_Server_getDataSetFieldConfig(server, ot.offsets[i].component, &dsfc);
            UA_NodeId_print(&dsfc.field.variable.publishParameters.publishedVariable, &out);
            UA_DataSetFieldConfig_clear(&dsfc);
        } else {
            UA_NodeId_print(&ot.offsets[i].component, &out);
        }
        printf("%u:\tOffset %u\tOffsetType %u\tComponent %.*s\n",
               (unsigned)i, (unsigned)ot.offsets[i].offset,
               (unsigned)ot.offsets[i].offsetType,
               (int)out.length, out.data);
        UA_String_clear(&out);
    }

    /* Cleanup */
    UA_PubSubOffsetTable_clear(&ot);
    UA_Server_delete(server);
    return EXIT_SUCCESS;
}

