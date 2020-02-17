/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * **Trace point setup**
 *
 *            +--------------+                        +----------------+
 *         T1 | OPCUA PubSub |  T8                 T5 | OPCUA loopback |  T4
 *         |  |  Application |  ^                  |  |  Application   |  ^
 *         |  +--------------+  |                  |  +----------------+  |
 *  User   |  |              |  |                  |  |                |  |
 *  Space  |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *------------|--------------|------------------------|----------------|--------
 *         |  |    Node 1    |  |                  |  |     Node 2     |  |
 *  Kernel |  |              |  |                  |  |                |  |
 *  Space  |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *         v  +--------------+  |                  v  +----------------+  |
 *         T2 |  TX tcpdump  |  T7<----------------T6 |   RX tcpdump   |  T3
 *         |  +--------------+                        +----------------+  ^
 *         |                                                              |
 *         ----------------------------------------------------------------
 */
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/io.h>

/* For thread operations */
#include <pthread.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <ua_server_internal.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/log.h>
#include <open62541/types_generated.h>
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include "ua_pubsub.h"

UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

/* to find load of each thread
 * ps -L -o pid,pri,%cpu -C TSN_ETF_publisher */

/* These defines enables the publisher and subscriber of the OPCUA stack */
/* To run only publisher, enable PUBLISHER define alone (comment SUBSCRIBER) */
#define             PUBLISHER
/* To run only subscriber, enable SUBSCRIBER define alone (comment PUBLISHER) */
#define             SUBSCRIBER
//#define             UPDATE_MEASUREMENTS
#define             UA_ENABLE_STATICVALUESOURCE
/* Milli sec and sec conversion to nano sec */
#define             MILLI_SECONDS                         1000 * 1000
#define             SECONDS                               1000 * 1000 * 1000
/* Cycle time in milliseconds */
#define             CYCLE_TIME                            0.25
#define             SECONDS_SLEEP                         5
/* Publisher will sleep for 60% of cycle time and then prepares the */
/* transmission packet within 40% */
#define             NANO_SECONDS_SLEEP_PUB                CYCLE_TIME * MILLI_SECONDS * 0.6
/* Subscriber will wakeup only during start of cycle and check whether */
/* the packets are received */
#define             NANO_SECONDS_SLEEP_SUB                0
/* User application Pub/Sub will wakeup at the 30% of cycle time and handles the */
/* user data such as read and write in Information model */
#define             NANO_SECONDS_SLEEP_USER_APPLICATION   CYCLE_TIME * MILLI_SECONDS * 0.3
/* Priority of Publisher, Subscriber, User application and server are kept */
/* after some prototyping and analyzing it */
#define             PUB_SCHED_PRIORITY                    78
#define             SUB_SCHED_PRIORITY                    81
#define             USERAPPLICATION_SCHED_PRIORITY        75
#define             SERVER_SCHED_PRIORITY                 1
#if defined(PUBLISHER)
#define             PUBLISHER_ID                          2234
#define             WRITER_GROUP_ID                       101
#define             DATA_SET_WRITER_ID                    62541
#define             PUBLISHING_INTERFACE                  "enp4s0"
#define             PUBLISHING_MAC_ADDRESS                "opc.eth://01-00-5E-7F-00-01"
#endif
#if defined(SUBSCRIBER)
#define             PUBLISHER_ID_SUB                      2235
#define             WRITER_GROUP_ID_SUB                   100
#define             DATA_SET_WRITER_ID_SUB                62541
#define             SUBSCRIBING_INTERFACE                 "enp4s0"
#define             SUBSCRIBING_MAC_ADDRESS               "opc.eth://01-00-5E-00-00-01"
#endif
#define             KEY_FRAME_COUNT                       10
#define             MAX_MEASUREMENTS                      30000000
#define             CORE_TWO                              2
#define             CORE_THREE                            3
#define             SECONDS_INCREMENT                     1
#define             CONNECTION_NUMBER                     2
#define             PORT_NUMBER                           62541
#define             REPEATED_NODECOUNTS                   0
#define             FAILURE_EXIT                          -1
#define             CLOCKID                               CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             UDP_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
#define             PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS

/* This is a hardcoded publisher/subscriber IP address. If the IP address need
 * to be changed for running over UDP, change it in the below lines.
 * If UA_ENABLE_PUBSUB_REALTIME_PUBLISH_ETF is enabled,
 * also change PUBSUB_IP_ADDRESS  in plugins/include/open62541/plugin/pubsub_realtime_etf.h
*/

#define             PUBSUB_IP_ADDRESS              "192.168.8.105"
#define             PUBLISHER_MULTICAST_ADDRESS    "opc.udp://224.0.0.32:4840/"
#define             SUBSCRIBER_MULTICAST_ADDRESS   "opc.udp://224.0.0.22:4840/"

/* If UA_ENABLE_PUBSUB_REALTIME_PUBLISH_ETF and UA_ENABLE_PUBSUB_ETH_UADP is enabled,
 * If the Hardcoded publisher/subscriber MAC addresses need to be changed,
 * change PUBLISHING_MAC_ADDRESS and SUBSCRIBING_MAC_ADDRESS,
 * If the Hardcoded interface name need to be changed,
 * change PUBLISHING_INTERFACE and SUBSCRIBING_INTERFACE
 * To pass the MAC addresses as arguments,
 * Two nodes connected in peer-to-peer network - Run TSN_ETF_publisher.c in node1 and TSN_ETF_loopback.c in node2
 * use the command  ./executable "opc.eth://publisher_mac(MAC of node2)" "opc.eth://subscriber_mac(MAC of node 1)" interface_name
 * To run only subscriber, use the command ./executable "opc.eth://publisher_mac(MAC of node 2)" interface_name
 * Interface_name - i210 NIC interface */

/* When the timer was created */
struct timespec     pubStartTime;
/* Set server running as true */
UA_Boolean          running                = UA_TRUE;
/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_NodeId           connectionIdent;
UA_NodeId           publishedDataSetIdent;
UA_NodeId           writerGroupIdent;
UA_NodeId           pubNodeID;
UA_NodeId           subNodeID;
UA_NodeId           pubRepeatedCountNodeID;
UA_NodeId           subRepeatedCountNodeID;
/* Variable for PubSub callback */
UA_ServerCallback   pubCallback;
/* Variables for counter data handling in address space */
UA_UInt64           pubCounterData       = 0;
UA_UInt64           repeatedCounter      = 10;
UA_UInt64           subCounterData       = 0;
UA_Variant          pubCounter;
UA_Variant          subCounter;

/* For adding nodes in the server information model */
static void addServerNodes(UA_Server *server);

/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);

#if defined(PUBLISHER)

#if defined(UPDATE_MEASUREMENTS)
/* File to store the data and timestamps for different traffic */
FILE               *fpPublisher;
char               *filePublishedData      = "publisher_T1.csv";

/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;

/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];
#endif

/* Thread for publisher */
pthread_t           pubthreadID;

struct timespec     dataModificationTime;
UA_WriterGroup      *currentWriterGroup;
/* Publisher thread routine for ETF */
void               *publisherETF(void *arg);
#endif

#if defined(SUBSCRIBER)

#if defined(UPDATE_MEASUREMENTS)
/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileSubscribedData     = "subscriber_T8.csv";

/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;

/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];
#endif

/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
UA_ReaderGroup     *currentReaderGroup;

/* Thread for subscriber */
pthread_t           subthreadID;

struct timespec     dataReceiveTime;
/* Subscriber thread routine */
void               *subscriber(void *arg);

#endif
/* Thread for server */
pthread_t           serverThreadID;

typedef struct {
UA_Server*                   ServerRun;
} serverConfigStruct;

/* Structure to define thread parameters */
typedef struct {
UA_Server*                   server;
void*                        data;
UA_ServerCallback            callback;
UA_Duration                  interval_ms;
UA_UInt64*                   callbackId;
} threadArg;

static pthread_t threadCreation(UA_Int16 threadPriority, size_t coreAffinity, void *(*thread) (void *),
                                char *applicationName, void *serverConfig);

#if defined(PUBLISHER) || defined(SUBSCRIBER)
pthread_t           userApplicationThreadID;
void *userApplicationPubSub(void *arg);
#endif

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
}

/**
 * **Nanosecond field handling**
 *
 * Nanosecond field in timespec is checked for overflowing and one second
 * is added to seconds field and nanosecond field is set to zero
*/

static void nanoSecondFieldConversion(struct timespec *timeSpecValue) {
    /* Check if ns field is greater than '1 ns less than 1sec' */
    while (timeSpecValue->tv_nsec > (SECONDS -1)) {
        /* Move to next second and remove it from ns field */
        timeSpecValue->tv_sec  += SECONDS_INCREMENT;
        timeSpecValue->tv_nsec -= SECONDS;
    }

}

#if defined(SUBSCRIBER)
static void
addPubSubConnectionSubscriber(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlSubscriber){
    UA_StatusCode    retval                                 = UA_STATUSCODE_GOOD;
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Subscriber Connection");
    connectionConfig.enabled                                = UA_TRUE;
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = *networkAddressUrlSubscriber;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = {UA_STRING(PUBSUB_IP_ADDRESS),
                                                               UA_STRING(SUBSCRIBER_MULTICAST_ADDRESS)};
    connectionConfig.transportProfileUri                    = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric                    = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if (retval == UA_STATUSCODE_GOOD)
         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"The PubSub Connection was created successfully!");
#ifndef UA_ENABLE_PUBSUB_ETH_UADP
    UA_PubSubConnection_regist(server, &connectionIdentSubscriber);
#endif
}
/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    if (server == NULL) {
        return;
    }

    UA_ReaderGroupConfig     readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name   = UA_STRING("ReaderGroup1");
    UA_Server_addReaderGroup(server, connectionIdentSubscriber, &readerGroupConfig,
                             &readerGroupIdentifier);

}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
    UA_Int32 iterator = 0;
    if (server == NULL) {
        return;
    }

    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name                 = UA_STRING("DataSet Reader 1");
    UA_UInt16 publisherIdentifier     = PUBLISHER_ID_SUB;
    readerConfig.publisherId.type     = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data     = &publisherIdentifier;
    readerConfig.writerGroupId        = WRITER_GROUP_ID_SUB;
    readerConfig.dataSetWriterId      = DATA_SET_WRITER_ID_SUB;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name                   = UA_STRING ("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             = REPEATED_NODECOUNTS + 1;
    pMetaData->fields                 = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    for (iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_FieldMetaData_init (&pMetaData->fields[iterator]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                        &pMetaData->fields[iterator].dataType);
        pMetaData->fields[iterator].builtInType  = UA_NS0ID_UINT64;
        pMetaData->fields[iterator].valueRank    = -1; /* scalar */
    }

    /* Unsigned Integer DataType */
    UA_FieldMetaData_init (&pMetaData->fields[iterator]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                    &pMetaData->fields[iterator].dataType);
    pMetaData->fields[iterator].builtInType  = UA_NS0ID_UINT64;
    pMetaData->fields[iterator].valueRank    = -1; /* scalar */

    /* Setting up Meta data configuration in DataSetReader */
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                               &readerIdentifier);
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add SubscriberCounter variable to the DataSetReader */
static void addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    UA_Int32 iterator = 0;
    if (server == NULL) {
        return;
    }

    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = REPEATED_NODECOUNTS + 1;
    targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                      UA_calloc(targetVars.targetVariablesSize,
                                      sizeof(UA_FieldTargetDataType));
    /* For creating Targetvariable */
    for (iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
        targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[iterator].targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+50000);
    }

    UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
    targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[iterator].targetNodeId = subNodeID;
    UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                  &targetVars);

    UA_TargetVariablesDataType_deleteMembers(&targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
}

#endif

/* Add a callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    /* Initialize arguments required for the thread to run */
    threadArg *threadArguments = (threadArg *) UA_malloc(sizeof(threadArg));

    /* Pass the value required for the threads */
    threadArguments->server      = server;
    threadArguments->data        = data;
    threadArguments->callback    = callback;
    threadArguments->interval_ms = interval_ms;
    threadArguments->callbackId  = callbackId;

    /* Check the writer group identifier and create the thread accordingly */
    UA_WriterGroup *tmpWriter = (UA_WriterGroup *) data;
    if(UA_NodeId_equal(&tmpWriter->identifier, &writerGroupIdent)) {
#if defined(PUBLISHER)
        /* Create the publisher thread with the required priority and core affinity */
        char threadNamePub[10] = "Publisher";
        pubthreadID = threadCreation(PUB_SCHED_PRIORITY, CORE_TWO, publisherETF, threadNamePub, threadArguments);
#endif
    }
    else {
#if defined(SUBSCRIBER)
        /* Create the subscriber thread with the required priority and core affinity */
        char threadNameSub[11] = "Subscriber";
        subthreadID = threadCreation(SUB_SCHED_PRIORITY, CORE_TWO, subscriber, threadNameSub, threadArguments);
#endif
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms) {
    /* Callback interval need not be modified as it is thread based implementation.
     * The thread uses nanosleep for calculating cycle time and modification in
     * nanosleep value changes cycle time if required */
    return UA_STATUSCODE_GOOD;
}

/* Remove the callback added for cyclic repetition */
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
//    printf("Remove repeated callback\n");
    /* TODO Thread exit functions using pthread join and exit */
}

#if defined(PUBLISHER)
/**
 * **PubSub connection handling**
 *
 * Create a new ConnectionConfig. The addPubSubConnection function takes the
 * config and creates a new connection. The Connection identifier is
 * copied to the NodeId parameter.
 */
static void
addPubSubConnection(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlPub){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                          = UA_STRING("Publisher Connection");
    connectionConfig.enabled                       = UA_TRUE;
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrl = *networkAddressUrlPub;
    connectionConfig.transportProfileUri           = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(PUBSUB_IP_ADDRESS),
                                                      UA_STRING(PUBLISHER_MULTICAST_ADDRESS)};
    connectionConfig.transportProfileUri           = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric           = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * Details about the connection configuration and handling are located
 * in the pubsub connection tutorial
 */
static void
addPublishedDataSet(UA_Server *server) {
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name                 = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one
 * published field.
 */
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdentRepeated;
    UA_DataSetFieldConfig dataSetFieldConfig;
    for (UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
    {
       memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
       dataSetFieldConfig.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
       dataSetFieldConfig.field.variable.fieldNameAlias                      = UA_STRING("Repeated Counter Variable");
       dataSetFieldConfig.field.variable.promotedField                       = UA_FALSE;
       dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);
       dataSetFieldConfig.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
       dataSetFieldConfig.field.variable.staticValueSourceEnabled            = UA_TRUE;
       UA_DataValue_init(&dataSetFieldConfig.field.variable.staticValueSource);

       UA_Variant_setScalar(&dataSetFieldConfig.field.variable.staticValueSource.value,
                            &repeatedCounter, &UA_TYPES[UA_TYPES_UINT64]);
       dataSetFieldConfig.field.variable.staticValueSource.value.storageType = UA_VARIANT_DATA_NODELETE;
#endif
       UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdentRepeated);
   }

    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig counterValue;
    memset(&counterValue, 0, sizeof(UA_DataSetFieldConfig));
    counterValue.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue.field.variable.fieldNameAlias                      = UA_STRING("Counter Variable");
    counterValue.field.variable.promotedField                       = UA_FALSE;
    counterValue.field.variable.publishParameters.publishedVariable = pubNodeID;
    counterValue.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    counterValue.field.variable.staticValueSourceEnabled            = UA_TRUE;
    UA_DataValue_init(&counterValue.field.variable.staticValueSource);
    UA_Variant_setScalar(&counterValue.field.variable.staticValueSource.value,
                         &pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    counterValue.field.variable.staticValueSource.value.storageType = UA_VARIANT_DATA_NODELETE;

#endif
    UA_Server_addDataSetField(server, publishedDataSetIdent, &counterValue, &dataSetFieldIdent);

}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation.
 */
static void
addWriterGroup(UA_Server *server) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name               = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = CYCLE_TIME;
    writerGroupConfig.enabled            = UA_FALSE;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    writerGroupConfig.rtLevel            = UA_PUBSUB_RT_FIXED_SIZE;
#endif
    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
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
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation.
 */
static void
addDataSetWriter(UA_Server *server) {
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name            = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATA_SET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount   = KEY_FRAME_COUNT;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

#if defined(UPDATE_MEASUREMENTS)
/**
 * **Published data handling**
 *
 * The published data is updated in the array using this function
 */
static void
updateMeasurementsPublisher(struct timespec start_time,
                            UA_UInt64 counterValue) {
    publishTimestamp[measurementsPublisher]        = start_time;
    publishCounterValue[measurementsPublisher]     = counterValue;
    measurementsPublisher++;
}
#endif

/**
 * **Publisher thread routine**
 *
 * The publisherETF function is the routine used by the publisher thread.
 * This routine publishes the data at a cycle time of 250us.
 */
void *publisherETF(void *arg) {
    struct timespec nextnanosleeptime;
    UA_Server*      server;
    UA_UInt64       interval_ns;

    /* Initialise value for nextnanosleeptime timespec */
    nextnanosleeptime.tv_nsec                      = 0;

    threadArg *threadArgumentsPublisher = (threadArg *)arg;
    server                     = threadArgumentsPublisher->server;
    currentWriterGroup         = (UA_WriterGroup *)threadArgumentsPublisher->data;
    interval_ns                = (threadArgumentsPublisher->interval_ms * MILLI_SECONDS);

    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptime.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptime.tv_nsec                      = NANO_SECONDS_SLEEP_PUB;
    nanoSecondFieldConversion(&nextnanosleeptime);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        UA_WriterGroup_publishCallback(server, currentWriterGroup);
        nextnanosleeptime.tv_nsec += interval_ns;
        nanoSecondFieldConversion(&nextnanosleeptime);
    }

    UA_free(threadArgumentsPublisher);

    return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)

#if defined(UPDATE_MEASUREMENTS)
/**
 * Subscribed data handling**
 * The subscribed data is updated in the array using this function Subscribed data handling**
 */
static void
updateMeasurementsSubscriber(struct timespec receive_time, UA_UInt64 counterValue) {
    subscribeTimestamp[measurementsSubscriber]     = receive_time;
    subscribeCounterValue[measurementsSubscriber]  = counterValue;
    measurementsSubscriber++;
}
#endif
/**
 * **Subscriber thread routine**
 *
 * The subscriber function is the routine used by the subscriber thread.
 */

void *subscriber(void *arg) {
    UA_Server* server;
    struct timespec nextnanosleeptimeSub;
    threadArg *threadArgumentsSubscriber = (threadArg *)arg;
    server = threadArgumentsSubscriber->server;
    currentReaderGroup = (UA_ReaderGroup *)threadArgumentsSubscriber->data;

    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeSub);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeSub.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptimeSub.tv_nsec                      = NANO_SECONDS_SLEEP_SUB;
    nanoSecondFieldConversion(&nextnanosleeptimeSub);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSub, NULL);
        /* Read subscribed data from the SubscriberCounter variable */
        UA_ReaderGroup_subscribeCallback(server, currentReaderGroup);
        nextnanosleeptimeSub.tv_nsec += (CYCLE_TIME * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimeSub);
    }

    UA_free(threadArgumentsSubscriber);

    return (void*)NULL;
}
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)
/**
 * **UserApplication thread routine**
 *
 */
void *userApplicationPubSub(void *arg) {
    UA_Server* server;
    struct timespec nextnanosleeptimeUserApplication;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeUserApplication);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeUserApplication.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptimeUserApplication.tv_nsec                      = NANO_SECONDS_SLEEP_USER_APPLICATION;
    nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    serverConfigStruct *serverConfig = (serverConfigStruct*)arg;
    server = serverConfig->ServerRun;
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeUserApplication, NULL);
#if defined(PUBLISHER)
        pubCounterData++;
        repeatedCounter++;
        clock_gettime(CLOCKID, &dataModificationTime);
#if defined(UPDATE_MEASUREMENTS)
        updateMeasurementsPublisher(dataModificationTime, pubCounterData);
#endif
        UA_Variant_setScalar(&pubCounter, &pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
        UA_NodeId currentNodeId = UA_NODEID_STRING(1, "PublisherCounter");
        UA_Server_writeValue(server, currentNodeId, pubCounter);
#endif
#if defined(SUBSCRIBER)
        const UA_NodeId nodeid = UA_NODEID_STRING(1,"SubscriberCounter");
        UA_Variant_init(&subCounter);
        UA_Server_readValue(server, nodeid, &subCounter);
        subCounterData = *(UA_UInt64 *)subCounter.data;

        clock_gettime(CLOCKID, &dataReceiveTime);
#if defined(UPDATE_MEASUREMENTS)
        if (subCounterData > 0)
            updateMeasurementsSubscriber(dataReceiveTime, subCounterData);
#endif
        UA_Variant_deleteMembers(&subCounter);
#endif
        nextnanosleeptimeUserApplication.tv_nsec += (CYCLE_TIME * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    }
    return (void*)NULL;
}
#endif
/**
 * **Deletion of nodes**
 *
 * The removeServerNodes function is used to delete the publisher and subscriber
 * nodes.
 */
static void removeServerNodes(UA_Server *server) {
    /* Delete the Publisher Counter Node*/
    UA_Server_deleteNode(server, pubNodeID, UA_TRUE);
    UA_NodeId_deleteMembers(&pubNodeID);

    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, pubRepeatedCountNodeID, UA_TRUE);
        UA_NodeId_deleteMembers(&pubRepeatedCountNodeID);
    }
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, subRepeatedCountNodeID, UA_TRUE);
        UA_NodeId_deleteMembers(&subRepeatedCountNodeID);
    }
}

static pthread_t threadCreation(UA_Int16 threadPriority, size_t coreAffinity, void *(*thread) (void *), char *applicationName, void *serverConfig){

    /* Core affinity set */
    cpu_set_t           cpuset;
    pthread_t           threadID;
    struct sched_param  schedParam;
    UA_Int32         returnValue         = 0;
    UA_Int32         errorSetAffinity    = 0;
    /* Return the ID for thread */
    threadID = pthread_self();
    schedParam.sched_priority = threadPriority;
    returnValue = pthread_setschedparam(threadID, SCHED_FIFO, &schedParam);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"pthread_setschedparam: failed\n");
        exit(1);
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,\
                "\npthread_setschedparam:%s Thread priority is %d \n", \
                applicationName, schedParam.sched_priority);
    CPU_ZERO(&cpuset);
    CPU_SET(coreAffinity, &cpuset);
    errorSetAffinity = pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset);
    if (errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        exit(1);
    }

    returnValue = pthread_create(&threadID, NULL, thread, serverConfig);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,":%s Cannot create thread\n", applicationName);
    }

    if (CPU_ISSET(coreAffinity, &cpuset)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s CPU CORE: %ld\n", applicationName, coreAffinity);
    }

   return threadID;

}
/**
 * **Creation of nodes**
 *
 * The addServerNodes function is used to create the publisher and subscriber
 * nodes.
 */
static void addServerNodes(UA_Server *server) {
    UA_NodeId objectId;
    UA_ObjectAttributes object           = UA_ObjectAttributes_default;
    object.displayName                   = UA_LOCALIZEDTEXT("en-US", "Counter Object");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Counter Object"), UA_NODEID_NULL,
                            object, NULL, &objectId);

    UA_VariableAttributes publisherAttr  = UA_VariableAttributes_default;
    UA_UInt64 publishValue               = 0;
    publisherAttr.accessLevel            = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&publisherAttr.value, &publishValue, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    UA_NodeId newNodeId                  = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, publisherAttr, NULL, &pubNodeID);

    UA_VariableAttributes subscriberAttr = UA_VariableAttributes_default;
    UA_UInt64 subscribeValue             = 0;
    subscriberAttr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&subscriberAttr.value, &subscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
    subscriberAttr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId                            = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, subscriberAttr, NULL, &subNodeID);

    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodePub = UA_VariableAttributes_default;
        UA_UInt64 repeatedPublishValue        = 0;
        repeatedNodePub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Variant_setScalar(&repeatedNodePub.value, &repeatedPublishValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodePub.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher RepeatedCounter");
        newNodeId                             = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);
        UA_Server_addVariableNode(server, newNodeId, objectId,
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                 UA_QUALIFIEDNAME(1, "Publisher RepeatedCounter"),
                                 UA_NODEID_NULL, repeatedNodePub, NULL, &pubRepeatedCountNodeID);
    }
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodeSub = UA_VariableAttributes_default;
        UA_UInt64 repeatedSubscribeValue;
        UA_Variant_setScalar(&repeatedNodeSub.value, &repeatedSubscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodeSub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        repeatedNodeSub.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber RepeatedCounter");
        newNodeId                             = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+50000);
        UA_Server_addVariableNode(server, newNodeId, objectId,
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                 UA_QUALIFIEDNAME(1, "Subscriber RepeatedCounter"),
                                 UA_NODEID_NULL, repeatedNodeSub, NULL, &subRepeatedCountNodeID);
    }

}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
 }

/**
 * **Main Server code**
 *
 * The main function contains publisher and subscriber threads running in
 * parallel.
 */
int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Int32         returnValue         = 0;
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    UA_Server       *server              = UA_Server_new();
    UA_ServerConfig *config              = UA_Server_getConfig(server);
    pthread_t        userThreadID;
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);
    UA_NetworkAddressUrlDataType networkAddressUrlPub;

#if defined(SUBSCRIBER)
    UA_NetworkAddressUrlDataType networkAddressUrlSub;
#endif

#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }

        /* check MAC addresses of publisher and subscriber */
        else if ((strncmp(argv[1], "opc.eth://", 10) == 0) && (strncmp(argv[2], "opc.eth://", 10) == 0)) {
            if (argc < 4) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }

#if defined(PUBLISHER)
            networkAddressUrlPub.networkInterface = UA_STRING(argv[3]);
            networkAddressUrlPub.url = UA_STRING(argv[1]);
#endif
#if defined(SUBSCRIBER)
            networkAddressUrlSub.networkInterface = UA_STRING(argv[3]);
            networkAddressUrlSub.url = UA_STRING(argv[2]);
#endif
        }
        /* check MAC address of publisher alone*/
        else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }

#if defined(PUBLISHER)
            networkAddressUrlPub.networkInterface = UA_STRING(argv[2]);
            networkAddressUrlPub.url = UA_STRING(argv[1]);
#endif
        }
    }

    else {
#if defined(PUBLISHER)
        networkAddressUrlPub.networkInterface = UA_STRING(PUBLISHING_INTERFACE);
        networkAddressUrlPub.url = UA_STRING(PUBLISHING_MAC_ADDRESS); /* MAC address of subscribing node*/
#endif
#if defined(SUBSCRIBER)
        networkAddressUrlSub.url = UA_STRING(SUBSCRIBING_MAC_ADDRESS); /* Self MAC address */
        networkAddressUrlSub.networkInterface = UA_STRING(SUBSCRIBING_INTERFACE);
#endif
    }

#endif

#if defined(UPDATE_MEASUREMENTS)
#if defined(PUBLISHER)
    fpPublisher                   = fopen(filePublishedData, "w");
#endif
#if defined(SUBSCRIBER)
    fpSubscriber                  = fopen(fileSubscribedData, "w");
#endif
#endif

#if defined(PUBLISHER) && defined(SUBSCRIBER)
/* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
                                    UA_malloc(CONNECTION_NUMBER * sizeof(UA_PubSubTransportLayer));
#else
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
                                    UA_malloc(sizeof(UA_PubSubTransportLayer));
#endif

    if (!config->pubsubTransportLayers) {
        return FAILURE_EXIT;
    }

/* It is possible to use multiple PubSubTransportLayers on runtime.
 * The correct factory is selected on runtime by the standard defined
 * PubSub TransportProfileUri's.
*/
#ifndef UA_ENABLE_PUBSUB_ETH_UADP
#if defined(PUBLISHER) && defined(SUBSCRIBER)
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#else
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#endif
#endif

#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
#if defined (PUBLISHER) && defined(SUBSCRIBER)
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#else
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    /* Create variable nodes for publisher and subscriber in address space */
    addServerNodes(server);
#endif

#if defined(PUBLISHER)
    addPubSubConnection(server, &networkAddressUrlPub);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
#endif

#if defined(SUBSCRIBER)
    addPubSubConnectionSubscriber(server, &networkAddressUrlSub);
    addReaderGroup(server);
    addDataSetReader(server);
    addSubscribedVariables(server, readerIdentifier);
#endif
    serverConfigStruct *serverConfig;
    serverConfig = (serverConfigStruct*)UA_malloc(sizeof(serverConfigStruct));
    serverConfig->ServerRun = server;

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    char threadNameUserApplication[22] = "UserApplicationPubSub";
    userThreadID = threadCreation(USERAPPLICATION_SCHED_PRIORITY, CORE_THREE, userApplicationPubSub, threadNameUserApplication, serverConfig);
#endif
    retval |= UA_Server_run(server, &running);

#if defined(PUBLISHER)
    returnValue = pthread_join(pubthreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for publisher thread:%d\n", returnValue);
    }
#endif
#if defined(SUBSCRIBER)
    returnValue = pthread_join(subthreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for subscriber thread:%d\n", returnValue);
    }
#endif
#if defined(PUBLISHER) || defined(SUBSCRIBER)
    returnValue = pthread_join(userThreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for User thread:%d\n", returnValue);
    }
#endif

#if defined(PUBLISHER)
#if defined(UPDATE_MEASUREMENTS)
    /* Write the published data in the publisher_T1.csv file */
   size_t pubLoopVariable               = 0;
   for (pubLoopVariable = 0; pubLoopVariable < measurementsPublisher;
        pubLoopVariable++) {
        fprintf(fpPublisher, "%ld,%ld.%09ld\n",
                publishCounterValue[pubLoopVariable],
                publishTimestamp[pubLoopVariable].tv_sec,
                publishTimestamp[pubLoopVariable].tv_nsec);
    }
#endif
#endif
#if defined(SUBSCRIBER)
#if defined(UPDATE_MEASUREMENTS)
    /* Write the subscribed data in the subscriber_T8.csv file */
    size_t subLoopVariable               = 0;
    for (subLoopVariable = 0; subLoopVariable < measurementsSubscriber;
         subLoopVariable++) {
        fprintf(fpSubscriber, "%ld,%ld.%09ld\n",
                subscribeCounterValue[subLoopVariable],
                subscribeTimestamp[subLoopVariable].tv_sec,
                subscribeTimestamp[subLoopVariable].tv_nsec);
    }
#endif
#endif
#if defined(PUBLISHER) || defined(SUBSCRIBER)
    removeServerNodes(server);
    UA_Server_delete(server);
    UA_free(serverConfig);
#endif
#if defined(PUBLISHER)
#if defined(UPDATE_MEASUREMENTS)
    fclose(fpPublisher);
#endif
#endif

#if defined(SUBSCRIBER)
#if defined(UPDATE_MEASUREMENTS)
    fclose(fpSubscriber);
#endif
#endif
    return (int)retval;
}
