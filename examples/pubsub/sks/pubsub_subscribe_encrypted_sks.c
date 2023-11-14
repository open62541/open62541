/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "common.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define DEMO_SECURITYGROUPNAME "DemoSecurityGroup"
#define SKS_SERVER_DISCOVERYURL "opc.tcp://localhost:4840"

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);

/**
 * Followed by the main server code, making use of the above definitions */
UA_Boolean running = true;
static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
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
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();
    retval |=
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    return retval;
}

static void
sksPullRequestCallback(UA_Server *server, UA_StatusCode sksPullRequestStatus, void *data) {
    if(sksPullRequestStatus != UA_STATUSCODE_GOOD)
        running = false;

    UA_Server_setReaderGroupActivateKey(server, readerGroupIdentifier);
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the
 * DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server, UA_ClientConfig *sksClientConfig) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");

    /* Encryption settings */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    /* we need to set the SecurityGroup for this reader group.
     * It is used to provide access to the security keys on SKS*/
    readerGroupConfig.securityGroupId = UA_STRING(DEMO_SECURITYGROUPNAME);
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    retval |= UA_Server_enableReaderGroup(server, readerGroupIdentifier);
    /* We need to set the sks client before setting Reader/Writer Group into operational
     * because it will fetch the initial set of keys. The sks client is required to set
     * once per security group on publisher/subscriber application.*/
    UA_Server_setSksClient(server, readerGroupConfig.securityGroupId, (UA_ClientConfig *)sksClientConfig,
                           SKS_SERVER_DISCOVERYURL, sksPullRequestCallback, NULL);

    return retval;
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
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

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables(UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

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

    /**
     * **TargetVariables**
     *
     * The SubscribedDataSet option TargetVariables defines a list of Variable mappings
     * between received DataSet fields and target Variables in the Subscriber
     * AddressSpace. The values subscribed from the Publisher are updated in the value
     * field of these variables */
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)UA_calloc(
        readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(
            server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000), folderId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(
        server, dataSetReaderId, readerConfig.dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/**
 * **DataSetMetaData**
 *
 * The DataSetMetaData describes the content of a DataSet. It provides the information
 * necessary to decode DataSetMessages on the Subscriber side. DataSetMessages received
 * from the Publisher are decoded into DataSet and each field is updated in the Subscriber
 * based on datatype match of TargetVariable fields of Subscriber and
 * PublishedDataSetFields of Publisher */
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

/*
 * TODO: add something similary ass addDataSetMetadata for security configuration
 */

static int
run(UA_UInt16 port, UA_String *transportProfile,
    UA_NetworkAddressUrlDataType *networkAddressUrl, UA_ClientConfig *sksClientConfig) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, port, NULL);

    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server, sksClientConfig);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if(retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ClientConfig_delete(sksClientConfig);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static UA_ClientConfig *
encyrptedClient(const char *username, const char *password,
                UA_ByteString certificate, UA_ByteString privateKey) {
    UA_ClientConfig *cc = (UA_ClientConfig *)UA_calloc(1, sizeof(UA_ClientConfig));

    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey, 0, 0, NULL, 0);

    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
    identityToken->userName = UA_STRING_ALLOC(username);
    identityToken->password = UA_STRING_ALLOC(password);
    UA_ExtensionObject_clear(&cc->userIdentityToken);
    cc->userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
    cc->userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
    cc->userIdentityToken.content.decoded.data = identityToken;

    return cc;
}

static void
usage(char *progname) {
    printf("usage: %s  server_ctt <server-certificate.der> <private-key.der>\n"
           "<uri> [device]\n",
           progname);
}

int
main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};

    UA_UInt16 port = 4801;

    /* Load certificate */
    size_t pos = 1;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        certificate = loadFile(argv[1]);
        if(certificate.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

    /* Load the private key */
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    if((size_t)argc >= pos + 1) {
        privateKey = loadFile(argv[2]);
        if(privateKey.length == 0) {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Unable to load file %s.", argv[pos]);
            return EXIT_FAILURE;
        }
        pos++;
    }

    if(argc > 3) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if(strncmp(argv[pos], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[pos]);
        } else if(strncmp(argv[pos], "opc.eth://", 10) == 0) {
            transportProfile = UA_STRING(
                "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if(argc < 4) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[pos + 1]);
            networkAddressUrl.url = UA_STRING(argv[pos]);
        } else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }
    /* we need to setup a client with encrypted connection and user credentials
     * that can be trusted by the SKS Server instance.
     * This Client will be used by Publisher/Subscriber application to regularly
     * fetch the security keys from SKS.*/
    UA_ClientConfig *sksClientConfig =
        encyrptedClient("user1", "password", certificate, privateKey);

    return run(port, &transportProfile, &networkAddressUrl, sksClientConfig);
}
