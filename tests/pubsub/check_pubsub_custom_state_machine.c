/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#define PUBSUB_CONFIG_PUBLISH_CYCLE_MS 100
#define PUBSUB_CONFIG_FIELD_COUNT 10

static int listenSocket;
static UA_Server *server;
static pthread_t listenThread;
static timer_t writerGroupTimer;
static UA_DataSetReaderConfig readerConfig;
static UA_NodeId publishedDataSetIdent, dataSetFieldIdent, writerGroupIdent,
                 connectionIdentifier, readerGroupIdentifier, readerIdentifier;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static UA_NetworkAddressUrlDataType networkAddressUrl =
    {{0, NULL}, UA_STRING_STATIC("opc.udp://224.0.0.22:4840/")};
static UA_String transportProfile =
    UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

/* Values in static locations. We cycle the dvPointers double-pointer to the
 * next with atomic operations. */
UA_UInt32 valueStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue dvStore[PUBSUB_CONFIG_FIELD_COUNT];
UA_DataValue *dvPointers[PUBSUB_CONFIG_FIELD_COUNT];

static void
valueUpdateCallback(UA_Server *server, void *data) {
    for(int i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; ++i) {
        if(dvPointers[i] < &dvStore[PUBSUB_CONFIG_FIELD_COUNT - 1])
            UA_atomic_xchg((void**)&dvPointers[i], dvPointers[i]+1);
        else
            UA_atomic_xchg((void**)&dvPointers[i], &dvStore[0]);
    }
}

/* WriterGroup timer managed by a custom state machine. This uses
 * UA_Server_triggerWriterGroupPublish. The server can block its internal mutex,
 * so this can have some jitter. For hard realtime the publish callback has to
 * send out the packet without going through the server. */

static void
writerGroupPublishTrigger(union sigval signal) {
    printf("XXX Publish Callback\n");
    UA_Server_triggerWriterGroupPublish(server, writerGroupIdent);
}

static UA_StatusCode
writerGroupStateMachine(UA_Server *server, const UA_NodeId componentId,
                        void *componentContext, UA_PubSubState *state,
                        UA_PubSubState targetState) {
    UA_WriterGroupConfig config;
    struct itimerspec interval;
    memset(&interval, 0, sizeof(interval));

    if(targetState == *state)
        return UA_STATUSCODE_GOOD;
    
    switch(targetState) {
        /* Disabled or Error */
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            printf("XXX Disabling the WriterGroup\n");
            timer_settime(writerGroupTimer, 0, &interval, NULL);
            *state = targetState;
            break;

        /* Operational */
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            if(*state == UA_PUBSUBSTATE_OPERATIONAL)
                break;
            printf("XXX Enabling the WriterGroup\n");
            UA_Server_getWriterGroupConfig(server, writerGroupIdent, &config);
            interval.it_interval.tv_sec = config.publishingInterval / 1000;
            interval.it_interval.tv_nsec =
                ((long long)(config.publishingInterval * 1000 * 1000)) % (1000 * 1000 * 1000);
            interval.it_value = interval.it_interval;
            UA_WriterGroupConfig_clear(&config);
            int res = timer_settime(writerGroupTimer, 0, &interval, NULL);
            if(res != 0)
                return UA_STATUSCODE_BADINTERNALERROR;
            *state = UA_PUBSUBSTATE_OPERATIONAL;
            break;

        /* Unknown state */
        default:
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

/* Add the custom state machine to the writer group */
static UA_StatusCode
testComponentLifecycleCallback(UA_Server *server, const UA_NodeId id,
                               const UA_PubSubComponentType componentType,
                               UA_Boolean remove) {
    if(remove)
        return UA_STATUSCODE_GOOD;
    if(componentType != UA_PUBSUBCOMPONENT_WRITERGROUP)
        return UA_STATUSCODE_GOOD;

    printf("XXX Set the custom state machine for the new WriterGroup\n");

    UA_WriterGroupConfig c;
    UA_StatusCode res = UA_Server_getWriterGroupConfig(server, id, &c);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    c.customStateMachine = writerGroupStateMachine;
    res = UA_Server_updateWriterGroupConfig(server, id, &c);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig_clear(&c);

    return UA_STATUSCODE_GOOD;
}

START_TEST(CustomPublisher) {
    /* Prepare the values */
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        valueStore[i] = (UA_UInt32) i + 1;
        UA_Variant_setScalar(&dvStore[i].value, &valueStore[i], &UA_TYPES[UA_TYPES_UINT32]);
        dvStore[i].hasValue = true;
        dvPointers[i] = &dvStore[i];
    }

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.componentLifecycleCallback = testComponentLifecycleCallback;

    /* Initialize the timer */
    struct sigevent sigev;
    memset(&sigev, 0, sizeof(sigev));
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = writerGroupPublishTrigger;
    timer_create(CLOCK_REALTIME, &sigev, &writerGroupTimer);

    /* Add a PubSubConnection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = 2234;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentifier);

    /* Add a PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);

    /* Add DataSetFields with static value source to PDS */
    UA_DataSetFieldConfig dsfConfig;
    for(size_t i = 0; i < PUBSUB_CONFIG_FIELD_COUNT; i++) {
        /* TODO: Point to a variable in the information model */
        memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
        UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
    }

    /* Add a WriterGroup */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUBSUB_CONFIG_PUBLISH_CYCLE_MS;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;

    /* Change message settings of writerGroup to send PublisherId, WriterGroupId
     * in GroupHeader and DataSetWriterId in PayloadHeader of NetworkMessage */
    UA_UadpWriterGroupMessageDataType writerGroupMessage;
    UA_UadpWriterGroupMessageDataType_init(&writerGroupMessage);
    writerGroupMessage.networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    UA_ExtensionObject_setValue(&writerGroupConfig.messageSettings, &writerGroupMessage,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);

    UA_Server_addWriterGroup(server, connectionIdentifier, &writerGroupConfig, &writerGroupIdent);

    /* Add a DataSetWriter to the WriterGroup */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    dataSetWriterConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    UA_UadpDataSetWriterMessageDataType uadpDataSetWriterMessageDataType;
    UA_UadpDataSetWriterMessageDataType_init(&uadpDataSetWriterMessageDataType);
    uadpDataSetWriterMessageDataType.dataSetMessageContentMask =
        UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    UA_ExtensionObject_setValue(&dataSetWriterConfig.messageSettings,
                                &uadpDataSetWriterMessageDataType,
                                &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]);

    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);

    UA_Server_enableAllPubSubComponents(server);

    /* Add a callback that updates the value */
    UA_UInt64 callbackId;
    UA_Server_addRepeatedCallback(server, valueUpdateCallback, NULL,
                                  PUBSUB_CONFIG_PUBLISH_CYCLE_MS, &callbackId);

    UA_fakeSleep(PUBSUB_CONFIG_PUBLISH_CYCLE_MS);
    UA_Server_run_iterate(server, true);
    UA_fakeSleep(PUBSUB_CONFIG_PUBLISH_CYCLE_MS);
    UA_Server_run_iterate(server, true);

    /* Cleanup */
    UA_Server_run_shutdown(server);
    UA_StatusCode rv = UA_Server_delete(server);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    timer_delete(writerGroupTimer);
} END_TEST

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
        if(pfd.revents & POLLERR || pfd.revents & POLLHUP || pfd.revents & POLLNVAL)
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

/* Define MetaData for TargetVariables */
static void
fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL)
        return;

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to PUBSUB_CONFIG_FIELD_COUNT
     * to create targetVariables */
    pMetaData->fieldsSize = PUBSUB_CONFIG_FIELD_COUNT;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    for(size_t i = 0; i < pMetaData->fieldsSize; i++) {
        /* UInt32 DataType */
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId,
                       &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = UA_NS0ID_UINT32;
        pMetaData->fields[i].name =  UA_STRING ("UInt32 varibale");
        pMetaData->fields[i].valueRank = -1; /* scalar */
    }
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
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NS0ID(OBJECTSFOLDER),
                            UA_NS0ID(ORGANIZES), folderBrowseName,
                            UA_NS0ID(BASEOBJECTTYPE), oAttr,
                            NULL, &folderId);

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    /* Create the TargetVariables with respect to DataSetMetaData fields */
    readerConfig.subscribedDataSet.target.targetVariablesSize = readerConfig.dataSetMetaData.fieldsSize;
    readerConfig.subscribedDataSet.target.targetVariables = (UA_FieldTargetDataType*)
        UA_calloc(readerConfig.subscribedDataSet.target.targetVariablesSize, sizeof(UA_FieldTargetDataType));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
        vAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed UInt32");
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

        UA_FieldTargetDataType *tv = &readerConfig.subscribedDataSet.target.targetVariables[i];
        tv->attributeId  = UA_ATTRIBUTEID_VALUE;
        tv->targetNodeId = newnodeId;
    }
}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    UA_UInt16 publisherIdentifier = 2234;
    readerConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    readerConfig.publisherId.id.uint16 = publisherIdentifier;
    readerConfig.writerGroupId    = 100;
    readerConfig.dataSetWriterId  = 62541;
    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage =
        UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    dataSetReaderMessage->dataSetMessageContentMask = UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;

    readerConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

    addSubscribedVariables(server);
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig, &readerIdentifier);

    UA_free(readerConfig.subscribedDataSet.target.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}

/* Add the custom state machine to the subscriber connection */
static UA_StatusCode
subscriberComponentLifecycleCallback(UA_Server *server, const UA_NodeId id,
                                     const UA_PubSubComponentType componentType,
                                     UA_Boolean remove) {
    if(remove)
        return UA_STATUSCODE_GOOD;
    if(componentType != UA_PUBSUBCOMPONENT_CONNECTION)
        return UA_STATUSCODE_GOOD;

    printf("XXX Set the custom state machine for the new Connction\n");

    UA_PubSubConnectionConfig cc;
    UA_StatusCode res = UA_Server_getPubSubConnectionConfig(server, id, &cc);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    cc.customStateMachine = connectionStateMachine;
    res = UA_Server_updatePubSubConnectionConfig(server, id, &cc);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubConnectionConfig_clear(&cc);

    return UA_STATUSCODE_GOOD;
}

START_TEST(CustomSubscriber) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.componentLifecycleCallback = subscriberComponentLifecycleCallback;

    addPubSubConnection(server);
    addReaderGroup(server);
    addDataSetReader(server);
    UA_Server_enableAllPubSubComponents(server);

    UA_fakeSleep(PUBSUB_CONFIG_PUBLISH_CYCLE_MS);
    UA_Server_run_iterate(server, true);
    UA_fakeSleep(PUBSUB_CONFIG_PUBLISH_CYCLE_MS);
    UA_Server_run_iterate(server, true);

    UA_Server_run_shutdown(server);

    pthread_join(listenThread, NULL);

    UA_Server_delete(server);
} END_TEST

int main(void) {
    TCase *tc_custom = tcase_create("Custom State Machine");
    tcase_add_checked_fixture(tc_custom, setup, NULL);
    tcase_add_test(tc_custom, CustomPublisher);
    tcase_add_test(tc_custom, CustomSubscriber);

    Suite *s = suite_create("PubSub Custom State Machine");
    suite_add_tcase(s, tc_custom);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
