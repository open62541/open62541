/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include <signal.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;

/**
 * For realtime publishing the following is configured:
 *
 * - Direct-access data source with double-pointers to a DataValue.
 *   That allows atomic updates by switching the pointer to a new DataValue.
 * - The WriterGroup has UA_PUBSUB_RT_FIXED_SIZE configured. So
 *   the message is pre-computed and updated only at fixed offsets.
 * - A dedicated EventLoop is instantiated for PubSub.
 *   So the client/server operations do not interfere with the publisher.
 *
 * Note that for true realtime the following additions are needed (at least):
 *
 * - An EventLoop + PubSubConnection that supports TSN.
 * - The EventLoop uses a fixed TX buffer instead of a malloc each.
 * - Preparing the message with a time-offset before sending.
 */

/* Values in static locations. We cycle the dvPointers double-pointer to the
 * next with atomic operations. */
UA_UInt32 valueStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue dvStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *dvPointers[PUBSUB_CONFIG_FIELD_COUNT];

static void
valueUpdateCallback(UA_Server *server, void *data) {
    for(int i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; ++i) {
        if(dvPointers[i] < &dvStore[PUBSUB_CONFIG_FIELD_COUNT - 1])
            UA_atomic_xchg((void**)&dvPointers[i], dvPointers[i]+1);
        else
            UA_atomic_xchg((void**)&dvPointers[i], &dvStore[0]);
    }
}

/* Dedicated EventLoop for PubSub */
volatile UA_Boolean pubSubELRunning = true;
UA_EventLoop *pubSubEL;

static void *
runPubSubEL(void *_) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    while(pubSubELRunning)
        pubSubEL->run(pubSubEL, 100);
    return NULL;
}

int main(void) {
    UA_Server *server = UA_Server_new();

    /* Instantiate the custom EventLoop.
     * Will be attached to the PubSubConnection and gets used for everything "above".
     * This should be bound to a dedicated core for RT. */
    pubSubEL = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_ConnectionManager *udpCM =
        UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp connection manager"));
    pubSubEL->registerEventSource(pubSubEL, (UA_EventSource *)udpCM);
    pubSubEL->start(pubSubEL);

    pthread_t pubSubELThread;
    pthread_create(&pubSubELThread, NULL, runPubSubEL, NULL);

    /* Prepare the values */
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        valueStore[i] = (UA_UInt32) i + 1;
        UA_Variant_setScalar(&dvStore[i].value, &valueStore[i], &UA_TYPES[UA_TYPES_UINT32]);
        dvStore[i].hasValue = true;
        dvPointers[i] = &dvStore[i];
    }

    /* Add a PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = true;
    connectionConfig.eventLoop = pubSubEL;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
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
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = true;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &dvPointers[i];
        UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
    }

    /* Add a WriterGroup */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUBSUB_CONFIG_PUBLISH_CYCLE_MS;
    writerGroupConfig.enabled = true;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;

    /* Change message settings of writerGroup to send PublisherId, WriterGroupId
     * in GroupHeader and DataSetWriterId in PayloadHeader of NetworkMessage */
    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_UadpWriterGroupMessageDataType_init(&writerGroupMessage);
    writerGroupMessage.networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID | UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID | UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
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

    /* Freeze the PubSub configuration and enable */
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);

    /* Add a callback that updates the value */
    UA_UInt64 callbackId;
    UA_Server_addRepeatedCallback(server, valueUpdateCallback, NULL,
                                  PUBSUB_CONFIG_PUBLISH_CYCLE_MS, &callbackId);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    pubSubELRunning = false;
    pthread_join(pubSubELThread, NULL);

    pubSubEL->stop(pubSubEL);
    while(pubSubEL->state != UA_EVENTLOOPSTATE_STOPPED)
        pubSubEL->run(pubSubEL, 0);
    pubSubEL->free(pubSubEL);

    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
