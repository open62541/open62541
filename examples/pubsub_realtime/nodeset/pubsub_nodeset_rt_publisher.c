/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-nodeset-tutorial:
 *
 * Publisher Realtime example using custom nodes
 * ---------------------------------------------
 *
 * The purpose of this example file is to use the custom nodes of the XML
 * file(pubDataModel.xml) for publisher. This Publisher example uses the two
 * custom nodes (PublisherCounterVariable and Pressure) created using the XML
 * file(pubDataModel.xml) for publishing the packet. The pubDataModel.csv will
 * contain the nodeids of custom nodes(object and variables) and the nodeids of
 * the custom nodes are harcoded inside the addDataSetField API. This example
 * uses two threads namely the Publisher and UserApplication. The Publisher
 * thread is used to publish data at every cycle. The UserApplication thread
 * serves the functionality of the Control loop, which increments the
 * counterdata to be published by the Publisher and also writes the published
 * data in a csv along with transmission timestamp.
 *
 * Run steps of the Publisher application as mentioned below:
 *
 * ``./bin/examples/pubsub_nodeset_rt_publisher -i <iface>``
 *
 * For more information run ``./bin/examples/pubsub_nodeset_rt_publisher -h``. */

#define _GNU_SOURCE

/* For thread operations */
#include <pthread.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/pubsub_ethernet.h>

#include "ua_pubsub.h"
#include "open62541/namespace_example_publisher_generated.h"

/* to find load of each thread
 * ps -L -o pid,pri,%cpu -C pubsub_nodeset_rt_publisher */

/* Configurable Parameters */
/* Cycle time in milliseconds */
#define             DEFAULT_CYCLE_TIME                    0.25
/* Qbv offset */
#define             QBV_OFFSET                            25 * 1000
#define             DEFAULT_SOCKET_PRIORITY               3
#define             PUBLISHER_ID                          2234
#define             WRITER_GROUP_ID                       101
#define             DATA_SET_WRITER_ID                    62541
#define             PUBLISHING_MAC_ADDRESS                "opc.eth://01-00-5E-7F-00-01:8.3"
#define             PORT_NUMBER                           62541

/* Non-Configurable Parameters */
/* Milli sec and sec conversion to nano sec */
#define             MILLI_SECONDS                         1000 * 1000
#define             SECONDS                               1000 * 1000 * 1000
#define             SECONDS_SLEEP                         5
#define             DEFAULT_PUB_SCHED_PRIORITY            78
#define             DEFAULT_PUBSUB_CORE                   2
#define             DEFAULT_USER_APP_CORE                 3
#define             MAX_MEASUREMENTS                      30000000
#define             SECONDS_INCREMENT                     1
#define             CLOCKID                               CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             DEFAULT_USERAPPLICATION_SCHED_PRIORITY 75

/* Below mentioned parameters can be provided as input using command line arguments
 * If user did not provide the below mentioned parameters as input through command line
 * argument then default value will be used */
static UA_Double  cycleTimeMsec   = DEFAULT_CYCLE_TIME;
static UA_Boolean consolePrint    = UA_FALSE;
static UA_Int32   socketPriority  = DEFAULT_SOCKET_PRIORITY;
static UA_Int32   pubPriority     = DEFAULT_PUB_SCHED_PRIORITY;
static UA_Int32   userAppPriority = DEFAULT_USERAPPLICATION_SCHED_PRIORITY;
static UA_Int32   pubSubCore      = DEFAULT_PUBSUB_CORE;
static UA_Int32   userAppCore     = DEFAULT_USER_APP_CORE;
static UA_Boolean useSoTxtime     = UA_TRUE;

/* User application Pub will wakeup at the 30% of cycle time and handles the */
/* user data write in Information model */
/* First 30% is left for subscriber for future use*/
static UA_Double  userAppWakeupPercentage = 0.3;
/* Publisher will sleep for 60% of cycle time and then prepares the */
/* transmission packet within 40% */
/* after some prototyping and analyzing it */
static UA_Double  pubWakeupPercentage     = 0.6;
static UA_Boolean fileWrite = UA_FALSE;

/* If the Hardcoded publisher MAC addresses need to be changed,
 * change PUBLISHING_MAC_ADDRESS
 */

/* Set server running as true */
UA_Boolean          running = UA_TRUE;
UA_UInt16           nsIdx = 0;
/* Variables corresponding to PubSub connection creation,
 * published data set and writer group */
UA_NodeId           connectionIdent;
UA_NodeId           publishedDataSetIdent;
UA_NodeId           writerGroupIdent;
/* Variables for counter data handling in address space */
UA_UInt64           *pubCounterData;
UA_DataValue        *pubDataValueRT;
/* Variables for counter data handling in address space */
UA_Double           *pressureData;
UA_DataValue        *pressureValueRT;

/* File to store the data and timestamps for different traffic */
FILE               *fpPublisher;
char               *fileName      = "publisher_T1.csv";
/* Array to store published counter data */
UA_UInt64           publishCounterValue[MAX_MEASUREMENTS];
UA_Double           pressureValues[MAX_MEASUREMENTS];
size_t              measurementsPublisher  = 0;
/* Array to store timestamp */
struct timespec     publishTimestamp[MAX_MEASUREMENTS];

/* Thread for publisher */
pthread_t           pubthreadID;
struct timespec     dataModificationTime;

/* Thread for user application*/
pthread_t           userApplicationThreadID;

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

/* Publisher thread routine for ETF */
void *publisherETF(void *arg);
/* User application thread routine */
void *userApplicationPub(void *arg);
/* To create multi-threads */
static pthread_t threadCreation(UA_Int32 threadPriority, UA_Int32 coreAffinity, void *(*thread) (void *),
                                char *applicationName, void *serverConfig);

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
    /* Create the publisher thread with the required priority and core affinity */
    char threadNamePub[10] = "Publisher";
    pubthreadID            = threadCreation(pubPriority, pubSubCore, publisherETF, threadNamePub, threadArguments);
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
removePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId){
    if(callbackId && (pthread_join((pthread_t)callbackId, NULL) != 0))
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Pthread Join Failed thread: %lu\n", (long unsigned)callbackId);
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
    connectionConfig.name                                   = UA_STRING("Publisher Connection");
    connectionConfig.enabled                                = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrl          = *networkAddressUrlPub;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType                        = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16                     = PUBLISHER_ID;
    /* Connection options are given as Key/Value Pairs - Sockprio and Txtime */
    UA_KeyValuePair connectionOptions[2];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sockpriority");
    UA_UInt32 sockPriority   = (UA_UInt32)socketPriority;
    UA_Variant_setScalar(&connectionOptions[0].value, &sockPriority, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "enablesotxtime");
    UA_Boolean enableTxTime  = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[1].value, &enableTxTime, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionConfig.connectionProperties     = connectionOptions;
    connectionConfig.connectionPropertiesSize = 2;
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
/* This example only uses two addDataSetField which uses the custom nodes of the XML file
 * (pubDataModel.xml) */
static void
_addDataSetField(UA_Server *server) {
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dsfConfig;
    memset(&dsfConfig, 0, sizeof(UA_DataSetFieldConfig));
    pubCounterData = UA_UInt64_new();
    *pubCounterData = 0;
    pubDataValueRT = UA_DataValue_new();
    UA_Variant_setScalar(&pubDataValueRT->value, pubCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    pubDataValueRT->hasValue = UA_TRUE;
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &pubDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    /* If user need to change the nodeid of the custom nodes in the application then it must be
     * changed inside the xml and .csv file inside examples\pubsub_realtime\nodeset\*/
    /* The nodeid of the Custom node PublisherCounterVariable is 2005 which is used below */
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(nsIdx, 2005), valueBackend);
    /* setup RT DataSetField config */
    dsfConfig.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
    dsfConfig.field.variable.publishParameters.publishedVariable =  UA_NODEID_NUMERIC(nsIdx, 2005);
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig, &dataSetFieldIdent);
    UA_NodeId dataSetFieldIdent1;
    UA_DataSetFieldConfig dsfConfig1;
    memset(&dsfConfig1, 0, sizeof(UA_DataSetFieldConfig));
    pressureData = UA_Double_new();
    *pressureData = 17.07;
    pressureValueRT = UA_DataValue_new();
    UA_Variant_setScalar(&pressureValueRT->value, pressureData, &UA_TYPES[UA_TYPES_DOUBLE]);
    pressureValueRT->hasValue = UA_TRUE;
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend1;
    valueBackend1.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend1.backend.external.value = &pressureValueRT;
    valueBackend1.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend1.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    /* The nodeid of the Custom node Pressure is 2006 which is used below */
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(nsIdx, 2006), valueBackend1);
    /* setup RT DataSetField config */
    dsfConfig1.field.variable.rtValueSource.rtInformationModelNode = UA_TRUE;
    dsfConfig1.field.variable.publishParameters.publishedVariable =  UA_NODEID_NUMERIC(nsIdx, 2006);
    UA_Server_addDataSetField(server, publishedDataSetIdent, &dsfConfig1, &dataSetFieldIdent1);

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
    writerGroupConfig.publishingInterval = cycleTimeMsec;
    writerGroupConfig.enabled            = UA_FALSE;
    writerGroupConfig.encodingMimeType   = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.writerGroupId      = WRITER_GROUP_ID;
    writerGroupConfig.rtLevel            = UA_PUBSUB_RT_FIXED_SIZE;
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
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional information for the
 * message generation.
 */
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
                            UA_UInt64 counterValue, UA_Double pressureValue) {
    publishTimestamp[measurementsPublisher]        = start_time;
    publishCounterValue[measurementsPublisher]     = counterValue;
    pressureValues[measurementsPublisher]          = pressureValue;
    measurementsPublisher++;
}

/**
 * **Publisher thread routine**
 *
 * The Publisher thread sleeps for 60% of the cycletime (250us) and prepares the tranmission packet within 40% of
 * cycletime. The data published by this thread in one cycle is subscribed by the subscriber thread of pubsub_nodeset_rt_subscriber in the
 * next cycle (two cycle timing model).
 *
 * The publisherETF function is the routine used by the publisher thread.
 */
void *publisherETF(void *arg) {
    struct timespec   nextnanosleeptime;
    UA_ServerCallback pubCallback;
    UA_Server*        server;
    UA_WriterGroup*   currentWriterGroup;
    UA_UInt64         interval_ns;
    UA_UInt64         transmission_time;

    /* Initialise value for nextnanosleeptime timespec */
    nextnanosleeptime.tv_nsec                      = 0;

    threadArg *threadArgumentsPublisher = (threadArg *)arg;
    server                              = threadArgumentsPublisher->server;
    pubCallback                         = threadArgumentsPublisher->callback;
    currentWriterGroup                  = (UA_WriterGroup *)threadArgumentsPublisher->data;
    interval_ns                         = (UA_UInt64)(threadArgumentsPublisher->interval_ms * MILLI_SECONDS);

    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptime);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptime.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptime.tv_nsec                      = (__syscall_slong_t)(cycleTimeMsec * pubWakeupPercentage * MILLI_SECONDS);
    nanoSecondFieldConversion(&nextnanosleeptime);

    /* Define Ethernet ETF transport settings */
    UA_EthernetWriterGroupTransportDataType ethernettransportSettings;
    memset(&ethernettransportSettings, 0, sizeof(UA_EthernetWriterGroupTransportDataType));
    ethernettransportSettings.transmission_time = 0;

    /* Encapsulate ETF config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    /* TODO: transportSettings encoding and type to be defined */
    transportSettings.content.decoded.data       = &ethernettransportSettings;
    currentWriterGroup->config.transportSettings = transportSettings;
    UA_UInt64 roundOffCycleTime                  = (UA_UInt64)((cycleTimeMsec * MILLI_SECONDS) - (cycleTimeMsec * pubWakeupPercentage * MILLI_SECONDS));

    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        transmission_time                           = ((UA_UInt64)nextnanosleeptime.tv_sec * SECONDS + (UA_UInt64)nextnanosleeptime.tv_nsec) + roundOffCycleTime + QBV_OFFSET;
        ethernettransportSettings.transmission_time = transmission_time;
        pubCallback(server, currentWriterGroup);
        nextnanosleeptime.tv_nsec                   += (__syscall_slong_t)interval_ns;
        nanoSecondFieldConversion(&nextnanosleeptime);
    }

    UA_free(threadArgumentsPublisher);

    return (void*)NULL;
}

/**
 * **UserApplication thread routine**
 *
 * The userapplication thread will wakeup at 30% of cycle time and handles the userdata in the Information Model.
 * This thread is used to increment the counterdata that will be published by the Publisher thread and also writes the published data in a csv.
 */
void *userApplicationPub(void *arg) {
    struct timespec nextnanosleeptimeUserApplication;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeUserApplication);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeUserApplication.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptimeUserApplication.tv_nsec                      = (__syscall_slong_t)(cycleTimeMsec * userAppWakeupPercentage * MILLI_SECONDS);
    nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    *pubCounterData      = 0;
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeUserApplication, NULL);
        *pubCounterData      = *pubCounterData + 1;
        *pressureData        = *pressureData + 1;
        clock_gettime(CLOCKID, &dataModificationTime);
        if ((fileWrite == UA_TRUE) || (consolePrint == UA_TRUE))
            updateMeasurementsPublisher(dataModificationTime, *pubCounterData, *pressureData);
        nextnanosleeptimeUserApplication.tv_nsec += (__syscall_slong_t)(cycleTimeMsec * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);
    }

    return (void*)NULL;
}

/**
 * **Thread creation**
 *
 * The threadcreation functionality creates thread with given threadpriority, coreaffinity. The function returns the threadID of the newly
 * created thread.
 */
static pthread_t threadCreation(UA_Int32 threadPriority, UA_Int32 coreAffinity, void *(*thread) (void *), char *applicationName, void *serverConfig){

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
    CPU_SET((size_t)coreAffinity, &cpuset);
    errorSetAffinity = pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset);
    if (errorSetAffinity) {
        fprintf(stderr, "pthread_setaffinity_np: %s\n", strerror(errorSetAffinity));
        exit(1);
    }

    returnValue = pthread_create(&threadID, NULL, thread, serverConfig);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,":%s Cannot create thread\n", applicationName);
    }

    if (CPU_ISSET((size_t)coreAffinity, &cpuset)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s CPU CORE: %d\n", applicationName, coreAffinity);
    }

   return threadID;

}

/**
 * **Usage function**
 *
 * The usage function gives the list of options that can be configured in the application.
 *
 * ./bin/examples/pubsub_nodeset_rt_publisher -h gives the list of options for running the application.
 */
static void usage(char *appname)
{
    fprintf(stderr,
        "\n"
        "usage: %s [options]\n"
        "\n"
        " -i [name]     use network interface 'name'\n"
        " -C [num]      cycle time in milli seconds (default %lf)\n"
        " -p            Do you need to print the data in console output\n"
        " -s [num]      set SO_PRIORITY to 'num' (default %d)\n"
        " -P [num]      Publisher priority value (default %d)\n"
        " -U [num]      User application priority value (default %d)\n"
        " -c [num]      run on CPU for publisher'num'(default %d)\n"
        " -u [num]      run on CPU for userApplication'num'(default %d)\n"
        " -t            do not use SO_TXTIME\n"
        " -m [mac_addr] ToDO:dst MAC address\n"
        " -h            prints this message and exits\n"
        "\n",
        appname, DEFAULT_CYCLE_TIME, DEFAULT_SOCKET_PRIORITY, DEFAULT_PUB_SCHED_PRIORITY, \
        DEFAULT_USERAPPLICATION_SCHED_PRIORITY, DEFAULT_PUBSUB_CORE, DEFAULT_USER_APP_CORE);
}

/**
 * **Main Server code**
 *
 * The main function contains publisher threads running
 */
int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Int32         returnValue         = 0;
    char             *interface          = NULL;
    char             *progname;
    UA_Int32         argInputs           = -1;
    UA_StatusCode    retval              = UA_STATUSCODE_GOOD;
    UA_Server       *server              = UA_Server_new();
    UA_ServerConfig *config              = UA_Server_getConfig(server);
    pthread_t        userThreadID;
    UA_ServerConfig_setMinimal(config, PORT_NUMBER, NULL);

    /* Files namespace_example_publisher_generated.h and namespace_example_publisher_generated.c are created from
     * pubDataModel.xml in the /src_generated directory by CMake */
    /* Loading the user created variables into the information model from the generated .c and .h files */
    if(namespace_example_publisher_generated(server) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
                     "Check previous output for any error.");
    }
    else
    {
        nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    }

    UA_NetworkAddressUrlDataType networkAddressUrlPub;

    /* Process the command line arguments */
    /* For more information run ./bin/examples/pubsub_nodeset_rt_publisher -h */
    progname = strrchr(argv[0], '/');
    progname = progname ? 1 + progname : argv[0];
    while (EOF != (argInputs = getopt(argc, argv, "i:C:f:ps:P:U:c:u:tm:h:"))) {
        switch (argInputs) {
            case 'i':
                interface = optarg;
                break;
            case 'C':
                cycleTimeMsec = atof(optarg);
                break;
            case 'f':
                fileName = optarg;
                fileWrite = UA_TRUE;
                fpPublisher = fopen(fileName, "w");
                break;
            case 'p':
                consolePrint = UA_TRUE;
                break;
            case 's':
                socketPriority = atoi(optarg);
                break;
            case 'P':
                pubPriority = atoi(optarg);
                break;
            case 'U':
                userAppPriority = atoi(optarg);
                break;
            case 'c':
                pubSubCore = atoi(optarg);
                break;
            case 'u':
                userAppCore = atoi(optarg);
                break;
            case 't':
                useSoTxtime = UA_FALSE;
                break;
            case 'm':
                /*ToDo:Need to handle for mac address*/
                break;
            case 'h':
                usage(progname);
                return -1;
            case '?':
                usage(progname);
                return -1;
        }
    }

    if (cycleTimeMsec < 0.125) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%f Bad cycle time", cycleTimeMsec);
        usage(progname);
        return -1;
    }

    if (!interface) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Need a network interface to run");
        usage(progname);
        return -1;
    }

    networkAddressUrlPub.networkInterface = UA_STRING(interface);
    networkAddressUrlPub.url              = UA_STRING(PUBLISHING_MAC_ADDRESS);

    /* It is possible to use multiple PubSubTransportLayers on runtime.
     * The correct factory is selected on runtime by the standard defined
     * PubSub TransportProfileUri's. */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());

    addPubSubConnection(server, &networkAddressUrlPub);
    addPublishedDataSet(server);
    _addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);
    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);

    serverConfigStruct *serverConfig;
    serverConfig            = (serverConfigStruct*)UA_malloc(sizeof(serverConfigStruct));
    serverConfig->ServerRun = server;
    char threadNameUserApplication[22] = "UserApplicationPub";
    userThreadID                       = threadCreation(userAppPriority, userAppCore, userApplicationPub, threadNameUserApplication, serverConfig);
    retval |= UA_Server_run(server, &running);
    returnValue = pthread_join(pubthreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for publisher thread:%d\n", returnValue);
    }
    returnValue = pthread_join(userThreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for User thread:%d\n", returnValue);
    }

    if (fileWrite == UA_TRUE) {
        /* Write the published data in a file */
        size_t pubLoopVariable               = 0;
        for (pubLoopVariable = 0; pubLoopVariable < measurementsPublisher;
             pubLoopVariable++) {
            fprintf(fpPublisher, "%lu,%ld.%09ld,%lf\n",
                    (long unsigned)publishCounterValue[pubLoopVariable],
                    publishTimestamp[pubLoopVariable].tv_sec,
                    publishTimestamp[pubLoopVariable].tv_nsec,
                    pressureValues[pubLoopVariable]);
        }
        fclose(fpPublisher);
    }
    if (consolePrint == UA_TRUE) {
        size_t pubLoopVariable               = 0;
        for (pubLoopVariable = 0; pubLoopVariable < measurementsPublisher;
             pubLoopVariable++) {
             printf("%lu,%ld.%09ld,%lf\n",
                    (long unsigned)publishCounterValue[pubLoopVariable],
                    publishTimestamp[pubLoopVariable].tv_sec,
                    publishTimestamp[pubLoopVariable].tv_nsec,
                    pressureValues[pubLoopVariable]);
        }
    }

    UA_Server_delete(server);
    UA_free(serverConfig);
    UA_free(pubCounterData);
    /* Free external data source */
    UA_free(pubDataValueRT);
    UA_free(pressureData);
    /* Free external data source */
    UA_free(pressureValueRT);
    return (int)retval;
}
