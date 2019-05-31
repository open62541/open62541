/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.

 * Copyright (c) 2019 Kalycito Infotech Private Limited */
/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress:
 * This Tutorial will be continuously extended during the next PubSub batches. More details about
 * the PubSub extension and corresponding open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub mqtt publish subscribe example demonstrate the simplest way to publish
 * informations from the information model over MQTT using
 * the UADP (or later JSON) encoding.
 * To receive information the subscribe functionality of mqtt is used.
 * A periodical call to yield is necessary to update the mqtt stack.
 *
 * **Connection handling**
 * PubSubConnections can be created and deleted on runtime. More details about the system preconfiguration and
 * connection can be found in ``tutorial_pubsub_connection.c``.
 */

#include "open62541/server.h"
#include "open62541/server_config_default.h"
#include "ua_pubsub.h"
#include "ua_network_pubsub_mqtt.h"
#include "open62541/plugin/log_stdout.h"
#include <signal.h>

static UA_Boolean useJson = false;
static UA_NodeId connectionIdent;
static UA_NodeId publishedDataSetIdent;
static UA_NodeId writerGroupIdent;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);
static void initMetadata(int MetaDataCounter, UA_String strTmp, UA_DataSetMetaDataType* pMetaData,
                         int MetaDataType, char* MetaDataName, UA_Byte MetaDataNameSpaceId );

static void
addPubSubConnection(UA_Server *server, char *addressUrl) {
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("MQTT Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    connectionConfig.enabled = UA_TRUE;

    /* configure address of the mqtt broker (local on default port) */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING(addressUrl)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();

    /* configure options, set mqtt client id */
    UA_KeyValuePair connectionOptions[1];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "mqttClientId");
    UA_String mqttClientId = UA_STRING("TESTCLIENTPUBSUBMQTT");
    UA_Variant_setScalar(&connectionOptions[0].value, &mqttClientId, &UA_TYPES[UA_TYPES_STRING]);
    connectionConfig.connectionProperties = connectionOptions;
    connectionConfig.connectionPropertiesSize = 1;

    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {

    if(server == NULL) {
        return;
    }
    else {
        UA_ReaderGroupConfig readerGroupConfig;
        memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
        readerGroupConfig.name = UA_STRING("ReaderGroup1");
        UA_Server_addReaderGroup(server, connectionIdent, &readerGroupConfig,
                                        &readerGroupIdentifier);
    }
}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {

    if(server == NULL) {
        return;
    }
    else {
        memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
        readerConfig.name = UA_STRING("DataSet Reader 1");
        readerConfig.dataSetWriterId = 1;

        /* Setting up Meta data configuration in DataSetReader */
        fillTestDataSetMetaData(&readerConfig.dataSetMetaData);
        UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                          &readerIdentifier);
    }
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static void addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {

    if(server == NULL) {
        return;
    }
    else {
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

        UA_Server_addObjectNode (server, UA_NODEID_NULL,
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                                 UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                                 folderBrowseName, UA_NODEID_NUMERIC (0,
                                 UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

        UA_Server_DataSetReader_addTargetVariables (server, &folderId,
                                                    dataSetReaderId,
                                                    UA_PUBSUB_SDS_TARGET);
    }
}

/* Initialize DataSetMetaData  */
static void initMetadata(int MetaDataCounter, UA_String strTmp, UA_DataSetMetaDataType* pMetaData,
                         int MetaDataType, char* MetaDataName, UA_Byte MetaDataNameSpaceId ) {
    UA_FieldMetaData_init (&pMetaData->fields[MetaDataCounter]);
    UA_NodeId_copy(&UA_TYPES[MetaDataType].typeId,
                   &pMetaData->fields[MetaDataCounter].dataType);
    pMetaData->fields[MetaDataCounter].builtInType = MetaDataNameSpaceId;
    strTmp = UA_STRING (MetaDataName);
    UA_String_copy (&strTmp, &pMetaData->fields[MetaDataCounter].name);
    pMetaData->fields[MetaDataCounter].valueRank = -1; /* scalar */
}

/* Define MetaData for TargetVariables */
static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {

    if(pMetaData == NULL) {
        return;
    }
    else {
        UA_DataSetMetaDataType_init (pMetaData);
        UA_String strTmp = UA_STRING ("DataSet 1");
        UA_String_copy (&strTmp, &pMetaData->name);

        /* Static definition of number of fields size to 4 to create four different
         * targetVariables of distinct datatype
         * Currently the publisher sends only DateTime data type */
        pMetaData->fieldsSize = 4;
        pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                             &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        /* DateTime DataType */
        initMetadata(0, strTmp, pMetaData, UA_TYPES_DATETIME, "DateTime", UA_NS0ID_DATETIME);
        /* Int32 DataType */
        initMetadata(1, strTmp, pMetaData, UA_TYPES_INT32, "Int32", UA_NS0ID_INT32);
        /* Int64 DataType */
        initMetadata(2, strTmp, pMetaData, UA_TYPES_INT32, "Int32Fast", UA_NS0ID_INT32);
        /* Boolean DataType */
        initMetadata(3, strTmp, pMetaData, UA_TYPES_BOOLEAN, "BoolToggle", UA_NS0ID_BOOLEAN);
    }
}
/**
 * **PublishedDataSet handling**
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and can exist alone. The PDS contains
 * the collection of the published fields.
 * All other PubSub elements are directly or indirectly linked with the PDS or connection.
 */
static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 * The DataSetField (DSF) is part of the PDS and describes exactly one published field.
 */
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;

    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, NULL);

}

/**
 * **WriterGroup handling**
 * The WriterGroup (WG) is part of the connection and contains the primary configuration
 * parameters for the message creation.
 */
static void
addWriterGroup(UA_Server *server, int interval) {
    /* Now we create a new WriterGroupConfig and add the group to the existing PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = interval;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;

    /* decide whether to use JSON or UADP encoding*/
#ifdef UA_ENABLE_JSON_ENCODING
    if(useJson)
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    else
#endif
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    /* configure the mqtt publish topic */
    UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
    memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));
    brokerTransportSettings.queueName = UA_STRING("customTopic");
    brokerTransportSettings.resourceUri = UA_STRING_NULL;
    brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

    /* Choose the QOS Level for MQTT */
    brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    /* Encapsulate config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings.content.decoded.data = &brokerTransportSettings;

    writerGroupConfig.transportSettings = transportSettings;
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

/**
 * **DataSetWriter handling**
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is linked to exactly one
 * PDS and contains additional informations for the message generation.
 */
static void
addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;

#ifdef UA_ENABLE_JSON_ENCODING
    UA_JsonDataSetWriterMessageDataType jsonDswMd;
    UA_ExtensionObject messageSettings;
    if(useJson) {
        /* JSON config for the dataSetWriter */
        jsonDswMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
            (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
             UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER |
             UA_JSONDATASETMESSAGECONTENTMASK_STATUS |
             UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION |
             UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP);

        messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
        messageSettings.content.decoded.data = &jsonDswMd;

        dataSetWriterConfig.messageSettings = messageSettings;
    }
#endif

    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

/**
 * That's it! You're now publishing the selected fields.
 * Open a packet inspection tool of trust e.g. wireshark and take a look on the outgoing packages.
 * The following graphic figures out the packages created by this tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process the the datagrams,
 * take a look on the ua_network_pubsub_networkmessage.c which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void usage(void) {
    printf("Usage: tutorial_pubsub_mqtt [--url <opc.mqtt://hostname:port>] "
           "[--topic <mqttTopic>] "
           "[--freq <frequency in ms> "
           "[--json]\n"
           "  Defaults are:\n"
           "  - Url: opc.mqtt://127.0.0.1:1883\n"
           "  - Topic: customTopic\n"
           "  - Frequency: 500\n"
           "  - JSON: Off\n");
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    char *addressUrl = "opc.mqtt://127.0.0.1:1883";
    char *topic = "customTopic";
    int interval = 500;

    /* Parse arguments */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0) {
            usage();
            return 0;
        }

        if(strcmp(argv[argpos], "--json") == 0) {
            useJson = true;
            continue;
        }

        if(strcmp(argv[argpos], "--url") == 0) {
            if(argpos + 1 == argc) {
                usage();
                return -1;
            }
            argpos++;
            addressUrl = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "--topic") == 0) {
            if(argpos + 1 == argc) {
                usage();
                return -1;
            }
            argpos++;
            topic = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "--freq") == 0) {
            if(argpos + 1 == argc) {
                usage();
                return -1;
            }
            argpos++;
            topic = argv[argpos];
            if(sscanf(argv[argpos], "%d", &interval) != 1) {
                usage();
                return -1;
            }
            if(interval <= 10) {
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                               "Publication interval too small");
                return -1;
            }
            continue;
        }

        usage();
        return -1;
    }

    /* Set up the server config */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    /* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    UA_ServerConfig_setDefault(config);
     config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(1 * sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerMQTT();
    config->pubsubTransportLayersSize++;

    addPubSubConnection(server, addressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server, interval);
    addDataSetWriter(server);
    UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);

    if(!connection) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Could not create a PubSubConnection");
        UA_Server_delete(server);
        return -1;
    }
    /* Assign the Topic subscribed by the MQTT subcriber to mqttSubscribeCallback API*/
    mqttTopic = topic;
    addReaderGroup(server);
    addDataSetReader(server);
    addSubscribedVariables(server, readerIdentifier);
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    return 0;
}

