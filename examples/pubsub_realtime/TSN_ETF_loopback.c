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
/* Publish interval in milliseconds */
#define             PUB_INTERVAL                    250
/* Cycle time in ns. Eg: For 250us: 250*1000 */
#define             CYCLE_TIME                      250 * 1000
#define             SECONDS_SLEEP                   1
#define             NANO_SECONDS_SLEEP_PUB          999900000L
#define             NANO_SECONDS_SLEEP_SUB          20000L /* 20us sleep time */
#define             MILLI_SECONDS                   1000 * 1000
#define             SECONDS                         1000 * 1000 * 1000
#define             PUB_SCHED_PRIORITY              88
#define             SUB_SCHED_PRIORITY              87
#define             DATA_SET_WRITER_ID              62541
#define             KEY_FRAME_COUNT                 10
#define             MAX_MEASUREMENTS                30000000
#define             CORE_TWO                        2
#define             SECONDS_INCREMENT               1
#define             CONNECTION_NUMBER               2
#define             PORT_NUMBER                     62541
#define             FAILURE_EXIT                    -1
#define             DATETIME_NODECOUNTS             25
#define             CLOCKID                         CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE           "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             UDP_TRANSPORT_PROFILE           "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"

/* This is a hardcoded publisher/subscriber IP address. If the IP address need
 * to be changed for running over UDP, change it in the below lines.
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING is enabled,
 * change in line number 46 and 47 in plugins/ua_pubsub_realtime.c and line number 30 in plugins/ua_pubsub_udp.c
 */

#define             PUBSUB_IP_ADDRESS              "192.168.9.11"
#define             PUBLISHER_MULTICAST_ADDRESS    "opc.udp://224.0.0.22:4840/"
#define             SUBSCRIBER_MULTICAST_ADDRESS   "opc.udp://224.0.0.32:4840/"

/* If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING and UA_ENABLE_PUBSUB_ETH_UADP is enabled,
 * If the Hardcoded publisher/subscriber MAC addresses need to be changed,
 * change in line number 660 and 663  ,
 * If the Hardcoded interface name need to be changed,
 * change in line number 659 and 664
 * To pass the MAC addresses as arguments,
 * Two nodes connected in peer-to-peer network - Run TSN_ETF_publisher.c in node1 and TSN_ETF_loopback.c in node2
 * use the command  ./executable "opc.eth://publisher_mac(MAC of node1)" "opc.eth://subscriber_mac(MAC of node 2)" interface_name
 * To run only subscriber, use the command ./executable "opc.eth://subscriber_mac(MAC of node 2)" interface_name
 * Interface_name - i210 NIC interface
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
UA_NodeId           DateNodeID;
/* Variable for PubSub callback */
UA_ServerCallback   pubCallback;
/* Variables for counter data handling in address space */
UA_UInt64           pubCounterData         = 0;
UA_UInt64           subCounterData         = 0;
UA_Variant          pubCounter;
UA_Variant          subCounter;
UA_Server          *serverCopy;

/* For adding nodes in the server information model */
static void addServerNodes(UA_Server *server);

/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);

#if defined(PUBLISHER)
/* File to store the data and timestamps for different traffic */
FILE               *fpPublisher;
char               *filePublishedData      = "publisher_T5.csv";

/* Thread for publisher */
pthread_t           pubThreadID;

/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;

/* Process scheduling parameter for publisher */
struct sched_param  schedParamPublisher;

/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];
struct timespec     dataModificationTime;

/* Publisher thread routine for ETF */
void               *publisherETF(void *arg);
#endif

#if defined(SUBSCRIBER)
/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
UA_ReaderGroup     *currentReaderGroupCallback;

/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileSubscribedData     = "subscriber_T4.csv";

/* Thread for subscriber */
pthread_t           subThreadID;

/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;

/* Process scheduling parameter for subscriber */
struct sched_param  schedParamSubscriber;

/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];
struct timespec     dataReceiveTime;

/* Subscriber thread routine */
void               *subscriber(void *arg);

/* OPCUA Subscribe API */
void                subscribe(void);
#endif

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
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
    readerConfig.writerGroupId        = 101;
    readerConfig.dataSetWriterId      = DATA_SET_WRITER_ID;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             = DATETIME_NODECOUNTS + 1;
    pMetaData->fields                 = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for (iterator = 0; iterator < DATETIME_NODECOUNTS; iterator++)
    {
        UA_FieldMetaData_init (&pMetaData->fields[iterator]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                         &pMetaData->fields[iterator].dataType);
        pMetaData->fields[iterator].builtInType  = UA_NS0ID_DATETIME;
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
 * Add subscribedvariables to the DataSetReader */
static void addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    UA_Int32 iterator = 0;
    if(server == NULL) {
        return;
    }

    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = DATETIME_NODECOUNTS + 1;
    targetVars.targetVariables     = (UA_FieldTargetDataType *)
                                      UA_calloc(targetVars.targetVariablesSize,
                                      sizeof(UA_FieldTargetDataType));
    for (iterator = 0; iterator < DATETIME_NODECOUNTS; iterator++)
    {
        UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
        targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars.targetVariables[iterator].targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator + 1);
    }
    /* For creating Targetvariable */
    UA_FieldTargetDataType_init(&targetVars.targetVariables[iterator]);
    targetVars.targetVariables[iterator].attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[iterator].targetNodeId = subNodeID;
    UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                  &targetVars);
    UA_TargetVariablesDataType_deleteMembers(&targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
}

/* For one publish callback only... */
UA_Server      *pubServer;
void           *pubData;

/* Add a callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    pubServer                       = server;
    pubCallback                     = callback;
    pubData                         = data;
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
    UA_NodeId dataSetFieldIdent1;
    for (UA_Int32 iterator = 0; iterator < DATETIME_NODECOUNTS; iterator++)
    {
       UA_DataSetFieldConfig dataSetFieldConfig;
       memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
       dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
       dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
       dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
       dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
       dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
       UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent1);
   }

    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig counterValue;
    memset(&counterValue, 0, sizeof(UA_DataSetFieldConfig));
    counterValue.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue.field.variable.fieldNameAlias                      = UA_STRING("Counter Variable");
    counterValue.field.variable.promotedField                       = UA_FALSE;
    counterValue.field.variable.publishParameters.publishedVariable = pubNodeID;
    counterValue.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
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
    writerGroupConfig.publishingInterval = PUB_INTERVAL;
    writerGroupConfig.enabled            = UA_FALSE;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.writerGroupId      = 100;
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
    /* Initialise value for nextnanosleeptime timespec */
    nextnanosleeptime.tv_nsec                      = 0;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptime.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptime.tv_nsec                      = NANO_SECONDS_SLEEP_PUB;
    nanoSecondFieldConversion(&nextnanosleeptime);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        clock_gettime(CLOCKID, &dataModificationTime);
        UA_Variant_setScalar(&pubCounter, &pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
        UA_NodeId currentNodeId         = UA_NODEID_STRING(1, "PublisherCounter");
        UA_Server_writeValue(pubServer, currentNodeId, pubCounter);
         /* OPC UA Publish */
        pubCallback(pubServer, pubData);
        UA_UInt64 values = 0;
        UA_Variant *cntVal = UA_Variant_new();
        UA_StatusCode retVal            = UA_Server_readValue(pubServer, currentNodeId, cntVal);
        if (retVal == UA_STATUSCODE_GOOD && UA_Variant_isScalar(cntVal) && cntVal->type == &UA_TYPES[UA_TYPES_UINT64]) {
            values =  *(UA_UInt64*)cntVal->data;
        }

        if (subCounterData > values)
            updateMeasurementsPublisher(dataModificationTime, subCounterData);

        pubCounterData = subCounterData;
        UA_Variant_delete(cntVal);
        nextnanosleeptime.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextnanosleeptime);
      }

       return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)

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

void *subscriber(void *arg) {
   currentReaderGroupCallback = UA_ReaderGroup_findRGbyId(serverCopy, readerGroupIdentifier);
   while (running) {
       subscribe();
  }

   return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)
void subscribe(void) {
    /* TODO  Modify Packet preparation logic **
     * The existing implementation in nanosleep*/
    struct timespec sleepTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = NANO_SECONDS_SLEEP_SUB;
    nanosleep(&sleepTime, NULL);
    UA_ReaderGroup_subscribeCallback(serverCopy, currentReaderGroupCallback);
    const UA_NodeId nodeid = UA_NODEID_STRING(1,"SubscriberCounter");
    UA_Variant_init(&subCounter);
    clock_gettime(CLOCKID, &dataReceiveTime);
    UA_Server_readValue(serverCopy, nodeid, &subCounter);
    if (*(UA_UInt64 *)subCounter.data > subCounterData) {
        updateMeasurementsSubscriber(dataReceiveTime, *(UA_UInt64 *)subCounter.data);
        subCounterData = *(UA_UInt64 *)subCounter.data;
    }

    UA_Variant_deleteMembers(&subCounter);
}
#endif

/**
 * **Creation of nodes**
 * The addServerNodes function is used to create the publisher and subscriber
 * nodes.
 */
static void addServerNodes(UA_Server *server) {
    UA_NodeId counterId;
    UA_ObjectAttributes oAttr    = UA_ObjectAttributes_default;
    oAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Counter Object");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Counter Object"), UA_NODEID_NULL,
                            oAttr, NULL, &counterId);

    UA_VariableAttributes p4Attr = UA_VariableAttributes_default;
    UA_UInt64 pubcountervalue      = 0;
    UA_Variant_setScalar(&p4Attr.value, &pubcountervalue, &UA_TYPES[UA_TYPES_UINT64]);
    p4Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    UA_NodeId newNodeId          = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, counterId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, p4Attr, NULL, &pubNodeID);
    UA_VariableAttributes p5Attr = UA_VariableAttributes_default;
    UA_UInt64 subcountervalue      = 0;
    UA_Variant_setScalar(&p5Attr.value, &subcountervalue, &UA_TYPES[UA_TYPES_UINT64]);
    p5Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId                    = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, counterId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, p5Attr, NULL, &subNodeID);
    for (UA_Int32 iterator = 0; iterator < DATETIME_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes p6Attr = UA_VariableAttributes_default;
        UA_UInt64 axis6position = 0;
        UA_Variant_setScalar(&p6Attr.value, &axis6position, &UA_TYPES[UA_TYPES_DATETIME]);
        p6Attr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)iterator + 1), counterId,
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                 UA_QUALIFIEDNAME(1, "DateTime"),
                                 UA_NODEID_NULL, p6Attr, NULL, &DateNodeID);
    }

}

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
    UA_Int32         errorSetAffinity    = 0;
    UA_Server       *server              = UA_Server_new();
    UA_ServerConfig *config              = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

#if defined(PUBLISHER)
    UA_NetworkAddressUrlDataType networkAddressUrlEthernet;
#endif
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrlSub;
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
        networkAddressUrlEthernet.networkInterface = UA_STRING("enp4s0");
        networkAddressUrlEthernet.url = UA_STRING("opc.eth://00-1b-21-de-c4-3f"); /* MAC address of subscribing node*/
#endif
#if defined(SUBSCRIBER)
        networkAddressUrlSub.url = UA_STRING("opc.eth://00-1b-21-de-ee-a9"); /* Self MAC address */
        networkAddressUrlSub.networkInterface = UA_STRING("enp4s0");
#endif
    }

#endif

    #if defined(PUBLISHER)
    fpPublisher                   = fopen(filePublishedData, "w");
#endif
#if defined(SUBSCRIBER)
    fpSubscriber                  = fopen(fileSubscribedData, "w");
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
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                = UA_STRING("Subscriber Connection");
    connectionConfig.enabled             = UA_TRUE;
#if defined(UA_ENABLE_PUBSUB_ETH_UADP)
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = networkAddressUrlSub;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = {UA_STRING(PUBSUB_IP_ADDRESS),
                                                               UA_STRING(SUBSCRIBER_MULTICAST_ADDRESS)};
    connectionConfig.transportProfileUri                    = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    retval    = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if (retval == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The PubSub Connection was created successfully!");

    serverCopy = server;
    addReaderGroup(serverCopy);
    addDataSetReader(serverCopy);
    addSubscribedVariables(serverCopy, readerIdentifier);

#endif

#if defined(PUBLISHER)
    /* Core affinity set for publisher */
    cpu_set_t cpusetPub;
    /* Return the ID for publisher thread */
    pubThreadID = pthread_self();
    schedParamPublisher.sched_priority = PUB_SCHED_PRIORITY; /* sched_get_priority_max(SCHED_FIFO) */

    returnValue = pthread_setschedparam(pubThreadID, SCHED_FIFO, &schedParamPublisher);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"pthread_setschedparam: failed\n");
    }

    CPU_ZERO(&cpusetPub);
    CPU_SET(CORE_TWO, &cpusetPub);
    errorSetAffinity = pthread_setaffinity_np(pubThreadID, sizeof(cpu_set_t), &cpusetPub);
    if (errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        return -1;
    }

    /* TODO: Bring Generic code logic for Multiple Publisher thread creation */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
              "pthread_setschedparam: publisher thread priority is %d \n",
              schedParamPublisher.sched_priority);
    returnValue = pthread_create(&pubThreadID, NULL, &publisherETF, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"publisherETF: cannot create thread\n");
        exit(1);
    }

    returnValue = pthread_getaffinity_np(pubThreadID, sizeof(cpu_set_t), &cpusetPub);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Get affinity fail\n");
    }

    if (CPU_ISSET(CORE_TWO, &cpusetPub)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CPU CORE: %d\n", CORE_TWO);
    }

#endif

#if defined(SUBSCRIBER)
    /* Data structure for representing CPU for subscriber */
    cpu_set_t cpusetSub;
    /* Return the ID for subscriber thread */
    subThreadID = pthread_self();
    /* Get maximum priority for subscriber thread */
    schedParamSubscriber.sched_priority = SUB_SCHED_PRIORITY;
    returnValue = pthread_setschedparam(subThreadID, SCHED_FIFO, &schedParamSubscriber);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"pthread_setschedparam: failed\n");
        exit(1);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
               "\npthread_setschedparam: subscriber thread priority is %d \n",
               schedParamSubscriber.sched_priority);
    CPU_ZERO(&cpusetSub);
    CPU_SET(CORE_TWO, &cpusetSub);
    errorSetAffinity = pthread_setaffinity_np(subThreadID, sizeof(cpu_set_t), &cpusetSub);
    if (errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        return -1;
    }

    /* TODO: Bring Generic code logic for Multiple Subscriber thread creation */
    returnValue = pthread_create(&subThreadID, NULL, &subscriber, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"subscriber: cannot create thread\n");
    }

    if (CPU_ISSET(CORE_TWO, &cpusetSub)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CPU CORE: %d\n", CORE_TWO);
    }

#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    retval |= UA_Server_run(server, &running);
#endif

#if defined(PUBLISHER)
    returnValue = pthread_join(pubThreadID, NULL);
    if (returnValue != 0) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
               "\nPthread Join Failed for publisher thread:%d\n", returnValue);
    }

#endif

#if defined(SUBSCRIBER)
    returnValue = pthread_join(subThreadID, NULL);
    if (returnValue != 0) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
               "\nPthread Join Failed for subscriber thread:%d\n", returnValue);
    }

#endif

#if defined(PUBLISHER)
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

#if defined(SUBSCRIBER)
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

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    removeServerNodes(server);
    UA_Server_delete(server);
#endif

#if defined(PUBLISHER)
    fclose(fpPublisher);
#endif

#if defined(SUBSCRIBER)
    fclose(fpSubscriber);
#endif

    return (int)retval;
}
