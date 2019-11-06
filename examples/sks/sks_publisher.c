/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * documentation not really included, placeholders
 * .. _sksPulisher_tutorial:
 * this is an example PubSubCode working with securityEnhancement, based on "tutorial_pubsub_publish.c"
 * make sure an instance of sks_server is running before run this code
 * it will:
 * 1. connect to SKS server with dedicated username and password
 * 2. add a securityGroup with name "SecurityGroup1"
 * 3. create a writerGroup with securityGroupId="SecurityGroup1"
 * 4. call "getSecurityKeys" method on sks server
 * 5. encrypt and sign PubMessages using the key
 * 6. update key regularly and pull keys  from SKS server when there is no new key available
 * 
 * usage of  marcos:
 * UA_MESSAGESECURITYMODE_TESTLEVEL to set securityLevel
 * UA_PUBSUB_SECURITYPOLICY_TEST to define the securityPolicyUri(now only supports aes128ctr and aes256ctr)
 * UA_SETSECURITYKEYS_TEST will manually set keys of zero bytes only for test 
 */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_sks.h>

#include <signal.h>

#include "common.h"
#include <unistd.h>
#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_SIGNANDENCRYPT
/*#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_NONE */
/*#define UA_MESSAGESECURITYMODE_TESTLEVEL UA_MESSAGESECURITYMODE_SIGN */
#define UA_SETSECURITYKEYS_TEST UA_FALSE
#define UA_PUBSUB_SECURITYPOLICY_TEST "http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR"
UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;
/*SKS related */
static UA_StatusCode
addSecurityGroup(UA_Client *client, size_t *outputsize,
                 UA_Variant **addSecurityGroupOutput) {
    /*Add SecurityGroup */
    size_t inputsize = 5;
    UA_Variant *addSecurityGroupInput =
        (UA_Variant *)UA_calloc(inputsize, sizeof(UA_Variant));
    UA_String securityGroupName = UA_STRING("SecurityGroup1");
    /*input just for tests*/
    UA_Duration keyLifetime = 10000;
    UA_String securityPolicyUri =
        UA_STRING(UA_PUBSUB_SECURITYPOLICY_TEST);
    UA_UInt32 maxFutureKeyCount = 3;
    UA_UInt32 maxPastKeyCount = 5;
    UA_Variant_setScalarCopy(&addSecurityGroupInput[0], &securityGroupName,
                             &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&addSecurityGroupInput[1], &keyLifetime,
                             &UA_TYPES[UA_TYPES_DURATION]);
    UA_Variant_setScalarCopy(&addSecurityGroupInput[2], &securityPolicyUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalarCopy(&addSecurityGroupInput[3], &maxFutureKeyCount,
                             &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&addSecurityGroupInput[4], &maxPastKeyCount,
                             &UA_TYPES[UA_TYPES_UINT32]);

    UA_StatusCode ret =
        UA_Client_call(client, NODEID_SKS_SecurityRootFolder,
                       NODEID_SKS_AddSecurityGroup, inputsize,
                       addSecurityGroupInput, outputsize, addSecurityGroupOutput);
    UA_Array_delete(addSecurityGroupInput, inputsize, &UA_TYPES[UA_TYPES_VARIANT]);
    return ret;
}

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
		connectionConfig.publisherId.numeric = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
     * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                  &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId =
        UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig,
                              &dataSetFieldIdent);
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static UA_StatusCode
addWriterGroup(UA_Server *server, UA_Client *client) {
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 3000;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
		writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
    /*SKS and Security Parameters*/
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_TESTLEVEL;
    writerGroupConfig.securityParameters.securityGroupId = UA_STRING("SecurityGroup1");
    writerGroupConfig.securityParameters.getSecurityKeysEnabled = UA_TRUE;
    writerGroupConfig.sksConfig.client=client;
	/* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    /* Change message settings of writerGroup to send PublisherId,
     * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
     * of NetworkMessage */
    writerGroupMessage->networkMessageContentMask          = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                              (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    UA_StatusCode ret=UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig,
                                 &writerGroupIdent);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
    return ret;
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static void
addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

static UA_StatusCode
connectToSKS(UA_Client *client,UA_String clientCertPath,UA_String clientKeyPath) {
    // UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* Load certificate and private key */
    UA_ByteString certificate = loadFile((char*)clientCertPath.data);
    UA_ByteString privateKey = loadFile((char*)clientKeyPath.data);

    /* Load the trustList. Load revocationList is not supported now */

    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;
    size_t trustListSize = 0;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);

    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_ClientConfig_setDefaultEncryption(config, certificate, privateKey, trustList,
                                         trustListSize, revocationList,
                                         revocationListSize);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++) {
        UA_ByteString_clear(&trustList[deleteCount]);
    }
    return UA_Client_connect_username(client, "opc.tcp://localhost:4841", "peter",
                                      "peter123");
}
/**
 * That's it! You're now publishing the selected fields. Open a packet
 * inspection tool of trust e.g. wireshark and take a look on the outgoing
 * packages. The following graphic figures out the packages created by this
 * tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process
 * the the datagrams, take a look on the ua_network_pubsub_networkmessage.c
 * which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */
UA_Boolean running = true;
static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl,UA_String clientCertPath,UA_String clientKeyPath) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privatekey = UA_BYTESTRING_NULL;
    UA_ServerConfig_addSecurityPolicy_Pubsub_Aes128ctr(config, &certificate, &privatekey);
    UA_ServerConfig_addSecurityPolicy_Pubsub_Aes256ctr(config,&certificate,&privatekey);
    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
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
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Client *client = UA_Client_new();
    retval = connectToSKS(client,clientCertPath,clientKeyPath);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_Variant *addSecurityGroupOutput;
    size_t outputsize;
    UA_Boolean deleteOutputs = UA_TRUE;
    retval = addSecurityGroup(client, &outputsize, &addSecurityGroupOutput);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADNODEIDEXISTS){
            deleteOutputs = UA_FALSE;
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                           "addSecurityGroupWarn:%s", UA_StatusCode_name(retval));
        }
        else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "addSecurityGroupfailed:%s", UA_StatusCode_name(retval));
            goto cleanup;
        }
    }

    if(deleteOutputs)
        UA_Array_delete(addSecurityGroupOutput, outputsize, &UA_TYPES[UA_TYPES_VARIANT]);

    if(UA_SETSECURITYKEYS_TEST) {
        retval = UA_Server_addPubSubSKSPush(server);
        if (retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /*SKS part End */
    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    retval = addWriterGroup(server, client);
    if(retval != UA_STATUSCODE_GOOD) 
        goto cleanup;
        

    addDataSetWriter(server);

    retval = UA_Server_run(server, &running);
cleanup:
    UA_Server_delete(server);
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
    UA_String clientCertPath=UA_STRING_NULL;
    UA_String clientKeyPath=UA_STRING_NULL;
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if(argc>3)
        {
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
        clientCertPath=UA_STRING(argv[1]);
        clientKeyPath=UA_STRING(argv[2]);
        
    }

    return run(&transportProfile, &networkAddressUrl,clientCertPath,clientKeyPath);
}
