/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * With the OPC UA Part 14 1.0.5, the concept of StandaloneSubscribedDataSet (SSDS) was
 * introduced. The SSDS is the counterpart to the PublishedDataSet and has its own
 * lifecycle. The SSDS can be connected to exactly one DataSetReader. In general,
 * the SSDS is optional and DSR's can still be defined without referencing a SSDS.
 * This example shows, how to configure a SSDS.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif


#include <signal.h>

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void
addTargetVariable(UA_Server *server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "subscribedTargetVar");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "subscribedTargetVar");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "demoVar");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "subscribedTargetVar");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(
        server, myIntegerNodeId, parentNodeId, parentReferenceNodeId, myIntegerName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void
addStandaloneSubscribedDataSet(UA_Server *server) {
    UA_StandaloneSubscribedDataSetConfig cfg;
    UA_NodeId ret;
    memset(&cfg, 0, sizeof(UA_StandaloneSubscribedDataSetConfig));

    /* Fill the SSDS MetaData */
    UA_DataSetMetaDataType_init(&cfg.dataSetMetaData);
    cfg.dataSetMetaData.name = UA_STRING("DemoStandaloneSDS");
    cfg.dataSetMetaData.fieldsSize = 1;
    UA_FieldMetaData fieldMetaData;
    cfg.dataSetMetaData.fields = &fieldMetaData;

    /* DateTime DataType */
    UA_FieldMetaData_init(&cfg.dataSetMetaData.fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &cfg.dataSetMetaData.fields[0].dataType);
    cfg.dataSetMetaData.fields[0].builtInType = UA_NS0ID_INT32;
    cfg.dataSetMetaData.fields[0].name = UA_STRING("subscribedTargetVar");
    cfg.dataSetMetaData.fields[0].valueRank = -1; /* scalar */

    cfg.name = UA_STRING("DemoStandaloneSDS");
    cfg.isConnected = UA_FALSE;
    cfg.subscribedDataSet.target.targetVariablesSize = 1;
    UA_FieldTargetDataType fieldTargetDataType;
    cfg.subscribedDataSet.target.targetVariables = &fieldTargetDataType;

    UA_FieldTargetDataType_init(&cfg.subscribedDataSet.target.targetVariables[0]);
    cfg.subscribedDataSet.target.targetVariables[0].attributeId = UA_ATTRIBUTEID_VALUE;
    cfg.subscribedDataSet.target.targetVariables[0].targetNodeId =
        UA_NODEID_STRING(1, "demoVar");

    UA_Server_addStandaloneSubscribedDataSet(server, &cfg, &ret);
}

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) || (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    retval |=
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    retval |= UA_PubSubConnection_regist(server, &connectionIdentifier);
    return retval;
}

/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
    return retval;
}

/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId = 100;
    readerConfig.dataSetWriterId = 62541;
    readerConfig.linkedStandaloneSubscribedDataSetName = UA_STRING("DemoStandaloneSDS");

    /* DataSetMetaData already contained in Standalone SDS no need to set up */
    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);

    readerConfig.publisherId.data = NULL;

    return retval;
}

UA_Boolean running = true;
static void
stopHandler(int sign) {
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
    UA_ServerConfig_setDefault(config);

    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
    #ifdef UA_ENABLE_PUBSUB_ETH_UADP
        UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
    #endif

    addTargetVariable(server);
    addStandaloneSubscribedDataSet(server);

    /* Add PubSub configuration */
    addPubSubConnection(server, transportProfile, networkAddressUrl);

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server);

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);

    retval |= UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int
main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if(strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if(strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile = UA_STRING(
                "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if(argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }

            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
