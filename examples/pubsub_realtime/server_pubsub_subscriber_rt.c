/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PUBSUB_CONFIG_FIELD_COUNT 10

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
int listenSocket;
pthread_t listenThread;

UA_DataSetReaderConfig readerConfig;

static void *
listenUDP(void *) {
    /* Open the listen socket */
    int socket;

    /* The connection is open, change the state to OPERATIONAL */
    printf("XXX The UDP multicast connection is fully open\n");
    UA_Server_enablePubSubConnection(server, connectionIdentifier);

    /* Poll and process in a loop.
     * The socket is closed in the state machine and */
    while(poll) {

        UA_Server_processPubSubConnectionReceive(server, connectionIdentifier, packet);
    }

    /* Clean up and notify the state machine */
    close(listenSocket);
    listenSocket = 0;
    UA_Server_disablePubSubConnection(server, connectionIdentifier);
    return NULL;
}

static UA_StatusCode
connectionStateMachine(UA_Server *server, const UA_NodeId componentId,
                       void *componentContext, UA_PubSubState *state,
                       UA_PubSubState targetState) {
    UA_PubSubConnectionConfig config;

    if(targetState == *state)
        return UA_STATUSCODE_GOOD;
    
    switch(targetState) {
        /* Disabled or Error */
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            printf("XXX Closing the UDP multicast connection\n");
            if(listenSocket != 0)
                shutdown(listenSocket);
            *state = targetState;
            break;

        /* Operational */
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(listenSocket != 0) {
                *state = UA_PUBSUBSTATE_OPERATIONAL;
                break;
            }
            printf("XXX Opening the UDP multicast connection\n");
            *state = UA_PUBSUBSTATE_PREOPERATIONAL;
            int res = pthread_create(&listenThread, NULL, listenUDP, NULL);
            if(res != 0)
                return UA_STATUSCODE_BADINTERNALERROR;
            break;

        /* Unknown state */
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/* Simulate a custom data sink (e.g. shared memory) */
UA_UInt32     repeatedFieldValues[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *repeatedDataValueRT[PUBSUB_CONFIG_FIELD_COUNT];

/* If the external data source is written over the information model, the
 * externalDataWriteCallback will be triggered. The user has to take care and assure
 * that the write leads not to synchronization issues and race conditions. */
static UA_StatusCode
externalDataWriteCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, const UA_NumericRange *range,
                          const UA_DataValue *data){
    //node values are updated by using variables in the memory
    //UA_Server_write is not used for updating node values.
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
externalDataReadNotificationCallback(UA_Server *server, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeid,
                                     void *nodeContext, const UA_NumericRange *range){
    //allow read without any preparation
    return UA_STATUSCODE_GOOD;
}

static void
subscribeAfterWriteCallback(UA_Server *server, const UA_NodeId *dataSetReaderId,
                            const UA_NodeId *readerGroupId,
                            const UA_NodeId *targetVariableId,
                            void *targetVariableContext,
                            UA_DataValue **externalDataValue) {
    (void) server;
    (void) dataSetReaderId;
    (void) readerGroupId;
    (void) targetVariableContext;

    assert(targetVariableId != 0);
    assert(externalDataValue != 0);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(targetVariableId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "subscribeAfterWriteCallback(): "
        "WriteUpdate() for node Id = '%.*s'. New Value = %u", (UA_Int32) strId.length, strId.data,
        *((UA_UInt32*) (**externalDataValue).value.data));
    UA_String_clear(&strId);
}

/* Callback gets triggered before subscriber has received data received data
 * hasn't been copied/handled yet */
static void
subscribeBeforeWriteCallback(UA_Server *server, const UA_NodeId *dataSetReaderId,
                             const UA_NodeId *readerGroupId, const UA_NodeId *targetVariableId,
                             void *targetVariableContext, UA_DataValue **externalDataValue) {
    (void) server;
    (void) dataSetReaderId;
    (void) readerGroupId;
    (void) targetVariableContext;

    assert(targetVariableId != 0);
    assert(externalDataValue != 0);

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(targetVariableId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "subscribeBeforeWriteCallback(): "
                "WriteUpdate() for node Id = '%.*s'",
                (UA_Int32) strId.length, strId.data);
    UA_String_clear(&strId);
}

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
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
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
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &repeatedDataValueRT[i];
        valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
        valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
        UA_Server_setVariableNode_valueBackend(server, newnodeId, valueBackend);

        UA_FieldTargetVariable *tv =
            &readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        UA_FieldTargetDataType *ftdt = &tv->targetVariable;

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(ftdt);
        ftdt->attributeId  = UA_ATTRIBUTEID_VALUE;
        ftdt->targetNodeId = newnodeId;
        /* set both before and after write callback to show the usage */
        tv->beforeWrite = subscribeBeforeWriteCallback;
        tv->externalDataValue = &repeatedDataValueRT[i];
        tv->afterWrite = subscribeAfterWriteCallback;
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

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addReaderGroup(server);
    addDataSetReader(server);

    UA_Server_enableAllPubSubComponents(server);
    retval = UA_Server_runUntilInterrupt(server);

    UA_Server_delete(server);

    for(UA_Int32 i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        UA_DataValue_delete(repeatedDataValueRT[i]);
    }

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
    UA_String transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
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
    if (argc > 2) {
        networkAddressUrl.networkInterface = UA_STRING(argv[2]);
    }

    return run(&transportProfile, &networkAddressUrl);
}
