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
 *         |  |              |  |                  |  |                |  |
 *  Kernel |  |              |  |                  |  |                |  |
 *  Space  |  |              |  |                  |  |                |  |
 *         |  |              |  |                  |  |                |  |
 *         v  +--------------+  |                  v  +----------------+  |
 *         T2 |  TX tcpdump  |  T7<----------------T6 |   RX tcpdump   |  T3
 *         |  +--------------+                        +----------------+  ^
 *         |                                                              |
 *         ----------------------------------------------------------------
 */


#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/io.h>
/* For thread operations */
#include <pthread.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <ua_server_internal.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/log_stdout.h>

/* These defines enables the publisher and subscriber of the OPCUA stack */
/* To run only publisher, enable PUBLISHER define alone (comment SUBSCRIBER) */
#define             PUBLISHER
/* To run only subscriber, enable SUBSCRIBER define alone
 * (comment PUBLISHER) */
#define             SUBSCRIBER
/* Use server interrupt or system interrupt? */
#define             PUB_SYSTEM_INTERRUPT
/* Publish interval in milliseconds */
#define             PUB_INTERVAL                    250
/* Cycle time in ns. Eg: For 100us: 100*1000 */
#define             CYCLE_TIME                      100 * 1000
#define             SECONDS_SLEEP                   2
#define             NANO_SECONDS_SLEEP              999000000
#define             NEXT_CYCLE_START_TIME           3
#define             CYCLE_TIME_NINTY_FIVE_PERCENT   0.95
#define             FIVE_MICRO_SECOND               5
#define             MILLI_SECONDS                   1000 * 1000
#define             SECONDS                         1000 * 1000 * 1000
#define             PUB_SCHED_PRIORITY              87
#define             SUB_SCHED_PRIORITY              88
#define             DATA_SET_WRITER_ID              62541
#define             KEY_FRAME_COUNT                 10
#define             BUFFER_LENGTH                   512
#define             MAX_MEASUREMENTS                10000000
#define             NETWORK_MSG_COUNT               1
#define             CORE_TWO                        2
#define             CORE_THREE                      3
#define             TX_TIME_ONE                     1
#define             TX_TIME_ZERO                    0
#define             ONE                             1
#define             CONNECTION_NUMBER               2
#define             PORT_NUMBER                     62541
#define             FAILURE_EXIT                    -1
#define             CLOCKID                         CLOCK_TAI
#define             SIG                             SIGUSR1

/* This is a hardcoded publisher/subscriber IP address. If the IP address need
 * to be changed, change it in the below line.
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING only is enabled,
 * Change in line number 13 in plugins/ua_pubsub_udp.c
 *
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_INTERRUPT and
 * UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING_TSN is enabled,
 * change in line number 46 in plugins/ua_pubsub_udp_custom_handling.c
 */
#define             PUBSUB_IP_ADDRESS              "192.168.9.11"
#define             PUBLISHER_MULTICAST_ADDRESS    "opc.udp://224.0.0.22:4840/"
#define             SUBSCRIBER_MULTICAST_ADDRESS   "opc.udp://224.0.0.32:4840/"
/* Variable for next cycle start time */
struct timespec              nextCycleStartTime;
/* When the timer was created */
struct timespec              pubStartTime;
/* Set server running as true */
UA_Boolean                   running                = UA_TRUE;
/* Interval in ns */
UA_Int64                     pubIntervalNs;
/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_PubSubConnection*         connection;
UA_NodeId                    connectionIdent;
UA_NodeId                    publishedDataSetIdent;
UA_NodeId                    writerGroupIdent;
UA_NodeId                    pubNodeID;
UA_NodeId                    subNodeID;
/* Variable for PubSub callback */
UA_ServerCallback            pubCallback;
/* Variables for counter data handling in address space */
UA_UInt64                    pubCounterData         = 0;
UA_UInt64                    subCounterData         = 0;
UA_Variant                   pubCounter;
UA_Variant                   subCounter;

/* For adding nodes in the server information model */
static void addServerNodes(UA_Server* server);

/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);

#if defined(PUBLISHER)
/* File to store the data and timestamps for different traffic */
FILE*                        fpPublisher;
char*                        filePublishedData      = "publisher_T5.csv";

/* To lock the thread */
pthread_mutex_t              lock;

/* Thread for publisher */
pthread_t                    pubThreadID;

/* Array to store published counter data */
UA_UInt64                    publishCounterValue[MAX_MEASUREMENTS];
size_t                       measurementsPublisher  = 0;

/* Process scheduling parameter for publisher */
struct sched_param           schedParamPublisher;

/* Array to store timestamp */
struct timespec              publishTimestamp[MAX_MEASUREMENTS];
struct timespec              dataModificationTime;

/* Publisher thread routine for ETF */
void*                        publisherETF(void* arg);
#endif


#if defined(SUBSCRIBER)
/* Variable for PubSub connection creation */
UA_NodeId                    connectionIdentSubscriber;

/* File to store the data and timestamps for different traffic */
FILE*                        fpSubscriber;
char*                        fileSubscribedData     = "subscriber_T4.csv";

/* Thread for subscriber */
pthread_t                    subThreadID;

/* Array to store subscribed counter data */
UA_UInt64                    subscribeCounterValue[MAX_MEASUREMENTS];
size_t                       measurementsSubscriber = 0;

/* Process scheduling parameter for subscriber */
struct sched_param           schedParamSubscriber;

/* Array to store timestamp */
struct timespec              subscribeTimestamp[MAX_MEASUREMENTS];
struct timespec              dataReceiveTime;

/* Subscriber thread routine */
void*                        subscriber(void* arg);

/* OPCUA Subscribe API */
void                         subscribe(void);
#endif

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
}

#ifndef PUB_SYSTEM_INTERRUPT
/*****************************/
/* Server Event Loop Publish */
/*****************************/
/* Add a publishing callback function */
static void publishCallback(UA_Server* server, void* data) {
    struct timespec start_time;
    struct timespec end_time;

    clock_gettime(CLOCKID, &start_time);
    pubCallback(server, data);
    clock_gettime(CLOCKID, &end_time);
    updateMeasurementsPublisher(start_time, end_time);
}

/* Add a callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    pubIntervalNs = interval_ms * MILLI_SECONDS;
    pubCallback = callback;
    clock_gettime(CLOCKID, &pubStartTime);
    return UA_Timer_addRepeatedCallback(&server->timer, (UA_TimerCallback)publishCallback,
                                        data, interval_ms, callbackId);
}

/* Modify the interval of the callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms) {
    pubIntervalNs = interval_ms * MILLI_SECONDS;
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId, interval_ms);
}

/* Remove the callback added for cyclic repetition */
UAStatusCode
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&server->timer, callbackId);
}

#else
/*****************************/
/* System Interrupt Callback */
/*****************************/

/* For one publish callback only... */
UA_Server*      pubServer;
void*           pubData;
struct sigevent pubEvent;
timer_t         pubEventTimer;

static void handler(int sig, siginfo_t *si, void *uc) {
    if(si->si_value.sival_ptr != &pubEventTimer) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "stray signal");
        return;
    }
}

/* Add a callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    pubServer                       = server;
    pubCallback                     = callback;
    pubData                         = data;
    pubIntervalNs                   = (UA_Int64)interval_ms * MILLI_SECONDS;

    /* Handle the signal */
    struct sigaction sa;
    sa.sa_flags                     = SA_SIGINFO;
    sa.sa_sigaction                 = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG, &sa, NULL);

    /* Create the event */
    pubEvent.sigev_notify           = SIGEV_NONE;
    pubEvent.sigev_signo            = SIG;
    pubEvent.sigev_value.sival_ptr  = &pubEventTimer;
    int resultTimerCreate           = timer_create(CLOCKID, &pubEvent, &pubEventTimer);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to create a system event");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Arm the timer */
    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec   = 0;
    timerspec.it_interval.tv_nsec  = pubIntervalNs;
    timerspec.it_value.tv_sec      = 0;
    timerspec.it_value.tv_nsec     = pubIntervalNs;
    resultTimerCreate = timer_settime(pubEventTimer, 0, &timerspec, NULL);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to arm the system timer");
        timer_delete(pubEventTimer);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    clock_gettime(CLOCKID, &pubStartTime);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Created Publish Callback with interval %f", interval_ms);
    return UA_STATUSCODE_GOOD;
}

/* Modify the interval of the callback for cyclic repetition */
UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms) {
    pubIntervalNs                 = (UA_Int64)interval_ms * MILLI_SECONDS;
    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec  = 0;
    timerspec.it_interval.tv_nsec = pubIntervalNs;
    timerspec.it_value.tv_sec     = 0;
    timerspec.it_value.tv_nsec    = pubIntervalNs;
    int resultTimerCreate         = timer_settime(pubEventTimer, 0, &timerspec, NULL);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to arm the system timer");
        timer_delete(pubEventTimer);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Changed the publish callback to interval %f", interval_ms);
    return UA_STATUSCODE_GOOD;
}

/* Remove the callback added for cyclic repetition */
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    timer_delete(pubEventTimer);
}
#endif

#if defined(PUBLISHER)
/**
 * **PubSub connection handling**
 *
 * Create a new ConnectionConfig. The addPubSubConnection function takes the
 * config and create a new connection. The Connection identifier is
 * copied to the NodeId parameter.
 */
static void
addPubSubConnection(UA_Server *server){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                          = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri           = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled                       = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(PUBSUB_IP_ADDRESS),
        UA_STRING(PUBLISHER_MULTICAST_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric           = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **Published dataset handling**
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
    UA_DataSetFieldConfig counterValue1;
    memset(&counterValue1, 0, sizeof(UA_DataSetFieldConfig));
    counterValue1.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue1.field.variable.fieldNameAlias                      = UA_STRING("Counter Variable 1");
    counterValue1.field.variable.promotedField                       = UA_FALSE;
    counterValue1.field.variable.publishParameters.publishedVariable = pubNodeID;
    counterValue1.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &counterValue1, &dataSetFieldIdent1);

    UA_NodeId dataSetFieldIdent2;
    UA_DataSetFieldConfig counterValue2;
    memset(&counterValue2, 0, sizeof(UA_DataSetFieldConfig));
    counterValue2.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue2.field.variable.fieldNameAlias                      = UA_STRING("Counter Variable 2");
    counterValue2.field.variable.promotedField                       = UA_FALSE;
    counterValue2.field.variable.publishParameters.publishedVariable = subNodeID;
    counterValue2.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    /* Revert this line to add second node in the information model. As of now, dataSetFieldIdent1 is enough to send the counter data
     * on the network. */
    UA_Server_addDataSetField(server, publishedDataSetIdent, &counterValue2, &dataSetFieldIdent2);
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
    writerGroupConfig.name                      = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval        = PUB_INTERVAL;
    writerGroupConfig.enabled                   = UA_FALSE;
    writerGroupConfig.encodingMimeType          = UA_PUBSUB_ENCODING_UADP;
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
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATA_SET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount = KEY_FRAME_COUNT;
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

static void nanoSecondFieldConversion(struct timespec *timeSpecValue) {
    /* Check if ns field is greater than '1 ns less than 1sec' */
    while(timeSpecValue->tv_nsec > (SECONDS -1)) {
        /* Move to next second and remove it from ns field */
        timeSpecValue->tv_sec  += ONE;
        timeSpecValue->tv_nsec -= SECONDS;
    }
}

/**
 * **Publisher thread routine**
 *
 * The publisherTBS function is the routine used by the publisher thread.
 * This routine publishes the data at a cycle time of 100us.
 */
void* publisherETF(void *arg) {
    struct timespec nextnanosleeptime, currenttime;
    UA_Int64        clockNanoSleep                 = 0;
    UA_Int32        txtime_reach                   = 0;

    /* Initialise value for nextnanosleeptime timespec */
    nextnanosleeptime.tv_nsec                      = 0;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptime.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptime.tv_nsec                      = NANO_SECONDS_SLEEP;
    /* For spinloop until the second boundary is reached */
    clock_gettime(CLOCKID, &nextCycleStartTime);
    nextCycleStartTime.tv_sec                     += NEXT_CYCLE_START_TIME ;
    nextCycleStartTime.tv_nsec                     = 0;
    clockNanoSleep                                 = clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);

    while(txtime_reach == TX_TIME_ZERO) {
        clock_gettime(CLOCKID, &currenttime);
        if(currenttime.tv_sec == nextCycleStartTime.tv_sec) {
            txtime_reach = TX_TIME_ONE;
        }
    }

    txtime_reach = TX_TIME_ONE;

    while(running) {
        /* TODO: For lower cycletimes, the value may have to be less than 90% of cycle time */
        UA_Double cycletimeval          = CYCLE_TIME_NINTY_FIVE_PERCENT * (UA_Double)CYCLE_TIME;
        nextnanosleeptime.tv_nsec       = nextCycleStartTime.tv_nsec + (__syscall_slong_t)cycletimeval;
        nanoSecondFieldConversion(&nextnanosleeptime);
        clockNanoSleep                  = clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        clock_gettime(CLOCKID, &dataModificationTime);
        UA_Variant_setScalar(&pubCounter, &subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
        UA_NodeId currentNodeId         = UA_NODEID_STRING(1, "PublisherCounter");
        UA_Server_writeValue(pubServer, currentNodeId, pubCounter);
         /* OPC UA Publish */
        /* Lock the code section */
        pthread_mutex_lock(&lock);
        pubCallback(pubServer, pubData);
        /* Unlock the code section */
        pthread_mutex_unlock(&lock);
        UA_UInt64 values                = 0;
        UA_Variant *cntVal              = UA_Variant_new();
        UA_StatusCode retVal            = UA_Server_readValue(pubServer, currentNodeId, cntVal);

        if(retVal == UA_STATUSCODE_GOOD && UA_Variant_isScalar(cntVal) && cntVal->type == &UA_TYPES[UA_TYPES_UINT64]) {
            values =  *(UA_UInt64*)cntVal->data;
        }
        if(values > 0) {
            updateMeasurementsPublisher(dataModificationTime, values);
        }
        UA_Variant_delete(cntVal);
        switch (clockNanoSleep) {
            case 0:
                txtime_reach = TX_TIME_ZERO;
                while(txtime_reach == TX_TIME_ZERO) {
                    clock_gettime(CLOCKID, &currenttime);
                    if(currenttime.tv_sec == nextCycleStartTime.tv_sec && currenttime.tv_nsec > nextCycleStartTime.tv_nsec) {
                        nextCycleStartTime.tv_nsec = nextCycleStartTime.tv_nsec + (CYCLE_TIME);
                        nanoSecondFieldConversion(&nextCycleStartTime);
                        txtime_reach = TX_TIME_ONE;
                    }
                    else if(currenttime.tv_sec > nextCycleStartTime.tv_sec) {
                        nextCycleStartTime.tv_nsec = nextCycleStartTime.tv_nsec + (CYCLE_TIME);
                        nanoSecondFieldConversion(&nextCycleStartTime);
                        txtime_reach = TX_TIME_ONE;
                    }
               }
         }
    }
    return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)
/**
 * Subscribed data handling**
 * The subscribed data is updated in the array using this function
 */
static void
updateMeasurementsSubscriber(struct timespec receive_time, UA_UInt64 counterValue) {
    subscribeTimestamp[measurementsSubscriber]     = receive_time;
    subscribeCounterValue[measurementsSubscriber]  = counterValue;
    measurementsSubscriber++;
}

/**
 * **Subscriber thread routine**
 *
 * The subscriber function is the routine used by the subscriber thread.
 */
void* subscriber(void *arg) {
    while(running) {
       subscribe();
    }
    /* pthread_exit(NULL); */
    return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)
void subscribe(void) {
    UA_UInt64  counterDataLocalBuffer = 0;
    UA_ByteString buffer;
    if(UA_ByteString_allocBuffer(&buffer, BUFFER_LENGTH) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return;
    }

    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval =
        connection->channel->receive(connection->channel, &buffer, NULL, FIVE_MICRO_SECOND);
    if(retval != UA_STATUSCODE_GOOD || buffer.length == 0) {
        /* Workaround!! Reset buffer length. Receive can set the length to zero.
         * Then the buffer is not deleted because no memory allocation is
         * assumed.
         * TODO: Return an error code in 'receive' instead of setting the buf
         * length to zero. */
        buffer.length             = BUFFER_LENGTH;
        UA_ByteString_deleteMembers(&buffer);
        return;
    }

    /* Decode the message */
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(UA_NetworkMessage));
    size_t currentPosition = 0;
    UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &networkMessage);
    UA_ByteString_deleteMembers(&buffer);

    /* Is this the correct message type? */
    if(networkMessage.networkMessageType != UA_NETWORKMESSAGE_DATASET) {
        goto cleanup;
    }

    /* At least one DataSetMessage in the NetworkMessage? */
    if(networkMessage.payloadHeaderEnabled &&
       networkMessage.payloadHeader.dataSetPayloadHeader.count < NETWORK_MSG_COUNT) {
        goto cleanup;
    }

    /* Is this a KeyFrame-DataSetMessage? */
    UA_DataSetMessage *dsm = &networkMessage.payload.dataSetPayload.dataSetMessages[0];
    if(dsm->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME) {
        goto cleanup;
    }

    /* Loop over the fields and print well-known content types */
    for(int fieldCount = 0; fieldCount < dsm->data.keyFrameData.fieldCount; fieldCount++) {
        const UA_DataType *currentType = dsm->data.keyFrameData.dataSetFields[fieldCount].value.type;
        if(currentType == &UA_TYPES[UA_TYPES_BYTE]) {

        }
        else if(currentType == &UA_TYPES[UA_TYPES_DATETIME]) {

        }
        else if(currentType == &UA_TYPES[UA_TYPES_UINT64]) {

        }
        else {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Message content is not of type UInt64 ");
        }
    }
    subCounterData = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[1].value.data;
    counterDataLocalBuffer = subCounterData;
    clock_gettime(CLOCKID, &dataReceiveTime);
    UA_Variant_setScalar(&subCounter, &subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    UA_NodeId currentNodeId1 = UA_NODEID_STRING(1, "SubscriberCounter");
    /* Lock the code section */
    pthread_mutex_lock(&lock);
    UA_Server_writeValue(pubServer, currentNodeId1, subCounter);
    /* Unlock the code section */
    pthread_mutex_unlock(&lock);
    updateMeasurementsSubscriber(dataReceiveTime, counterDataLocalBuffer);

 cleanup:
    UA_NetworkMessage_deleteMembers(&networkMessage);
}
#endif

/**
 * **Creation of nodes**
 * The addServerNodes function is used to create the publisher and subscriber
 * nodes.
 */
static void addServerNodes(UA_Server *server) {
    UA_NodeId robotId;
    UA_ObjectAttributes oAttr    = UA_ObjectAttributes_default;
    oAttr.displayName            = UA_LOCALIZEDTEXT("en-US", "Robot Simulation");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Robot Simulation"), UA_NODEID_NULL,
                            oAttr, NULL, &robotId);

    UA_VariableAttributes p4Attr = UA_VariableAttributes_default;
    UA_UInt64 axis4position      = 0;
    UA_Variant_setScalar(&p4Attr.value, &axis4position, &UA_TYPES[UA_TYPES_UINT64]);
    p4Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    UA_NodeId newNodeId          = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, robotId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, p4Attr, NULL, &pubNodeID);
    UA_VariableAttributes p5Attr = UA_VariableAttributes_default;
    UA_UInt64 axis5position      = 0;
    UA_Variant_setScalar(&p5Attr.value, &axis5position, &UA_TYPES[UA_TYPES_UINT64]);
    p5Attr.displayName           = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId                    = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, robotId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, p5Attr, NULL, &subNodeID);
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

    /* Delete the Subscriber Counter Node*/
    UA_Server_deleteNode(server, subNodeID, UA_TRUE);
    UA_NodeId_deleteMembers(&subNodeID);
}

/**
 * **Main Server code**
 *
 * The main function contains publisher and subscriber threads running in
 * parallel.
 */
int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Int32         returnValue         = 0;
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    UA_Int32         errorSetAffinity    = 0;
    UA_Server*       server              = UA_Server_new();
    UA_ServerConfig* config              = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

#if defined(PUBLISHER)
    fpPublisher                          = fopen(filePublishedData, "a");
#endif
#if defined(SUBSCRIBER)
    fpSubscriber                         = fopen(fileSubscribedData, "a");
#endif

#if defined(PUBLISHER) && defined(SUBSCRIBER)
/* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    config->pubsubTransportLayers     = (UA_PubSubTransportLayer *)
                                     UA_malloc(CONNECTION_NUMBER * sizeof(UA_PubSubTransportLayer));
#else
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
                                     UA_malloc(sizeof(UA_PubSubTransportLayer));
#endif
    if(!config->pubsubTransportLayers) {
        return FAILURE_EXIT;
    }
/* It is possible to use multiple PubSubTransportLayers on runtime.
 * The correct factory is selected on runtime by the standard defined
 * PubSub TransportProfileUri's.
 */
#if defined(PUBLISHER) && defined(SUBSCRIBER)
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#else
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    /* Server is the new OPCUA model which has both publisher and subscriber configuration */
    /* add axis node and OPCUA pubsub client server counter nodes */
    addServerNodes(server);
#endif

#if defined(PUBLISHER)
    addPubSubConnection(server);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
#endif

    if(pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n Mutex initialization has failed\n");
        return 1;
    }

#if defined(SUBSCRIBER)
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                               = UA_STRING("UDP-UADP Connection 2");
    connectionConfig.transportProfileUri                = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl      = {UA_STRING(PUBSUB_IP_ADDRESS),
        UA_STRING(SUBSCRIBER_MULTICAST_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric                = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentSubscriber);
    if(connection != NULL) {
        UA_StatusCode rv = connection->channel->regist(connection->channel, NULL, NULL);
        if(rv != UA_STATUSCODE_GOOD) {
            printf("PubSub registration failed for the requested channel\n");
        }
    }
#endif

#if defined(PUBLISHER)
    /* Core affinity set for publisher */
    cpu_set_t cpusetPub;
    /* Return the ID for publisher thread */
    pubThreadID = pthread_self();
    schedParamPublisher.sched_priority = PUB_SCHED_PRIORITY; /* sched_get_priority_max(SCHED_FIFO) */

    returnValue = pthread_setschedparam(pubThreadID, SCHED_FIFO, &schedParamPublisher);
    if(returnValue != 0) {
        printf("pthread_setschedparam: failed\n");
        exit(1);
    }

    CPU_ZERO(&cpusetPub);
    CPU_SET(CORE_TWO, &cpusetPub);
    errorSetAffinity = pthread_setaffinity_np(pubThreadID, sizeof(cpu_set_t), &cpusetPub);
    if(errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        return -1;
    }

    printf("pthread_setschedparam: publisher thread priority is %d \n", schedParamPublisher.sched_priority);
    returnValue = pthread_create(&pubThreadID, NULL, &publisherETF, NULL);
    if(returnValue != 0) {
        printf("publisherETF: cannot create thread\n");
        exit(1);
    }

    returnValue = pthread_getaffinity_np(pubThreadID, sizeof(cpu_set_t), &cpusetPub);
    if(returnValue != 0) {
        printf("Get affinity fail\n");
    }

    if(CPU_ISSET(CORE_TWO, &cpusetPub)) {
        printf("CPU %d\n", CORE_TWO);
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
    if(returnValue != 0) {
        printf("pthread_setschedparam: failed\n");
    }

    printf("\npthread_setschedparam: subscriber thread priority is %d \n", schedParamSubscriber.sched_priority);
    CPU_ZERO(&cpusetSub);
    CPU_SET(CORE_THREE, &cpusetSub);
    errorSetAffinity = pthread_setaffinity_np(subThreadID, sizeof(cpu_set_t), &cpusetSub);
    if(errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        return -1;
    }

    returnValue = pthread_create(&subThreadID, NULL, &subscriber, NULL);
    if(returnValue != 0) {
        printf("subscriber: cannot create thread\n");
    }

    returnValue = pthread_getaffinity_np(subThreadID, sizeof(cpu_set_t), &cpusetSub);
    if(returnValue != 0) {
        printf("Get affinity fail\n");
    }

    if(CPU_ISSET(CORE_THREE, &cpusetSub)) {
        printf("CPU %d\n", CORE_THREE);
    }

    /* Create the subscriber thread */
    returnValue                          = pthread_create(&subThreadID, NULL,
                                                          &subscriber, NULL);
    if (returnValue != 0) {
        printf("Subscriber thread cannot be created\n");
    }

#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER)
    retval |= UA_Server_run(server, &running);
#endif

#if defined(PUBLISHER)
    returnValue = pthread_join(pubThreadID, NULL);
    if (returnValue != 0) {
        printf("\nPthread Join Failed for publisher thread:%d\n", returnValue);
    }
#endif

#if defined(SUBSCRIBER)
    returnValue = pthread_join(subThreadID, NULL);
    if (returnValue != 0) {
        printf("\nPthread Join Failed for subscriber thread:%d\n", returnValue);
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
