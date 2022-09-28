
/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/**
 * The PubSub connection example demonstrate the PubSub TransportLayer configuration and
 * the dynamic creation of PubSub Connections on runtime.
 */
int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Add the PubSubTransportLayer implementation to the server config.
     * The PubSubTransportLayer is a factory to create new connections
     * on runtime. The UA_PubSubTransportLayer is used for all kinds of
     * concrete connections e.g. UDP, MQTT, AMQP...
     *
     * It is possible to use multiple PubSubTransportLayers on runtime. The
     * correct factory is selected on runtime by the standard defined PubSub
     * TransportProfileUri's. */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    /* Create a new ConnectionConfig. The addPubSubConnection function takes the
     * config and create a new connection. The Connection identifier is
     * copied to the NodeId parameter.*/
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    /* The address and interface is part of the standard
     * defined UA_NetworkAddressUrlDataType. */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    /* Connection options are given as Key/Value Pairs. The available options are
     * maybe standard or vendor defined. */
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[0].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[2].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionConfig.connectionProperties = connectionOptions;
    connectionConfig.connectionPropertiesSize = 3;
    /* Create a new concrete connection and add the connection
     * to the current PubSub configuration. */
    UA_NodeId connectionIdentifier;
    UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retval == UA_STATUSCODE_GOOD){
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "The PubSub Connection was created successfully!");
    }

    retval |= UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
