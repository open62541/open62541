/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>
#include "open62541.h"

/* Work in progress: This Tutorial/Example will be continuously extended during the next PubSub batches */

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static UA_StatusCode
addPubSubConnection(UA_Server *server){
    /* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    UA_NodeId connectionIdentifier;
    UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    return retval;
}

/**
 * The PubSub publish example demonstrate the simplest way to publish
 * informations from the information model over UDP Multicast.
 */
int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    /* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    UA_Server *server = UA_Server_new(config);

    if(addPubSubConnection(server) != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed!");

    /* The PublishedDataSetConfig contains all necessary public informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Robot Axis");

    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_NodeId publishedDataSetIdent;
    if(UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent).addResult == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub PublishedDataSet creation successful!");

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
