/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/log_stdout.h>
#include "../common.h"


#include <stdio.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 1

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

void addAxisVariable(UA_Server *server, const char *name, UA_UInt16 namespaceIndex, UA_UInt32 nodeIdValue) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("de-DE", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT16].typeId; // Built-in Type Int16
    attr.valueRank = 1; // Array
    UA_UInt32 arrayDimensions[1] = {76};
    attr.arrayDimensions = arrayDimensions;
    attr.arrayDimensionsSize = 1;

    // Wert initialisieren
    UA_Int16 initialValue[76] = {0}; // Beispielwerte
    UA_Variant_setArray(&attr.value, initialValue, 76, &UA_TYPES[UA_TYPES_INT16]);

    // Variable in den Namespace einfügen
    UA_NodeId newNodeId = UA_NODEID_NUMERIC(namespaceIndex, nodeIdValue);
    UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(namespaceIndex, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_StatusCode status = UA_Server_addVariableNode(
        server, newNodeId, parentNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        qualifiedName, variableTypeNodeId, attr, NULL, NULL);

    if (status == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Variable %s erfolgreich erstellt.", name);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Fehler beim Erstellen der Variable %s.", name);
    }
}


int main(int argc, char** argv) {
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
    UA_Server_addNamespace(server, "a");
    UA_Server_addNamespace(server, "b");
    UA_Server_addNamespace(server, "c");
    UA_Server_addNamespace(server, "d");
    UA_Server_addNamespace(server, "e");
    UA_Server_addNamespace(server, "f");
    UA_Server_addNamespace(server, "g");

    pConfig->pubSubConfig.componentLifecycleCallback = componentLifecycleCallback;
    pConfig->pubSubConfig.beforeStateChangeCallback = beforeStateChangeCallback;
    pConfig->pubSubConfig.stateChangeCallback = stateChangeCallback;
    addAxisVariable(server, "Axis1", 7, 6127);
    addAxisVariable(server, "Axis2", 7, 6128);
    addAxisVariable(server, "Axis3", 7, 6129);
    addAxisVariable(server, "Axis4", 7, 6135);


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


    /* file based config */
    UA_ByteString configuration = loadFile(argv[1]);
    UA_Server_loadPubSubConfigFromByteString(server, configuration);


    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    statusCode |= UA_Server_enableAllPubSubComponents(server);

    /* Print the Offset Table */
    UA_PubSubOffsetTable ot;
    memset(&ot, 0, sizeof(UA_PubSubOffsetTable));
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

