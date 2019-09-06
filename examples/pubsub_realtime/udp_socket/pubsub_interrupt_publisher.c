/*********************************************************************************

\file   pubsub_interrupt_publisher.c

\brief  Real-time OPC UA Pub/Sub

This file contains the main implementation of the standalone interrupt based
publisher, standalone thread based publisher and standalone subscriber. For
benchmarking the application data round trip time, the counter data loopback
logic is added.

\ingroup interrupt based publisher
*******************************************************************************/

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

/* These defines enables the publisher and subscriber of the OPCUA stack */
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <ua_server_internal.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/log_stdout.h>

/* Note: This module supports two types of publisher, i.e. thread based publisher
 * and interrupt/event based publisher; and one subscriber, i.e. thread based subscriber.
 * Based on the node requirement, a subscriber or publisher can be enabled in the module
 * using the macros provided below. Note that, only one type of publisher can be started
 * for any instance.
 * To run, uncomment: SUBSCRIBER and PUB_SYSTEM_INTERRUPT or PUBLISHER
 */

/* To run thread based publisher
 * uncomment: PUBLISHER
 * comment: PUB_SYSTEM_INTERRUPT
 */
#define             PUBLISHER

/* To run thread based standalone subscriber
 * uncomment: SUBSCRIBER
 */
#define             SUBSCRIBER

/* To run interrupt/event based standalone publisher
 * uncomment: PUB_SYSTEM_INTERRUPT
 * comment: PUBLISHER
 */
//#define            PUB_SYSTEM_INTERRUPT

/* To calculate round trip time enable LOOPBACK_T1
 * on PC1 and LOOPBACK_T4 on PC2
 * Refer the above architecture 
 */

/* Loopback_T4 subscribes(T4) to the counter data and
 * loops back i.e publishes the same data from Publisher(T5).
 * To run loop back in T4,
 * uncomment: SUBSCRIBER, LOOPBACK_T4
 * uncomment: PUBLISHER or PUB_SYSTEM_INTERRUPT
 */
//#define              LOOPBACK_T4

/* Loopback_T1 subscribes(T8) to the published data from
 * T5 and stores the counter data with timestamp in
 * a csv file.
 * To enable,
 * uncomment: SUBSCRIBER, LOOPBACK_T1
 * uncomment: PUBLISHER or PUB_SYSTEM_INTERRUPT
 */
//#define              LOOPBACK_T1

#if defined(PUBLISHER) && defined(PUB_SYSTEM_INTERRUPT)
	#error "Cannot enable both interrupt based publisher and thread based publisher.\
	        Enable any one type of publisher"
#endif

#if defined(LOOPBACK_T1) && defined(LOOPBACK_T4)
	#error "Cannot enable both loopbacks on the same node.\
	        Enable any one."
#endif

/* Publish interval in milliseconds- Now @100us */
#define             PUB_INTERVAL                    0.1
#define             FIVE_MILLI_SECOND               5
#define             MILLI_SECONDS                   1000 * 1000
#define             CYCLE_TIME                      (long)(PUB_INTERVAL * MILLI_SECONDS)
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
#define             ONE_SECOND                      1
#define             CONNECTION_NUMBER               2
#define             PORT_NUMBER                     62541
#define             FAILURE_EXIT                    -1
#define             CLOCKID                         CLOCK_TAI
#define             SIG                             SIGRTMAX
#define             LEADTIME                        150000

/* This is a hardcoded publisher/subscriber IP address. If the IP address need
 * to be changed, change it in the below line.
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING only is enabled,
 * Change in line number 13 in plugins/ua_pubsub_udp.c
 *
 * If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_INTERRUPT and
 * UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING_TSN is enabled,
 * change in line number 46 in plugins/ua_pubsub_udp_custom_handling.c
 */
#define             PUBSUB_IP_ADDRESS              "192.168.1.11"
#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
#define             PUBLISHER_MULTICAST_ADDRESS    "opc.udp://224.0.0.42:4840/"
#endif
#if defined(SUBSCRIBER)
#define             SUBSCRIBER_MULTICAST_ADDRESS   "opc.udp://224.0.0.32:4840/"
#endif

/* Set server running as true */
UA_Boolean                   running                = UA_TRUE;

/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_NodeId                    pubNodeID;
UA_NodeId                    subNodeID;

/* Variables for counter data handling in address space */
UA_UInt64                    pubCounterData         = 0;
UA_Variant                   pubCounter;
UA_Variant                   subCounter;

/* Extern variable for next cycle start time(SO_TXTIME) */
struct timespec              nextCycleStartTime;

/* For adding nodes in the server information model */
static void addServerNodes(UA_Server* server);

/* For deleting the nodes created */
static void removeServerNodes(UA_Server *server);

#if defined(LOOPBACK_T4)
/* To lock the thread */
pthread_mutex_t              lock;
#endif

#if defined(PUBLISHER)

/* Publisher thread routine for ETF */
void*                        publisherETF(void* arg);

#endif

#if defined(PUB_SYSTEM_INTERRUPT)

/* event timer */
timer_t                      pubEventTimer;

/* calculating firstTxtime */
int                          firstTxTime           = 0;

/* Interval in ns */
UA_Int64                     pubIntervalNs;

/* Handle the signal */
struct                       sigaction sa;
struct timespec              pubStartTime;

/* signal */
struct sigevent              pubEvent;

#endif


#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)

/* File to store the data and timestamps for different traffic */
FILE*                        fpPublisher;

#if defined(LOOPBACK_T4)
char*                        filePublishedData      = "publisher_T5.csv";
#else
char*                        filePublishedData      = "publisher_T1.csv";
#endif

/* Array to store published counter data */
UA_UInt64                    publishCounterValue[MAX_MEASUREMENTS];
size_t                       measurementsPublisher  = 0;

/* For one publish callback only... */
UA_Server*                   pubServer;
void*                        pubData;

/* Thread for subscriber */
pthread_t                    pubThreadID;

/* Variable for PubSub callback */
UA_ServerCallback            pubCallback;

/* Node ID*/
UA_NodeId                    publishedDataSetIdent;
UA_NodeId                    writerGroupIdent;
UA_NodeId                    connectionIdent;

/* Array to store timestamp */
struct timespec              publishTimestamp[MAX_MEASUREMENTS];

struct sched_param schedParamPublisher;

/* File operations */
struct timespec              dataModificationTime;
static void updateMeasurementsPublisher(struct timespec, UA_UInt64);
/* Nanoseconds conversion */
static void nanoSecondFieldConversion(struct timespec *);

/* Set Priority */
static int setSelfPrio(void);

#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER) || defined(PUB_SYSTEM_INTERRUPT)
UA_Int32                     errorSetAffinity    = 0;
UA_Int32                     returnValue         = 0;
#endif

#if defined(SUBSCRIBER)

UA_PubSubConnection*         connection;

/* Variable for PubSub connection creation */
UA_NodeId                    connectionIdentSubscriber;

/* File to store the data and timestamps for different traffic */
FILE*                        fpSubscriber;

#if defined(LOOPBACK_T4)
char*                        fileSubscribedData     = "subscriber_T4.csv";
#else
char*                        fileSubscribedData     = "subscriber_T8.csv";
#endif

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

//------------------------------------------------------------------------------
/**
\brief  Signal handler ctrl+c

This function closes the application and destroys the mutex objects gracefully.
*/
//------------------------------------------------------------------------------
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
#if defined(LOOPBACK_T4)
    pthread_mutex_destroy(&lock);
#endif
}

//------------------------------------------------------------------------------
/**
\brief  Custom callback for cyclic repitition(Thread based publisher)

The function creates a callback that triggers the collection and publish of
NetworkMessages and the contained DataSetMessages

\param[in]     pubServer          Server instance
\param[in]     pubCallback        Callback function
\param[in]     data               Counter data to be published
\param[in]     interval_ms        Writer group interval
\param[in]     callbackId         Callback identifier

\return The function returns a status code
*/
//------------------------------------------------------------------------------
#if defined(PUBLISHER)
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    pubServer                       = server;
    pubCallback                     = callback;
    pubData                         = data;

    return UA_STATUSCODE_GOOD;
}

//------------------------------------------------------------------------------
/**
\brief Remove the callback added for cyclic repetition(Thread based publisher)

The function removes the callback

\param[in]     callbackId         Callback identifier
\param[in]     server             Server instance

\return The function returns a status code
*/
//------------------------------------------------------------------------------
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
 /*Since this is a custom while loop execution, no timer is required for this callback
 * function. So timer_delete() is not used and call back can be registered
 */
 pubCallback = NULL;
}
#endif

#if defined (PUB_SYSTEM_INTERRUPT)
//------------------------------------------------------------------------------
/**
\brief  Interrupt based publisher handler

The function is a signal handler function that tiggers for every timer event

*/
//------------------------------------------------------------------------------
static void handler(int sig, siginfo_t *si, void *uc) {
    if(firstTxTime == 0) {
        clock_gettime(CLOCKID, &nextCycleStartTime);
        nextCycleStartTime.tv_sec += 0;
        /* Initial start time 100us offset */
        nextCycleStartTime.tv_nsec += LEADTIME;
        nanoSecondFieldConversion(&nextCycleStartTime);
        firstTxTime++;
    }
    else {
        nextCycleStartTime.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextCycleStartTime);
    }

    pubCounterData++;
    UA_Variant_setScalar(&pubCounter, &pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    UA_NodeId currentNodeId         = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_writeValue(pubServer, currentNodeId, pubCounter);
    clock_gettime(CLOCKID, &dataModificationTime);
    if(measurementsPublisher < MAX_MEASUREMENTS) {
        updateMeasurementsPublisher(dataModificationTime, pubCounterData);
    }

#if defined(LOOPBACK_T4)
    /* Lock the code section */
    pthread_mutex_lock(&lock);
#endif
    pubCallback(pubServer, pubData);
#if defined(LOOPBACK_T4)
    /* Lock the code section */
    pthread_mutex_unlock(&lock);
#endif
    if(si->si_value.sival_ptr != &pubEventTimer) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "stray signal");
        return;
    }

}

//------------------------------------------------------------------------------
/**
\brief  Custom callback for cyclic repetition(Interrupt based publisher)

The function creates a callback that triggers the collection and publish of
NetworkMessages and the contained DataSetMessages

\param[in]     server          Server instance
\param[in]     data            Publisher data
\param[in]     callback        callback function
\param[in]     callbackId      Callback identifier
\param[in]     interval_ms     Writer group interval

\return The function returns a status code
*/
//------------------------------------------------------------------------------
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    int  retVal;

    pubServer                       = server;
    pubCallback                     = callback;
    pubData                         = data;
    pubIntervalNs                   = (UA_Int64)(interval_ms * MILLI_SECONDS);

    /*set self priority for the application */
    retVal = setSelfPrio();
    if(retVal != 0) {
        printf("Not able to self priority\n");
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags                     = SA_SIGINFO;
    sa.sa_sigaction                 = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG, &sa, NULL);

    /* Create the event */
    memset(&pubEventTimer, 0, sizeof(pubEventTimer));
    pubEvent.sigev_notify           = SIGEV_SIGNAL;
    pubEvent.sigev_signo            = SIG;
    pubEvent.sigev_value.sival_ptr  = &pubEventTimer;
    int resultTimerCreate           = timer_create(CLOCKID, &pubEvent, &pubEventTimer);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to create a system event");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Arm the timer */
    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec   = (long int) (pubIntervalNs / (SECONDS));
    timerspec.it_interval.tv_nsec  = CYCLE_TIME;
    timerspec.it_value.tv_sec      = ONE_SECOND;
    timerspec.it_value.tv_nsec     = (long int) (pubIntervalNs / (SECONDS));
    resultTimerCreate = timer_settime(pubEventTimer, 0, &timerspec, NULL);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Failed to arm the system timer");
        timer_delete(pubEventTimer);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Created Publish Callback with interval %f", interval_ms);
    return UA_STATUSCODE_GOOD;
}

//------------------------------------------------------------------------------
/**
\brief Modify interval of callback - cyclic repetition(Interrupt based publisher)

The function modifies the callback interval the triggers the collection and
publish of NetworkMessages and the contained DataSetMessages

\param[in]     server          Server instance
\param[in]     interval_ms     Publisher interval
\param[in]     callbackId      Callback identifier

\return The function returns a status code
*/
//------------------------------------------------------------------------------
UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms) {
    pubIntervalNs                 = (UA_Int64)(interval_ms * MILLI_SECONDS);
    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec   = (long int) (pubIntervalNs / (SECONDS));
    timerspec.it_interval.tv_nsec  = (long int) (pubIntervalNs % (SECONDS));
    timerspec.it_value.tv_sec      = (long int) (pubIntervalNs / (SECONDS));
    timerspec.it_value.tv_nsec     = (long int) (pubIntervalNs % (SECONDS));
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

//------------------------------------------------------------------------------
/**
\brief Remove the callback added for cyclic repetition(Thread based publisher)

The function removes the callback

\param[in]     pubCallback          Callback
\param[in]     callbackId           Callback identifier
*/
//------------------------------------------------------------------------------
void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    printf("Timer event deleted\n");
    timer_delete(pubEventTimer);
    pubCallback = NULL;
}
#endif


#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
//------------------------------------------------------------------------------
/**
\brief Publisher measurements

The function stores the timestamp with data for latency and Jitter measurements

\param[in]     start_time          Publisher start time
\param[in]     counterValue        Counter value from information model
*/
//------------------------------------------------------------------------------
static void
updateMeasurementsPublisher(struct timespec start_time,
                            UA_UInt64 counterValue) {
    publishTimestamp[measurementsPublisher]        = start_time;
    publishCounterValue[measurementsPublisher]     = counterValue;
    measurementsPublisher++;
}

//------------------------------------------------------------------------------
/**
\brief Nanosecond field conversion

The function check the timespec to avoid overflow in nano second field and
increment the seconds field

\param[in]     timeSpecValue       Current time
*/
//------------------------------------------------------------------------------
static void nanoSecondFieldConversion(struct timespec *timeSpecValue) {
    /* Check if ns field is greater than '1 ns less than 1sec' */
    while(timeSpecValue->tv_nsec > (SECONDS -1)) {
        /* Move to next second and remove it from ns field */
        timeSpecValue->tv_sec  += ONE_SECOND;
        timeSpecValue->tv_nsec -= SECONDS;
    }
}

//------------------------------------------------------------------------------
/**
\brief Pubsub connection

Create a new ConnectionConfig. The addPubSubConnection function takes the config
and create a new connection. The Connection identifier is copied to the NodeId
parameter

\param[in]       server              Server instance
*/
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
/**
\brief Pubsub dataset handling

Create a new dataset

\param[in]       server              Server instance
*/
//------------------------------------------------------------------------------
static void
addPublishedDataSet(UA_Server *server) {
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

//------------------------------------------------------------------------------
/**
\brief Dataset field handling

The DataSetField (DSF) is part of the PDS and describes exactly one published
field

\param[in]       server              Server instance
*/
//------------------------------------------------------------------------------
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId pubCounterFieldIdent;
    UA_DataSetFieldConfig pubCounterField;
    memset(&pubCounterField, 0, sizeof(UA_DataSetFieldConfig));
    pubCounterField.dataSetFieldType                                   = UA_PUBSUB_DATASETFIELD_VARIABLE;
    pubCounterField.field.variable.fieldNameAlias                      = UA_STRING("Pub Counter Dataset");
    pubCounterField.field.variable.promotedField                       = UA_FALSE;
    pubCounterField.field.variable.publishParameters.publishedVariable = pubNodeID;
    pubCounterField.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent, &pubCounterField, &pubCounterFieldIdent);

    UA_NodeId subCounterFieldIdent;
    UA_DataSetFieldConfig subCounterField;
    memset(&subCounterField, 0, sizeof(UA_DataSetFieldConfig));
    subCounterField.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    subCounterField.field.variable.fieldNameAlias                      = UA_STRING("Sub Counter Dataset");
    subCounterField.field.variable.promotedField                       = UA_FALSE;
    subCounterField.field.variable.publishParameters.publishedVariable = subNodeID;
    subCounterField.field.variable.publishParameters.attributeId       = UA_ATTRIBUTEID_VALUE;
    /* Revert this line to add second node in the information model. As of now, dataSetFieldIdent1 is enough to send the counter data
     * on the network. */
    UA_Server_addDataSetField(server, publishedDataSetIdent, &subCounterField, &subCounterFieldIdent);
}

//------------------------------------------------------------------------------
/**
\brief Writer group handling

The WriterGroup (WG) is part of the connection and contains the primary
configuration parameters for the message creation.

\param[in]       server              Server instance
*/
//------------------------------------------------------------------------------
static void
addWriterGroup(UA_Server *server) {
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name               = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUB_INTERVAL;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

//------------------------------------------------------------------------------
/**
\brief DataSetWriter handling

A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is linked
to exactly one PDS and contains additional informations for the message
generation.

\param[in]       server              Server instance
*/
//------------------------------------------------------------------------------
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
#endif


#if defined(PUBLISHER)
//------------------------------------------------------------------------------
/**
\brief Publisher thread routine

The function is the routine used by the publisher thread. This
routine publishes the data at every given  cycle time
*/
//------------------------------------------------------------------------------
void* publisherETF(void *arg) {
    struct timespec nextnanosleeptime;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    nextnanosleeptime.tv_sec   += 0;
    nextnanosleeptime.tv_nsec  += CYCLE_TIME;
    clock_gettime(CLOCKID, &nextCycleStartTime);
    nextCycleStartTime.tv_sec  += 0;
    /*This is the lead time gap for the first tx, after the loop has been started.
     * This value can be adjusted as per the target platform
     */
    nextCycleStartTime.tv_nsec += LEADTIME;
    while(running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        pubCounterData++;
        clock_gettime(CLOCKID, &dataModificationTime);
        UA_Variant_setScalar(&pubCounter, &pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
        UA_NodeId currentNodeId = UA_NODEID_STRING(1, "PublisherCounter");
        UA_Server_writeValue(pubServer, currentNodeId, pubCounter);
        #if defined(LOOPBACK_T4)
        /* Lock the code section */
        pthread_mutex_lock(&lock);
        #endif
        pubCallback(pubServer, pubData);
        #if defined(LOOPBACK_T4)
        /* Unlock the code section */
        pthread_mutex_unlock(&lock);
        #endif
        if(measurementsPublisher < MAX_MEASUREMENTS) {
            updateMeasurementsPublisher(dataModificationTime, pubCounterData);
        }

        nextnanosleeptime.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextnanosleeptime);
        nextCycleStartTime.tv_nsec += CYCLE_TIME;
        nanoSecondFieldConversion(&nextCycleStartTime);
    }

    return (void*)NULL;
}
#endif

#if defined(SUBSCRIBER)
//------------------------------------------------------------------------------
/**
\brief Subscriber measurements

The function stores the subcriber timestamp with data for latency and Jitter
measurements
\param[in]     receive_time        Subscribed date receive time
\param[in]     counterValue        Counter value from information model
*/
//------------------------------------------------------------------------------
static void
updateMeasurementsSubscriber(struct timespec receive_time, UA_UInt64 counterValue) {
    subscribeTimestamp[measurementsSubscriber]     = receive_time;
    subscribeCounterValue[measurementsSubscriber]  = counterValue;
    measurementsSubscriber++;
}

//------------------------------------------------------------------------------
/**
\brief Subscriber thread routine

The function is the routine used by the subscriber thread. This routine
subscribes the data in continous loop with us sleep
*/
//------------------------------------------------------------------------------
void* subscriber(void *arg) {
    while(running) {
       subscribe();
    }
    return (void*)NULL;
}

//------------------------------------------------------------------------------
/**
\brief Subscriber thread routine

The function is the routine used by the subscriber thread. This routine
subscribes the data in continous loop with us sleep
*/
//------------------------------------------------------------------------------
void subscribe(void) {
    UA_ByteString buffer;
    if(UA_ByteString_allocBuffer(&buffer, BUFFER_LENGTH) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Message buffer allocation failed!");
        return;
    }

    /* Receive the message. Blocks for 5ms */
    UA_StatusCode retval =
        connection->channel->receive(connection->channel, &buffer, NULL, FIVE_MILLI_SECOND);
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

    clock_gettime(CLOCKID, &dataReceiveTime);
#if !defined(LOOPBACK_T1) && !defined(LOOPBACK_T4) && defined(SUBSCRIBER) 
    UA_UInt64 value = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[1].value.data;
#endif

#if defined(LOOPBACK_T4)
    UA_UInt64 value = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[1].value.data;
    UA_Variant_setScalar(&subCounter, &value, &UA_TYPES[UA_TYPES_UINT64]);
    UA_NodeId currentNodeId1 = UA_NODEID_STRING(1, "SubscriberCounter");
    /* Lock the code section */
    pthread_mutex_lock(&lock);
    UA_Server_writeValue(pubServer, currentNodeId1, subCounter);
    pthread_mutex_unlock(&lock);
    /* Unlock the code section */
#elif defined(LOOPBACK_T1)
    UA_UInt64 value = *(UA_UInt64 *)dsm->data.keyFrameData.dataSetFields[0].value.data;
#endif

    if (value > 0 && (measurementsSubscriber < MAX_MEASUREMENTS)) {
        updateMeasurementsSubscriber(dataReceiveTime, value);
    }
cleanup:
    UA_NetworkMessage_deleteMembers(&networkMessage);
}
#endif

//------------------------------------------------------------------------------
/**
\brief Creation of nodes

The addServerNodes function is used to create new nodes into the information model

\param[in]     server              Server instance
*/
//------------------------------------------------------------------------------
static void addServerNodes(UA_Server *server) {
    UA_NodeId robotId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Robot Simulation");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Robot Simulation"), UA_NODEID_NULL,
                            oAttr, NULL, &robotId);

    UA_VariableAttributes p4Attr = UA_VariableAttributes_default;
    UA_UInt64 axis4position = 0;
    UA_Variant_setScalar(&p4Attr.value, &axis4position, &UA_TYPES[UA_TYPES_UINT64]);
    p4Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "PublisherCounter");
    UA_Server_addVariableNode(server, newNodeId, robotId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Publisher Counter"),
                              UA_NODEID_NULL, p4Attr, NULL, &pubNodeID);
    UA_VariableAttributes p5Attr = UA_VariableAttributes_default;
    UA_UInt64 axis5position = 0;
    UA_Variant_setScalar(&p5Attr.value, &axis5position, &UA_TYPES[UA_TYPES_UINT64]);
    p5Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Subscriber Counter");
    newNodeId = UA_NODEID_STRING(1, "SubscriberCounter");
    UA_Server_addVariableNode(server, newNodeId, robotId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Subscriber Counter"),
                              UA_NODEID_NULL, p5Attr, NULL, &subNodeID);
}

//------------------------------------------------------------------------------
/**
\brief Deletion of nodes

The removeServerNodes function is used to remove the nodes from the information
model

\param[in]     server              Server instance
*/
//------------------------------------------------------------------------------
static void removeServerNodes(UA_Server *server) {
    /* Delete the Publisher Counter Node*/
    UA_Server_deleteNode(server, pubNodeID, UA_TRUE);
    UA_NodeId_deleteMembers(&pubNodeID);

    /* Delete the Subscriber Counter Node*/
    UA_Server_deleteNode(server, subNodeID, UA_TRUE);
    UA_NodeId_deleteMembers(&subNodeID);
}

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
//------------------------------------------------------------------------------
/**
\brief  Set priority for the main thread

The function schedules a priority for the self thread(Main thread) and set
core affinity

\return The function returns a status code
*/
//------------------------------------------------------------------------------
static int setSelfPrio(void) {
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

    returnValue = pthread_getaffinity_np(pubThreadID, sizeof(cpu_set_t), &cpusetPub);
    if(returnValue != 0) {
        printf("Get affinity fail\n");
    }

    if(CPU_ISSET(CORE_TWO, &cpusetPub)) {
            printf("CPU %d\n", CORE_TWO);
    }

    printf("pthread_setschedparam: publisher thread priority is %d \n", schedParamPublisher.sched_priority);

    return 0;
}
#endif

//------------------------------------------------------------------------------
/**
\brief Main function

Intialize a server with pubsub connection and creates two independent threads
(publisher and subscriber)
*/
//------------------------------------------------------------------------------
int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    UA_Server*       server              = UA_Server_new();
    UA_ServerConfig* config              = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
    fpPublisher                          = fopen(filePublishedData, "a");
#endif

#if defined(SUBSCRIBER)
    fpSubscriber                         = fopen(fileSubscribedData, "a");
#endif

#if defined(PUBLISHER) && defined(SUBSCRIBER)
/* Details about the connection configuration and handling are located in the pubsub connection tutorial */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
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

#if defined(LOOPBACK_T4)
    if(pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n Mutex initialization has failed\n");
        return 1;
    }
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER) || defined(PUB_SYSTEM_INTERRUPT)
    /* Server is the new OPCUA model which has both publisher and subscriber configuration
     * add axis node and OPCUA pubsub client server counter nodes */
    addServerNodes(server);
#endif

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
    addPubSubConnection(server);
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
    connectionConfig.name = UA_STRING("UDP-UADP Connection 2");
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.enabled = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(PUBSUB_IP_ADDRESS), UA_STRING(SUBSCRIBER_MULTICAST_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
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
    returnValue = setSelfPrio();
    returnValue = pthread_create(&pubThreadID, NULL, &publisherETF, NULL);
    if(returnValue != 0) {
        printf("publisherETF: cannot create thread\n");
        exit(1);
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
#endif

#if defined(PUBLISHER) || defined(SUBSCRIBER) || defined(PUB_SYSTEM_INTERRUPT)
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

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
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
#if defined(SUBSCRIBER)
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

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT) || defined(SUBSCRIBER)
    removeServerNodes(server);
    UA_Server_delete(server);
#endif

#if defined(PUBLISHER) || defined(PUB_SYSTEM_INTERRUPT)
fclose(fpPublisher);
#endif

#if defined(SUBSCRIBER)
fclose(fpSubscriber);
#endif

    return (int)retval;
}
