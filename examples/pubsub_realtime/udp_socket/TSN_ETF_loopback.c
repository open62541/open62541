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
#include <open62541/plugin/pubsub_ethernet.h>
#include "ua_pubsub.h"

UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

UA_DataSetReader *dataSetReader;

/*to find load of each thread
 * ps -L -o pid,pri,%cpu -C TSN_ETF_loopback
 *
*/

/* These defines enables the publisher and subscriber of the OPCUA stack */
/* To run only publisher, enable PUBLISHER define alone (comment SUBSCRIBER) */
#define             PUBLISHER
/* To run only subscriber, enable SUBSCRIBER define alone
 * (comment PUBLISHER) */
#define             SUBSCRIBER
//#define             UPDATE_MEASUREMENTS
#define             UA_ENABLE_STATICVALUESOURCE
/* Publish interval in milliseconds */
#define             PUB_INTERVAL                    250
/* Cycle time in ns. Eg: For 100us: 100*1000 */
#define             CYCLE_TIME                            200 * 1000
#define             SECONDS_SLEEP                         1
/* Publisher will sleep for 80% of cycle time and then prepares the */
/* transmission packet within 20% */
#define             NANO_SECONDS_SLEEP_PUB                CYCLE_TIME * 0.8
/* Subscriber will wakeup only during start of cycle and check whether */
/* the packets are received */
#define             NANO_SECONDS_SLEEP_SUB                0
/* User application Pub/Sub will wakeup at the 30% of cycle time and handles the */
/* user data such as read and write in Information model */
#define             NANO_SECONDS_SLEEP_USER_APPLICATION   CYCLE_TIME * 0.3
#define             MILLI_SECONDS                         1000 * 1000
#define             SECONDS                               1000 * 1000 * 1000
/* Priority of Publisher, subscriber, User application and server are kept */
/* after some prototyping and analyzing it */
#define             PUB_SCHED_PRIORITY                    78
#define             SUB_SCHED_PRIORITY                    81
#define             USERAPPLICATION_SCHED_PRIORITY        75
#define             SERVER_SCHED_PRIORITY                 1
#define             DATA_SET_WRITER_ID                    62541
#define             KEY_FRAME_COUNT                       10
#define             MAX_MEASUREMENTS                      30000000
#define             CORE_TWO                              2
#define             CORE_THREE                            3
#define             SECONDS_INCREMENT                     1
#define             CONNECTION_NUMBER                     2
#define             PORT_NUMBER                           62541
#define             FAILURE_EXIT                          -1
#define             CLOCKID                               CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             UDP_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
#define             PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS

/* This is a hardcoded publisher/subscriber IP address. If the IP address need
 * to be changed for running over UDP, change it in the below lines.
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING is enabled,
 * change in line number 40 and 41 in plugins/ua_pubsub_realtime.c and line number 27 in plugins/ua_pubsub_udp.c
 */

#define             PUBSUB_IP_ADDRESS              "192.168.9.11"
#define             PUBLISHER_MULTICAST_ADDRESS    "opc.udp://224.0.0.22:4840/"
#define             SUBSCRIBER_MULTICAST_ADDRESS   "opc.udp://224.0.0.32:4840/"

/* If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING and UA_ENABLE_PUBSUB_ETH_UADP is enabled,
 * If the Hardcoded publisher/subscriber MAC addresses need to be changed,
 * change in line number 658 and 661  ,
 * If the Hardcoded interface name need to be changed,
 * change in line number 657 and 662
 * To pass the MAC addresses as arguments, use the command ./executable "publsiher_mac" "subscriber_mac" interface name
 * To run only subscriber, use the command ./executable "subscriber_mac" interface name
 * if UA_ENABLE_PUBSUB_ETH_UADP is enabled
 */

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
UA_NodeId           DateNodeIDPub;
UA_NodeId           DateNodeIDSub;
/* Variable for PubSub callback */
UA_ServerCallback   pubCallback;
/* Variables for counter data handling in address space */
UA_UInt64           pubCounterData         = 0;
UA_UInt64           currentTime            = 10;
UA_UInt64           subCounterData         = 0;
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
char               *filePublishedData      = "publisher_T5.csv";

/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;

/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];
#endif
/* Process scheduling parameter for publisher */
struct sched_param  schedParamPublisher;

struct timespec     dataModificationTime;
UA_WriterGroup      *currentWriterGroupCallback;
/* Publisher thread routine for ETF */
void               *publisherETF(void *arg);
#endif

#if defined(SUBSCRIBER)

#if defined(UPDATE_MEASUREMENTS)
/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileSubscribedData     = "subscriber_T4.csv";

/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;

/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];

#endif
/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
UA_ReaderGroup     *currentReaderGroupCallback;

/* Process scheduling parameter for subscriber */
struct sched_param  schedParamSubscriber;

struct timespec     dataReceiveTime;

/* Subscriber thread routine */
void               *subscriber(void *arg);

/* OPCUA Subscribe API */
void                subscribe(void);
#endif
/* Thread for server */
pthread_t           serverThreadID;
/* Process scheduling parameter for server */
struct sched_param  schedParamServer;

typedef struct {
UA_Server*                   ServerRun;
} serverConfigStruct;

#if defined(PUBLISHER) || defined(SUBSCRIBER)
pthread_t           userApplicationThreadID;
struct sched_param  schedParamUserApplication;
void *userApplicationPubSub(void *arg);
#endif

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
}

#if defined(SUBSCRIBER)
static void
addPubSubConnection1(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlSubscriber){
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                = UA_STRING("Subscriber Connection");
    connectionConfig.enabled             = UA_TRUE;
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = *networkAddressUrlSubscriber;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = {UA_STRING(PUBSUB_IP_ADDRESS),
                                                               UA_STRING(SUBSCRIBER_MULTICAST_ADDRESS)};
    connectionConfig.transportProfileUri                    = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if (retval == UA_STATUSCODE_GOOD)
         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"The PubSub Connection was created successfully!");
#ifndef UA_ENABLE_PUBSUB_ETH_UADP
         retval |= UA_PubSubConnection_regist(server, &connectionIdentSubscriber);
#endif
}
/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    if(server == NULL) {
        return;
    }

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup");

    UA_Server_addReaderGroup(server, connectionIdentSubscriber, &readerGroupConfig,
                             &readerGroupIdentifier);
}

/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
    UA_Int32 iterator = 0;
    if(server == NULL) {
        return;
    }

    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name                 = UA_STRING("DataSet Reader");
    UA_UInt16 publisherIdentifier     = 2234;
    readerConfig.publisherId.type     = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data     = &publisherIdentifier;
    readerConfig.writerGroupId        = 100;
    readerConfig.dataSetWriterId      = 62541;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             = REPEATED_NODECOUNTS + 1;
    pMetaData->fields                 = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
/*    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType  = UA_NS0ID_UINT64;
    pMetaData->fields[0].valueRank    = -1; */
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
    UA_free(pMetaData->fields);
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static void addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    UA_Int32 iterator = 0;
    if(server == NULL) {
        return;
    }

    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = REPEATED_NODECOUNTS + 1;
    targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                      UA_calloc(targetVars.targetVariablesSize,
                                      sizeof(UA_FieldTargetDataType));
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
    targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[iterator].targetNodeId = UA_NODEID_STRING(1, "SubscriberCounter");

    for (iterator = 1; iterator < REPEATED_NODECOUNTS + 1; iterator++)
    {
        UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
        targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[iterator].targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)(iterator-1) + 50000);
    }
    /* For creating Targetvariable */
/*    UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
    targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[iterator].targetNodeId = UA_NODEID_STRING(1, "SubscriberCounter");*/
    UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                  &targetVars);
}
#endif

/* Add a callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    /* TODO: Need to handle for Thread based Implementation */
    return UA_STATUSCODE_GOOD;
}

/* Remove the callback added for cyclic repetition */
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    /* TODO This section will contain timer exit functions in future */
}

#if defined(PUBLISHER)
/**
 * **PubSub connection handling**
 *
 * Create a new ConnectionConfig. The addPubSubConnection function takes the
 * config and create a new connection. The Connection identifier is
 * copied to the NodeId parameter.
 */
static void
addPubSubConnection(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlEthernet){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                          = UA_STRING("Publisher Connection");
    connectionConfig.enabled                       = UA_TRUE;
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrl = *networkAddressUrlEthernet;
    connectionConfig.transportProfileUri           = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(PUBSUB_IP_ADDRESS),
                                                      UA_STRING(PUBLISHER_MULTICAST_ADDRESS)};
    connectionConfig.transportProfileUri           = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric           = 2235;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataset handling**
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
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig counterValue;
    memset(&counterValue, 0, sizeof(UA_DataSetFieldConfig));
#if defined PUB_WITH_INFORMATION_MODEL
    counterValue.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue.field.variable.fieldNameAlias                      = UA_STRING("Counter Variable");
    counterValue.field.variable.promotedField                       = UA_FALSE;
    counterValue.field.variable.publishParameters.publishedVariable = pubNodeID;
    counterValue.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
#endif
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    counterValue.field.variable.staticValueSourceEnabled = UA_TRUE;
    UA_DataValue_init(&counterValue.field.variable.staticValueSource);
    UA_Variant_setScalar(&counterValue.field.variable.staticValueSource.value,
                         &subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    counterValue.field.variable.staticValueSource.value.storageType = UA_VARIANT_DATA_NODELETE;
    //counterValue.field.variable.staticValueSource.value =  variant;
#endif
    UA_Server_addDataSetField(server, publishedDataSetIdent, &counterValue, &dataSetFieldIdent);
    UA_NodeId dataSetFieldIdent1;
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
       UA_DataSetFieldConfig dataSetFieldConfig;
       memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
#if defined PUB_WITH_INFORMATION_MODEL
       dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
       dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
       dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
       dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
         //   UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
                                UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);

       dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
#endif
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
//       UA_UInt64  currentTime = 10;//UA_DateTime_now();
       dataSetFieldConfig.field.variable.staticValueSourceEnabled = UA_TRUE;
       UA_DataValue_init(&dataSetFieldConfig.field.variable.staticValueSource);
       UA_Variant_setScalar(&dataSetFieldConfig.field.variable.staticValueSource.value,
                            &currentTime, &UA_TYPES[UA_TYPES_UINT64]);
       dataSetFieldConfig.field.variable.staticValueSource.value.storageType = UA_VARIANT_DATA_NODELETE;
    //dataSetFieldConfig.field.variable.staticValueSource.value =  variant;
#endif
       UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent1);
   }

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
    writerGroupConfig.publishingInterval = PUB_INTERVAL;
    writerGroupConfig.enabled            = UA_FALSE;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
#endif
    writerGroupConfig.writerGroupId      = 101;
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
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    //UA_Server_setWriterGroupOperational(server, writerGroupIdent);
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

/**
 * **Publisher thread routine**
 *
 * The publisherETF function is the routine used by the publisher thread.
 * This routine publishes the data at a cycle time of 100us.
 */
void *publisherETF(void *arg) {
    struct timespec nextnanosleeptime;
    UA_Server* server;
    /* Initialise value for nextnanosleeptime timespec */
    nextnanosleeptime.tv_nsec                      = 0;
    serverConfigStruct *serverConfig = (serverConfigStruct*)arg;
    server = serverConfig->ServerRun;
    currentWriterGroupCallback = UA_WriterGroup_findWGbyId(server, writerGroupIdent);
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptime.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptime.tv_nsec                      = NANO_SECONDS_SLEEP_PUB;
    nanoSecondFieldConversion(&nextnanosleeptime);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);

        UA_WriterGroup_publishCallback(server, currentWriterGroupCallback);
/*#if defined(UPDATE_MEASUREMENTS)
        if (subCounterData > 0)
             updateMeasurementsPublisher(dataModificationTime, subCounterData);
#endif*/
        nextnanosleeptime.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextnanosleeptime);
      }

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
    serverConfigStruct *serverConfig = (serverConfigStruct*)arg;
    server = serverConfig->ServerRun;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeSub);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeSub.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptimeSub.tv_nsec                      = NANO_SECONDS_SLEEP_SUB;
    nanoSecondFieldConversion(&nextnanosleeptimeSub);
    /* Identify the readergroup through the readerGroupIdentifier */
    currentReaderGroupCallback = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSub, NULL);
        /* Read subscribed data from the SubscriberCounter variable */
        UA_ReaderGroup_subscribeCallback(server, currentReaderGroupCallback);

        nextnanosleeptimeSub.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextnanosleeptimeSub);
    }

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
    dataSetReader      = UA_ReaderGroup_findDSRbyId(server, readerIdentifier);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeUserApplication, NULL);
//        const UA_NodeId nodeid = UA_NODEID_STRING(1,"SubscriberCounter");

//        UA_Server_readValue(server, nodeid, &subCounter);
//        subCounterData = *(UA_UInt64 *)subCounter.data;
        subCounterData = dataSetReader->subscribedcounter[0];
        clock_gettime(CLOCKID, &dataReceiveTime);
        clock_gettime(CLOCKID, &dataModificationTime);

#if defined(PUBLISHER) 
#if defined(PUB_WITH_INFORMATION_MODEL)

        UA_Variant_setScalar(&pubCounter, &subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
        UA_NodeId currentNodeId         = UA_NODEID_STRING(1, "PublisherCounter");
        UA_Server_writeValue(server, currentNodeId, pubCounter);
#endif
#if defined(UPDATE_MEASUREMENTS)
        if (subCounterData > 0)
        {
             updateMeasurementsSubscriber(dataReceiveTime, subCounterData);
             updateMeasurementsPublisher(dataModificationTime, subCounterData);
        }
        /*if (pubCounterData > 0)
        {
             updateMeasurementsPublisher(dataModificationTime, pubCounterData);
        }*/

#endif
#endif
        //pubCounterData = subCounterData;
        //FirstPacket = UA_FALSE;
        nextnanosleeptimeUserApplication.tv_nsec += CYCLE_TIME;
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

    /* Delete the Subscriber Counter Node*/
    UA_Server_deleteNode(server, subNodeID, UA_TRUE);
    UA_NodeId_deleteMembers(&subNodeID);
#if defined(PUB_WITH_INFORMATION_MODEL)
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, DateNodeIDPub, UA_TRUE);
        UA_NodeId_deleteMembers(&DateNodeIDPub);
    }
#endif
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, DateNodeIDSub, UA_TRUE);
        UA_NodeId_deleteMembers(&DateNodeIDSub);
    }
}

static pthread_t threadCreation(UA_Int16 threadPriority, size_t coreAffinity, void *(*thread) (void *), char *applicationName, \
                                void *serverConfig){
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
 * The addServerNodes function is used to create the publisher and subscriber
 * nodes.
 */
static void addServerNodes(UA_Server *server) {
    UA_NodeId counterId;
    UA_NodeId newNodeId;
    UA_ObjectAttributes oAttr    = UA_ObjectAttributes_default;
    oAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Counter Object");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Counter Object"), UA_NODEID_NULL,
                            oAttr, NULL, &counterId);
#if defined(PUB_WITH_INFORMATION_MODEL)
    UA_VariableAttributes p4Attr = UA_VariableAttributes_default;
    UA_UInt64 axis4position      = 0;
    p4Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&p4Attr.value, &axis4position, &UA_TYPES[UA_TYPES_UINT64]);
    p4Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    newNodeId          = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, counterId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, p4Attr, NULL, &pubNodeID);
#endif
    UA_VariableAttributes p5Attr = UA_VariableAttributes_default;
    UA_UInt64 axis5position      = 0;
    p5Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&p5Attr.value, &axis5position, &UA_TYPES[UA_TYPES_UINT64]);
    p5Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId                    = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, counterId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, p5Attr, NULL, &subNodeID);
#if defined(PUB_WITH_INFORMATION_MODEL)
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes p6Attr = UA_VariableAttributes_default;
        UA_UInt64 axis6position = 0;
        p6Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Variant_setScalar(&p6Attr.value, &axis6position, &UA_TYPES[UA_TYPES_UINT64]);

        p6Attr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
        newNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);
        UA_Server_addVariableNode(server, newNodeId, counterId,
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                 UA_QUALIFIEDNAME(1, "DateTime"),
                                 UA_NODEID_NULL, p6Attr, NULL, &DateNodeIDPub);
    }
#endif
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes p7Attr = UA_VariableAttributes_default;
       // UA_DateTime axis7position;
        UA_UInt64 axis7position = 0;
        UA_Variant_setScalar(&p7Attr.value, &axis7position, &UA_TYPES[UA_TYPES_UINT64]);
        p7Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        p7Attr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTimeSub");
        newNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+50000);
        UA_Server_addVariableNode(server, newNodeId, counterId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "DateTimeSub"),
                                  UA_NODEID_NULL, p7Attr, NULL, &DateNodeIDSub);
    }

}


#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}
#endif
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
    pthread_t           pubthreadID;
#if defined(SUBSCRIBER)
    pthread_t           subthreadID;
#endif
    pthread_t           userThreadID;
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

#if defined(PUBLISHER)
    UA_NetworkAddressUrlDataType networkAddressUrlEthernet;
#endif
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
            networkAddressUrlEthernet.networkInterface = UA_STRING(argv[3]);
            networkAddressUrlEthernet.url = UA_STRING(argv[1]);
#endif
#if defined(SUBSCRIBER)
            networkAddressUrlSub.networkInterface = UA_STRING(argv[3]);
            networkAddressUrlSub.url = UA_STRING(argv[2]);
#endif
        }
        /* check MAC address of subscriber alone*/
        else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }

#if defined(SUBSCRIBER)
            networkAddressUrlSub.networkInterface = UA_STRING(argv[2]);
            networkAddressUrlSub.url = UA_STRING(argv[1]);
#endif
        }
    }

    else {
#if defined(PUBLISHER)
        networkAddressUrlEthernet.networkInterface = UA_STRING("enp3s0");
        networkAddressUrlEthernet.url = UA_STRING("opc.eth://a0-36-9f-2d-01-bf"); /* MAC address of subscribing node*/
#endif
#if defined(SUBSCRIBER)
        networkAddressUrlSub.url = UA_STRING("opc.eth://a0-36-9f-04-5b-11"); /* Self MAC address */
        networkAddressUrlSub.networkInterface = UA_STRING("enp3s0");
#endif
    }

#endif

#if defined(PUBLISHER)
#if defined(UPDATE_MEASUREMENTS)
    fpPublisher                   = fopen(filePublishedData, "w");
#endif
#endif
#if defined(SUBSCRIBER)
#if defined(UPDATE_MEASUREMENTS)
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
    /* Server is the new OPCUA model which has both publisher and subscriber configuration */
    /* add axis node and OPCUA pubsub client server counter nodes */
    addServerNodes(server);
#endif

#if defined(PUBLISHER)
    addPubSubConnection(server, &networkAddressUrlEthernet);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
#endif

#if defined(SUBSCRIBER)
    addPubSubConnection1(server, &networkAddressUrlSub);
    addReaderGroup(server);
    addDataSetReader(server);
    addSubscribedVariables(server, readerIdentifier);
#endif
    serverConfigStruct *serverConfig;
    serverConfig = (serverConfigStruct*)malloc(sizeof(serverConfigStruct));
    serverConfig->ServerRun = server;
#if defined(PUBLISHER)
    char threadNamePub[10] = "Publisher";
    pubthreadID = threadCreation(PUB_SCHED_PRIORITY, CORE_TWO, publisherETF, threadNamePub, serverConfig);
#endif

#if defined(SUBSCRIBER)
    char threadNameSub[11] = "Subscriber";
    subthreadID = threadCreation(SUB_SCHED_PRIORITY, CORE_TWO, subscriber, threadNameSub, serverConfig);
#endif
#if defined(PUBLISHER) || defined(SUBSCRIBER)
    char threadNameUserAppl[22] = "UserApplicationPubSub";
    userThreadID = threadCreation(USERAPPLICATION_SCHED_PRIORITY, CORE_THREE, userApplicationPubSub, threadNameUserAppl, serverConfig);
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
    size_t pubLoopVariable = 0;
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
    size_t subLoopVariable = 0;
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
    free(serverConfig);
#endif

#if defined(UPDATE_MEASUREMENTS)
#if defined(PUBLISHER)
    fclose(fpPublisher);
#endif
#endif

#if defined(UPDATE_MEASUREMENTS)
#if defined(SUBSCRIBER)
    fclose(fpSubscriber);
#endif
#endif
    return (int)retval;
}
