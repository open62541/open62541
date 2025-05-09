/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#define PUBSUB_CONFIG_FIELD_COUNT 10

static UA_NetworkAddressUrlDataType networkAddressUrl =
    {{0, NULL}, UA_STRING_STATIC("opc.udp://224.0.0.22:4840/")};
static UA_String transportProfile =
    UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

/**
 * The main target of this example is to reduce the time spread and effort
 * during the subscribe cycle. This RT level example is based on buffered
 * DataSetMessages and NetworkMessages. Since changes in the
 * PubSub-configuration will invalidate the buffered frames, the PubSub
 * configuration must be frozen after the configuration phase.
 *
 * After enabling the subscriber (and when the first message is received), the
 * NetworkMessages and DataSetMessages will be calculated and buffered. During
 * the subscribe cycle, decoding will happen only to the necessary offsets and
 * the buffered NetworkMessage will only be updated.
 */

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_Server *server;
pthread_t listenThread;
int listenSocket;

UA_DataSetReaderConfig readerConfig;

static void *
listenUDP(void *_) {
    (void)_;

    /* Block SIGINT for correct shutdown via the main thread */
    sigset_t blockset;
    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    sigprocmask(SIG_BLOCK, &blockset, NULL);

    /* Extract the hostname */
    UA_UInt16 port = 0;
    UA_String hostname = UA_STRING_NULL;
    UA_parseEndpointUrl(&networkAddressUrl.url, &hostname, &port, NULL);

    /* Get all the interface and IPv4/6 combinations for the configured hostname */
    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 and IPv6 */
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    /* getaddrinfo */
    char portstr[6];
    char hostnamebuf[256];
    snprintf(portstr, 6, "%d", port);
    memcpy(hostnamebuf, hostname.data, hostname.length);
    hostnamebuf[hostname.length] = 0;
    int result = getaddrinfo(hostnamebuf, portstr, &hints, &info);
    if(result != 0) {
        printf("XXX getaddrinfo failed\n");
        return NULL;
    }

    /* Open the socket */
    listenSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(listenSocket <= 0) {
        printf("XXX Cannot create the socket\n");
        return NULL;
    }

    /* Set socket options */
    int opts = fcntl(listenSocket, F_GETFL);
    result |= fcntl(listenSocket, F_SETFL, opts | O_NONBLOCK);
    int optval = 1;
    result |= setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR,
                         (const char*)&optval, sizeof(optval));
    if(result < 0) {
        printf("XXX Cannot set the socket options\n");
        return NULL;
    }

    /* Bind the socket */
    result = bind(listenSocket, info->ai_addr, (socklen_t)info->ai_addrlen);
    if(result < 0) {
        printf("XXX Cannot bind the socket\n");
        return NULL;
    }

    /* Join the multicast group */
    if(info->ai_family == AF_INET) {
        struct ip_mreqn ipv4;
        struct sockaddr_in *sin = (struct sockaddr_in *)info->ai_addr;
        ipv4.imr_multiaddr = sin->sin_addr;
        ipv4.imr_address.s_addr = htonl(INADDR_ANY); /* default ANY */
        ipv4.imr_ifindex = 0;
        result = setsockopt(listenSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipv4, sizeof(ipv4));
    } else if(info->ai_family == AF_INET6) {
        struct ipv6_mreq ipv6;
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)info->ai_addr;
        ipv6.ipv6mr_multiaddr = sin6->sin6_addr;
        ipv6.ipv6mr_interface = 0; /* default ANY interface */
        result = setsockopt(listenSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &ipv6, sizeof(ipv6));
    }
    if(result < 0) {
        printf("XXX Cannot join the multicast group\n");
        return NULL;
    }

    freeaddrinfo(info);

    /* The connection is open, change the state to OPERATIONAL.
     * The state machine checks whether listenSocket != 0. */
    printf("XXX Listening on UDP multicast (%s, port %u)\n",
           hostnamebuf, (unsigned)port);
    UA_Server_enablePubSubConnection(server, connectionIdentifier);

    /* Poll and process in a loop.
     * The socket is closed in the state machine and */
    struct pollfd pfd;
    pfd.fd = listenSocket;
    pfd.events = POLLIN;
    while(true) {
        result = poll(&pfd, 1, -1); /* infinite timeout */
        if(result < 0 || pfd.revents & POLLERR || pfd.revents & POLLHUP || pfd.revents & POLLNVAL)
            break;

        if(pfd.revents & POLLIN) {
            static char buf[1024];
            ssize_t size = read(listenSocket, buf, sizeof(buf));
            if(size > 0) {
                printf("XXX Received a packet\n");
                UA_ByteString packet = {(size_t)size, (UA_Byte*)buf};
                UA_Server_processPubSubConnectionReceive(server, connectionIdentifier, packet);
            }
        }
    }

    printf("XXX The UDP multicast connection is closed\n");

    /* Clean up and notify the state machine */
    close(listenSocket);
    listenSocket = 0;
    UA_Server_disablePubSubConnection(server, connectionIdentifier);
    return NULL;
}

static UA_StatusCode
connectionStateMachine(UA_Server *server, const UA_NodeId componentId,
                       void *componentContext, UA_PubSubState *state,
                       UA_PubSubState targetState) {
    if(targetState == *state)
        return UA_STATUSCODE_GOOD;
    
    switch(targetState) {
        /* Disabled or Error */
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            printf("XXX Closing the UDP multicast connection\n");
            if(listenSocket != 0)
                shutdown(listenSocket, SHUT_RDWR);
            *state = targetState;
            break;

        /* Operational */
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(listenSocket != 0) {
                *state = UA_PUBSUBSTATE_OPERATIONAL;
                break;
            }
            printf("XXX Opening the UDP multicast connection\n");
            *state = UA_PUBSUBSTATE_PREOPERATIONAL;
            int res = pthread_create(&listenThread, NULL, listenUDP, NULL);
            if(res != 0)
                return UA_STATUSCODE_BADINTERNALERROR;
            break;

        /* Unknown state */
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/* Add new connection to the server */
static void
addPubSubConnection(UA_Server *server) {
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.id.uint32 = UA_UInt32_random();
    connectionConfig.customStateMachine = connectionStateMachine;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);
}

/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                             &readerGroupIdentifier);
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static void
addSubscribedVariables (UA_Server *server) {
    UA_NodeId folderId;
    UA_NodeId newnodeId;
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

    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), folderBrowseName,
                            UA_NS0ID(BASEOBJECTTYPE), oAttr,
                            NULL, &folderId);

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    readerConfig.subscribedDataSet.target.targetVariablesSize =
        readerConfig.dataSetMetaData.fieldsSize;
    readerConfig.subscribedDataSet.target.targetVariables = (UA_FieldTargetDataType*)
        UA_calloc(readerConfig.subscribedDataSet.target.targetVariablesSize,
                  sizeof(UA_FieldTargetDataType));

    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT("en-US", "Subscribed UInt32");
        vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscribed UInt32");
        vAttr.dataType    = UA_TYPES[UA_TYPES_UINT32].typeId;
        // Initialize the values at first to create the buffered NetworkMessage
        // with correct size and offsets
        UA_Variant value;
        UA_Variant_init(&value);
        UA_UInt32 intValue = 0;
        UA_Variant_setScalar(&value, &intValue, &UA_TYPES[UA_TYPES_UINT32]);
        vAttr.value = value;
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                  folderId, UA_NS0ID(HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscribed UInt32"),
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &newnodeId);

        UA_FieldTargetDataType *tv =
            &readerConfig.subscribedDataSet.target.targetVariables[i];
        tv->attributeId  = UA_ATTRIBUTEID_VALUE;
        tv->targetNodeId = newnodeId;
    }
}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING_ALLOC("DataSet Reader 1");
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.expectedEncoding = UA_PUBSUB_RT_RAW;
    readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage =
        UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    dataSetReaderMessage->dataSetMessageContentMask =
        UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;

    UA_ExtensionObject_setValue(&readerConfig.messageSettings, dataSetReaderMessage,
                                &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]);

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    pMetaData->name = UA_STRING_ALLOC("DataSet 1");

    /* Static definition of number of fields size to PUBSUB_CONFIG_FIELD_COUNT
     * to create targetVariables */
    pMetaData->fieldsSize = PUBSUB_CONFIG_FIELD_COUNT;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    for(size_t i = 0; i < pMetaData->fieldsSize; i++) {
        /* UInt32 DataType */
        UA_FieldMetaData_init(&pMetaData->fields[i]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                       &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[i].name =  UA_STRING_ALLOC("UInt32 varibale");
        pMetaData->fields[i].valueRank = -1; /* scalar */
    }

    addSubscribedVariables(server);
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig, &readerIdentifier);
    UA_DataSetReaderConfig_clear(&readerConfig);
}

int main(int argc, char **argv) {
    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            printf("usage: %s <uri> [device]\n", argv[0]);
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
    if(argc > 2)
        networkAddressUrl.networkInterface = UA_STRING(argv[2]);

    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    server = UA_Server_new();

    addPubSubConnection(server);
    addReaderGroup(server);
    addDataSetReader(server);

    UA_Server_enableAllPubSubComponents(server);
    retval = UA_Server_runUntilInterrupt(server);

    UA_Server_delete(server);
    pthread_join(listenThread, NULL);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
