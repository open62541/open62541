/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include <signal.h>
#include <stdio.h>
#include <time.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

static UA_Server *server;
static timer_t writerGroupTimer;
static UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;

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
UA_NodeId publishVariables[PUBSUB_CONFIG_FIELD_COUNT];

static UA_StatusCode
readCallback(UA_Server *server, const UA_NodeId *sessionId,
             void *sessionContext, const UA_NodeId *nodeId,
             void *nodeContext, UA_Boolean includeSourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    *value = *(UA_DataValue*)nodeContext;
    return UA_STATUSCODE_GOOD;
}

static void
valueUpdateCallback(UA_Server *server, void *data) {
    for(int i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; ++i) {
        valueStore[i]++;
    }
}

/* WriterGroup timer managed by a custom state machine. This uses
 * UA_Server_triggerWriterGroupPublish. The server can block its internal mutex,
 * so this can have some jitter. For hard realtime the publish callback has to
 * send out the packet without going through the server. */

static void
writerGroupPublishTrigger(union sigval signal) {
    printf("XXX Publish Callback\n");
    UA_Server_triggerWriterGroupPublish(server, writerGroupIdent);
}

static UA_StatusCode
writerGroupStateMachine(UA_Server *server, const UA_NodeId componentId,
                        void *componentContext, UA_PubSubState *state,
                        UA_PubSubState targetState) {
    UA_WriterGroupConfig config;
    struct itimerspec interval;
    memset(&interval, 0, sizeof(interval));

    if(targetState == *state)
        return UA_STATUSCODE_GOOD;
    
    switch(targetState) {
        /* Disabled or Error */
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            printf("XXX Disabling the WriterGroup\n");
            timer_settime(writerGroupTimer, 0, &interval, NULL);
            *state = targetState;
            break;

        /* Operational */
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(*state == UA_PUBSUBSTATE_OPERATIONAL)
                break;
            printf("XXX Enabling the WriterGroup\n");
            UA_Server_getWriterGroupConfig(server, writerGroupIdent, &config);
            interval.it_interval.tv_sec = config.publishingInterval / 1000;
            interval.it_interval.tv_nsec =
                ((long long)(config.publishingInterval * 1000 * 1000)) % (1000 * 1000 * 1000);
            interval.it_value = interval.it_interval;
            UA_WriterGroupConfig_clear(&config);
            int res = timer_settime(writerGroupTimer, 0, &interval, NULL);
            if(res != 0)
                return UA_STATUSCODE_BADINTERNALERROR;
            *state = UA_PUBSUBSTATE_OPERATIONAL;
            break;

        /* Unknown state */
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/* Intercept the creation of a new WriterGroup and update the config. We set the
 * custom state machine to control our own timer interrupts for the
 * WriterGroup. */
static UA_StatusCode
testComponentLifecycleCallback(UA_Server *server, const UA_NodeId id,
                               const UA_PubSubComponentType componentType,
                               UA_Boolean remove) {
    if(remove)
        return UA_STATUSCODE_GOOD;
    if(componentType != UA_PUBSUBCOMPONENT_WRITERGROUP)
        return UA_STATUSCODE_GOOD;

    printf("XXX Set the custom state machine for the new WriterGroup\n");

    UA_WriterGroupConfig c;
    UA_Server_getWriterGroupConfig(server, id, &c);
    c.customStateMachine = writerGroupStateMachine;
    UA_Server_updateWriterGroupConfig(server, id, &c);
    UA_WriterGroupConfig_clear(&c);

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    /* Prepare the values */
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        valueStore[i] = (UA_UInt32) i + 1;
        UA_Variant_setScalar(&dvStore[i].value, &valueStore[i], &UA_TYPES[UA_TYPES_UINT32]);
        dvStore[i].value.storageType = UA_VARIANT_DATA_NODELETE;
        dvStore[i].hasValue = true;
    }

    /* Initialize the timer */
    struct sigevent sigev;
    memset(&sigev, 0, sizeof(sigev));
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = writerGroupPublishTrigger;
    timer_create(CLOCK_REALTIME, &sigev, &writerGroupTimer);

    /* Initialize the server */
    server = UA_Server_new();

    /* Set the PubSub-Component lifecycle callback */
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.componentLifecycleCallback = testComponentLifecycleCallback;

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

        /* Set the node's external value source */
        UA_ExternalValueSource evs;
        memset(&evs, 0, sizeof(UA_ExternalValueSource));
        evs.read = readCallback;
        UA_Server_setNodeContext(server, publishVariables[i], &dvStore[i]);
        UA_Server_setVariableNode_externalValueSource(server, publishVariables[i], evs);

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
    /* Currently, delta-frames are not supported together with raw data
     * dataSetWriterConfig.keyFrameCount = 10; */
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

    UA_Server_enableAllPubSubComponents(server);

    /* Add a callback that updates the value */
    UA_UInt64 callbackId;
    UA_Server_addRepeatedCallback(server, valueUpdateCallback, NULL,
                                  PUBSUB_CONFIG_PUBLISH_CYCLE_MS, &callbackId);

    UA_Server_runUntilInterrupt(server);

    /* Cleanup */
    UA_Server_delete(server);
    timer_delete(writerGroupTimer);
    return EXIT_SUCCESS;
}
