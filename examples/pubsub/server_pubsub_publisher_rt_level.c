/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>
#include <open62541/server_pubsub.h>

UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent, connectionIdentifier;

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/**
 * The PubSub RT level example points out the configuration of different PubSub RT levels. These levels will be later
 * used for deterministic message generation. The underlying base concept is the target to reduce the time spread and
 * affort during the publish cycle. Most of the RT levels are based on a pregenerated and buffered DataSetMesseges and
 * NetworkMessages. Since changes in the PubSub configuration will invalidate the buffered frames, the pubsub
 * configuration can be frozen after the configuration phase
 */

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
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    /* Add one PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Add one DataSetField to the PDS */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_Server_delete(server);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;

    /*Add standard PubSub configuration (no difference to the std. configuration)*/
    addMinimalPubSubConfiguration(server);

    /* Add one WriterGroup with PubSub RT Level 0. If any rtLevel != UA_PUBSUB_RT_NONE is set, the
     * writerGroup does not start the publishing interval automatically.*/
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    /* RT Level 0 setup */
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent);
    /* Add one DataSetWriter */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent);
    /* Add one DataSetField with static value source to PDS */
    UA_DataSetFieldConfig dsfConfig;
    UA_Server_getDataSetFieldConfig(server, dataSetFieldIdent, &dsfConfig);
    /* Create Variant and configure as DataSetField source */
    UA_UInt32 *intValue = UA_UInt32_new();
    UA_Variant variant;
    memset(&variant, 0, sizeof(UA_Variant));
    UA_Variant_setScalar(&variant, intValue, &UA_TYPES[UA_TYPES_UINT32]);
    UA_DataValue staticValueSource;
    memset(&staticValueSource, 0, sizeof(staticValueSource));
    staticValueSource.value = variant;
    dsfConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
    dsfConfig.field.variable.staticValueSource.value = variant;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);

    /* The PubSub configuration is currently editable and the publish callback is not running */
    writerGroupConfig.publishingInterval = 1000;
    UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);

    /* Freeze the PubSub configuration (and start implicitly the publish callback) */
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);

    /* Changes of the PubSub configuration is restricted after freeze */
    UA_StatusCode retVal = UA_Server_updateWriterGroupConfig(server, writerGroupIdent, &writerGroupConfig);
    if(retVal != UA_STATUSCODE_BADCONFIGURATIONERROR)
        return EXIT_FAILURE;

    /* Unfreeze the PubSub configuration (and stop implicitly the publish callback) */
    UA_Server_setWriterGroupDisabled(server, writerGroupIdent);
    UA_Server_unfreezeWriterGroupConfiguration(server, writerGroupIdent);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
