/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub subscriber API is currently not finished. This examples can be used to
 * receive and print the values, which are published by the tutorial_pubsub_publish
 * example. The following code uses internal API which will be later replaced by the
 * higher-level PubSub subscriber API. */

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

UA_NodeId connectionIdentifier, readerGroupIdentifier, readerIdentifier;
UA_PubSubDataSetReaderConfig readerConfig;

static void FillTestDataSet1MetaData(UA_DataSetMetaDataType* pMetaData);

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
	//creation configuration for the connection creation
	UA_PubSubConnectionConfig connectionConfig;
	memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
	connectionConfig.name = UA_STRING("UDPMC Connection 1");
	connectionConfig.transportProfileUri = *transportProfile;
	connectionConfig.enabled = UA_TRUE;
	UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
						 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
	connectionConfig.publisherId.numeric = UA_UInt32_random();
	//adding new connection to server
	UA_StatusCode retval = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
	if(retval == UA_STATUSCODE_GOOD)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"The PubSub Connection was created successfully!");
}

static void
addReaderGroup(UA_Server *server) {
	UA_PubSubReaderGroupConfig readerGroupConfig;
	memset(&readerGroupConfig, 0, sizeof(UA_PubSubReaderGroupConfig));
	readerGroupConfig.name = UA_STRING("ReaderGroup1");
	UA_NodeId_init(&readerGroupIdentifier);
	UA_StatusCode retval = UA_PubSubConnection_addReaderGroup(server, connectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
	if (retval == UA_STATUSCODE_GOOD)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"The Reader Group was created successfully!");
}

static void
addDataSetReader(UA_Server *server) {
	memset(&readerConfig, 0, sizeof(UA_PubSubDataSetReaderConfig));
	readerConfig.name = UA_STRING("DataSet Reader 1");
	readerConfig.dataSetWriterId = 1;
	//readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;

	//Setting up Meta data configuration in DataSet Reader
	FillTestDataSet1MetaData(&readerConfig.dataSetMetaData);

	UA_StatusCode retval = UA_PubSubReaderGroup_addDataSetReader(server, readerGroupIdentifier, &readerConfig, &readerIdentifier);
	if (retval == UA_STATUSCODE_GOOD)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"The DataSet Reader was created successfully!");
}

static void AddSubscribedVariables(UA_Server* server, UA_NodeId dataSetReaderId) {
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if (folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    }

    UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        folderBrowseName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &folderId);

    //UA_PubSubDataSetReader* pDataSetReader = UA_PubSubDataSetReader_findDSRbyId(server, dataSetReaderId);
    //if (pDataSetReader->config.subscribedDataSetType == UA_PUBSUB_SDS_TARGET)
    //{
    UA_PubSubDataSetReader_addTargetVariables(server, &folderId, dataSetReaderId, UA_PUBSUB_SDS_TARGET);
    //}
}

static void FillTestDataSet1MetaData(UA_DataSetMetaDataType* pMetaData) {
    if (pMetaData == NULL)
    	return;

    UA_DataSetMetaDataType_init(pMetaData);

    UA_String strTmp;
    strTmp = UA_STRING("DataSet 1");
    UA_String_copy(&strTmp, &pMetaData->name);

    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    //DateTime DataType
    UA_FieldMetaData_init(&pMetaData->fields[0]);
	UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &pMetaData->fields[0].dataType);
	pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
	strTmp = UA_STRING("DateTime");
	UA_String_copy(&strTmp, &pMetaData->fields[0].name);
	pMetaData->fields[0].valueRank = -1; // scalar

	//Int32 DataType
    UA_FieldMetaData_init(&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
    strTmp = UA_STRING("Int32");
    UA_String_copy(&strTmp, &pMetaData->fields[1].name);
    pMetaData->fields[1].valueRank = -1; // scalar

    //Int64 DataType
    UA_FieldMetaData_init(&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT32;
    strTmp = UA_STRING("Int32Fast");
    UA_String_copy(&strTmp, &pMetaData->fields[2].name);
    pMetaData->fields[2].valueRank = -1; // scalar

    //Boolean DataType
    UA_FieldMetaData_init(&pMetaData->fields[3]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BOOLEAN].typeId, &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    strTmp = UA_STRING("BoolToggle");
    UA_String_copy(&strTmp, &pMetaData->fields[3].name);
    pMetaData->fields[3].valueRank = -1; // scalar
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

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_ServerConfig_new_minimal(4801, NULL);

    /* Add the PubSub network layer implementation to the server config.
    * The TransportLayer is acting as factory to create new connections
    * on runtime. */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)UA_malloc(sizeof(UA_PubSubTransportLayer));
    if (!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    //TODO
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    UA_Server *server = UA_Server_new(config);

    /* Build PubSub information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Server_initPubSubNS0(server);
#endif

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addReaderGroup(server);
    addDataSetReader(server);
    AddSubscribedVariables(server, readerIdentifier);

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://239.0.0.11:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
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

