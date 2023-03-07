/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub Subscriber API is currently not finished. This example can be used
 * to receive and display values that are published by tutorial_pubsub_publish
 * example in the TargetVariables of Subscriber Information Model .
 */
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>

#define PUBSUB_CONFIG_FIELD_COUNT 10

/**
 * The main target of this example is to reduce the time spread and effort during the subscribe cycle.
 * This RT level example is based on buffered DataSetMessages and NetworkMessages. Since changes in the
 * PubSub-configuration will invalidate the buffered frames, the PubSub configuration
 * must be frozen after the configuration phase.
 *
 * After the PubSub-configuration freeze, the NetworkMessages and DataSetMessages
 * will be calculated and buffered. During the subscribe cycle, decoding will happen only to the necessary offsets and
 * the buffered NetworkMessage will only be updated. Support only for one DSM.
 * PUBSUB_CONFIG_FASTPATH_STATIC_VALUES -> TODO
 */

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

UA_UInt32    *repeatedFieldValues[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *repeatedDataValueRT[PUBSUB_CONFIG_FIELD_COUNT];

static UA_UInt32 sSubscribedDataSink[PUBSUB_CONFIG_FIELD_COUNT];    /* simulate a custom data sink (e.g. shared memory) */
static UA_UInt32 sSubscribedTargetVarDataOffset[PUBSUB_CONFIG_FIELD_COUNT];

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
subscribeAfterWriteCallback(
   UA_Server *server,
   const UA_NodeId *dataSetReaderId,
   const UA_NodeId *readerGroupId,
   const UA_NodeId *targetVariableId,
   void *targetVariableContext,
   UA_DataValue **externalDataValue) {    // received value has already been copied to param externalDataValue

    (void) server;
    (void) dataSetReaderId;
    (void) readerGroupId;

    assert(targetVariableId != 0);
    assert(targetVariableContext != 0);
    assert(externalDataValue != 0);
    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(targetVariableId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "subscribeAfterWriteCallback(): "
        "WriteUpdate() for node Id = '%.*s'. New Value = %u", (UA_Int32) strId.length, strId.data,
        *((UA_UInt32*) (**externalDataValue).value.data));
    UA_String_clear(&strId);
}

/* callback gets triggered before subscriber has received data
    received data hasn't been copied/handled yet */
static void
subscribeBeforeWriteCallback(
    UA_Server *server,
    const UA_NodeId *dataSetReaderId,
    const UA_NodeId *readerGroupId,
    const UA_NodeId *targetVariableId,
    void *targetVariableContext,  /* custom target variable context (e.g. shared mem handle, offset) */
    UA_DataValue **externalDataValue) { /* received value */

    (void) server;
    (void) dataSetReaderId;
    (void) readerGroupId;

    assert(targetVariableId != 0);
    assert(targetVariableContext != 0);
    assert(externalDataValue != 0);
    void *data = (**externalDataValue).value.data;
    UA_UInt32 dataSize = (**externalDataValue).value.type->memSize;
    assert(data != 0);
    assert(dataSize == sizeof(UA_UInt32)); /* in this simple example all dataset fields are of type UInt32 */

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(targetVariableId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "subscribeBeforeWriteCallback(): "
        "WriteUpdate() for node Id = '%.*s'. New Value Data = %u", (UA_Int32) strId.length, strId.data,
        *(UA_UInt32*) data);
    UA_String_clear(&strId);

    /* We can prepare custom data sink before data is copied
        meta information is available at targetVariableContext
        data sink could be a shared memory or any other custom implementation which represents variables of the engineering system
        in this example we simulate a custom data sink */
    UA_UInt32 offset = *((UA_UInt32*) targetVariableContext);
    assert(offset < PUBSUB_CONFIG_FIELD_COUNT);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "subscribeBeforeWriteCallback(): "
        "offset = %u", offset);
    memcpy(sSubscribedDataSink + offset, data, dataSize);
}

/* Define MetaData for TargetVariables */
static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
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
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL))
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &connectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    return retval;
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables (UA_Server *server) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
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
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                            folderBrowseName, UA_NODEID_NUMERIC (0,
                            UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = readerConfig.dataSetMetaData.fieldsSize;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
        // Initialize the values at first to create the buffered NetworkMessage with correct size and offsets
        UA_Variant value;
        UA_Variant_init(&value);
        UA_UInt32 intValue = 0;
        UA_Variant_setScalar(&value, &intValue, &UA_TYPES[UA_TYPES_UINT32]);
        vAttr.value = value;
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newnodeId);
        repeatedFieldValues[i] = UA_UInt32_new();
        *repeatedFieldValues[i] = 0;
        repeatedDataValueRT[i] = UA_DataValue_new();
        UA_Variant_setScalar(&repeatedDataValueRT[i]->value, repeatedFieldValues[i],
                             &UA_TYPES[UA_TYPES_UINT32]);
        repeatedDataValueRT[i]->hasValue = true;

        /* Set the value backend of the above create node to 'external value source' */
        UA_ValueBackend valueBackend;
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &repeatedDataValueRT[i];
        valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
        valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
        UA_Server_setVariableNode_valueBackend(server, newnodeId, valueBackend);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable);
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable.targetNodeId = newnodeId;
        /* set both before and after write callback to show the usage */
        /* configure before write callback */
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].beforeWrite = subscribeBeforeWriteCallback;
        sSubscribedTargetVarDataOffset[i] = (UA_UInt32) i;
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariableContext = &sSubscribedTargetVarDataOffset[i];
        /* configure after write callback */
        readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].afterWrite = subscribeAfterWriteCallback;
    }

    return retval;
}

/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.expectedEncoding = UA_PUBSUB_RT_RAW;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask           = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                 (UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    dataSetReaderMessage->dataSetMessageContentMask           = (UA_UadpDataSetMessageContentMask)(UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER);
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;

    readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);
    retval |= addSubscribedVariables(server);

    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);

    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable);

    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);

    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
    return retval;
}

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4801, NULL);

    /* Add the PubSub network layer implementation to the server config.
     * The TransportLayer is acting as factory to create new connections
     * on runtime. Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Freeze the PubSub configuration (and start implicitly the subscribe callback) */
    UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);

    retval = UA_Server_run(server, &running);

    UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier);
    UA_Server_delete(server);

    for (UA_Int32 iterator = 0; iterator <  PUBSUB_CONFIG_FIELD_COUNT; iterator++)
    {
        UA_free(repeatedFieldValues[iterator]);
        UA_free(repeatedDataValueRT[iterator]);
    }

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
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

    return run(&transportProfile, &networkAddressUrl);
}

