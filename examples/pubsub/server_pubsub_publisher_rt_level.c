/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>
#include <open62541/server_pubsub.h>

/* possible options: PUBSUB_CONFIG_FASTPATH_NONE, PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS, PUBSUB_CONFIG_FASTPATH_STATIC_VALUES */
#define PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_PUBLISH_CYCLES 1000000
#define PUBSUB_CONFIG_FIELD_COUNT 10

/**
 * The PubSub RT level example points out the configuration of different PubSub RT-levels. These levels will be
 * used together with an RT capable OS for deterministic message generation. The main target is to reduce the time
 * spread and effort during the publish cycle. Most of the RT levels are based on pre-generated and buffered
 * DataSetMesseges and NetworkMessages. Since changes in the PubSub-configuration will invalidate the buffered
 * frames, the PubSub configuration must be frozen after the configuration phase.
 *
 * This example can be configured to compare and measure the different PubSub options by the following define options:
 * PUBSUB_CONFIG_FASTPATH_NONE -> The published fields are added to the information model and published in the default non-RT mode
 * PUBSUB_CONFIG_FASTPATH_STATIC_VALUES -> The published fields are not visible in the information model. The publish cycle is improved
 * by prevent the value lookup within the information model.
 * PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS -> The published fields are not visible in the information model. After the PubSub-configuration
 * freeze, the NetworkMessages and DataSetMessages will be calculated and buffered. During the publish cycle these buffers will only be updated.
 */


UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;
UA_UInt32 *valueStore[PUBSUB_CONFIG_FIELD_COUNT];

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* The following PubSub configuration does not differ from the 'normal' configuration */
static void
addMinimalPubSubConfiguration(UA_Server * server){
    /* Add one PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    /* Add one PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Add one DataSetField to the PDS */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

static void
valueUpdateCallback(UA_Server *server, void *data) {
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS || defined PUBSUB_CONFIG_FASTPATH_STATIC_VALUES
    for (int i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; ++i)
        *valueStore[i] = *valueStore[i] + 1;
    if(*valueStore[0] > PUBSUB_CONFIG_PUBLISH_CYCLES)
        running = false;
#endif
#if defined PUBSUB_CONFIG_FASTPATH_NONE
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++){
        UA_Variant value;
        UA_Variant_init(&value);
        if(UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 1000 + (UA_UInt32) i), &value) != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to read publish value. Node number: %zu", i);
            continue;
        }
        UA_UInt32 *intValue = (UA_UInt32 *) value.data;
        *intValue = *intValue + 1;
        if(UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1000 + (UA_UInt32) i), value) != UA_STATUSCODE_GOOD){
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to update publish value. Node number: %zu", i);
            continue;
        }
        if(i == 0 && *intValue  > PUBSUB_CONFIG_PUBLISH_CYCLES)
            running = false;
        UA_Variant_clear(&value);
    }
#endif
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    /*Add standard PubSub configuration (no difference to the std. configuration)*/
    addMinimalPubSubConfiguration(server);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUBSUB_CONFIG_PUBLISH_CYCLE_MS;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_UadpWriterGroupMessageDataType_init(&writerGroupMessage);
    /* Change message settings of writerGroup to send PublisherId,
     * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
     * of NetworkMessage */
    writerGroupMessage.networkMessageContentMask = (UA_UadpNetworkMessageContentMask) ((UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                    (UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                    (UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                    (UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
                                                    (UA_UadpNetworkMessageContentMask) UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = &writerGroupMessage;
#ifdef PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
#elif defined PUBSUB_CONFIG_FASTPATH_STATIC_VALUES
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_DIRECT_VALUE_ACCESS;
#endif
    UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent);
    /* Add one DataSetWriter */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;

    UA_UadpDataSetWriterMessageDataType uadpDataSetWriterMessageDataType;
    UA_UadpDataSetWriterMessageDataType_init(&uadpDataSetWriterMessageDataType);
    uadpDataSetWriterMessageDataType.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask) UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    dataSetWriterConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    dataSetWriterConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE];
    dataSetWriterConfig.messageSettings.content.decoded.data = &uadpDataSetWriterMessageDataType;

    /* Encode fields as RAW-Encoded */
    dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent);

#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS || defined PUBSUB_CONFIG_FASTPATH_STATIC_VALUES
    /* Add one DataSetField with static value source to PDS */
    UA_DataValue *staticValueSource = UA_DataValue_new();
    UA_DataSetFieldConfig dsfConfig;
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++){
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        /* Create Variant and configure as DataSetField source */
        UA_UInt32 *intValue = UA_UInt32_new();
        *intValue = (UA_UInt32) i * 1000;
        valueStore[i] = intValue;
        UA_Variant_setScalar(&staticValueSource->value, intValue, &UA_TYPES[UA_TYPES_UINT32]);
        dsfConfig.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
        dsfConfig.field.variable.rtValueSource.staticValueSource = &staticValueSource;
        UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
    }
#endif
#if defined PUBSUB_CONFIG_FASTPATH_NONE
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++){
        UA_VariableAttributes attributes = UA_VariableAttributes_default;
        UA_Variant value;
        UA_Variant_init(&value);
        UA_UInt32 intValue = (UA_UInt32) i * 1000;
        UA_Variant_setScalar(&value, &intValue, &UA_TYPES[UA_TYPES_UINT32]);
        attributes.value = value;
        if(UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,  1000 + (UA_UInt32) i),
                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                UA_QUALIFIEDNAME(1, "variable"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                 attributes, NULL, NULL) != UA_STATUSCODE_GOOD){
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to add Publish-Node to the server. Node number: %zu", i);
            continue;
        }
        dsfConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, 1000 + (UA_UInt32) i);
        dsfConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
    }
#endif
    /* The PubSub configuration is currently editable and the publish callback is not running */
    writerGroupConfig.publishingInterval = PUBSUB_CONFIG_PUBLISH_CYCLE_MS;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);

    /* Freeze the PubSub configuration (and start implicitly the publish callback) */
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);

    UA_UInt64 callbackId;
    UA_Server_addRepeatedCallback(server, valueUpdateCallback, NULL, PUBSUB_CONFIG_PUBLISH_CYCLE_MS, &callbackId);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Server_run(server, &running);

#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS || defined PUBSUB_CONFIG_FASTPATH_STATIC_VALUES
    UA_DataValue_delete(staticValueSource);
#endif
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
