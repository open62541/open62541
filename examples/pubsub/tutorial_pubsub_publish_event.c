/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2021 Stefan Joachim Hahn, Technische Hochschule Mittelhessen
 * Copyright (c) 2021 Florian Fischer, Technische Hochschule Mittelhessen
 */

/*
 * In this tutorial, an custom event gets triggered cyclically. Once it's triggered, it
 * gets published the next time the publishing interval expires.
 *
 * Therefore you need to create a PublishedDataSet of PublishedDataSetType UA_PUBSUB_DATASET_PUBLISHEDEVENTS
 * and set the EventNotifier to a node (e.g. the server node), whose events you would like to publish.
 * It would also be good to define some selectedFields for the PublishedDataSet, in this case, the
 * selected Fields are describing the information of an event.
 *
 * After creating the PublishedDataSet, you can add some DataSetFields to it. Make sure, they are of the type
 * UA_PUBSUB_DATASETFIELD_EVENT and get a selectedField.
 *
 * The rest of the config is equal to the tutorial_pubsub_publish.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541_queue.h>
#include <pubsub/ua_pubsub.h>
#include <pubsub/ua_pubsub_ns0.h>
#include <signal.h>
#include <stdlib.h>

UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;
static UA_NodeId eventType;
static UA_NodeId eventType2;

/*********************** PubSub Methods ***********************/

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
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
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */

    size_t selectedFieldSize = 2;
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS PubSub Events");
    publishedDataSetConfig.config.event.eventNotifier = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    publishedDataSetConfig.config.event.selectedFieldsSize = selectedFieldSize;

    UA_SimpleAttributeOperand selectedFields [selectedFieldSize];
    for(size_t i = 0; i < selectedFieldSize; ++i) {
        UA_SimpleAttributeOperand_init(&selectedFields[i]);
    }

    selectedFields[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectedFields[0].attributeId = UA_ATTRIBUTEID_VALUE;
    selectedFields[0].browsePathSize = 1;
    UA_QualifiedName browsePathSeverity[1];
    browsePathSeverity[0] = UA_QUALIFIEDNAME(0, "Severity");
    selectedFields[0].browsePath = browsePathSeverity;

    selectedFields[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    selectedFields[1].attributeId = UA_ATTRIBUTEID_VALUE;
    selectedFields[1].browsePathSize = 1;
    UA_QualifiedName browsePathMessage[1];
    browsePathMessage[0] = UA_QUALIFIEDNAME(0, "Message");
    selectedFields[1].browsePath = browsePathMessage;

    publishedDataSetConfig.config.event.selectedFields = selectedFields;

    /* Adds a ContentFilter to the PDS*/
    UA_ContentFilter contentFilter;
    UA_ContentFilter_init(&contentFilter);
    UA_ContentFilterElement contentFilterElement;
    UA_ContentFilterElement_init(&contentFilterElement);
    UA_ExtensionObject filterOperandExObj;
    UA_ExtensionObject_init(&filterOperandExObj);
    UA_LiteralOperand literalOperand;
    UA_LiteralOperand_init(&literalOperand);

    contentFilter.elementsSize = 1;
    contentFilter.elements = &contentFilterElement;

    contentFilterElement.filterOperandsSize = 1;
    contentFilterElement.filterOperands = &filterOperandExObj;
    contentFilterElement.filterOperator = UA_FILTEROPERATOR_OFTYPE;

    filterOperandExObj.encoding             = UA_EXTENSIONOBJECT_DECODED;
    filterOperandExObj.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    filterOperandExObj.content.decoded.data = &literalOperand;

    UA_Variant_setScalar(&literalOperand.value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);

    publishedDataSetConfig.config.event.filter = contentFilter;

    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}


static void addDataSetFields(UA_Server *server){
    /*Creates DataSetFields based on the PDS selectedFields*/
    UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, publishedDataSetIdent);
    for(size_t i = 0; i < publishedDataSet->config.config.event.selectedFieldsSize; i++){
        UA_SimpleAttributeOperand sao = publishedDataSet->config.config.event.selectedFields[i];
        UA_String name = sao.browsePath[0].name;

        UA_NodeId dataSetFieldIdent;
        UA_DataSetFieldConfig dataSetFieldConfig;
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_EVENT;
        dataSetFieldConfig.field.events.fieldNameAlias = name;
        dataSetFieldConfig.field.events.selectedField = sao;

        UA_Server_addDataSetField(server, publishedDataSetIdent,
                                  &dataSetFieldConfig, &dataSetFieldIdent);
    }
}

static void
addWriterGroup(UA_Server *server) {
    // adds a writerGroup to the connection
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup PubSub Events");
    writerGroupConfig.publishingInterval = 100;
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
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

static void
addDataSetWriter(UA_Server *server) {
    // adds a DataSetwriter to the WriterGroup
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DSW PubSub Events");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

/*********************** Event Methods ***********************/
void callEvent(UA_Server *server, void *data);

static UA_StatusCode
setUpEvent(UA_Server *server, UA_NodeId *outId) {
    UA_StatusCode retval = UA_Server_createEvent(server, eventType, outId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                       "createEvent failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    // Set the Event Attributes
    // Setting the Time is required or else the event will not show up in UAExpert!
    UA_DateTime eventTime = UA_DateTime_now();
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Time"),
                                         &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);

    UA_UInt16 eventSeverity = 100;
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Severity"),
                                         &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    UA_LocalizedText eventMessage = UA_LOCALIZEDTEXT("en-US", "An event has been generated.");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "Message"),
                                         &eventMessage, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_String eventSourceName = UA_STRING("Server");
    UA_Server_writeObjectProperty_scalar(server, *outId, UA_QUALIFIEDNAME(0, "SourceName"),
                                         &eventSourceName, &UA_TYPES[UA_TYPES_STRING]);
    return UA_STATUSCODE_GOOD;
}

void
callEvent(UA_Server *server, void *data) {
    // Is called cyclically to fill the queue of the DataSetWriter
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Trigger event");

    UA_NodeId eventNodeId;
    setUpEvent(server, &eventNodeId);
    UA_Server_triggerEvent(server, eventNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                    NULL, UA_TRUE);

}


static UA_StatusCode
addNewEventType(UA_Server *server) {
    // creates the event, which gets published
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "PublishEventsExampleEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "The simple event type we created");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                       UA_QUALIFIEDNAME(0, "PublishEventsExampleEventType"),
                                       attr, NULL, &eventType);

    UA_ObjectTypeAttributes attr1 = UA_ObjectTypeAttributes_default;
    attr1.displayName = UA_LOCALIZEDTEXT("en-US", "PublishEventsExampleEventType2");
    attr1.description = UA_LOCALIZEDTEXT("en-US", "A simple event type we created to test the ContentFilter");
    return UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                       UA_QUALIFIEDNAME(0, "PublishEventsExampleEventType2"),
                                       attr1, NULL, &eventType2);
}

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int run(UA_String *transportProfile,
               UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
    config->pubsubTransportLayers =
        (UA_PubSubTransportLayer *) UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    addNewEventType(server);

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetFields(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    const UA_Double interval = (UA_Double)5000;
    UA_String name;
    UA_String_init(&name);
    UA_UInt64 identifier;
    UA_UInt64_init(&identifier);
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)callEvent, &name, interval, &identifier);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char *argv[]){
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
