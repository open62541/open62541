/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>

#include <signal.h>

#include "common.h"

#define DEMO_SECURITYGROUPNAME "DemoSecurityGroup"
#define SKS_SERVER_DISCOVERYURL "opc.tcp://localhost:4840"

UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;

UA_Boolean running = true;
static void
stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

static void
addPublishedDataSet(UA_Server *server) {
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                  &publishedDataSetIdent);
}

static void
addDataSetField(UA_Server *server) {
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

static void
sksPullRequestCallback(UA_Server *server, UA_StatusCode sksPullRequestStatus, void *data) {
    UA_PubSubState state = UA_PUBSUBSTATE_OPERATIONAL;
    UA_Server_WriterGroup_getState(server, writerGroupIdent, &state);
    if(sksPullRequestStatus == UA_STATUSCODE_GOOD && state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_Server_setWriterGroupActivateKey(server, writerGroupIdent);
}

static void
addWriterGroup(UA_Server *server, UA_ClientConfig *sksClientConfig) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];

    /* Encryption settings */
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    /* we need to set the SecurityGroup for this reader group.
     * It is used to provide access to the security keys on SKS*/
    writerGroupConfig.securityGroupId = UA_STRING(DEMO_SECURITYGROUPNAME);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];

    UA_UadpWriterGroupMessageDataType *writerGroupMessage =
        UA_UadpWriterGroupMessageDataType_new();
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                           (UA_UadpNetworkMessageContentMask)
                                               UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig,
                             &writerGroupIdent);
    UA_Server_enableWriterGroup(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);

    /* We need to set the sks client before setting Reader/Writer Group into operational
     * because it will fetch the initial set of keys. The sks client is required to set
     * once per security group on publisher/subscriber application.*/
    UA_Server_setSksClient(server, writerGroupConfig.securityGroupId, sksClientConfig,
                           SKS_SERVER_DISCOVERYURL, sksPullRequestCallback, NULL);
}

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


static int
run(UA_UInt16 port, UA_String *transportProfile,
    UA_NetworkAddressUrlDataType *networkAddressUrl, UA_ClientConfig *sksClientConfig) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, port, NULL);

    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
    UA_PubSubSecurityPolicy_Aes256Ctr(config->pubSubConfig.securityPolicies,
                                      config->logging);

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server, sksClientConfig);
    addDataSetWriter(server);

    UA_StatusCode retval = UA_Server_run(server, &running);
    
    UA_ClientConfig_delete(sksClientConfig);
    UA_Server_delete(server);
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
    UA_UInt16 port = 4841;
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
