/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>

#include <stdio.h>

#define PUBSUB_CONFIG_FIELD_COUNT 10

static UA_NetworkAddressUrlDataType networkAddressUrl =
    {{0, NULL}, UA_STRING_STATIC("opc.udp://224.0.0.22:4840/")};
static UA_String transportProfile =
    UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

/**
 * The main target of this example is to reduce the time spread and effort
 * during the subscribe cycle. This RT level example is based on buffered
 * DataSetMessages and NetworkMessages. Since changes in the
 * PubSub-configuration will invalidate the buffered frames, the PubSub
 * configuration must be frozen after the configuration phase.
 *
 * After enabling the subscriber (and when the first message is received), the
 * NetworkMessages and DataSetMessages will be calculated and buffered. During
 * the subscribe cycle, decoding will happen only to the necessary offsets and
 * the buffered NetworkMessage will only be updated.
 */

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_Server *server;

UA_DataSetReaderConfig readerConfig;

/* Simulate a custom data sink (e.g. shared memory) */
UA_UInt32     repeatedFieldValues[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *repeatedDataValueRT[PUBSUB_CONFIG_FIELD_COUNT];

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
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_DETERMINISTIC;
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
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize =
        readerConfig.dataSetMetaData.fieldsSize;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables =
        (UA_FieldTargetVariable *)UA_calloc(
            readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize,
            sizeof(UA_FieldTargetVariable));
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
        repeatedFieldValues[i] = 0;
        repeatedDataValueRT[i] = UA_DataValue_new();
        UA_Variant_setScalar(&repeatedDataValueRT[i]->value, &repeatedFieldValues[i],
                             &UA_TYPES[UA_TYPES_UINT32]);
        repeatedDataValueRT[i]->value.storageType = UA_VARIANT_DATA_NODELETE;
        repeatedDataValueRT[i]->hasValue = true;

        /* Set the value backend of the above create node to 'external value source' */
        UA_ValueBackend valueBackend;
        memset(&valueBackend, 0, sizeof(UA_ValueBackend));
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &repeatedDataValueRT[i];
        UA_Server_setVariableNode_valueBackend(server, newnodeId, valueBackend);

        UA_FieldTargetVariable *tv =
            &readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        UA_FieldTargetDataType *ftdt = &tv->targetVariable;

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(ftdt);
        ftdt->attributeId  = UA_ATTRIBUTEID_VALUE;
        ftdt->targetNodeId = newnodeId;
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

    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        UA_FieldTargetVariable *tv =
            &readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        UA_FieldTargetDataType *ftdt = &tv->targetVariable;
        UA_FieldTargetDataType_clear(ftdt);
    }

    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}

int main(int argc, char **argv) {
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            printf("usage: %s <uri> [device]\n", argv[0]);
            return EXIT_SUCCESS;
        } else if(strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if(strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if(argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf ("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }
    if(argc > 2)
        networkAddressUrl.networkInterface = UA_STRING(argv[2]);

    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    server = UA_Server_new();

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

    UA_Server_delete(server);

    UA_PubSubOffsetTable_clear(&ot);
    for(UA_Int32 i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        UA_DataValue_delete(repeatedDataValueRT[i]);
    }

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

