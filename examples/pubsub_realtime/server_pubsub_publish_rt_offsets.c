/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>


#include <stdio.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

static UA_NodeId publishedDataSetIdent, dataSetFieldIdent, dataSetFieldIdent2, writerGroupIdent, connectionIdentifier;
static UA_UInt32 ds2UInt32ArrValue[10] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };
static UA_NodeId ds2UInt32ArrId;

/* Values in static locations. We cycle the dvPointers double-pointer to the
 * next with atomic operations. */
UA_UInt32 valueStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue dvStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *dvPointers[PUBSUB_CONFIG_FIELD_COUNT];
UA_NodeId publishVariables[PUBSUB_CONFIG_FIELD_COUNT];

static UA_NodeId writerGroupIdent;

#define CONTENTMASK_UAPERIODIC_FIXED   (                                                             \
UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |            \
UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |            \
UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |          \
UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION |           \
UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER |   \
UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER           \
)


static bool checkNetworkMessageContentMask(UA_WriterGroupConfig * pWgConfig, UA_UadpNetworkMessageContentMask contentMask)
{
    bool isSet = false;

    if(pWgConfig->messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED)
    {
        if(pWgConfig->messageSettings.content.decoded.type == &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        {
            UA_UadpWriterGroupMessageDataType *msgSettings = (UA_UadpWriterGroupMessageDataType*)pWgConfig->messageSettings.content.decoded.data;
            if(msgSettings->networkMessageContentMask == CONTENTMASK_UAPERIODIC_FIXED)
            {
                isSet = true;
            }
        }
    }

    return isSet;
}

static UA_StatusCode connectionStateMachine(UA_Server *server, const UA_NodeId componentId, void *componentContext, UA_PubSubState *state, UA_PubSubState targetState) {
    UA_PubSubConnectionConfig confConn;

    if(*state == targetState)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_PUBSUBCOMPONENT_CONNECTION: custom state machine called '%lu'", componentId.identifier.numeric);

    *state = targetState;

    if(UA_Server_getPubSubConnectionConfig(server, componentId, &confConn) == UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionConfig_clear(&confConn);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writerGroupStateMachine(UA_Server *server, const UA_NodeId componentId, void *componentContext, UA_PubSubState *state, UA_PubSubState targetState) {
    if(*state == targetState)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_PUBSUBCOMPONENT_WRITERGROUP: custom state machine called '%lu'", componentId.identifier.numeric);

    switch(targetState) {
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            *state = targetState;
            break;

        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(*state == UA_PUBSUBSTATE_OPERATIONAL)
                break;

            UA_WriterGroupConfig wgConn;
            if(UA_Server_getWriterGroupConfig(server, componentId, &wgConn) == UA_STATUSCODE_GOOD) {
                if(checkNetworkMessageContentMask(&wgConn, CONTENTMASK_UAPERIODIC_FIXED)) {
                    *state = targetState;
                }
                UA_WriterGroupConfig_clear(&wgConn);
            }
            break;

        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode dataSetWriterStateMachine(UA_Server *server, const UA_NodeId componentId, void *componentContext, UA_PubSubState *state, UA_PubSubState targetState) {
    if(*state == targetState)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_PUBSUBCOMPONENT_DATASETWRITER: custom state machine called '%lu'", componentId.identifier.numeric);

    *state = targetState;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readerGroupStateMachine(UA_Server *server, const UA_NodeId componentId, void *componentContext, UA_PubSubState *state, UA_PubSubState targetState) {
    if(*state == targetState)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_PUBSUBCOMPONENT_READERGROUP: custom state machine called '%lu'", componentId.identifier.numeric);

    switch(targetState) {
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            *state = targetState;
            break;

        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(*state == UA_PUBSUBSTATE_OPERATIONAL)
                break;

            *state = targetState;
            break;

        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode dataSetReaderStateMachine(UA_Server *server, const UA_NodeId componentId, void *componentContext, UA_PubSubState *state, UA_PubSubState targetState) {
    if(*state == targetState)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_PUBSUBCOMPONENT_DATASETREADER: custom state machine called '%lu'", componentId.identifier.numeric);

    *state = targetState;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode componentLifecycleCallback(UA_Server *server, const UA_NodeId id, const UA_PubSubComponentType componentType, UA_Boolean remove) {
    if(remove) {
        return UA_STATUSCODE_GOOD;
    }

    switch(componentType) {
        case UA_PUBSUBCOMPONENT_CONNECTION:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_CONNECTION");
            UA_PubSubConnectionConfig confConn;
            if(UA_Server_getPubSubConnectionConfig(server, id, &confConn) == UA_STATUSCODE_GOOD) {
                confConn.customStateMachine = connectionStateMachine;
                UA_Server_updatePubSubConnectionConfig(server, id, &confConn);
                UA_PubSubConnectionConfig_clear(&confConn);
            }
            break;
        case UA_PUBSUBCOMPONENT_WRITERGROUP:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_WRITERGROUP");
            UA_WriterGroupConfig confWrGrp;
            if(UA_Server_getWriterGroupConfig(server, id, &confWrGrp) == UA_STATUSCODE_GOOD) {
                writerGroupIdent = id;
                confWrGrp.customStateMachine = writerGroupStateMachine;
                UA_Server_updateWriterGroupConfig(server, id, &confWrGrp);
                UA_WriterGroupConfig_clear(&confWrGrp);
            }
            break;
        case UA_PUBSUBCOMPONENT_DATASETWRITER:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_DATASETWRITER");
            UA_DataSetWriterConfig confDsWr;
            if(UA_Server_getDataSetWriterConfig(server, id, &confDsWr) == UA_STATUSCODE_GOOD) {
                confDsWr.customStateMachine = dataSetWriterStateMachine;
                UA_Server_updateDataSetWriterConfig(server, id, &confDsWr);
                UA_DataSetWriterConfig_clear(&confDsWr);
            }
            break;
        case UA_PUBSUBCOMPONENT_READERGROUP:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_READERGROUP");
            UA_ReaderGroupConfig confRdGrp;
            if(UA_Server_getReaderGroupConfig(server, id, &confRdGrp) == UA_STATUSCODE_GOOD) {
                confRdGrp.customStateMachine = readerGroupStateMachine;
                UA_Server_updateReaderGroupConfig(server, id, &confRdGrp);
                UA_ReaderGroupConfig_clear(&confRdGrp);
            }
            break;
        case UA_PUBSUBCOMPONENT_DATASETREADER:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_DATASETREADER");
            UA_DataSetReaderConfig confDsRd;
            if(UA_Server_getDataSetReaderConfig(server, id, &confDsRd) == UA_STATUSCODE_GOOD) {
                confDsRd.customStateMachine = dataSetReaderStateMachine;
                UA_Server_updateDataSetReaderConfig(server, id, &confDsRd);
                UA_DataSetReaderConfig_clear(&confDsRd);
            }
            break;
        case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_PUBLISHEDDATASET");
            break;
        case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "componentLifecycleCallback: UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET");
            break;
        default:
            break;
    }

    return UA_STATUSCODE_GOOD;
}

static void beforeStateChangeCallback(UA_Server *server, const UA_NodeId id, UA_PubSubState *targetState) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "State of the PubSubComponent '%lu' will change to '%i'", id.identifier.numeric, *targetState);
}

static void stateChangeCallback(UA_Server *server, const UA_NodeId id, UA_PubSubState state, UA_StatusCode status) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGLEVEL_INFO, "State of the PubSubComponent '%lu' changed to '%i' with status '%s'", id.identifier.numeric, state, UA_StatusCode_name(status));
}


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
    UA_ServerConfig *pConfig = UA_Server_getConfig(server);
    pConfig->pubSubConfig.componentLifecycleCallback = componentLifecycleCallback;
    pConfig->pubSubConfig.beforeStateChangeCallback = beforeStateChangeCallback;
    pConfig->pubSubConfig.stateChangeCallback = stateChangeCallback;

    // UInt32Array
    UA_NodeId_init(&ds2UInt32ArrId);
    UA_VariableAttributes uint32ArrAttr = UA_VariableAttributes_default;
    uint32ArrAttr.valueRank = 1;    // 1-dimensional array
    uint32ArrAttr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDims[1] = { 10 };
    uint32ArrAttr.arrayDimensions = arrayDims;

    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &uint32ArrAttr.dataType);
    uint32ArrAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;

    UA_Variant_setArray(&uint32ArrAttr.value, ds2UInt32ArrValue, 10, &UA_TYPES[UA_TYPES_UINT32]);
    uint32ArrAttr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32Array");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.UInt32Array"), UA_NS0ID(OBJECTSFOLDER),
                              UA_NS0ID(HASCOMPONENT), UA_QUALIFIEDNAME(1, "UInt32Array"),
                              UA_NS0ID(BASEDATAVARIABLETYPE), uint32ArrAttr, NULL, &ds2UInt32ArrId);

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

    UA_DataSetFieldConfig uint32ArrConfig;
    memset(&uint32ArrConfig, 0, sizeof(UA_DataSetFieldConfig));
    uint32ArrConfig.field.variable.fieldNameAlias = UA_STRING("UInt32Array");
    uint32ArrConfig.field.variable.promotedField = false;
    uint32ArrConfig.field.variable.publishParameters.publishedVariable = ds2UInt32ArrId;
    uint32ArrConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &uint32ArrConfig, &dataSetFieldIdent2);

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
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server_computeWriterGroupOffsetTable(server, writerGroupIdent, &ot);
for(size_t i = 0; i < ot.offsetsSize; i++) {
    UA_String out = UA_STRING_NULL;
    if(ot.offsets[i].offsetType >= UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE) {
        /* For writers the component is the NodeId is the DataSetField.
         * Instead print the source node that contains the data */
        UA_DataSetFieldConfig dsfc;
        retval = UA_Server_getDataSetFieldConfig(server, ot.offsets[i].component, &dsfc);
        if(retval != UA_STATUSCODE_GOOD)
            continue;
        UA_NodeId_print(&dsfc.field.variable.publishParameters.publishedVariable, &out);
        UA_DataSetFieldConfig_clear(&dsfc);
    } else {
        UA_NodeId_print(&ot.offsets[i].component, &out);
    }

    // Map OffsetType to its string representation
    const char *offsetTypeString = NULL;
    switch(ot.offsets[i].offsetType) {
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_GROUPVERSION:
            offsetTypeString = "NETWORKMESSAGE_GROUPVERSION";
            break;
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
            offsetTypeString = "NETWORKMESSAGE_SEQUENCENUMBER";
            break;
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_TIMESTAMP:
            offsetTypeString = "NETWORKMESSAGE_TIMESTAMP";
            break;
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_PICOSECONDS:
            offsetTypeString = "NETWORKMESSAGE_PICOSECONDS";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE:
            offsetTypeString = "DATASETMESSAGE";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
            offsetTypeString = "DATASETMESSAGE_SEQUENCENUMBER";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS:
            offsetTypeString = "DATASETMESSAGE_STATUS";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP:
            offsetTypeString = "DATASETMESSAGE_TIMESTAMP";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS:
            offsetTypeString = "DATASETMESSAGE_PICOSECONDS";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE:
            offsetTypeString = "DATASETFIELD_DATAVALUE";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT:
            offsetTypeString = "DATASETFIELD_VARIANT";
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW:
            offsetTypeString = "DATASETFIELD_RAW";
            break;
        default:
            offsetTypeString = "UNKNOWN_OFFSET_TYPE";
            break;
    }

    // Print the details including the string representation of OffsetType
    printf("%u:\tOffset %u\tOffsetType %u (%s)\tComponent %.*s\n",
           (unsigned)i, (unsigned)ot.offsets[i].offset,
           (unsigned)ot.offsets[i].offsetType, offsetTypeString,
           (int)out.length, out.data);

    UA_String_clear(&out);
    }



    UA_Server_runUntilInterrupt(server);

    /* Cleanup */
    UA_PubSubOffsetTable_clear(&ot);
    UA_Server_delete(server);
    return EXIT_SUCCESS;
}

