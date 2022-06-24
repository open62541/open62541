/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Realtime Publish Example
 * ------------------------
 *
 * This tutorial shows publishing and subscribing information in Realtime.
 * This example has both Publisher and Subscriber(used as threads, running in different core), the Publisher thread publishes counterdata
 * (an incremental data), that is subscribed by the Subscriber thread of pubsub_TSN_loopback.c example. The Publisher thread of
 * pusbub_TSN_loopback.c publishes the received counterdata, which is subscribed by the Subscriber thread of this example.
 * Thus a round-trip of counterdata is achieved. User application function of publisher and subscriber is used to collect the publisher and
 * subscriber data.
 *
 * Another additional feature called the Blocking Socket is employed in the Subscriber thread. When using Blocking Socket,
 * the Subscriber thread remains in "blocking mode" until a message is received from every wake up time of the thread. In other words,
 * the timeout is overwritten and the thread continuously waits for the message from every wake up time of the thread.
 * Once the message is received, the Subscriber thread updates the value in the Information Model, sleeps up to wake up time and
 * again waits for the next message. This process is repeated until the application is terminated.
 *
 * Run step of the example is as mentioned below:
 *
 * ./bin/examples/pubsub_TSN_publisher_multiple_thread -interface <interface> -operBaseTime <Basetime> -monotonicOffset <offset>
 *
 * For more options, run ./bin/examples/pubsub_TSN_publisher_multiple_thread -h
 */

/**
 *  Trace point setup
 *
 *            +--------------+                        +----------------+
 *         T1 | OPCUA PubSub |  T8                 T5 | OPCUA loopback |  T4
 *         |  |  Application |  ^                  |  |  Application   |  ^
 *         |  +--------------+  |                  |  +----------------+  |
 *   User  |  |              |  |                  |  |                |  |
 *   Space |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *  ----------|--------------|------------------------|----------------|-------
 *         |  |    Node 1    |  |                  |  |     Node 2     |  |
 *   Kernel|  |              |  |                  |  |                |  |
 *   Space |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *         v  +--------------+  |                  v  +----------------+  |
 *         T2 |  TX tcpdump  |  T7<----------------T6 |   RX tcpdump   |  T3
 *         |  +--------------+                        +----------------+  ^
 *         |                                                              |
 *         ----------------------------------------------------------------
 */

#define _GNU_SOURCE
//#include "open62541.h"
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/io.h>
#include <getopt.h>

/* For thread operations */
#include <pthread.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/log.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/pubsub_udp.h>

#include "ua_pubsub.h"

/*to find load of each thread
 * ps -L -o pid,pri,%cpu -C pubsub_TSN_publisher_multiple_thread */

/* Configurable Parameters */
//If you disable the below macro then two way communication then only publisher will be active
#define             TWO_WAY_COMMUNICATION
/* Cycle time in milliseconds */
#define             DEFAULT_CYCLE_TIME                    0.25
/* Qbv offset */
#define             DEFAULT_QBV_OFFSET                    125
#define             DEFAULT_SOCKET_PRIORITY               7
#define             PUBLISHER_ID                          2234
#define             WRITER_GROUP_ID                       101
#define             DATA_SET_WRITER_ID                    62541
#define             DEFAULT_PUBLISHING_MAC_ADDRESS        "opc.eth://01-00-5E-7F-00-01:8.7"
#define             DEFAULT_PUBLISHER_MULTICAST_ADDRESS   "opc.udp://224.0.0.22:4840/"
#define             PUBLISHER_ID_SUB                      2235
#define             WRITER_GROUP_ID_SUB                   100
#define             DATA_SET_WRITER_ID_SUB                62541

#define             REPEATED_NODECOUNTS                   2    // Default to publish 64 bytes
#define             PORT_NUMBER                           62541
#define             DEFAULT_PUBAPP_THREAD_PRIORITY        85
#define             DEFAULT_PUBAPP_THREAD_CORE            1

#define             DEFAULT_SUBAPP_THREAD_PRIORITY        90
#define             DEFAULT_SUBAPP_THREAD_CORE            0
#define             DEFAULT_SUBSCRIBING_MAC_ADDRESS       "opc.eth://01-00-5E-00-00-01:8.7"
#define             DEFAULT_SUBSCRIBER_MULTICAST_ADDRESS  "opc.udp://224.0.0.32:4840/"

/* Non-Configurable Parameters */
/* Milli sec and sec conversion to nano sec */
#define             MILLI_SECONDS                          1000000
#if defined(__arm__)
#define             SECONDS                                1e9
#else
#define             SECONDS                                1000000000
#endif
#define             SECONDS_SLEEP                          5

#define             MAX_MEASUREMENTS                       100000
#define             SECONDS_INCREMENT                      1
#ifndef CLOCK_MONOTONIC
#define             CLOCK_MONOTONIC                        1
#endif
#define             CLOCKID                                CLOCK_MONOTONIC
#define             ETH_TRANSPORT_PROFILE                  "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             UDP_TRANSPORT_PROFILE                  "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"


/* If the Hardcoded publisher/subscriber MAC addresses need to be changed,
 * change PUBLISHING_MAC_ADDRESS and SUBSCRIBING_MAC_ADDRESS
 */

/* Set server running as true */
UA_Boolean        runningServer           = UA_TRUE;

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
char*             pubUri               = DEFAULT_PUBLISHING_MAC_ADDRESS;
char*             subUri               = DEFAULT_SUBSCRIBING_MAC_ADDRESS;
#else
char*             pubUri               = DEFAULT_PUBLISHER_MULTICAST_ADDRESS;
char*             subUri               = DEFAULT_SUBSCRIBER_MULTICAST_ADDRESS;
#endif
static UA_Double  cycleTimeInMsec      = DEFAULT_CYCLE_TIME;
static UA_Int32   socketPriority       = DEFAULT_SOCKET_PRIORITY;
static UA_Int32   pubAppPriority       = DEFAULT_PUBAPP_THREAD_PRIORITY;
static UA_Int32   subAppPriority       = DEFAULT_SUBAPP_THREAD_PRIORITY;
static UA_Int32   pubAppCore           = DEFAULT_PUBAPP_THREAD_CORE;
static UA_Int32   subAppCore           = DEFAULT_SUBAPP_THREAD_CORE;
static UA_Int32   qbvOffset            = DEFAULT_QBV_OFFSET;
static UA_Boolean disableSoTxtime      = UA_TRUE;
static UA_Boolean enableCsvLog         = UA_FALSE;
static UA_Boolean consolePrint         = UA_FALSE;
static UA_Boolean enableBlockingSocket = UA_FALSE;
static UA_Boolean signalTerm           = UA_FALSE;

/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_NodeId           connectionIdent;
UA_NodeId           publishedDataSetIdent;
UA_NodeId           writerGroupIdent;
UA_NodeId           pubNodeID;
UA_NodeId           runningPubStatusNodeID;
UA_NodeId           pubRepeatedCountNodeID;

/* Variables for counter data handling in address space */
UA_UInt64           *pubCounterData = NULL;
UA_DataValue        *pubDataValueRT = NULL;
UA_Boolean          *runningPub = NULL;
UA_DataValue        *runningPubDataValueRT = NULL;
UA_UInt64           *repeatedCounterData[REPEATED_NODECOUNTS] = {NULL};
UA_DataValue        *repeatedDataValueRT[REPEATED_NODECOUNTS] = {NULL};

#ifdef TWO_WAY_COMMUNICATION
UA_NodeId           subNodeID;
UA_NodeId           subRepeatedCountNodeID;
UA_NodeId           runningSubStatusNodeID;
UA_UInt64           *subCounterData = NULL;
UA_DataValue        *subDataValueRT = NULL;
UA_Boolean          *runningSub = NULL;
UA_DataValue        *runningSubDataValueRT =  NULL;
UA_UInt64           *subRepeatedCounterData[REPEATED_NODECOUNTS] = {NULL};
UA_DataValue        *subRepeatedDataValueRT[REPEATED_NODECOUNTS] = {NULL};
#endif
/**
 * **CSV file handling**
 *
 * csv files are written for pubSubApp thread.
 * csv files include the counterdata that is being either Published or Subscribed
 * along with the timestamp. These csv files can be used to compute latency for following
 * combinations of Tracepoints, T1-T4 and T1-T8.
 *
 * T1-T8 - Gives the Round-trip time of a counterdata, as the value published by the Publisher thread
 * in pubsub_TSN_publisher_multiple_thread.c example is subscribed by the pubSubApp thread in pubsub_TSN_loopback_single_thread.c
 * example and is published back to the pubsub_TSN_publisher_multiple_thread.c example
 */
/* File to store the data and timestamps for different traffic */
FILE               *fpPublisher;
char               *filePublishedData      = "publisher_T1.csv";
/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;
/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];

struct timespec     dataModificationTime;

#ifdef TWO_WAY_COMMUNICATION
/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileSubscribedData     = "subscriber_T8.csv";
/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;
/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];

/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
struct timespec     dataReceiveTime;
UA_UInt64           previousSubCounterData;
UA_NodeId           readerGroupIdentifier;
UA_NodeId           readerIdentifier;
UA_DataSetReaderConfig readerConfig;
#endif

/* Structure to define thread parameters */
typedef struct {
UA_Server*                   server;
void*                        pubData;
void*                        subData;
UA_ServerCallback            pubCallback;
UA_ServerCallback            subCallback;
UA_Duration                  interval_ms;
UA_UInt64                    operBaseTime;
UA_UInt64                    monotonicOffset;
UA_UInt64                    packetLossCount;
} threadArgPubSub;

threadArgPubSub *threadArgPubSub1;

/* Pub application thread routine */
void *pubApp(void *arg);
/* For adding nodes in the server information model */
static void addServerNodes(UA_Server *server);
/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);
/* To create multi-threads */
static pthread_t threadCreation(UA_Int16 threadPriority, size_t coreAffinity, void *(*thread) (void *),
                                char *applicationName, void *serverConfig);
void userApplicationPublisher(UA_UInt64 monotonicOffsetValue);
#ifdef TWO_WAY_COMMUNICATION
/* Sub application thread routine */
void *subApp(void *arg);
void userApplicationSubscriber(UA_UInt64 monotonicOffsetValue);
#endif

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    signalTerm = UA_TRUE;
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
       timeSpecValue->tv_nsec -= (__syscall_slong_t)SECONDS;
    }

}

/**
 * **Custom callback handling**
 *
 * Custom callback thread handling overwrites the default timer based
 * callback function with the custom (user-specified) callback interval. */
/* Add a callback for cyclic repetition */
static UA_StatusCode
addPubSubApplicationCallback(UA_Server *server, UA_NodeId identifier,
                             UA_ServerCallback callback,
                             void *data, UA_Double interval_ms,
                             UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                             UA_UInt64 *callbackId) {

    /* Check the writer group identifier and create the thread accordingly */
    if(UA_NodeId_equal(&identifier, &writerGroupIdent)) {
        threadArgPubSub1->pubData        = data;
        threadArgPubSub1->pubCallback    = callback;
        threadArgPubSub1->interval_ms    = interval_ms;
    }
    else {
        threadArgPubSub1->subData        = data;
        threadArgPubSub1->subCallback    = callback;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
changePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier,
                                UA_UInt64 callbackId, UA_Double interval_ms,
                                UA_DateTime *baseTime, UA_TimerPolicy timerPolicy) {
    /* Callback interval need not be modified as it is thread based implementation.
     * The thread uses nanosleep for calculating cycle time and modification in
     * nanosleep value changes cycle time */
    return UA_STATUSCODE_GOOD;
}


/* Remove the callback added for cyclic repetition */
static void
removePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId) {
/* ToDo: Handle thread id */
}

/**
 * **External data source handling**
 *
 * If the external data source is written over the information model, the
 * externalDataWriteCallback will be triggered. The user has to take care and assure
 * that the write leads not to synchronization issues and race conditions. */
static UA_StatusCode
externalDataWriteCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, const UA_NumericRange *range,
                          const UA_DataValue *data){
    //node values are updated by using variables in the memory
    //UA_Server_write is not used for updating node values.
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
externalDataReadNotificationCallback(UA_Server *server, const UA_NodeId *sessionId,
                                     void *sessionContext, const UA_NodeId *nodeid,
                                     void *nodeContext, const UA_NumericRange *range){
    //allow read without any preparation
    return UA_STATUSCODE_GOOD;
}

#ifdef TWO_WAY_COMMUNICATION
/**
 * **Subscriber**
 *
 * Create connection, readergroup, datasetreader, subscribedvariables for the Subscriber thread.
 */
static void
addPubSubConnectionSubscriber(UA_Server *server, UA_String *transportProfile,
                              UA_NetworkAddressUrlDataType *networkAddressUrlSubscriber){
    UA_StatusCode    retval                                 = UA_STATUSCODE_GOOD;
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Subscriber Connection");
    connectionConfig.enabled                                = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = *networkAddressUrlSubscriber;

    connectionConfig.transportProfileUri                    = *transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric                    = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if (retval == UA_STATUSCODE_GOOD)
         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"The PubSub Connection was created successfully!");
}

/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    if(server == NULL)
        return;

    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name    = UA_STRING("ReaderGroup");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    readerGroupConfig.subscribingInterval = cycleTimeInMsec;
    /* Timeout is modified when blocking socket is enabled, and the default timeout is used when blocking socket is disabled */
    if (enableBlockingSocket == UA_FALSE)
        readerGroupConfig.timeout = 50;  // As we run in 250us cycle time, modify default timeout (1ms) to 50us
    else {
        readerGroupConfig.enableBlockingSocket = UA_TRUE;
        readerGroupConfig.timeout = 0;  //Blocking  socket
    }

    readerGroupConfig.pubsubManagerCallback.addCustomCallback = addPubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.changeCustomCallback = changePubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.removeCustomCallback = removePubSubApplicationCallback;

    UA_Server_addReaderGroup(server, connectionIdentSubscriber, &readerGroupConfig,
                             &readerGroupIdentifier);
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static void addSubscribedVariables (UA_Server *server) {
    UA_Int32 iterator = 0;
    UA_Int32 iteratorRepeatedCount = 0;

    if(server == NULL) {
        return;
    }

    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable*) UA_calloc((REPEATED_NODECOUNTS + 2), sizeof(UA_FieldTargetVariable));
    if(!targetVars) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "FieldTargetVariable - Bad out of memory");
        return;
    }

    runningSub = UA_Boolean_new();
    if(!runningSub) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "runningsub - Bad out of memory");
        UA_free(targetVars);
        return;
    }

    *runningSub = UA_TRUE;
    runningSubDataValueRT = UA_DataValue_new();
    if(!runningSubDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "runningsubDatavalue - Bad out of memory");
        UA_free(targetVars);
        return;
    }

    UA_Variant_setScalar(&runningSubDataValueRT->value, runningSub, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningSubDataValueRT->hasValue = UA_TRUE;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend runningSubvalueBackend;
    runningSubvalueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    runningSubvalueBackend.backend.external.value = &runningSubDataValueRT;
    runningSubvalueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    runningSubvalueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)30000), runningSubvalueBackend);

    UA_FieldTargetDataType_init(&targetVars[iterator].targetVariable);
    targetVars[iterator].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[iterator].targetVariable.targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)30000);
    iterator++;
    /* For creating Targetvariable */
    for (iterator = 1, iteratorRepeatedCount = 0; iterator <= REPEATED_NODECOUNTS; iterator++, iteratorRepeatedCount++)
    {
        subRepeatedCounterData[iteratorRepeatedCount] = UA_UInt64_new();
        if(!subRepeatedCounterData[iteratorRepeatedCount]) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SubscribeRepeatedCounterData - Bad out of memory");
            UA_free(targetVars);
            return;
        }

        *subRepeatedCounterData[iteratorRepeatedCount] = 0;
        subRepeatedDataValueRT[iteratorRepeatedCount] = UA_DataValue_new();
        if(!subRepeatedDataValueRT[iteratorRepeatedCount]) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SubscribeRepeatedCounterDataValue - Bad out of memory");
            UA_free(targetVars);
            return;
        }

        UA_Variant_setScalar(&subRepeatedDataValueRT[iteratorRepeatedCount]->value, subRepeatedCounterData[iteratorRepeatedCount], &UA_TYPES[UA_TYPES_UINT64]);
        subRepeatedDataValueRT[iteratorRepeatedCount]->hasValue = UA_TRUE;
        /* Set the value backend of the above create node to 'external value source' */
        UA_ValueBackend valueBackend;
        valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
        valueBackend.backend.external.value = &subRepeatedDataValueRT[iteratorRepeatedCount];
        valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
        valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
        UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)iteratorRepeatedCount+50000), valueBackend);

        UA_FieldTargetDataType_init(&targetVars[iterator].targetVariable);
        targetVars[iterator].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[iterator].targetVariable.targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)iteratorRepeatedCount + 50000);
    }

    subCounterData = UA_UInt64_new();
    if(!subCounterData) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SubscribeCounterData - Bad out of memory");
        UA_free(targetVars);
        return;
    }

    *subCounterData = 0;
    subDataValueRT = UA_DataValue_new();
    if(!subDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SubscribeDataValue - Bad out of memory");
        UA_free(targetVars);
        return;
    }

    UA_Variant_setScalar(&subDataValueRT->value, subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    subDataValueRT->hasValue = UA_TRUE;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &subDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, subNodeID, valueBackend);

    UA_FieldTargetDataType_init(&targetVars[iterator].targetVariable);
    targetVars[iterator].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[iterator].targetVariable.targetNodeId = subNodeID;

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = targetVars;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = REPEATED_NODECOUNTS + 2;
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
    UA_UInt16 publisherIdentifier     = PUBLISHER_ID_SUB;
    readerConfig.publisherId.type     = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data     = &publisherIdentifier;
    readerConfig.writerGroupId        = WRITER_GROUP_ID_SUB;
    readerConfig.dataSetWriterId      = DATA_SET_WRITER_ID_SUB;

    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask           = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                                 (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             = REPEATED_NODECOUNTS + 2;
    pMetaData->fields                 = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    /* Boolean  DataType */
    UA_FieldMetaData_init (&pMetaData->fields[iterator]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[iterator].dataType);
    pMetaData->fields[iterator].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[iterator].valueRank   = -1; /* scalar */
    iterator++;
    for (iterator = 1; iterator <= REPEATED_NODECOUNTS; iterator++)
    {
        UA_FieldMetaData_init (&pMetaData->fields[iterator]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                         &pMetaData->fields[iterator].dataType);
        pMetaData->fields[iterator].builtInType = UA_NS0ID_UINT64;
        pMetaData->fields[iterator].valueRank   = -1; /* scalar */
    }

    /* Unsigned Integer DataType */
    UA_FieldMetaData_init (&pMetaData->fields[iterator]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                    &pMetaData->fields[iterator].dataType);
    pMetaData->fields[iterator].builtInType = UA_NS0ID_UINT64;
    pMetaData->fields[iterator].valueRank   = -1; /* scalar */

    /* Setup Target Variables in DSR config */
    addSubscribedVariables(server);

    /* Setting up Meta data configuration in DataSetReader */
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                               &readerIdentifier);

    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}
#endif

/**
 * **Publisher**
 *
 * Create connection, writergroup, datasetwriter and publisheddataset for Publisher thread.
 */
static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrlPub){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Publisher Connection");
    connectionConfig.enabled                                = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl          = *networkAddressUrlPub;
    connectionConfig.transportProfileUri                    = *transportProfile;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric                    = PUBLISHER_ID;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    /* Connection options are given as Key/Value Pairs - Sockprio and Txtime */
    UA_KeyValuePair connectionOptions[2];
#else
    UA_KeyValuePair connectionOptions[1];
#endif
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sockpriority");
    UA_Variant_setScalar(&connectionOptions[0].value, &socketPriority, &UA_TYPES[UA_TYPES_UINT32]);
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "enablesotxtime");
    UA_Variant_setScalar(&connectionOptions[1].value, &disableSoTxtime, &UA_TYPES[UA_TYPES_BOOLEAN]);
#endif
    connectionConfig.connectionProperties     = connectionOptions;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    connectionConfig.connectionPropertiesSize = 2;
#else
    connectionConfig.connectionPropertiesSize = 1;
#endif
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/* PublishedDataset handling */
static void
addPublishedDataSet(UA_Server *server) {
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name                 = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/* DataSetField handling */
static void
_addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent1;
    UA_DataSetFieldConfig dataSetFieldConfig;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
    staticValueSource = UA_DataValue_new();
#endif

    UA_NodeId dataSetFieldIdentRunning;
    UA_DataSetFieldConfig dsfConfigPubStatus;
    memset(&dsfConfigPubStatus, 0, sizeof(UA_DataSetFieldConfig));

    runningPub = UA_Boolean_new();
    if(!runningPub) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "runningPub - Bad out of memory");
        return;
    }

    *runningPub = UA_TRUE;
    runningPubDataValueRT = UA_DataValue_new();
    if(!runningPubDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "runningPubDataValue - Bad out of memory");
        return;
    }

    UA_Variant_setScalar(&runningPubDataValueRT->value, runningPub, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningPubDataValueRT->hasValue = UA_TRUE;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend runningPubvalueBackend;
    runningPubvalueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    runningPubvalueBackend.backend.external.value = &runningPubDataValueRT;
    runningPubvalueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    runningPubvalueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)20000), runningPubvalueBackend);

    /* setup RT DataSetField config */
    dsfConfigPubStatus.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
    dsfConfigPubStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, (UA_UInt32)20000);

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfigPubStatus, &dataSetFieldIdentRunning);
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
       memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));

       repeatedCounterData[iterator] = UA_UInt64_new();
       if(!repeatedCounterData[iterator]) {
           UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PublishRepeatedCounter - Bad out of memory");
           return;
       }

       *repeatedCounterData[iterator] = 0;
       repeatedDataValueRT[iterator] = UA_DataValue_new();
       if(!repeatedDataValueRT[iterator]) {
           UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PublishRepeatedCounterDataValue - Bad out of memory");
           return;
       }

       UA_Variant_setScalar(&repeatedDataValueRT[iterator]->value, repeatedCounterData[iterator], &UA_TYPES[UA_TYPES_UINT64]);
       repeatedDataValueRT[iterator]->hasValue = UA_TRUE;

       /* Set the value backend of the above create node to 'external value source' */
       UA_ValueBackend valueBackend;
       valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
       valueBackend.backend.external.value = &repeatedDataValueRT[iterator];
       valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
       valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
       UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000), valueBackend);

       /* setup RT DataSetField config */
       dataSetFieldConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
       dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);

       UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent1);
   }

    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));

    pubCounterData = UA_UInt64_new();
    if(!pubCounterData) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PublishCounter - Bad out of memory");
        return;
    }

    *pubCounterData = 0;
    pubDataValueRT = UA_DataValue_new();
    if(!pubDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PublishDataValue - Bad out of memory");
        return;
    }

    UA_Variant_setScalar(&pubDataValueRT->value, pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    pubDataValueRT->hasValue = UA_TRUE;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &pubDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, pubNodeID, valueBackend);

    /* setup RT DataSetField config */
    dsfConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
    dsfConfig.field.variable.publishParameters.publishedVariable = pubNodeID;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
}

/* WriterGroup handling */
static void
addWriterGroup(UA_Server *server) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name                                 = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval                   = cycleTimeInMsec;
    writerGroupConfig.enabled                              = UA_FALSE;
    writerGroupConfig.encodingMimeType                     = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.writerGroupId                        = WRITER_GROUP_ID;
    writerGroupConfig.rtLevel                              = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.pubsubManagerCallback.addCustomCallback = addPubSubApplicationCallback;
    writerGroupConfig.pubsubManagerCallback.changeCustomCallback = changePubSubApplicationCallback;
    writerGroupConfig.pubsubManagerCallback.removeCustomCallback = removePubSubApplicationCallback;

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

/* DataSetWriter handling */
static void
addDataSetWriter(UA_Server *server) {
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name            = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATA_SET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount   = 10;
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
                            UA_UInt64 counterValue, UA_UInt64 monotonicOffsetValue) {
    if(measurementsPublisher >= MAX_MEASUREMENTS) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Publisher: Maximum log measurements reached - Closing the application");
        signalTerm = UA_TRUE;
        return;
    }

    if(consolePrint)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Pub:%"PRId64",%ld.%09ld\n", counterValue, start_time.tv_sec, start_time.tv_nsec);

    if (signalTerm != UA_TRUE){
        UA_UInt64 actualTimeValue = (UA_UInt64)((start_time.tv_sec * SECONDS) + start_time.tv_nsec) + monotonicOffsetValue;
        publishTimestamp[measurementsPublisher].tv_sec       = (__time_t)(actualTimeValue/(UA_UInt64)SECONDS);
        publishTimestamp[measurementsPublisher].tv_nsec      = (__syscall_slong_t)(actualTimeValue%(UA_UInt64)SECONDS);
        publishCounterValue[measurementsPublisher]     = counterValue;
        measurementsPublisher++;
    }
}

/**
 * userApplication function is used to increment the counterdata to be published by the Publisher  and
 * writes the updated counterdata in distinct csv files
 **/
void userApplicationPublisher(UA_UInt64 monotonicOffsetValue) {

    *pubCounterData      = *pubCounterData + 1;
    for (UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
        *repeatedCounterData[iterator] = *repeatedCounterData[iterator] + 1;

    clock_gettime(CLOCKID, &dataModificationTime);


    if (enableCsvLog || consolePrint) {
        if (*pubCounterData > 0)
            updateMeasurementsPublisher(dataModificationTime, *pubCounterData, monotonicOffsetValue);
    }

    /* *runningPub variable made false and send to the publisher application which is running in another node
       which will close the application during blocking socket condition */
    if (signalTerm == UA_TRUE) {
        *runningPub = UA_FALSE;
    }

}
#ifdef TWO_WAY_COMMUNICATION
/**
 * **Subscribed data handling**
 *
 * The subscribed data is updated in the array using this function Subscribed data handling**
 */
static void
updateMeasurementsSubscriber(struct timespec receive_time, UA_UInt64 counterValue, UA_UInt64 monotonicOffsetValue) {
    if(measurementsSubscriber >= MAX_MEASUREMENTS) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Subscriber: Maximum log measurements reached - Closing the application");
        signalTerm = UA_TRUE;
        return;
    }

    if(consolePrint)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Sub:%"PRId64",%ld.%09ld\n", counterValue, receive_time.tv_sec, receive_time.tv_nsec);

    if (signalTerm != UA_TRUE){
        UA_UInt64 actualTimeValue = (UA_UInt64)((receive_time.tv_sec * SECONDS) + receive_time.tv_nsec) + monotonicOffsetValue;
        subscribeTimestamp[measurementsSubscriber].tv_sec = (__time_t)(actualTimeValue/(UA_UInt64)SECONDS);
        subscribeTimestamp[measurementsSubscriber].tv_nsec = (__syscall_slong_t)(actualTimeValue%(UA_UInt64)SECONDS);
        subscribeCounterValue[measurementsSubscriber]  = counterValue;
        measurementsSubscriber++;
    }
}

/**
 * userApplicationSubscriber function is used to read the data from Information Model for the Subscriber and
 * writes the updated counterdata in distinct csv files
 **/
void userApplicationSubscriber(UA_UInt64 monotonicOffsetValue) {

    clock_gettime(CLOCKID, &dataReceiveTime);

    /* Check packet loss count */
    if ((*subCounterData > 0) && (*subCounterData != (previousSubCounterData + 1))) {
        /* Check for duplicate packet */
        if (*subCounterData != previousSubCounterData) {
            UA_UInt64 missedCount = *subCounterData - (previousSubCounterData + 1);
            threadArgPubSub1->packetLossCount += missedCount;
        }
    }

    if (enableCsvLog || consolePrint) {
        if (*subCounterData > 0)
            updateMeasurementsSubscriber(dataReceiveTime, *subCounterData, monotonicOffsetValue);
    }

    previousSubCounterData = *subCounterData;
    if (signalTerm == UA_TRUE)
        *runningSub = UA_FALSE;

}
#endif

/**
 * **Pub thread routine**
 */
void *pubApp(void *arg) {
    UA_Server*        server;
    UA_ServerCallback pubCallback;
    UA_WriterGroup*   currentWriterGroup;
    UA_UInt64         interval_ms;
    struct timespec   nextnanosleeptimePubApplication;
    UA_UInt64 monotonicOffsetValue = 0;

    server             = threadArgPubSub1->server;
    currentWriterGroup = (UA_WriterGroup *)threadArgPubSub1->pubData;
    pubCallback        = threadArgPubSub1->pubCallback;
    interval_ms        = (UA_UInt64)(threadArgPubSub1->interval_ms * MILLI_SECONDS);

    //To synchronize the application along with gating cycle the below calculations are made
    //Below calculations are done for monotonic clock
    struct timespec currentTimeInTs;
    UA_UInt64 addingValueToStartTime;
    clock_gettime(CLOCKID, &currentTimeInTs);
    UA_UInt64 currentTimeInNs = (UA_UInt64)((currentTimeInTs.tv_sec * (SECONDS)) + currentTimeInTs.tv_nsec);
    currentTimeInNs = currentTimeInNs + threadArgPubSub1->monotonicOffset;
    UA_UInt64 timeToStart = currentTimeInNs + (SECONDS_SLEEP * (UA_UInt64)(SECONDS)); //Adding 5 seconds to start the cycle
    if (threadArgPubSub1->operBaseTime != 0){
        UA_UInt64 moduloValueOfOperBaseTime = timeToStart % threadArgPubSub1->operBaseTime;
        if(moduloValueOfOperBaseTime > interval_ms)
            addingValueToStartTime = interval_ms - (moduloValueOfOperBaseTime % interval_ms);
        else
            addingValueToStartTime = interval_ms - (moduloValueOfOperBaseTime);

        timeToStart = timeToStart + addingValueToStartTime;
        timeToStart = timeToStart - (threadArgPubSub1->monotonicOffset);
    }
    else{
        timeToStart = timeToStart - timeToStart%interval_ms;
        timeToStart = timeToStart - (threadArgPubSub1->monotonicOffset);
    }

    UA_UInt64 CycleStartTimeS = (UA_UInt64)(timeToStart / (UA_UInt64)(SECONDS));
    UA_UInt64 CycleStartTimeNs = (UA_UInt64)(timeToStart - (CycleStartTimeS * (UA_UInt64)(SECONDS)));
    nextnanosleeptimePubApplication.tv_sec = (__time_t )(CycleStartTimeS);
    nextnanosleeptimePubApplication.tv_nsec = (__syscall_slong_t)(CycleStartTimeNs);
    nanoSecondFieldConversion(&nextnanosleeptimePubApplication);
    monotonicOffsetValue = threadArgPubSub1->monotonicOffset;

    while (*runningPub) {
        //Sleep for cycle time
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimePubApplication, NULL);

        //Increments the counterdata
        userApplicationPublisher(monotonicOffsetValue);

        //ToDo:Handled only for without SO_TXTIME
        //Call the publish callback to publish the data into the network
        pubCallback(server, currentWriterGroup);

        //Calculate nextwakeup time
        nextnanosleeptimePubApplication.tv_nsec += (__syscall_slong_t)(cycleTimeInMsec * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimePubApplication);
    }

#ifndef TWO_WAY_COMMUNICATION
    runningServer = UA_FALSE;
#endif
    return (void*)NULL;
}

#ifdef TWO_WAY_COMMUNICATION
/**
 * **Sub thread routine**
 */
void *subApp(void *arg) {
    UA_Server*        server;
    UA_ReaderGroup*   currentReaderGroup;
    UA_ServerCallback subCallback;
    UA_UInt64         interval_ms;
	struct timespec   nextnanosleeptimeSubApplication;
	UA_UInt64 monotonicOffsetValue = 0;

    server             = threadArgPubSub1->server;
    currentReaderGroup = (UA_ReaderGroup*)threadArgPubSub1->subData;
    subCallback        = threadArgPubSub1->subCallback;
    interval_ms        = (UA_UInt64)(threadArgPubSub1->interval_ms * MILLI_SECONDS);

    //To synchronize the application along with gating cycle the below calculations are made
    //Below calculations are done for monotonic clock
    struct timespec currentTimeInTs;
    UA_UInt64 addingValueToStartTime;
    clock_gettime(CLOCKID, &currentTimeInTs);
    UA_UInt64 currentTimeInNs = (UA_UInt64)((currentTimeInTs.tv_sec * (SECONDS)) + currentTimeInTs.tv_nsec);
    currentTimeInNs = currentTimeInNs + threadArgPubSub1->monotonicOffset;
    UA_UInt64 timeToStart = currentTimeInNs + (SECONDS_SLEEP * (UA_UInt64)(SECONDS)); //Adding 5 seconds to start the cycle
    if (threadArgPubSub1->operBaseTime != 0){
        UA_UInt64 moduloValueOfOperBaseTime = timeToStart % threadArgPubSub1->operBaseTime;
        if(moduloValueOfOperBaseTime > interval_ms)
            addingValueToStartTime = interval_ms - (moduloValueOfOperBaseTime % interval_ms);
        else
            addingValueToStartTime = interval_ms - (moduloValueOfOperBaseTime);

        timeToStart = timeToStart + addingValueToStartTime;
        timeToStart = timeToStart - (threadArgPubSub1->monotonicOffset);
    }
    else{
        timeToStart = timeToStart - timeToStart%interval_ms;
        timeToStart = timeToStart - (threadArgPubSub1->monotonicOffset);
    }

    UA_UInt64 CycleStartTimeS = (UA_UInt64)(timeToStart / (UA_UInt64)(SECONDS));
    UA_UInt64 CycleStartTimeNs = (UA_UInt64)(timeToStart - (CycleStartTimeS * (UA_UInt64)(SECONDS)));
    nextnanosleeptimeSubApplication.tv_sec = (__time_t )(CycleStartTimeS);
    nextnanosleeptimeSubApplication.tv_nsec = (__syscall_slong_t)(CycleStartTimeNs);
    nanoSecondFieldConversion(&nextnanosleeptimeSubApplication);
    clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSubApplication, NULL);
    monotonicOffsetValue = threadArgPubSub1->monotonicOffset;

    while (*runningSub) {
        //Call the subscriber callback to receive the data
        //Subscriber called at last because during blocking socket condition
        //Publisher cannot publlish packet
        subCallback(server, currentReaderGroup);

        //Check whether there is a packet loss
        userApplicationSubscriber(monotonicOffsetValue);
    }

    sleep(1);
    runningServer = UA_FALSE;
    return (void*)NULL;
}
#endif
/**
 * **Thread creation**
 *
 * The threadcreation functionality creates thread with given threadpriority, coreaffinity. The function returns the threadID of the newly
 * created thread.
 */

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
    if (returnValue != 0)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,":%s Cannot create thread\n", applicationName);

    if (CPU_ISSET(coreAffinity, &cpuset))
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s CPU CORE: %zu\n", applicationName, coreAffinity);

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
    UA_NodeId newNodeId;
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
    publisherAttr.dataType               = UA_TYPES[UA_TYPES_UINT64].typeId;
    UA_Variant_setScalar(&publisherAttr.value, &publishValue, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    newNodeId                            = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, publisherAttr, NULL, &pubNodeID);
#ifdef TWO_WAY_COMMUNICATION
    UA_VariableAttributes subscriberAttr = UA_VariableAttributes_default;
    UA_UInt64 subscribeValue             = 0;
    subscriberAttr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    subscriberAttr.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
    UA_Variant_setScalar(&subscriberAttr.value, &subscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
    subscriberAttr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId                            = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, subscriberAttr, NULL, &subNodeID);
#endif
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodePub = UA_VariableAttributes_default;
        UA_UInt64 repeatedPublishValue        = 0;
        repeatedNodePub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        repeatedNodePub.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
        UA_Variant_setScalar(&repeatedNodePub.value, &repeatedPublishValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodePub.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher RepeatedCounter");
        newNodeId                             = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);
        UA_Server_addVariableNode(server, newNodeId, objectId,
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                 UA_QUALIFIEDNAME(1, "Publisher RepeatedCounter"),
                                 UA_NODEID_NULL, repeatedNodePub, NULL, &pubRepeatedCountNodeID);
    }
    UA_VariableAttributes runningStatusPub = UA_VariableAttributes_default;
    UA_Boolean runningPubStatus            = 0;
    runningStatusPub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&runningStatusPub.value, &runningPubStatus, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningStatusPub.displayName           = UA_LOCALIZEDTEXT("en-US", "RunningStatus Pub");
    runningStatusPub.dataType              = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    newNodeId                              = UA_NODEID_NUMERIC(1, (UA_UInt32)20000);
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "RunningStatus Pub"),
                              UA_NODEID_NULL, runningStatusPub, NULL, &runningPubStatusNodeID);
#ifdef TWO_WAY_COMMUNICATION
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodeSub = UA_VariableAttributes_default;
        UA_DateTime repeatedSubscribeValue;
        UA_Variant_setScalar(&repeatedNodeSub.value, &repeatedSubscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodeSub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        repeatedNodeSub.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
        repeatedNodeSub.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber RepeatedCounter");
        newNodeId                             = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+50000);
        UA_Server_addVariableNode(server, newNodeId, objectId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Subscriber RepeatedCounter"),
                                  UA_NODEID_NULL, repeatedNodeSub, NULL, &subRepeatedCountNodeID);
    }
    UA_VariableAttributes runningStatusSubscriber = UA_VariableAttributes_default;
    UA_Boolean runningSubStatusValue              = 0;
    runningStatusSubscriber.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&runningStatusSubscriber.value, &runningSubStatusValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningStatusSubscriber.displayName           = UA_LOCALIZEDTEXT("en-US", "RunningStatus Sub");
    runningStatusSubscriber.dataType              = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    newNodeId                                     = UA_NODEID_NUMERIC(1, (UA_UInt32)30000);
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "RunningStatus Sub"),
                              UA_NODEID_NULL, runningStatusSubscriber, NULL, &runningSubStatusNodeID);
#endif
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
    UA_NodeId_clear(&pubNodeID);
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, pubRepeatedCountNodeID, UA_TRUE);
        UA_NodeId_clear(&pubRepeatedCountNodeID);
    }
    UA_Server_deleteNode(server, runningPubStatusNodeID, UA_TRUE);
    UA_NodeId_clear(&runningPubStatusNodeID);
#ifdef TWO_WAY_COMMUNICATION
    UA_Server_deleteNode(server, subNodeID, UA_TRUE);
    UA_NodeId_clear(&subNodeID);
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_Server_deleteNode(server, subRepeatedCountNodeID, UA_TRUE);
        UA_NodeId_clear(&subRepeatedCountNodeID);
    }
    UA_Server_deleteNode(server, runningSubStatusNodeID, UA_TRUE);
    UA_NodeId_clear(&runningSubStatusNodeID);
#endif
}

/**
 * **Usage function**
 *
 * The usage function gives the information to run the application.
 *
 * ./bin/examples/pubsub_TSN_loopback_single_thread -interface <interface> -operBaseTime <Basetime> -monotonicOffset <offset>
 *
 * For more options, use ./bin/examples/pubsub_TSN_loopback_single_thread -h.
 */

static void usage(char *appname)
{
    fprintf(stderr,
        "\n"
        "usage: %s [options]\n"
        "\n"
        " -interface         [name]   Use network interface 'name'\n"
        " -cycleTimeInMsec   [num]    Cycle time in milli seconds (default %lf)\n"
        " -socketPriority    [num]    Set publisher SO_PRIORITY to (default %d)\n"
        " -pubAppPriority    [num]    publisher and userApp thread priority value (default %d)\n"
        " -subAppPriority    [num]    subscriber and userApp thread priority value (default %d)\n"
        " -pubAppCore        [num]    Run on CPU for publisher+pubUserApplication thread (default %d)\n"
        " -subAppCore        [num]    Run on CPU for subscriber+subUserApplication thread (default %d)\n"
        " -pubUri            [name]   Publisher Mac address(default %s - where 8 is the VLAN ID and 3 is the PCP)\n"
        "                             or multicast address(default %s)\n"
        " -subUri            [name]   Subscriber Mac address or multicast address (default %s - where 8 is the VLAN ID and 3 is the PCP)\n"
        "                             or multicast address(default %s) \n"
        " -qbvOffset         [num]    QBV offset value (default %d)\n"
        " -operBaseTime [location]    Bastime file location\n"
        " -monotonicOffset [location] Monotonic offset file location\n"
        " -disableSoTxtime            Do not use SO_TXTIME\n"
        " -enableCsvLog               Experimental: To log the data in csv files. Support up to 1 million samples\n"
        " -enableconsolePrint         Experimental: To print the data in console output. Support for higher cycle time\n"
        " -enableBlockingSocket       Run application with blocking socket option. While using blocking socket option need to\n"
        "                             run both the Publisher and Loopback application. Otherwise application will not terminate.\n"
        "\n",
        appname, DEFAULT_CYCLE_TIME, DEFAULT_SOCKET_PRIORITY, \
        DEFAULT_PUBAPP_THREAD_PRIORITY, \
        DEFAULT_SUBAPP_THREAD_PRIORITY, \
        DEFAULT_PUBAPP_THREAD_CORE, \
        DEFAULT_SUBAPP_THREAD_CORE, \
        DEFAULT_PUBLISHING_MAC_ADDRESS, DEFAULT_PUBLISHER_MULTICAST_ADDRESS, \
        DEFAULT_SUBSCRIBING_MAC_ADDRESS, DEFAULT_SUBSCRIBER_MULTICAST_ADDRESS, DEFAULT_QBV_OFFSET);
}

/**
 * **Main Server code**
 */
int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Int32         returnValue         = 0;
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    UA_Server       *server              = UA_Server_new();
    UA_ServerConfig *config              = UA_Server_getConfig(server);
    char            *interface           = NULL;
    UA_Int32         argInputs           = 0;
    UA_Int32         long_index          = 0;
    char            *progname;
    pthread_t        pubAppThreadID;
#ifdef TWO_WAY_COMMUNICATION
    pthread_t        subAppThreadID;
#endif
    char            *operBaseTimeFileName = NULL;
    char            *monotonicOffsetFileName = NULL;
    FILE            *operBaseTimefile;
    FILE            *monotonicOffsetFile;
    UA_String       transportProfile;

    /* Process the command line arguments */
    progname = strrchr(argv[0], '/');
    progname = progname ? 1 + progname : argv[0];

    static struct option long_options[] = {
        {"interface",            required_argument, 0, 'a'},
        {"cycleTimeInMsec",      required_argument, 0, 'b'},
        {"socketPriority",       required_argument, 0, 'c'},
        {"pubAppPriority",       required_argument, 0, 'd'},
        {"subAppPriority",       required_argument, 0, 'e'},
        {"pubAppCore",           required_argument, 0, 'f'},
        {"subAppCore",           required_argument, 0, 'g'},
        {"pubUri",               required_argument, 0, 'h'},
        {"subUri",               required_argument, 0, 'i'},
        {"qbvOffset",            required_argument, 0, 'j'},
        {"operBaseTime",         required_argument, 0, 'k'},
        {"monotonicOffset",      required_argument, 0, 'l'},
        {"disableSoTxtime",      no_argument,       0, 'm'},
        {"enableCsvLog",         no_argument,       0, 'n'},
        {"enableconsolePrint",   no_argument,       0, 'o'},
        {"enableBlockingSocket", no_argument,       0, 'p'},
        {"help",                 no_argument,       0, 'q'},
        {0,                      0,                 0,  0 }
    };

    while ((argInputs = getopt_long_only(argc, argv,"", long_options, &long_index)) != -1) {
        switch (argInputs) {
            case 'a':
                interface = optarg;
                break;
            case 'b':
                cycleTimeInMsec = atof(optarg);
                break;
            case 'c':
                socketPriority = atoi(optarg);
                break;
            case 'd':
                pubAppPriority = atoi(optarg);
                break;
            case 'e':
                subAppPriority = atoi(optarg);
                break;
            case 'f':
                pubAppCore = atoi(optarg);
                break;
            case 'g':
                subAppCore = atoi(optarg);
                break;
            case 'h':
                pubUri = optarg;
                break;
            case 'i':
                subUri = optarg;
                break;
            case 'j':
                qbvOffset = atoi(optarg);
                break;
            case 'k':
                operBaseTimeFileName = optarg;
                break;
            case 'l':
                monotonicOffsetFileName = optarg;
                break;
            case 'm':
                disableSoTxtime = UA_FALSE;
                break;
            case 'n':
                enableCsvLog = UA_TRUE;
                break;
            case 'o':
                consolePrint = UA_TRUE;
                break;
            case 'p':
                /* TODO: Application need to be exited independently */
                enableBlockingSocket = UA_TRUE;
                break;
            case 'q':
                usage(progname);
                return -1;
            case '?':
                usage(progname);
                return -1;
        }
    }

    if (!interface) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Need a network interface to run");
        usage(progname);
        return -1;
    }

    if (cycleTimeInMsec < 0.125) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%f Bad cycle time", cycleTimeInMsec);
        usage(progname);
        return -1;
    }

#ifdef TWO_WAY_COMMUNICATION
    /* The subscriber thread runs in a while loop so while running this application without blocking socket option
     * the running application should be run in the seperate core where no process running in it */
    if (enableBlockingSocket == UA_FALSE)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Without blocking socket option the application will cause issues");
#endif

    if (disableSoTxtime == UA_TRUE) {
       UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "With sotxtime handling not supported so the application will run without soTxtime");
       disableSoTxtime = UA_FALSE;
    }

    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

    UA_NetworkAddressUrlDataType networkAddressUrlPub;

    //If you are running for UDP provide ip address as input. Connection creation
    //Failed if we provide the interface name
    networkAddressUrlPub.networkInterface = UA_STRING(interface);
    networkAddressUrlPub.url              = UA_STRING(pubUri);
#ifdef TWO_WAY_COMMUNICATION
    UA_NetworkAddressUrlDataType networkAddressUrlSub;
    networkAddressUrlSub.networkInterface = UA_STRING(interface);
    networkAddressUrlSub.url              = UA_STRING(subUri);
#endif

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    transportProfile = UA_STRING(ETH_TRANSPORT_PROFILE);
#else
    transportProfile = UA_STRING(UDP_TRANSPORT_PROFILE);
#endif

    if (enableCsvLog)
        fpPublisher                   = fopen(filePublishedData, "w");
#ifdef TWO_WAY_COMMUNICATION
    if (enableCsvLog)
        fpSubscriber                  = fopen(fileSubscribedData, "w");
#endif

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#else
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#endif

    /* Initialize arguments required for the thread to run */
    threadArgPubSub1 = (threadArgPubSub *) UA_malloc(sizeof(threadArgPubSub));

    /* Server is the new OPCUA model which has both publisher and subscriber configuration */
    /* add axis node and OPCUA pubsub client server counter nodes */
    addServerNodes(server);

    addPubSubConnection(server, &transportProfile, &networkAddressUrlPub);
    addPublishedDataSet(server);
    _addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
#ifdef TWO_WAY_COMMUNICATION
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#else
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#endif

    addPubSubConnectionSubscriber(server, &transportProfile, &networkAddressUrlSub);
    addReaderGroup(server);
    addDataSetReader(server);
    UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
#endif
    threadArgPubSub1->server = server;

    if (operBaseTimeFileName != NULL) {
        long double floatValueBaseTime;
        operBaseTimefile = fopen(operBaseTimeFileName, "r");
        fscanf(operBaseTimefile,"%Lf", &floatValueBaseTime);
        uint64_t operBaseTimeInNs = (uint64_t)(floatValueBaseTime * SECONDS);
        threadArgPubSub1->operBaseTime = operBaseTimeInNs;
    }
    else
        threadArgPubSub1->operBaseTime = 0;

    if (monotonicOffsetFileName != NULL) {
        monotonicOffsetFile = fopen(monotonicOffsetFileName, "r");
        char fileParseBuffer[255];
        if (fgets(fileParseBuffer, sizeof(fileParseBuffer), monotonicOffsetFile) != NULL) {
            UA_UInt64 monotonicOffsetValueSecondsField = 0;
            UA_UInt64 monotonicOffsetValueNanoSecondsField = 0;
            UA_UInt64 monotonicOffsetInNs = 0;
            const char* monotonicOffsetValueSec = strtok(fileParseBuffer, " ");
            if (monotonicOffsetValueSec != NULL)
                monotonicOffsetValueSecondsField = (UA_UInt64)(atoll(monotonicOffsetValueSec));

            const char* monotonicOffsetValueNSec = strtok(NULL, " ");
            if (monotonicOffsetValueNSec != NULL)
                monotonicOffsetValueNanoSecondsField = (UA_UInt64)(atoll(monotonicOffsetValueNSec));

            monotonicOffsetInNs = (monotonicOffsetValueSecondsField * (UA_UInt64)(SECONDS)) + monotonicOffsetValueNanoSecondsField;
            threadArgPubSub1->monotonicOffset = monotonicOffsetInNs;
        }
        else
            threadArgPubSub1->monotonicOffset = 0;
    }
    else
        threadArgPubSub1->monotonicOffset = 0;

    threadArgPubSub1->packetLossCount  = 0;

    char threadNamePubApp[22]     = "PubApp";
    pubAppThreadID                = threadCreation((UA_Int16)pubAppPriority, (size_t)pubAppCore, pubApp, threadNamePubApp, NULL);
#ifdef TWO_WAY_COMMUNICATION
    char threadNameSubApp[22]     = "SubApp";
    subAppThreadID                = threadCreation((UA_Int16)subAppPriority, (size_t)subAppCore, subApp, threadNameSubApp, NULL);
#endif
    retval |= UA_Server_run(server, &runningServer);

#ifdef TWO_WAY_COMMUNICATION
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nTotal Packet Loss Count of publisher application :%"PRIu64"\n", \
                threadArgPubSub1->packetLossCount);
    UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier);
#endif
    returnValue = pthread_join(pubAppThreadID, NULL);
    if (returnValue != 0)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for pubApp thread:%d\n", returnValue);

#ifdef TWO_WAY_COMMUNICATION
    returnValue = pthread_join(subAppThreadID, NULL);
    if (returnValue != 0)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for subApp thread:%d\n", returnValue);
#endif

    if (enableCsvLog) {
        /* Write the published data in the publisher_T1.csv file */
        size_t pubLoopVariable = 0;
        for (pubLoopVariable = 0; pubLoopVariable < measurementsPublisher;
            pubLoopVariable++) {
                fprintf(fpPublisher, "%"PRId64",%ld.%09ld\n",
                        publishCounterValue[pubLoopVariable],
                        publishTimestamp[pubLoopVariable].tv_sec,
                        publishTimestamp[pubLoopVariable].tv_nsec);
        }
#ifdef TWO_WAY_COMMUNICATION
        /* Write the subscribed data in the subscriber_T8.csv file */
        size_t subLoopVariable = 0;
        for (subLoopVariable = 0; subLoopVariable < measurementsSubscriber;
            subLoopVariable++) {
                fprintf(fpSubscriber, "%"PRId64",%ld.%09ld\n",
                        subscribeCounterValue[subLoopVariable],
                        subscribeTimestamp[subLoopVariable].tv_sec,
                        subscribeTimestamp[subLoopVariable].tv_nsec);
        }
#endif
    }
    removeServerNodes(server);
    UA_Server_delete(server);
    if (operBaseTimeFileName != NULL)
        fclose(operBaseTimefile);

    if (monotonicOffsetFileName != NULL)
        fclose(monotonicOffsetFile);

    UA_free(runningPub);
    UA_free(pubCounterData);
    for (UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
        UA_free(repeatedCounterData[iterator]);

    /* Free external data source */
    UA_free(pubDataValueRT);
    UA_free(runningPubDataValueRT);
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
        UA_free(repeatedDataValueRT[iterator]);

    if (enableCsvLog)
        fclose(fpPublisher);

#ifdef TWO_WAY_COMMUNICATION
    UA_free(runningSub);
    UA_free(subCounterData);
    for (UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
        UA_free(subRepeatedCounterData[iterator]);
    /* Free external data source */
    UA_free(subDataValueRT);
    UA_free(runningSubDataValueRT);
    for (UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
        UA_free(subRepeatedDataValueRT[iterator]);
    UA_free(threadArgPubSub1);
    if (enableCsvLog)
        fclose(fpSubscriber);
#endif
    return (int)retval;
}
