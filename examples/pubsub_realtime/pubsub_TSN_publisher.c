/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tsn-publisher:
 *
 * Realtime Publish Example
 * ------------------------
 *
 * This tutorial shows publishing and subscribing information in Realtime. This
 * example has both Publisher and Subscriber(used as threads, running in same
 * core), the Publisher thread publishes counterdata (an incremental data), that
 * is subscribed by the Subscriber thread of pubsub_TSN_loopback.c example. The
 * Publisher thread of pusbub_TSN_loopback.c publishes the received counterdata,
 * which is subscribed by the Subscriber thread of this example. Thus a
 * round-trip of counterdata is achieved. In a realtime system, the round-trip
 * time of the counterdata is 4x cycletime, in this example, the round-trip time
 * is 1ms. The flow of this communication and the trace points are given in the
 * diagram below.
 *
 * Another thread called the UserApplication thread is also used in the example,
 * which serves the functionality of the Control loop. In this example,
 * UserApplication threads increments the counterData, which is published by the
 * Publisher thread and also reads the subscribed data from the Information
 * Model and writes the updated counterdata into distinct csv files during each
 * cycle. Buffered Network Message will be used for publishing and subscribing
 * in the RT path. Further, DataSetField will be accessed via direct pointer
 * access between the user interface and the Information Model.
 *
 * Another additional feature called the Blocking Socket is employed in the
 * Subscriber thread. This feature is optional and can be enabled or disabled
 * when running application by using command line argument
 * "-enableBlockingSocket". When using Blocking Socket, the Subscriber thread
 * remains in "blocking mode" until a message is received from every wake up
 * time of the thread. In other words, the timeout is overwritten and the thread
 * continuously waits for the message from every wake up time of the thread.
 * Once the message is received, the Subscriber thread updates the value in the
 * Information Model, sleeps up to wake up time and again waits for the next
 * message. This process is repeated until the application is terminated.
 *
 * To ensure realtime capabilities, Publisher uses ETF(Earliest Tx-time First)
 * to publish information at the calculated tranmission time over Ethernet.
 * Subscriber can be used with or without XDP(Xpress Data Processing) over
 * Ethernet
 *
 * Run step of the example is as mentioned below:
 *
 * ``./bin/examples/pubsub_TSN_publisher -interface <interface>``
 *
 * For more options, run ./bin/examples/pubsub_TSN_publisher -help */

/*  Trace point setup
 *
 *             +--------------+                  +----------------+
 *          T1 | OPCUA PubSub |  T8           T5 | OPCUA loopback |  T4
 *          |  |  Application |  ^            |  |  Application   |  ^
 *          |  +--------------+  |            |  +----------------+  |
 *   User   |  |              |  |            |  |                |  |
 *   Space  |  |              |  |            |  |                |  |
 *          |  |              |  |            |  |                |  |
 *  -----------|--------------|------------------|----------------|--------
 *          |  |    Node 1    |  |            |  |     Node 2     |  |
 *   Kernel |  |              |  |            |  |                |  |
 *   Space  |  |              |  |            |  |                |  |
 *          |  |              |  |            |  |                |  |
 *          v  +--------------+  |            v  +----------------+  |
 *          T2 |  TX tcpdump  |  T7<----------T6 |   RX tcpdump   |  T3
 *          |  +--------------+                  +----------------+  ^
 *          |                                                        |
 *          ----------------------------------------------------------
 */

#define _GNU_SOURCE

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

#include <open62541/plugin/securitypolicy_default.h>

#include "ua_pubsub.h"

#include <linux/if_link.h>
#include <linux/if_xdp.h>

UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

/* to find load of each thread
 * ps -L -o pid,pri,%cpu -C pubsub_TSN_publisher */

/* Configurable Parameters */
/* These defines enables the publisher and subscriber of the OPCUA stack */
/* To run only publisher, enable PUBLISHER define alone (comment SUBSCRIBER) */
#define             PUBLISHER
/* To run only subscriber, enable SUBSCRIBER define alone (comment PUBLISHER) */
#define             SUBSCRIBER
/* Cycle time in milliseconds */
#define             DEFAULT_CYCLE_TIME                    0.25
/* Qbv offset */
#define             DEFAULT_QBV_OFFSET                    125
#define             DEFAULT_SOCKET_PRIORITY               3
#if defined(PUBLISHER)
#define             PUBLISHER_ID                          2234
#define             WRITER_GROUP_ID                       101
#define             DATA_SET_WRITER_ID                    62541
#define             DEFAULT_PUBLISHING_MAC_ADDRESS        "opc.eth://01-00-5E-7F-00-01:8.3"
#endif
#define             PUBLISHER_ID_SUB                      2235
#define             WRITER_GROUP_ID_SUB                   100
#define             DATA_SET_WRITER_ID_SUB                62541
#define             DEFAULT_SUBSCRIBING_MAC_ADDRESS       "opc.eth://01-00-5E-00-00-01:8.3"
#define             REPEATED_NODECOUNTS                   2    // Default to publish 64 bytes
#define             PORT_NUMBER                           62541
#define             DEFAULT_XDP_QUEUE                     2
#define             PUBSUB_CONFIG_RT_INFORMATION_MODEL

/* Non-Configurable Parameters */
/* Milli sec and sec conversion to nano sec */
#define             MILLI_SECONDS                         1000 * 1000
#define             SECONDS                               1000 * 1000 * 1000
#define             SECONDS_SLEEP                         5
/* Publisher will sleep for 60% of cycle time and then prepares the */
/* transmission packet within 40% */
static UA_Double  pubWakeupPercentage     = 0.6;
#if defined(SUBSCRIBER)
/* Subscriber will wakeup only during start of cycle and check whether */
/* the packets are received */
static UA_Double  subWakeupPercentage     = 0;
#endif
/* User application Pub/Sub will wakeup at the 30% of cycle time and handles the */
/* user data such as read and write in Information model */
static UA_Double  userAppWakeupPercentage = 0.3;
/* Priority of Publisher, Subscriber, User application and server are kept */
/* after some prototyping and analyzing it */
#define             DEFAULT_PUB_SCHED_PRIORITY              78
#define             DEFAULT_SUB_SCHED_PRIORITY              81
#define             DEFAULT_USERAPPLICATION_SCHED_PRIORITY  75
#define             MAX_MEASUREMENTS                        1000000
#define             MAX_MEASUREMENTS_FILEWRITE              100000000
#define             DEFAULT_PUB_CORE                        2
#define             DEFAULT_SUB_CORE                        2
#define             DEFAULT_USER_APP_CORE                   3
#define             SECONDS_INCREMENT                       1
#ifndef CLOCK_TAI
#define             CLOCK_TAI                               11
#endif
#define             CLOCKID                                 CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE                   "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             LATENCY_CSV_FILE_NAME                   "latencyT1toT8.csv"

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
#define             UA_AES128CTR_SIGNING_KEY_LENGTH          32
#define             UA_AES128CTR_KEY_LENGTH                  16
#define             UA_AES128CTR_KEYNONCE_LENGTH             4

#if defined(PUBLISHER)
UA_Byte signingKeyPub[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKeyPub[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNoncePub[UA_AES128CTR_KEYNONCE_LENGTH] = {0};
#endif

#if defined(SUBSCRIBER)
UA_Byte signingKeySub[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKeySub[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonceSub[UA_AES128CTR_KEYNONCE_LENGTH] = {0};
#endif
#endif

/* If the Hardcoded publisher/subscriber MAC addresses need to be changed,
 * change PUBLISHING_MAC_ADDRESS and SUBSCRIBING_MAC_ADDRESS
 */

/* Set server running as true */
UA_Boolean        runningServer        = true;
char*             pubMacAddress        = DEFAULT_PUBLISHING_MAC_ADDRESS;
char*             subMacAddress        = DEFAULT_SUBSCRIBING_MAC_ADDRESS;
static UA_Double  cycleTimeInMsec      = DEFAULT_CYCLE_TIME;
static UA_Int32   socketPriority       = DEFAULT_SOCKET_PRIORITY;
static UA_Int32   pubPriority          = DEFAULT_PUB_SCHED_PRIORITY;
static UA_Int32   subPriority          = DEFAULT_SUB_SCHED_PRIORITY;
static UA_Int32   userAppPriority      = DEFAULT_USERAPPLICATION_SCHED_PRIORITY;
static UA_Int32   pubCore              = DEFAULT_PUB_CORE;
static UA_Int32   subCore              = DEFAULT_SUB_CORE;
static UA_Int32   userAppCore          = DEFAULT_USER_APP_CORE;
static UA_Int32   qbvOffset            = DEFAULT_QBV_OFFSET;
static UA_UInt32  xdpQueue             = DEFAULT_XDP_QUEUE;
static UA_UInt32  xdpFlag              = XDP_FLAGS_SKB_MODE;
static UA_UInt32  xdpBindFlag          = XDP_COPY;
static UA_Boolean disableSoTxtime      = true;
static UA_Boolean enableCsvLog         = false;
static UA_Boolean enableLatencyCsvLog  = false;
static UA_Boolean consolePrint         = false;
static UA_Boolean enableBlockingSocket = false;
static UA_Boolean signalTerm           = false;
static UA_Boolean enableXdpSubscribe   = false;

/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_NodeId           connectionIdent;
UA_NodeId           publishedDataSetIdent;
UA_NodeId           writerGroupIdent;
UA_NodeId           pubNodeID;
UA_NodeId           subNodeID;
UA_NodeId           pubRepeatedCountNodeID;
UA_NodeId           subRepeatedCountNodeID;
UA_NodeId           runningPubStatusNodeID;
UA_NodeId           runningSubStatusNodeID;
/* Variables for counter data handling in address space */
UA_UInt64           *pubCounterData = NULL;
UA_DataValue        *pubDataValueRT = NULL;
UA_Boolean          *runningPub = NULL;
UA_DataValue        *runningPubDataValueRT = NULL;
UA_UInt64           *repeatedCounterData[REPEATED_NODECOUNTS] = {NULL};
UA_DataValue        *repeatedDataValueRT[REPEATED_NODECOUNTS] = {NULL};

UA_UInt64           *subCounterData = NULL;
UA_DataValue        *subDataValueRT = NULL;
UA_Boolean          *runningSub = NULL;
UA_DataValue        *runningSubDataValueRT =  NULL;
UA_UInt64           *subRepeatedCounterData[REPEATED_NODECOUNTS] = {NULL};
UA_DataValue        *subRepeatedDataValueRT[REPEATED_NODECOUNTS] = {NULL};

/**
 * CSV file handling
 * ~~~~~~~~~~~~~~~~~
 *
 * CSV files are written for Publisher and Subscriber thread. csv files include
 * the counterdata that is being either Published or Subscribed along with the
 * timestamp. These csv files can be used to compute latency for following
 * combinations of Tracepoints, T1-T4 and T1-T8.
 *
 * T1-T8 - Gives the Round-trip time of a counterdata, as the value published by
 * the Publisher thread in pubsub_TSN_publisher.c example is subscribed by the
 * Subscriber thread in pubsub_TSN_loopback.c example and is published back to
 * the pubsub_TSN_publisher.c example */

#if defined(PUBLISHER)
/* File to store the data and timestamps for different traffic */
FILE               *fpPublisher;
char               *filePublishedData      = "publisher_T1.csv";
/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;
/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];
/* Thread for publisher */
pthread_t           pubthreadID;
struct timespec     dataModificationTime;
#endif

#if defined(SUBSCRIBER)
/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileSubscribedData     = "subscriber_T8.csv";
/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;
/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];
/* Thread for subscriber */
pthread_t           subthreadID;
/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
struct timespec     dataReceiveTime;
#endif

/* Thread for user application*/
pthread_t           userApplicationThreadID;

/* Base time handling for the threads */
struct timespec     threadBaseTime;
UA_Boolean          baseTimeCalculated = false;

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

/**
 * Function calls for different threads */
/* Publisher thread routine for ETF */
void *publisherETF(void *arg);
/* Subscriber thread routine */
void *subscriber(void *arg);
/* User application thread routine */
void *userApplicationPubSub(void *arg);
/* For adding nodes in the server information model */
static void addServerNodes(UA_Server *server);
/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);
/* To create multi-threads */
static pthread_t threadCreation(UA_Int16 threadPriority, size_t coreAffinity, void *(*thread)(void *),
                                char *applicationName, void *serverConfig);

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    signalTerm = true;
}

/**
 * **Nanosecond field handling**
 *
 * Nanosecond field in timespec is checked for overflowing and one second
 * is added to seconds field and nanosecond field is set to zero
*/
static void nanoSecondFieldConversion(struct timespec *timeSpecValue) {
    /* Check if ns field is greater than '1 ns less than 1sec' */
    while(timeSpecValue->tv_nsec > (SECONDS -1)) {
        /* Move to next second and remove it from ns field */
        timeSpecValue->tv_sec  += SECONDS_INCREMENT;
        timeSpecValue->tv_nsec -= SECONDS;
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
    /* Initialize arguments required for the thread to run */
    threadArg *threadArguments = (threadArg *) UA_malloc(sizeof(threadArg));

    /* Pass the value required for the threads */
    threadArguments->server      = server;
    threadArguments->data        = data;
    threadArguments->callback    = callback;
    threadArguments->interval_ms = interval_ms;
    threadArguments->callbackId  = callbackId;

    /* Check the writer group identifier and create the thread accordingly */
    if(UA_NodeId_equal(&identifier, &writerGroupIdent)) {
#if defined(PUBLISHER)
        /* Create the publisher thread with the required priority and core affinity */
        char threadNamePub[10] = "Publisher";
        *callbackId = threadCreation((UA_Int16)pubPriority, (size_t)pubCore,
                                     publisherETF, threadNamePub, threadArguments);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Publisher thread callback Id: %lu\n", (unsigned long)*callbackId);
#endif
    }
    else {
#if defined(SUBSCRIBER)
        /* Create the subscriber thread with the required priority and core affinity */
        char threadNameSub[11] = "Subscriber";
        *callbackId = threadCreation((UA_Int16)subPriority, (size_t)subCore,
                                     subscriber, threadNameSub, threadArguments);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Subscriber thread callback Id: %lu\n", (unsigned long)*callbackId);
#endif
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
removePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier,
                                UA_UInt64 callbackId) {
    if(callbackId && (pthread_join((pthread_t)callbackId, NULL) != 0))
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Pthread Join Failed thread: %lu\n", (unsigned long)callbackId);
}

/**
 * External data source handling
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * If the external data source is written over the information model, the
 * externalDataWriteCallback will be triggered. The user has to take care and
 * assure that the write leads not to synchronization issues and race
 * conditions. */
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

/**
 * Subscriber
 * ~~~~~~~~~~
 *
 * Create connection, readergroup, datasetreader, subscribedvariables for the
 * Subscriber thread. */

#if defined(SUBSCRIBER)
static void
addPubSubConnectionSubscriber(UA_Server *server,
                              UA_NetworkAddressUrlDataType *networkAddressUrlSubscriber){
    UA_StatusCode    retval                                 = UA_STATUSCODE_GOOD;
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Subscriber Connection");
    connectionConfig.enabled                                = true;

    UA_KeyValuePair connectionOptions[4];
    connectionOptions[0].key                  = UA_QUALIFIEDNAME(0, "enableXdpSocket");
    UA_Boolean enableXdp                      = enableXdpSubscribe;
    UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[1].key                  = UA_QUALIFIEDNAME(0, "xdpflag");
    UA_UInt32 flags                           = xdpFlag;
    UA_Variant_setScalar(&connectionOptions[1].value, &flags, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key                  = UA_QUALIFIEDNAME(0, "hwreceivequeue");
    UA_UInt32 rxqueue                         = xdpQueue;
    UA_Variant_setScalar(&connectionOptions[2].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[3].key                  = UA_QUALIFIEDNAME(0, "xdpbindflag");
    UA_UInt32 bindflags                       = xdpBindFlag;
    UA_Variant_setScalar(&connectionOptions[3].value, &bindflags, &UA_TYPES[UA_TYPES_UINT16]);
    connectionConfig.connectionProperties     = connectionOptions;
    connectionConfig.connectionPropertiesSize = 4;


    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = *networkAddressUrlSubscriber;
    connectionConfig.transportProfileUri = UA_STRING(ETH_TRANSPORT_PROFILE);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if(retval == UA_STATUSCODE_GOOD)
         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "The PubSub Connection was created successfully!");
}

/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    if(server == NULL)
        return;

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name    = UA_STRING("ReaderGroup1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;

    readerGroupConfig.subscribingInterval = cycleTimeInMsec;
    /* Timeout is modified when blocking socket is enabled, and the default
     * timeout is used when blocking socket is disabled */
    if(enableBlockingSocket == false)
        readerGroupConfig.timeout = 50;  // As we run in 250us cycle time, modify default timeout (1ms) to 50us
    else {
        readerGroupConfig.enableBlockingSocket = true;
        readerGroupConfig.timeout = 0;  //Blocking  socket
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Encryption settings */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    readerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    readerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[1];
#endif

    readerGroupConfig.pubsubManagerCallback.addCustomCallback = addPubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.changeCustomCallback = changePubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.removeCustomCallback = removePubSubApplicationCallback;

    UA_Server_addReaderGroup(server, connectionIdentSubscriber, &readerGroupConfig,
                             &readerGroupIdentifier);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKeySub};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKeySub};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonceSub};
    // TODO security token not necessary for readergroup (extracted from security-header)
    UA_Server_setReaderGroupEncryptionKeys(server, readerGroupIdentifier, 1, sk, ek, kn);
#endif
}


/* Set SubscribedDataSet type to TargetVariables data type
 * Add SubscriberCounter variable to the DataSetReader */
static void
addSubscribedVariables(UA_Server *server) {
    UA_Int32 iterator = 0;
    UA_Int32 iteratorRepeatedCount = 0;
    if(server == NULL) {
        return;
    }

    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable*)
        UA_calloc((REPEATED_NODECOUNTS + 2), sizeof(UA_FieldTargetVariable));
    if(!targetVars) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "FieldTargetVariable - Bad out of memory");
        return;
    }

    runningSub = UA_Boolean_new();
    if(!runningSub) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "runningsub - Bad out of memory");
        return;
    }

    *runningSub = true;
    runningSubDataValueRT = UA_DataValue_new();
    if(!runningSubDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "runningsubDataValue - Bad out of memory");
        return;
    }

    UA_Variant_setScalar(&runningSubDataValueRT->value, runningSub, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningSubDataValueRT->hasValue = true;
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
    for(iterator = 1, iteratorRepeatedCount = 0; iterator <= REPEATED_NODECOUNTS; iterator++, iteratorRepeatedCount++)
    {
        subRepeatedCounterData[iteratorRepeatedCount] = UA_UInt64_new();
        if(!subRepeatedCounterData[iteratorRepeatedCount]) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "SubscribeRepeatedCounterData - Bad out of memory");
            return;
        }

       *subRepeatedCounterData[iteratorRepeatedCount] = 0;
       subRepeatedDataValueRT[iteratorRepeatedCount] = UA_DataValue_new();
       if(!subRepeatedDataValueRT[iteratorRepeatedCount]) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "SubscribeRepeatedCounterDataValue - Bad out of memory");
            return;
        }

        UA_Variant_setScalar(&subRepeatedDataValueRT[iteratorRepeatedCount]->value,
                             subRepeatedCounterData[iteratorRepeatedCount], &UA_TYPES[UA_TYPES_UINT64]);
        subRepeatedDataValueRT[iteratorRepeatedCount]->hasValue = true;

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
        return;
    }

    *subCounterData = 0;
    subDataValueRT = UA_DataValue_new();
    if(!subDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SubscribeDataValue - Bad out of memory");
        return;
    }

    UA_Variant_setScalar(&subDataValueRT->value, subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    subDataValueRT->hasValue = true;

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

    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name                 = UA_STRING("DataSet Reader 1");
    UA_UInt16 publisherIdentifier     = PUBLISHER_ID_SUB;
    readerConfig.publisherId.type     = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data     = &publisherIdentifier;
    readerConfig.writerGroupId        = WRITER_GROUP_ID_SUB;
    readerConfig.dataSetWriterId      = DATA_SET_WRITER_ID_SUB;

    readerConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    readerConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE];
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessage = UA_UadpDataSetReaderMessageDataType_new();
    dataSetReaderMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    readerConfig.messageSettings.content.decoded.data = dataSetReaderMessage;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData = &readerConfig.dataSetMetaData;
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name                   = UA_STRING("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             = REPEATED_NODECOUNTS + 2;
    pMetaData->fields                 = (UA_FieldMetaData*)
        UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* Boolean DataType */
    UA_FieldMetaData_init (&pMetaData->fields[iterator]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[iterator].dataType);
    pMetaData->fields[iterator].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[iterator].valueRank   = -1; /* scalar */
    iterator++;
    for(iterator = 1; iterator <= REPEATED_NODECOUNTS; iterator++) {
        UA_FieldMetaData_init(&pMetaData->fields[iterator]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId,
                       &pMetaData->fields[iterator].dataType);
        pMetaData->fields[iterator].builtInType = UA_NS0ID_UINT64;
        pMetaData->fields[iterator].valueRank   = -1; /* scalar */
    }

    /* Unsigned Integer DataType */
    UA_FieldMetaData_init(&pMetaData->fields[iterator]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId,
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

#if defined(PUBLISHER)

/**
 * Publisher
 * ~~~~~~~~~
 *
 * Create connection, writergroup, datasetwriter and publisheddataset for
 * Publisher thread. */

static void
addPubSubConnection(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlPub){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Publisher Connection");
    connectionConfig.enabled                                = true;
    UA_NetworkAddressUrlDataType networkAddressUrl          = *networkAddressUrlPub;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric                    = PUBLISHER_ID;
    /* Connection options are given as Key/Value Pairs - Sockprio and Txtime */
    UA_KeyValuePair connectionOptions[2];
    connectionOptions[0].key                  = UA_QUALIFIEDNAME(0, "sockpriority");
    UA_Variant_setScalar(&connectionOptions[0].value, &socketPriority, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key                  = UA_QUALIFIEDNAME(0, "enablesotxtime");
    UA_Variant_setScalar(&connectionOptions[1].value, &disableSoTxtime, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionConfig.connectionProperties     = connectionOptions;
    connectionConfig.connectionPropertiesSize = 2;

    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/* PublishedDataSet handling */
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
    UA_NodeId dataSetFieldIdentRepeated;
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

    *runningPub = true;
    runningPubDataValueRT = UA_DataValue_new();
    if(!runningPubDataValueRT) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "runningPubDataValue - Bad out of memory");
        return;
    }

    UA_Variant_setScalar(&runningPubDataValueRT->value, runningPub, &UA_TYPES[UA_TYPES_BOOLEAN]);
    runningPubDataValueRT->hasValue = true;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend runningPubvalueBackend;
    runningPubvalueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    runningPubvalueBackend.backend.external.value = &runningPubDataValueRT;
    runningPubvalueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    runningPubvalueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)20000), runningPubvalueBackend);

    /* setup RT DataSetField config */
    dsfConfigPubStatus.field.variable.rtValueSource.rtInformationModelNode = true;
    dsfConfigPubStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, (UA_UInt32)20000);

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfigPubStatus, &dataSetFieldIdentRunning);

    for(UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
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
       repeatedDataValueRT[iterator]->hasValue = true;

       /* Set the value backend of the above create node to 'external value source' */
       UA_ValueBackend valueBackend;
       valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
       valueBackend.backend.external.value = &repeatedDataValueRT[iterator];
       valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
       valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
       UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000), valueBackend);

       /* setup RT DataSetField config */
       dataSetFieldConfig.field.variable.rtValueSource.rtInformationModelNode = true;
       dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(1, (UA_UInt32)iterator+10000);
       UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdentRepeated);
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
    pubDataValueRT->hasValue = true;

    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &pubDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    UA_Server_setVariableNode_valueBackend(server, pubNodeID, valueBackend);

    /* setup RT DataSetField config */
    dsfConfig.field.variable.rtValueSource.rtInformationModelNode = true;
    dsfConfig.field.variable.publishParameters.publishedVariable = pubNodeID;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);

}

/* WriterGroup handling */
static void
addWriterGroup(UA_Server *server) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name               = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = cycleTimeInMsec;
    writerGroupConfig.enabled            = false;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
    writerGroupConfig.rtLevel            = UA_PUBSUB_RT_FIXED_SIZE;

    writerGroupConfig.pubsubManagerCallback.addCustomCallback = addPubSubApplicationCallback;
    writerGroupConfig.pubsubManagerCallback.changeCustomCallback = changePubSubApplicationCallback;
    writerGroupConfig.pubsubManagerCallback.removeCustomCallback = removePubSubApplicationCallback;

    writerGroupConfig.messageSettings.encoding             = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_ServerConfig *config = UA_Server_getConfig(server);
    writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
#endif

    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_UadpWriterGroupMessageDataType *writerGroupMessage  = UA_UadpWriterGroupMessageDataType_new();
    /* Change message settings of writerGroup to send PublisherId,
     * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
     * of NetworkMessage */
    writerGroupMessage->networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Add the encryption key informaton */
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKeyPub};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKeyPub};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNoncePub};
    UA_Server_setWriterGroupEncryptionKeys(server, writerGroupIdent, 1, sk, ek, kn);
#endif
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
 * Published data handling
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The published data is updated in the array using this function. */

#if defined(PUBLISHER)
static void
updateMeasurementsPublisher(struct timespec start_time,
                            UA_UInt64 counterValue) {
    if(measurementsPublisher >= MAX_MEASUREMENTS) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "Publisher: Maximum log measurements reached - Closing the application");
        signalTerm = true;
        return;
    }

    if(consolePrint)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Pub:%lu,%ld.%09ld\n",
                    (long unsigned)counterValue, start_time.tv_sec, start_time.tv_nsec);

    if(signalTerm != true){
        publishTimestamp[measurementsPublisher]        = start_time;
        publishCounterValue[measurementsPublisher]     = counterValue;
        measurementsPublisher++;
    }
}
#endif
#if defined(SUBSCRIBER)

/**
 * Subscribed data handling
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The subscribed data is updated in the array using this function Subscribed
 * data handling. */

static void
updateMeasurementsSubscriber(struct timespec receive_time,
                             UA_UInt64 counterValue) {
    if(measurementsSubscriber >= MAX_MEASUREMENTS) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "Subscriber: Maximum log measurements reached - Closing the application");
        signalTerm = true;
        return;
    }

    if(consolePrint)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Sub:%lu,%ld.%09ld\n",
                    (long unsigned)counterValue, receive_time.tv_sec, receive_time.tv_nsec);

    if(signalTerm != true)
    {
        subscribeTimestamp[measurementsSubscriber]     = receive_time;
        subscribeCounterValue[measurementsSubscriber]  = counterValue;
        measurementsSubscriber++;
    }
}
#endif

/**
 * Publisher thread routine
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This is the Publisher thread that sleeps for 60% of the cycletime (250us) and
 * prepares the tranmission packet within 40% of cycletime. The priority of this
 * thread is lower than the priority of the Subscriber thread, so the subscriber
 * thread executes first during every cycle. The data published by this thread
 * in one cycle is subscribed by the subscriber thread of pubsub_TSN_loopback in
 * the next cycle(two cycle timing model).
 *
 * The publisherETF function is the routine used by the publisher thread. */
void *publisherETF(void *arg) {
    struct timespec   nextnanosleeptime;
    UA_ServerCallback pubCallback;
    UA_Server*        server;
    UA_WriterGroup*   currentWriterGroup; // TODO: Remove WriterGroup Usage
    UA_UInt64         interval_ns;
    UA_UInt64         transmission_time;

    threadArg *threadArgumentsPublisher = (threadArg *)arg;
    server                              = threadArgumentsPublisher->server;
    pubCallback                         = threadArgumentsPublisher->callback;
    currentWriterGroup                  = (UA_WriterGroup *)threadArgumentsPublisher->data;
    interval_ns                         = (UA_UInt64)(threadArgumentsPublisher->interval_ms * MILLI_SECONDS);
    /* Verify whether baseTime has already been calculated */
    if(!baseTimeCalculated) {
        /* Get current time and compute the next nanosleeptime */
        clock_gettime(CLOCKID, &threadBaseTime);
        /* Variable to nano Sleep until SECONDS_SLEEP second boundary */
        threadBaseTime.tv_sec  += SECONDS_SLEEP;
        threadBaseTime.tv_nsec  = 0;
        baseTimeCalculated = true;
    }

    nextnanosleeptime.tv_sec  = threadBaseTime.tv_sec;
    /* Modify the nanosecond field to wake up at the pubWakeUp percentage */
    nextnanosleeptime.tv_nsec = threadBaseTime.tv_nsec +
        (__syscall_slong_t)(cycleTimeInMsec * MILLI_SECONDS * pubWakeupPercentage);
    nanoSecondFieldConversion(&nextnanosleeptime);

    /* Define Ethernet ETF transport settings */
    UA_EthernetWriterGroupTransportDataType ethernettransportSettings;
    memset(&ethernettransportSettings, 0, sizeof(UA_EthernetWriterGroupTransportDataType));
    ethernettransportSettings.transmission_time = 0;

    /* Encapsulate ETF config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    /* TODO: transportSettings encoding and type to be defined */
    transportSettings.content.decoded.data = &ethernettransportSettings;
    currentWriterGroup->config.transportSettings = transportSettings;
    UA_UInt64 roundOffCycleTime = (UA_UInt64)((cycleTimeInMsec * MILLI_SECONDS) -
                                              (cycleTimeInMsec * MILLI_SECONDS * pubWakeupPercentage));

    while(*runningPub) {
        /* The Publisher threads wakes up at the configured publisher wake up
         * percentage (60%) of each cycle */
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        /* Whenever Ctrl + C pressed, publish running boolean as false to stop
         * the subscriber before terminating the application */
        if(signalTerm == true)
            *runningPub = false;

        /* Calculation of transmission time using the configured qbv offset by
         * the user - Will be handled by publishingOffset in the future */
        transmission_time = ((UA_UInt64)nextnanosleeptime.tv_sec * SECONDS + (UA_UInt64)nextnanosleeptime.tv_nsec) +
            roundOffCycleTime + (UA_UInt64)(qbvOffset * 1000);
        ethernettransportSettings.transmission_time = transmission_time;
        /* Publish the data using the pubcallback - UA_WriterGroup_publishCallback() */
        pubCallback(server, currentWriterGroup);
        /* Calculation of the next wake up time by adding the interval with the previous wake up time */
        nextnanosleeptime.tv_nsec += (__syscall_slong_t)interval_ns;
        nanoSecondFieldConversion(&nextnanosleeptime);
    }

#if defined(PUBLISHER) && !defined(SUBSCRIBER)
    runningServer = UA_FALSE;
#endif
    UA_free(threadArgumentsPublisher);
    return NULL;
}
#endif

#if defined(SUBSCRIBER)

/**
 * Subscriber thread routine
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This Subscriber thread will wakeup during the start of cycle at 250us
 * interval and check if the packets are received. Subscriber thread has the
 * highest priority. This Subscriber thread subscribes to the data published by
 * the Publisher thread of pubsub_TSN_loopback in the previous cycle. The
 * subscriber function is the routine used by the subscriber thread. */

void *subscriber(void *arg) {
    UA_Server*        server;
    void*             currentReaderGroup;
    UA_ServerCallback subCallback;
    struct timespec   nextnanosleeptimeSub;
    UA_UInt64         subInterval_ns;

    threadArg *threadArgumentsSubscriber = (threadArg *)arg;
    server             = threadArgumentsSubscriber->server;
    subCallback        = threadArgumentsSubscriber->callback;
    currentReaderGroup = threadArgumentsSubscriber->data;
    subInterval_ns     = (UA_UInt64)(threadArgumentsSubscriber->interval_ms * MILLI_SECONDS);
    /* Verify whether baseTime has already been calculated */
    if(!baseTimeCalculated) {
        /* Get current time and compute the next nanosleeptime */
        clock_gettime(CLOCKID, &threadBaseTime);
        /* Variable to nano Sleep until SECONDS_SLEEP second boundary */
        threadBaseTime.tv_sec  += SECONDS_SLEEP;
        threadBaseTime.tv_nsec  = 0;
        baseTimeCalculated = true;
    }

    nextnanosleeptimeSub.tv_sec  = threadBaseTime.tv_sec;
    /* Modify the nanosecond field to wake up at the subWakeUp percentage */
    nextnanosleeptimeSub.tv_nsec = threadBaseTime.tv_nsec +
        (__syscall_slong_t)(cycleTimeInMsec * MILLI_SECONDS * subWakeupPercentage);
    nanoSecondFieldConversion(&nextnanosleeptimeSub);
    while(*runningSub) {
        /* The Subscriber threads wakes up at the configured subscriber wake up percentage (0%) of each cycle */
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSub, NULL);
        /* Receive and process the incoming data using the subcallback - UA_ReaderGroup_subscribeCallback() */
        subCallback(server, currentReaderGroup);
        /* Calculation of the next wake up time by adding the interval with the previous wake up time */
        nextnanosleeptimeSub.tv_nsec += (__syscall_slong_t)subInterval_ns;
        nanoSecondFieldConversion(&nextnanosleeptimeSub);

        /* Whenever Ctrl + C pressed, modify the runningSub boolean to false to end this while loop */
        if(signalTerm == true)
            *runningSub = false;
    }

    UA_free(threadArgumentsSubscriber);
    /* While ctrl+c is provided in publisher side then loopback application
     * need to be closed by after sending *running=0 for subscriber T4 */
    if(*runningSub == false)
        signalTerm = true;

    sleep(1);
    runningServer = false;
    return NULL;
}
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)

/**
 * UserApplication thread routine
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * The userapplication thread will wakeup at 30% of cycle time and handles the
 * userdata(read and write in Information Model). This thread serves the purpose
 * of a Control loop, which is used to increment the counterdata to be published
 * by the Publisher thread and read the data from Information Model for the
 * Subscriber thread and writes the updated counterdata in distinct csv files
 * for both threads. */

void *userApplicationPubSub(void *arg) {
    UA_UInt64  repeatedCounterValue = 10;
    struct timespec nextnanosleeptimeUserApplication;
    /* Verify whether baseTime has already been calculated */
    if(!baseTimeCalculated) {
        /* Get current time and compute the next nanosleeptime */
        clock_gettime(CLOCKID, &threadBaseTime);
        /* Variable to nano Sleep until SECONDS_SLEEP second boundary */
        threadBaseTime.tv_sec  += SECONDS_SLEEP;
        threadBaseTime.tv_nsec  = 0;
        baseTimeCalculated = true;
    }

    nextnanosleeptimeUserApplication.tv_sec  = threadBaseTime.tv_sec;
    /* Modify the nanosecond field to wake up at the userAppWakeUp percentage */
    nextnanosleeptimeUserApplication.tv_nsec = threadBaseTime.tv_nsec +
        (__syscall_slong_t)(cycleTimeInMsec * MILLI_SECONDS * userAppWakeupPercentage);
    nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    *pubCounterData      = 0;
    for(UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++) {
        *repeatedCounterData[iterator] = repeatedCounterValue;
    }

#if defined(PUBLISHER) && defined(SUBSCRIBER)
    while(*runningPub || *runningSub) {
#else
    while(*runningPub) {
#endif
        /* The User application threads wakes up at the configured userApp wake
         * up percentage (30%) of each cycle */
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeUserApplication, NULL);
#if defined(PUBLISHER)
        /* Increment the counter data and repeated counter data for the next cycle publish */
        *pubCounterData      = *pubCounterData + 1;
        for(UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
            *repeatedCounterData[iterator] = *repeatedCounterData[iterator] + 1;

        /* Get the time - T1, time where the counter data and repeated counter
         * data gets incremented. As this application uses FPM, we do not
         * require explicit call of UA_Server_write() to write the counter
         * values to the Information model. Hence, we take publish T1 time
         * here. */
        clock_gettime(CLOCKID, &dataModificationTime);
#endif

#if defined(SUBSCRIBER)
        /* Get the time - T8, time where subscribed varibles are read from the
         * Information model. At this point, the packet will be already
         * subscribed and written into the Information model. As this
         * application uses FPM, we do not require explicit call of
         * UA_Server_read() to read the subscribed value from the Information
         * model. Hence, we take subscribed T8 time here. */
        clock_gettime(CLOCKID, &dataReceiveTime);
#endif

        /* Update the T1, T8 time with the counter data in the user defined
         * publisher and subscriber arrays. */
        if(enableCsvLog || enableLatencyCsvLog || consolePrint) {
#if defined(PUBLISHER)
            updateMeasurementsPublisher(dataModificationTime, *pubCounterData);
#endif

#if defined(SUBSCRIBER)
            if(*subCounterData > 0)
                updateMeasurementsSubscriber(dataReceiveTime, *subCounterData);
#endif
        }

        /* Calculation of the next wake up time by adding the interval with the
         * previous wake up time. */
        nextnanosleeptimeUserApplication.tv_nsec +=
            (__syscall_slong_t)(cycleTimeInMsec * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    }

    return NULL;
}
#endif

/**
 * Thread creation
 * ~~~~~~~~~~~~~~~
 *
 * The threadcreation functionality creates thread with given threadpriority,
 * coreaffinity. The function returns the threadID of the newly created thread. */

static pthread_t
threadCreation(UA_Int16 threadPriority, size_t coreAffinity,
               void *(*thread)(void *), char *applicationName, void *serverConfig){
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
    if(returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"pthread_setschedparam: failed\n");
        exit(1);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,\
                "\npthread_setschedparam:%s Thread priority is %d \n", \
                applicationName, schedParam.sched_priority);
    CPU_ZERO(&cpuset);
    CPU_SET(coreAffinity, &cpuset);
    errorSetAffinity = pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset);
    if(errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        exit(1);
    }

    returnValue = pthread_create(&threadID, NULL, thread, serverConfig);
    if(returnValue != 0)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       ":%s Cannot create thread\n", applicationName);

    if(CPU_ISSET(coreAffinity, &cpuset))
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "%s CPU CORE: %lu\n", applicationName, (unsigned long)coreAffinity);

   return threadID;
}

/**
 * Creation of nodes
 * ~~~~~~~~~~~~~~~~~
 *
 * The addServerNodes function is used to create the publisher and subscriber
 * nodes. */

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
    UA_Variant_setScalar(&publisherAttr.value, &publishValue, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    publisherAttr.dataType               = UA_TYPES[UA_TYPES_UINT64].typeId;
    newNodeId                            = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, publisherAttr, NULL, &pubNodeID);
    UA_VariableAttributes subscriberAttr = UA_VariableAttributes_default;
    UA_UInt64 subscribeValue             = 0;
    subscriberAttr.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&subscriberAttr.value, &subscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
    subscriberAttr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    subscriberAttr.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
    newNodeId                            = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, objectId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, subscriberAttr, NULL, &subNodeID);
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodePub = UA_VariableAttributes_default;
        UA_UInt64 repeatedPublishValue        = 0;
        repeatedNodePub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Variant_setScalar(&repeatedNodePub.value, &repeatedPublishValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodePub.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher RepeatedCounter");
        repeatedNodePub.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
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
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
    {
        UA_VariableAttributes repeatedNodeSub = UA_VariableAttributes_default;
        UA_UInt64 repeatedSubscribeValue;
        UA_Variant_setScalar(&repeatedNodeSub.value, &repeatedSubscribeValue, &UA_TYPES[UA_TYPES_UINT64]);
        repeatedNodeSub.accessLevel           = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        repeatedNodeSub.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber RepeatedCounter");
        repeatedNodeSub.dataType              = UA_TYPES[UA_TYPES_UINT64].typeId;
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

}

/**
 * Deletion of nodes
 * ~~~~~~~~~~~~~~~~~~
 *
 * The removeServerNodes function is used to delete the publisher and subscriber
 * nodes. */

static void removeServerNodes(UA_Server *server) {
    /* Delete the Publisher Counter Node*/
    UA_Server_deleteNode(server, pubNodeID, true);
    UA_NodeId_clear(&pubNodeID);
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++) {
        UA_Server_deleteNode(server, pubRepeatedCountNodeID, true);
        UA_NodeId_clear(&pubRepeatedCountNodeID);
    }
    UA_Server_deleteNode(server, runningPubStatusNodeID, true);
    UA_NodeId_clear(&runningPubStatusNodeID);

    UA_Server_deleteNode(server, subNodeID, true);
    UA_NodeId_clear(&subNodeID);
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++) {
        UA_Server_deleteNode(server, subRepeatedCountNodeID, true);
        UA_NodeId_clear(&subRepeatedCountNodeID);
    }
    UA_Server_deleteNode(server, runningSubStatusNodeID, true);
    UA_NodeId_clear(&runningSubStatusNodeID);
}

#if defined (PUBLISHER) && defined(SUBSCRIBER)

/**
 * Time Difference Calculation
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This function is used to calculate the difference between the
 * publishertimestamp and subscribertimestamp and store the result. */

static void
timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result) {
    if((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

/**
 * Latency Calculation
 * ~~~~~~~~~~~~~~~~~~~
 *
 * When the application is run with "-enableLatencyCsvLog" option, this function
 * gets executed. This function calculates latency by computing the
 * publishtimestamp and subscribetimestamp taking the counterdata as reference
 * and writes the result in a csv. */

static void computeLatencyAndGenerateCsv(char *latencyFileName) {
    /* Character array of computed latency to write into a file */
    static char latency_measurements[MAX_MEASUREMENTS_FILEWRITE];
    struct timespec resultTime;
    size_t latencyComputePubIndex, latencyComputeSubIndex;
    UA_Double finalTime;
    UA_UInt64 missed_counter         = 0;
    UA_UInt64 repeated_counter       = 0;
    UA_UInt64 latencyCharIndex       = 0;
    UA_UInt64 prevLatencyCharIndex   = 0;
    /* Create the latency file and include the headers */
    FILE *fp_latency;
    fp_latency = fopen(latencyFileName, "w");
    latencyCharIndex += (UA_UInt64)sprintf(&latency_measurements[latencyCharIndex],
                                           "%s, %s, %s\n",
                                           "LatencyRTT", "Missed Counters", "Repeated Counters");

    for(latencyComputePubIndex = 0, latencyComputeSubIndex = 0;
         latencyComputePubIndex < measurementsPublisher && latencyComputeSubIndex < measurementsSubscriber; ) {
        /* Compute RTT latency by equating counter values */
        if(publishCounterValue[latencyComputePubIndex] == subscribeCounterValue[latencyComputeSubIndex]) {
            timespec_diff(&publishTimestamp[latencyComputePubIndex], &subscribeTimestamp[latencyComputeSubIndex], &resultTime);
            finalTime = ((UA_Double)((resultTime.tv_sec * 1000000000L) + resultTime.tv_nsec))/1000;
            latencyComputePubIndex++;
            latencyComputeSubIndex++;
        }
        else if(publishCounterValue[latencyComputePubIndex] < subscribeCounterValue[latencyComputeSubIndex]) {
            timespec_diff(&publishTimestamp[latencyComputePubIndex], &subscribeTimestamp[latencyComputeSubIndex], &resultTime);
            finalTime = ((UA_Double)((resultTime.tv_sec * 1000000000L) + resultTime.tv_nsec))/1000;
            missed_counter++;
            latencyComputePubIndex++;
        }
        else {
            if(subscribeCounterValue[latencyComputeSubIndex - 1] == subscribeCounterValue[latencyComputeSubIndex])
                repeated_counter++;

            timespec_diff(&publishTimestamp[latencyComputePubIndex], &subscribeTimestamp[latencyComputeSubIndex], &resultTime);
            finalTime = ((UA_Double)((resultTime.tv_sec * 1000000000L) + resultTime.tv_nsec))/1000;
            latencyComputeSubIndex++;
        }

        if(((latencyCharIndex - prevLatencyCharIndex) + latencyCharIndex + 3) < MAX_MEASUREMENTS_FILEWRITE) {
            latencyCharIndex += (UA_UInt64)sprintf(&latency_measurements[latencyCharIndex],
                                                   "%0.3f, %lu, %lu\n",
                                                   finalTime, (unsigned long)missed_counter, (unsigned long)repeated_counter);
        }
        else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Character array has been filled. Computation stopped and leading to latency csv generation");
            break;
        }

        prevLatencyCharIndex = latencyCharIndex;
    }

    /* Write into the latency file */
    fwrite(&latency_measurements[0], (size_t)prevLatencyCharIndex, 1, fp_latency);
    fclose(fp_latency);
}
#endif
/**
 * Usage function
 * ~~~~~~~~~~~~~~
 *
 * The usage function gives the information to run the application.
 *
 * ``./bin/examples/pubsub_TSN_publisher -interface <ethernet_interface>`` runs the application.
 *
 * For more options, use ./bin/examples/pubsub_TSN_publisher -h. */
static void usage(char *appname)
{
    fprintf(stderr,
        "\n"
        "usage: %s [options]\n"
        "\n"
        " -interface       [name] Use network interface 'name'\n"
        " -cycleTimeInMsec [num]  Cycle time in milli seconds (default %lf)\n"
        " -socketPriority  [num]  Set publisher SO_PRIORITY to (default %d)\n"
        " -pubPriority     [num]  Publisher thread priority value (default %d)\n"
        " -subPriority     [num]  Subscriber thread priority value (default %d)\n"
        " -userAppPriority [num]  User application thread priority value (default %d)\n"
        " -pubCore         [num]  Run on CPU for publisher (default %d)\n"
        " -subCore         [num]  Run on CPU for subscriber (default %d)\n"
        " -userAppCore     [num]  Run on CPU for userApplication (default %d)\n"
        " -pubMacAddress   [name] Publisher Mac address (default %s - where 8 is the VLAN ID and 3 is the PCP)\n"
        " -subMacAddress   [name] Subscriber Mac address (default %s - where 8 is the VLAN ID and 3 is the PCP)\n"
        " -qbvOffset       [num]  QBV offset value (default %d)\n"
        " -disableSoTxtime        Do not use SO_TXTIME\n"
        " -enableCsvLog           Experimental: To log the data in csv files. Support up to 1 million samples\n"
        " -enableLatencyCsvLog    Experimental: To compute and create RTT latency csv. Support up to 1 million samples\n"
        " -enableconsolePrint     Experimental: To print the data in console output. Support for higher cycle time\n"
        " -enableBlockingSocket   Run application with blocking socket option. While using blocking socket option need to\n"
        "                         run both the Publisher and Loopback application. Otherwise application will not terminate.\n"
        " -enableXdpSubscribe     Enable XDP feature for subscriber. XDP_COPY and XDP_FLAGS_SKB_MODE is used by default. Not recommended to be enabled along with blocking socket.\n"
        " -xdpQueue        [num]  XDP queue value (default %d)\n"
        " -xdpFlagDrvMode         Use XDP in DRV mode\n"
        " -xdpBindFlagZeroCopy    Use Zero-Copy mode in XDP\n"
        "\n",
        appname, DEFAULT_CYCLE_TIME, DEFAULT_SOCKET_PRIORITY, DEFAULT_PUB_SCHED_PRIORITY, \
        DEFAULT_SUB_SCHED_PRIORITY, DEFAULT_USERAPPLICATION_SCHED_PRIORITY, \
        DEFAULT_PUB_CORE, DEFAULT_SUB_CORE, DEFAULT_USER_APP_CORE, \
        DEFAULT_PUBLISHING_MAC_ADDRESS, DEFAULT_SUBSCRIBING_MAC_ADDRESS, DEFAULT_QBV_OFFSET, DEFAULT_XDP_QUEUE);
}

/**
 * Main Server code
 * ~~~~~~~~~~~~~~~~
 *
 * The main function contains publisher and subscriber threads running in
 * parallel. */
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
    char            *progname            = NULL;
    pthread_t        userThreadID;

    /* Process the command line arguments */
    progname = strrchr(argv[0], '/');
    progname = progname ? 1 + progname : argv[0];

    static struct option long_options[] = {
        {"interface",            required_argument, 0, 'a'},
        {"cycleTimeInMsec",      required_argument, 0, 'b'},
        {"socketPriority",       required_argument, 0, 'c'},
        {"pubPriority",          required_argument, 0, 'd'},
        {"subPriority",          required_argument, 0, 'e'},
        {"userAppPriority",      required_argument, 0, 'f'},
        {"pubCore",              required_argument, 0, 'g'},
        {"subCore",              required_argument, 0, 'h'},
        {"userAppCore",          required_argument, 0, 'i'},
        {"pubMacAddress",        required_argument, 0, 'j'},
        {"subMacAddress",        required_argument, 0, 'k'},
        {"qbvOffset",            required_argument, 0, 'l'},
        {"disableSoTxtime",      no_argument,       0, 'm'},
        {"enableCsvLog",         no_argument,       0, 'n'},
        {"enableLatencyCsvLog",  no_argument,       0, 'o'},
        {"enableconsolePrint",   no_argument,       0, 'p'},
        {"enableBlockingSocket", no_argument,       0, 'q'},
        {"xdpQueue",             required_argument, 0, 'r'},
        {"xdpFlagDrvMode",       no_argument,       0, 's'},
        {"xdpBindFlagZeroCopy",  no_argument,       0, 't'},
        {"enableXdpSubscribe",   no_argument,       0, 'u'},
        {"help",                 no_argument,       0, 'v'},
        {0,                      0,                 0,  0 }
    };

    while((argInputs = getopt_long_only(argc, argv,"", long_options, &long_index)) != -1) {
        switch(argInputs) {
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
                pubPriority = atoi(optarg);
                break;
            case 'e':
                subPriority = atoi(optarg);
                break;
            case 'f':
                userAppPriority = atoi(optarg);
                break;
            case 'g':
                pubCore = atoi(optarg);
                break;
            case 'h':
                subCore = atoi(optarg);
                break;
            case 'i':
                userAppCore = atoi(optarg);
                break;
            case 'j':
                pubMacAddress = optarg;
                break;
            case 'k':
                subMacAddress = optarg;
                break;
            case 'l':
                qbvOffset = atoi(optarg);
                break;
            case 'm':
                disableSoTxtime = false;
                break;
            case 'n':
                enableCsvLog = true;
                break;
            case 'o':
                enableLatencyCsvLog = true;
                break;
            case 'p':
                consolePrint = true;
                break;
            case 'q':
                 /* TODO: Application need to be exited independently */
                enableBlockingSocket = true;
                break;
            case 'r':
                xdpQueue = (UA_UInt32)atoi(optarg);
                break;
            case 's':
                xdpFlag = XDP_FLAGS_DRV_MODE;
                break;
            case 't':
                xdpBindFlag = XDP_ZEROCOPY;
                break;
            case 'u':
                enableXdpSubscribe = true;
                break;
            case 'v':
                usage(progname);
                return -1;
            case '?':
                usage(progname);
                return -1;
        }
    }

    if(!interface) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Need a network interface to run");
        usage(progname);
        return -1;
    }

    if(cycleTimeInMsec < 0.125) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%f Bad cycle time", cycleTimeInMsec);
        usage(progname);
        return -1;
    }

    if(enableBlockingSocket == true) {
        if(enableXdpSubscribe == true) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "Cannot enable blocking socket and xdp at the same time");
            usage(progname);
            return -1;
        }
    }

    if(xdpFlag == XDP_FLAGS_DRV_MODE || xdpBindFlag == XDP_ZEROCOPY) {
        if(enableXdpSubscribe == false)
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                        "Flag enableXdpSubscribe is false, running application without XDP");
    }

    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
#if defined(PUBLISHER) && defined(SUBSCRIBER)
    /* Instantiate the PubSub SecurityPolicy */
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_calloc(2, sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 2;
#else
    config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy*)
        UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    config->pubSubConfig.securityPoliciesSize = 1;
#endif
#endif

#if defined(UA_ENABLE_PUBSUB_ENCRYPTION) && defined(PUBLISHER)
    UA_PubSubSecurityPolicy_Aes128Ctr(&config->pubSubConfig.securityPolicies[0],
                                      &config->logger);
#endif

#if defined(PUBLISHER)
    UA_NetworkAddressUrlDataType networkAddressUrlPub;
#endif

#if defined(SUBSCRIBER)
    UA_NetworkAddressUrlDataType networkAddressUrlSub;
#endif

#if defined(PUBLISHER)
    networkAddressUrlPub.networkInterface = UA_STRING(interface);
    networkAddressUrlPub.url              = UA_STRING(pubMacAddress);
#endif
#if defined(SUBSCRIBER)
    networkAddressUrlSub.networkInterface = UA_STRING(interface);
    networkAddressUrlSub.url              = UA_STRING(subMacAddress);
#endif

    if(enableCsvLog) {
#if defined(PUBLISHER)
        fpPublisher = fopen(filePublishedData, "w");
#endif

#if defined(SUBSCRIBER)
        fpSubscriber = fopen(fileSubscribedData, "w");
#endif
    }

    /* It is possible to use multiple PubSubTransportLayers on runtime.
     * The correct factory is selected on runtime by the standard defined
     * PubSub TransportProfileUri's. */

#if defined (PUBLISHER)
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

    /* Create variable nodes for publisher and subscriber in address space */
    addServerNodes(server);

#if defined(PUBLISHER)
    addPubSubConnection(server, &networkAddressUrlPub);
    addPublishedDataSet(server);
    _addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
#endif

#if defined(UA_ENABLE_PUBSUB_ENCRYPTION) && defined(SUBSCRIBER)
    UA_PubSubSecurityPolicy_Aes128Ctr(&config->pubSubConfig.securityPolicies[1],
                                      &config->logger);
#endif

#if defined (PUBLISHER) && defined(SUBSCRIBER)
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

#if defined(SUBSCRIBER) && !defined(PUBLISHER)
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

#if defined(SUBSCRIBER)
    addPubSubConnectionSubscriber(server, &networkAddressUrlSub);
    addReaderGroup(server);
    addDataSetReader(server);
    UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
#endif

    serverConfigStruct *serverConfig;
    serverConfig            = (serverConfigStruct*)UA_malloc(sizeof(serverConfigStruct));
    serverConfig->ServerRun = server;
#if defined(PUBLISHER) || defined(SUBSCRIBER)
    char threadNameUserApplication[22] = "UserApplicationPubSub";
    userThreadID = threadCreation((UA_Int16)userAppPriority, (size_t)userAppCore,
                                  userApplicationPubSub, threadNameUserApplication, serverConfig);
#endif

    retval |= UA_Server_run(server, &runningServer);
#if defined(SUBSCRIBER)
    UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier);
#endif
#if defined(PUBLISHER) || defined(SUBSCRIBER)
    returnValue = pthread_join(userThreadID, NULL);
    if(returnValue != 0)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "\nPthread Join Failed for User thread:%d\n", returnValue);
#endif

    if(enableCsvLog) {
#if defined(PUBLISHER)
        /* Write the published data in the publisher_T1.csv file */
        size_t pubLoopVariable               = 0;
        for(pubLoopVariable = 0; pubLoopVariable < measurementsPublisher;
             pubLoopVariable++) {
            fprintf(fpPublisher, "%lu,%lu.%09lu\n",
                    (long unsigned)publishCounterValue[pubLoopVariable],
                    (long unsigned)publishTimestamp[pubLoopVariable].tv_sec,
                    (long unsigned)publishTimestamp[pubLoopVariable].tv_nsec);
        }
#endif
#if defined(SUBSCRIBER)
        /* Write the subscribed data in the subscriber_T8.csv file */
        size_t subLoopVariable               = 0;
        for(subLoopVariable = 0; subLoopVariable < measurementsSubscriber;
             subLoopVariable++) {
            fprintf(fpSubscriber, "%lu,%lu.%09lu\n",
                    (long unsigned)subscribeCounterValue[subLoopVariable],
                    (long unsigned)subscribeTimestamp[subLoopVariable].tv_sec,
                    (long unsigned)subscribeTimestamp[subLoopVariable].tv_nsec);
        }
#endif
    }

    if(enableLatencyCsvLog) {
#if defined (PUBLISHER) && defined(SUBSCRIBER)
        char *latencyCsvName = LATENCY_CSV_FILE_NAME;
        computeLatencyAndGenerateCsv(latencyCsvName);
#endif
    }

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    removeServerNodes(server);
    UA_Server_delete(server);
    UA_free(serverConfig);
#endif

#if defined(PUBLISHER)
    UA_free(runningPub);
    UA_free(pubCounterData);
    for(UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
        UA_free(repeatedCounterData[iterator]);

    /* Free external data source */
    UA_free(pubDataValueRT);
    UA_free(runningPubDataValueRT);
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
        UA_free(repeatedDataValueRT[iterator]);
    if(enableCsvLog)
        fclose(fpPublisher);
#endif
#if defined(SUBSCRIBER)
    UA_free(runningSub);
    UA_free(subCounterData);
    for(UA_Int32 iterator = 0; iterator <  REPEATED_NODECOUNTS; iterator++)
        UA_free(subRepeatedCounterData[iterator]);

    /* Free external data source */
    UA_free(subDataValueRT);
    UA_free(runningSubDataValueRT);
    for(UA_Int32 iterator = 0; iterator < REPEATED_NODECOUNTS; iterator++)
        UA_free(subRepeatedDataValueRT[iterator]);
    if(enableCsvLog)
        fclose(fpSubscriber);
#endif

    return (int)retval;
}
