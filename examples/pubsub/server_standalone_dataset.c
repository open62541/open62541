/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "ua_pubsub.h"

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
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
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DemoStandaloneSDS");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData *)UA_Array_new(
        pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* DateTime DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[0].name = UA_STRING("subscribedTargetVar");
    pMetaData->fields[0].valueRank = -1; /* scalar */
}

static void
addStandaloneSubscribedDataSet(UA_Server *server) {
    UA_StandaloneSubscribedDataSetConfig cfg;
    UA_NodeId ret;
    memset(&cfg, 0, sizeof(UA_StandaloneSubscribedDataSetConfig));

    cfg.name = UA_STRING("DemoStandaloneSDS");
    fillTestDataSetMetaData(&cfg.dataSetMetaData);
    cfg.isConnected = UA_FALSE;
    cfg.subscribedDataSet.target.targetVariablesSize = 1;
    cfg.subscribedDataSet.target.targetVariables =
        (UA_FieldTargetDataType *)UA_calloc(1, sizeof(UA_FieldTargetDataType));

    UA_FieldTargetDataType_init(&cfg.subscribedDataSet.target.targetVariables[0]);
    cfg.subscribedDataSet.target.targetVariables[0].attributeId = UA_ATTRIBUTEID_VALUE;
    cfg.subscribedDataSet.target.targetVariables[0].targetNodeId =
        UA_NODEID_STRING(1, "demoVar");

    UA_Server_addStandaloneSubscribedDataSet(server, &cfg, &ret);

    UA_free(cfg.dataSetMetaData.fields);
    UA_free(cfg.subscribedDataSet.target.targetVariables);
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
    connectionConfig.publisherId.numeric = UA_UInt32_random();
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
    readerConfig.subscribedDataSetName = UA_STRING("DemoStandaloneSDS");

    /* DataSetMetaData already contained in Standalone SDS no need to set up */
    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);

    readerConfig.publisherId.data = NULL;

    return retval;
}

static UA_StatusCode
addDemoSubscriberCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionHandle, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {

    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    UA_StatusCode retval =
        addPubSubConnection(server, &transportProfile, &networkAddressUrl);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return UA_STATUSCODE_GOOD;
}

static void
addAddDemoSubscriberMethod(UA_Server *server) {
    UA_MethodAttributes addDemoSubscriberAttr = UA_MethodAttributes_default;
    addDemoSubscriberAttr.description = UA_LOCALIZEDTEXT("en-US", "Add Demo Subscriber");
    addDemoSubscriberAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Add Demo Subscriber");
    addDemoSubscriberAttr.executable = true;
    addDemoSubscriberAttr.userExecutable = true;
    UA_Server_addMethodNode(
        server, UA_NODEID_NUMERIC(1, 62541), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Add Demo Subscriber"), addDemoSubscriberAttr, &addDemoSubscriberCallback,
        0, NULL, 0, NULL, NULL, NULL);
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
    UA_ServerConfig_setMinimal(config, 4801, NULL);

    addVariable(server);
    addStandaloneSubscribedDataSet(server);
    addAddDemoSubscriberMethod(server);

    retval = UA_Server_run(server, &running);
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
