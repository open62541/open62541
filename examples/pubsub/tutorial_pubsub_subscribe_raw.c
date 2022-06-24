/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
*/

/* During the RAW-Encoding decode process, the MetaData-Information are needed.
 * The OPC UA specification defines several ways to collect these information (see. Part 14 5.2.3).
 * Currently we use an OPC UA client to collect the MetaData form the publisher.
 * Attention: There can be more than one publisher servers later.
 * The following defines tell the server, where to collect the MetaData. (currently used for all DSR)"
 */
#define METADATA_SOURCE_SERVER "opc.tcp://localhost:4840"
#define METADATA_DATASET_NAME "Demo PDS"


#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>
#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <server/ua_server_internal.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <custom_datatype/custom_datatype.h>

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;


static void collectDataSetMetaDataFromServer(UA_Server *server, UA_DataSetMetaDataType *pMetaData);

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

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
    if (retval != UA_STATUSCODE_GOOD) {
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
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
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

    /* Setting up Meta data configuration in DataSetReader */
/*
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);
*/
    collectDataSetMetaDataFromServer(server, &readerConfig.dataSetMetaData);
    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
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

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);


    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
        UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                            folderId,
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                            UA_QUALIFIEDNAME(1, (char *) readerConfig.dataSetMetaData.fields[i].name.data),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                            vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                           readerConfig.dataSetMetaData.fieldsSize, targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

static void printDataSetMetaDataType(const UA_DataSetMetaDataType *pMetaData){
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"MetaDataName: %s", pMetaData->name.data);
    for(size_t i = 0; i < pMetaData->fieldsSize; ++i) {
        UA_String *typeName = UA_String_new();
        UA_NodeId_print(&pMetaData->fields[i].dataType, typeName);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "FieldNumber: %i, Name: %s, DataTypeId: %s, InternalTypeid: %i", (int) i,
                    pMetaData->fields[i].name.data, (char *) typeName->data, (int) pMetaData->fields[i].builtInType);
    }
}

/* Collect MetaData from remote server */
UA_StatusCode
UA_Server_DataSetReader_getMetaDataFromRemote(UA_Server *server, UA_String remoteServer,
                                              UA_QualifiedName pdsName, UA_DataSetMetaDataType *dsMetaData);

UA_StatusCode
UA_Server_DataSetReader_getMetaDataFromRemote(UA_Server *server, UA_String remoteServer,
                                              UA_QualifiedName pdsName, UA_DataSetMetaDataType *dsMetaData){
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    char remoteServer_ar[512];
    memcpy(remoteServer_ar, remoteServer.data, remoteServer.length);
    remoteServer_ar[remoteServer.length] = '\0';
    UA_StatusCode retval = UA_Client_connect(client, remoteServer_ar);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    //UA_TranslateBrowsePathsToNodeIdsRequest translateBrowsePathsToNodeIdsRequest;
    //UA_Client_Service_translateBrowsePathsToNodeIds(client, )
    UA_NodeId resultMetaDataNodeId = UA_NODEID_NULL;
    UA_BrowseRequest browseRequest;
    UA_BrowseRequest_init(&browseRequest);
    UA_BrowseDescription *browseDescription = UA_BrowseDescription_new();
    browseDescription->browseDirection = UA_BROWSEDIRECTION_INVERSE;
    browseDescription->resultMask = UA_BROWSERESULTMASK_ALL;
    browseDescription->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHEDDATAITEMSTYPE);
    browseDescription->referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    browseDescription->includeSubtypes = UA_TRUE;
    browseRequest.nodesToBrowseSize = 1;
    browseRequest.nodesToBrowse = browseDescription;
    browseRequest.requestedMaxReferencesPerNode = 1000;

    UA_BrowseResponse browseResponse = UA_Client_Service_browse(client, browseRequest);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Result size: %i result: %s, ref size: %i ",
                (int) browseResponse.resultsSize, UA_StatusCode_name(browseResponse.responseHeader.serviceResult), (int) browseResponse.results->referencesSize);
    for(size_t i = 0; i < browseResponse.resultsSize; ++i) {
        for(size_t j = 0; j < browseResponse.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(browseResponse.results[i].references[j]);
            if(UA_String_equal(&pdsName.name, &ref->browseName.name)) {
                UA_NodeId_copy(&ref->nodeId.nodeId, &resultMetaDataNodeId);
                break;
            }
        }
    }
    UA_BrowseResponse_clear(&browseResponse);
    if(UA_NodeId_isNull(&resultMetaDataNodeId)){
        UA_BrowseRequest_clear(&browseRequest);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    //find metadata property
    browseRequest.nodesToBrowse->nodeId = resultMetaDataNodeId;
    browseRequest.nodesToBrowse->browseDirection = UA_BROWSEDIRECTION_FORWARD;
    browseRequest.nodesToBrowse->referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    browseResponse = UA_Client_Service_browse(client, browseRequest);
    UA_String compareString = UA_STRING("DataSetMetaData");
    for(size_t i = 0; i < browseResponse.resultsSize; ++i) {
        for(size_t j = 0; j < browseResponse.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(browseResponse.results[i].references[j]);
            if(UA_String_equal(&compareString, &ref->browseName.name)) {
                UA_NodeId_copy(&ref->nodeId.nodeId, &resultMetaDataNodeId);
                break;
            }
        }
    }
    UA_BrowseResponse_clear(&browseResponse);
    UA_BrowseRequest_clear(&browseRequest);

    if(UA_NodeId_isNull(&resultMetaDataNodeId)){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Unable to request metadata from remote server!");
    } else {
        UA_Variant value;
        retval = UA_Client_readValueAttribute(client, resultMetaDataNodeId, &value);
        if(retval == UA_STATUSCODE_GOOD){
            UA_DataSetMetaDataType_copy((UA_DataSetMetaDataType *) value.data, dsMetaData);
        }
        UA_Variant_clear(&value);
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Found requested Metadata node");
    }
    UA_Client_delete(client);
    return retval;
}


static void
collectDataSetMetaDataFromServer(UA_Server *server, UA_DataSetMetaDataType *pMetaData){
    //init meta data structure
    UA_DataSetMetaDataType_init(pMetaData);
    UA_StatusCode getMetaDataResult = UA_Server_DataSetReader_getMetaDataFromRemote(server,
                                                  UA_STRING(METADATA_SOURCE_SERVER),
                                                  UA_QUALIFIEDNAME(0, METADATA_DATASET_NAME),
                                                  pMetaData);
    if(getMetaDataResult != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Unable to get metadata from remote server");
        return;
    }
    printDataSetMetaDataType(pMetaData);
}

static void add3DPointDataType(UA_Server* server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "3D Point Type");

    UA_Server_addDataTypeNode(
        server, PointType.typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(1, "3D.Point"), attr, NULL, NULL);
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

    /* Add custom Datatypes */
    UA_DataType *types = (UA_DataType*)UA_malloc(sizeof(UA_DataType));
    UA_DataTypeMember *pointMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 3);
    pointMembers[0] = Point_members[0];
    pointMembers[1] = Point_members[1];
    pointMembers[2] = Point_members[2];
    types[0] = PointType;
    types[0].members = pointMembers;

    UA_DataTypeArray customDataTypes = {config->customDataTypes, 1, types};
    config->customDataTypes = &customDataTypes;

    add3DPointDataType(server);

    /* Add the PubSub network layer implementation to the server config.
     * The TransportLayer is acting as factory to create new connections
     * on runtime. Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

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

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_free(pointMembers);
    UA_free(types);
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
