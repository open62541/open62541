/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub Subscriber API is currently not finished. This example can be used
 * to receive and display values that are published by tutorial_pubsub_publish
 * example in the TargetVariables of Subscriber Information Model .
 */
/**
 * documentation not really included, placeholders
 * .. _sksSubscriber_tutorial:
 * this is an example PubSubCode working with securityEnhancement, based on "tutorial_pubsub_subscriber.c"
 * make sure an instance of sks_server is running before run this code
 * it will:
 * 1. connect to SKS server with dedicated username and password
 * 2. create a readerGroup with securityGroupId="SecurityGroup1"
 * 3. call "getSecurityKeys" method on sks server
 * 4. decrypt and verify PubMessages using the key
 * 5. pull keys regularly from SKS server / pull keys when received msg with older keys
 * 
 * usage of marcos:
 * UA_MESSAGESECURITYMODE_TESTLEVEL to set securityLevel
 * UA_PUBSUB_SECURITYPOLICY_TEST to define the securityPolicyUri(now only supports aes128ctr and aes256ctr)
 * UA_SETSECURITYKEYS_TEST will manually set keys of zero bytes only for test 
 */
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include "ua_pubsub.h"

#include <open62541/client.h>
#include <open62541/client_config_default.h>

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_SIGNANDENCRYPT
//#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_NONE
//#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_SIGN
#define UA_SETSECURITYKEYS_TEST UA_FALSE

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);

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
addReaderGroup(UA_Server *server, UA_Client *client) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    /*Config securityparameters*/
    readerGroupConfig.securityParameters.securityGroupId = UA_STRING("SecurityGroup1");
    readerGroupConfig.securityParameters.securityMode = UA_MESSAGESECURITYMODE_TESTLEVEL;
    readerGroupConfig.securityParameters.getSecurityKeysEnabled = UA_TRUE;
    readerGroupConfig.sksConfig.client = client;

    UA_StatusCode ret = UA_Server_addReaderGroup(
        server, connectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
    return ret;
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

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);
    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables(UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    }

    UA_Server_addObjectNode(
        server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), folderBrowseName,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    retval |= UA_Server_DataSetReader_addTargetVariables(
        server, &folderId, dataSetReaderId, UA_PUBSUB_SDS_TARGET);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/* Define MetaData for TargetVariables */
static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name = UA_STRING("DataSet 1");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData *)UA_Array_new(
        pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* DateTime DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].name = UA_STRING("DateTime");
    pMetaData->fields[0].valueRank = -1; /* scalar */

    /* Int32 DataType */
    UA_FieldMetaData_init(&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[1].name = UA_STRING("Int32");
    pMetaData->fields[1].valueRank = -1; /* scalar */

    /* Int64 DataType */
    UA_FieldMetaData_init(&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId, &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
    pMetaData->fields[2].name = UA_STRING("Int64");
    pMetaData->fields[2].valueRank = -1; /* scalar */

    /* Boolean DataType */
    UA_FieldMetaData_init(&pMetaData->fields[3]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BOOLEAN].typeId, &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[3].name = UA_STRING("BoolToggle");
    pMetaData->fields[3].valueRank = -1; /* scalar */
}

UA_Boolean running = true;
static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}
static UA_StatusCode
connectToSKS(UA_Client *client, UA_String clientCertPath, UA_String clientKeyPath) {
    // UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Load certificate and private key */
    UA_ByteString certificate = loadFile((char *)clientCertPath.data);
    UA_ByteString privateKey = loadFile((char *)clientKeyPath.data);

    /* Load the trustList. Load revocationList is not supported now */

    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;
    size_t trustListSize = 0;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_StatusCode ret = UA_ClientConfig_setDefaultEncryption(config, certificate, privateKey, trustList,
                                         trustListSize, revocationList,
                                         revocationListSize);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }


    if (ret != UA_STATUSCODE_GOOD)
        return ret;


    return UA_Client_connect_username(client, "opc.tcp://localhost:4841", "peter",
                                      "peter123");
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl,
    UA_String clientCertPath, UA_String clientKeyPath) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4801, NULL);
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privatekey = UA_BYTESTRING_NULL;
    UA_ServerConfig_addSecurityPolicy_Pubsub_Aes128ctr(config, &certificate, &privatekey);
    UA_ServerConfig_addSecurityPolicy_Pubsub_Aes256ctr(config, &certificate, &privatekey);
    /* Add the PubSub network layer implementation to the server config.
     * The TransportLayer is acting as factory to create new connections
     * on runtime. Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    config->pubsubTransportLayers =
        (UA_PubSubTransportLayer *)UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    /*Connect To SKS */
    UA_Client *client = UA_Client_new();
    retval = connectToSKS(client, clientCertPath, clientKeyPath);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    
    /*Test setSecurityKeys*/
    if(UA_SETSECURITYKEYS_TEST) {
        retval = UA_Server_addPubSubSKSPush(server);
        if (retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server, client);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_Server_run(server, &running);
cleanup:
    UA_Server_delete(server);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", UA_StatusCode_name(retval));
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <clientCertPath> <clientKeyPath> <uri> [device]\n", progname);
}

int
main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_String clientCertPath = UA_STRING_NULL;
    UA_String clientKeyPath = UA_STRING_NULL;
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if(argc > 3) {
            if(strncmp(argv[3], "opc.udp://", 10) == 0) {
                networkAddressUrl.url = UA_STRING(argv[1]);
            } else if(strncmp(argv[3], "opc.eth://", 10) == 0) {
                transportProfile = UA_STRING(
                    "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
                if(argc < 3) {
                    printf("Error: UADP/ETH needs an interface name\n");
                    return EXIT_FAILURE;
                }
                networkAddressUrl.networkInterface = UA_STRING(argv[4]);
                networkAddressUrl.url = UA_STRING(argv[3]);
            } else {
                printf("Error: unknown URI\n");
                return EXIT_FAILURE;
            }
        }
        clientCertPath = UA_STRING(argv[1]);
        clientKeyPath = UA_STRING(argv[2]);
    }

    return run(&transportProfile, &networkAddressUrl, clientCertPath, clientKeyPath);
}
