/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-nodeset-subscriber-tutorial:
 *
 * Subscriber Realtime example using custom nodes
 * ----------------------------------------------
 *
 * The purpose of this example file is to use the custom nodes of the XML
 * file(subDataModel.xml) for subscriber. This Subscriber example uses the two
 * custom nodes (SubscriberCounterVariable and Pressure) created using the XML
 * file(subDataModel.xml) for subscribing the packet. The subDataModel.csv will
 * contain the nodeids of custom nodes(object and variables) and the nodeids of
 * the custom nodes are harcoded inside the addSubscribedVariables API
 *
 * This example uses two threads namely the Subscriber and UserApplication. The
 * Subscriber thread is used to subscribe to data at every cycle. The
 * UserApplication thread serves the functionality of the Control loop, which
 * reads the Information Model of the Subscriber and the new counterdata will be
 * written in the csv along with received timestamp.
 *
 * Run steps of the Subscriber application as mentioned below:
 *
 * ``./bin/examples/pubsub_nodeset_rt_subscriber -i <iface>``
 *
 * For more information run ``./bin/examples/pubsub_nodeset_rt_subscriber -h``. */

#define _GNU_SOURCE

/* For thread operations */
#include <pthread.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/pubsub_ethernet.h>

#include "ua_pubsub.h"
#include "open62541/namespace_example_subscriber_generated.h"

UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;
UA_DataSetReaderConfig readerConfig;

/* to find load of each thread
 * ps -L -o pid,pri,%cpu -C pubsub_nodeset_rt_subscriber*/

/* Configurable Parameters */
/* Cycle time in milliseconds */
#define             DEFAULT_CYCLE_TIME                    0.25
/* Qbv offset */
#define             QBV_OFFSET                            25 * 1000
#define             DEFAULT_SOCKET_PRIORITY               3
#define             PUBLISHER_ID_SUB                      2234
#define             WRITER_GROUP_ID_SUB                   101
#define             DATA_SET_WRITER_ID_SUB                62541
#define             SUBSCRIBING_MAC_ADDRESS               "opc.eth://01-00-5E-7F-00-01:8.3"
#define             PORT_NUMBER                           62541

/* Non-Configurable Parameters */
/* Milli sec and sec conversion to nano sec */
#define             MILLI_SECONDS                         1000 * 1000
#define             SECONDS                               1000 * 1000 * 1000
#define             SECONDS_SLEEP                         5
#define             DEFAULT_SUB_SCHED_PRIORITY            81
#define             MAX_MEASUREMENTS                      30000000
#define             DEFAULT_PUBSUB_CORE                   2
#define             DEFAULT_USER_APP_CORE                 3
#define             SECONDS_INCREMENT                     1
#define             CLOCKID                               CLOCK_TAI
#define             ETH_TRANSPORT_PROFILE                 "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define             DEFAULT_USERAPPLICATION_SCHED_PRIORITY 75

/* Below mentioned parameters can be provided as input using command line arguments
 * If user did not provide the below mentioned parameters as input through command line
 * argument then default value will be used */
static UA_Double  cycleTimeMsec   = DEFAULT_CYCLE_TIME;
static UA_Boolean consolePrint    = UA_FALSE;
static UA_Int32   subPriority     = DEFAULT_SUB_SCHED_PRIORITY;
static UA_Int32   userAppPriority = DEFAULT_USERAPPLICATION_SCHED_PRIORITY;
static UA_Int32   pubSubCore      = DEFAULT_PUBSUB_CORE;
static UA_Int32   userAppCore     = DEFAULT_USER_APP_CORE;
/* User application Pub will wakeup at the 30% of cycle time and handles the */
/* user data write in Information model */
/* After 60% is left for publisher */
static UA_Double  userAppWakeupPercentage = 0.3;
/* Subscriber will wake up at the start of cycle time and then receives the packet */
static UA_Double  subWakeupPercentage     = 0;
static UA_Boolean fileWrite = UA_FALSE;

/* Set server running as true */
UA_Boolean          running                = UA_TRUE;
UA_UInt16           nsIdx = 0;

UA_UInt64           *subCounterData;
UA_DataValue        *subDataValueRT;
UA_Double           *pressureData;
UA_DataValue        *pressureValueRT;

/* File to store the data and timestamps for different traffic */
FILE               *fpSubscriber;
char               *fileName     = "subscriber_T4.csv";
/* Array to store subscribed counter data */
UA_UInt64           subscribeCounterValue[MAX_MEASUREMENTS];
UA_Double           pressureValues[MAX_MEASUREMENTS];
size_t              measurementsSubscriber = 0;
/* Array to store timestamp */
struct timespec     subscribeTimestamp[MAX_MEASUREMENTS];

/* Thread for subscriber */
pthread_t           subthreadID;
/* Variable for PubSub connection creation */
UA_NodeId           connectionIdentSubscriber;
struct timespec     dataReceiveTime;

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

/* Subscriber thread routine */
void *subscriber(void *arg);
/* User application thread routine */
void *userApplicationSub(void *arg);
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
addPubSubApplicationCallback(UA_Server *server, UA_NodeId identifier, UA_ServerCallback callback,
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
    /* Create the subscriber thread with the required priority and core affinity */
    char threadNameSub[11] = "Subscriber";
    subthreadID            = threadCreation(subPriority, pubSubCore, subscriber, threadNameSub, threadArguments);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
changePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId,
                                UA_Double interval_ms, UA_DateTime *baseTime, UA_TimerPolicy timerPolicy) {
    /* Callback interval need not be modified as it is thread based implementation.
     * The thread uses nanosleep for calculating cycle time and modification in
     * nanosleep value changes cycle time */
    return UA_STATUSCODE_GOOD;
}

/* Remove the callback added for cyclic repetition */
static void
removePubSubApplicationCallback(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId) {
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
 * **Subscriber Connection Creation**
 *
 * Create Subscriber connection for the Subscriber thread
 */
static void
addPubSubConnectionSubscriber(UA_Server *server, UA_NetworkAddressUrlDataType *networkAddressUrlSubscriber){
    UA_StatusCode    retval                                 = UA_STATUSCODE_GOOD;
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name                                   = UA_STRING("Subscriber Connection");
    connectionConfig.enabled                                = UA_TRUE;
    UA_NetworkAddressUrlDataType networkAddressUrlsubscribe = *networkAddressUrlSubscriber;
    connectionConfig.transportProfileUri                    = UA_STRING(ETH_TRANSPORT_PROFILE);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrlsubscribe, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType                        = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32                     = UA_UInt32_random();
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdentSubscriber);
    if (retval == UA_STATUSCODE_GOOD)
         UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"The PubSub Connection was created successfully!");
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. */
/* Add ReaderGroup to the created connection */
static void
addReaderGroup(UA_Server *server) {
    if (server == NULL) {
        return;
    }

    UA_ReaderGroupConfig     readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name   = UA_STRING("ReaderGroup1");
    readerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    readerGroupConfig.pubsubManagerCallback.addCustomCallback = addPubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.changeCustomCallback = changePubSubApplicationCallback;
    readerGroupConfig.pubsubManagerCallback.removeCustomCallback = removePubSubApplicationCallback;
    UA_Server_addReaderGroup(server, connectionIdentSubscriber, &readerGroupConfig,
                             &readerGroupIdentifier);
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type
 * Add SubscriberCounter variable to the DataSetReader */
static void addSubscribedVariables (UA_Server *server) {
    if (server == NULL) {
        return;
    }

    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable*)
        UA_calloc(2, sizeof(UA_FieldTargetVariable));

    subCounterData = UA_UInt64_new();
    *subCounterData = 0;
    subDataValueRT = UA_DataValue_new();
    UA_Variant_setScalar(&subDataValueRT->value, subCounterData, &UA_TYPES[UA_TYPES_UINT64]);
    subDataValueRT->hasValue = UA_TRUE;
    /* Set the value backend of the above create node to 'external value source' */
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &subDataValueRT;
    valueBackend.backend.external.callback.userWrite = externalDataWriteCallback;
    valueBackend.backend.external.callback.notificationRead = externalDataReadNotificationCallback;
    /* If user need to change the nodeid of the custom nodes in the application then it must be
     * changed inside the xml and .csv file inside examples\pubsub_realtime\nodeset\*/
     /* The nodeid of the Custom node SubscriberCounterVariable is 2005 which is used below */
    UA_Server_setVariableNode_valueBackend(server, UA_NODEID_NUMERIC(nsIdx, 2005), valueBackend);
    UA_FieldTargetDataType_init(&targetVars[0].targetVariable);
    targetVars[0].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[0].targetVariable.targetNodeId = UA_NODEID_NUMERIC(nsIdx, 2005);

    pressureData = UA_Double_new();
    *pressureData = 0;
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
    UA_FieldTargetDataType_init(&targetVars[1].targetVariable);
    targetVars[1].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
    targetVars[1].targetVariable.targetNodeId = UA_NODEID_NUMERIC(nsIdx, 2006);

    /* Set the subscribed data to TargetVariable type */
    readerConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables = targetVars;
    readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 2;
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
/* Add DataSetReader to the ReaderGroup */
static void
addDataSetReader(UA_Server *server) {
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
    /* FilltestMetadata function in subscriber implementation */
    UA_DataSetMetaDataType_init(pMetaData);
    pMetaData->name                   = UA_STRING ("DataSet Test");
    /* Static definition of number of fields size to 1 to create one
       targetVariable */
    pMetaData->fieldsSize             =  2;
    pMetaData->fields                 = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                                                                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* Unsigned Integer DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_UINT64].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_UINT64;
    pMetaData->fields[0].valueRank   = -1; /* scalar */

    /* Double DataType */
    UA_FieldMetaData_init (&pMetaData->fields[1]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DOUBLE].typeId,
                    &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_DOUBLE;
    pMetaData->fields[1].valueRank   = -1;  /* scalar */

    /* Setup Target Variables in DSR config */
    addSubscribedVariables(server);

    /* Setting up Meta data configuration in DataSetReader */
    UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                               &readerIdentifier);

    UA_free(readerConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables);
    UA_free(readerConfig.dataSetMetaData.fields);
    UA_UadpDataSetReaderMessageDataType_delete(dataSetReaderMessage);
}

/**
 * **Subscribed data handling**
 *
 * The subscribed data is updated in the array using this function Subscribed data handling**
 */
static void
updateMeasurementsSubscriber(struct timespec receive_time, UA_UInt64 counterValue, UA_Double pressureValue) {
    subscribeTimestamp[measurementsSubscriber]     = receive_time;
    subscribeCounterValue[measurementsSubscriber]  = counterValue;
    pressureValues[measurementsSubscriber]         = pressureValue;
    measurementsSubscriber++;
}

/**
 * **Subscriber thread routine**
 *
 * Subscriber thread will wakeup during the start of cycle at 250us interval and check if the packets are received.
 * The subscriber function is the routine used by the subscriber thread.
 */
void *subscriber(void *arg) {
    UA_Server*        server;
    UA_ReaderGroup*   currentReaderGroup;
    UA_ServerCallback subCallback;
    struct timespec   nextnanosleeptimeSub;

    threadArg *threadArgumentsSubscriber = (threadArg *)arg;
    server                               = threadArgumentsSubscriber->server;
    subCallback                          = threadArgumentsSubscriber->callback;
    currentReaderGroup                   = (UA_ReaderGroup *)threadArgumentsSubscriber->data;

    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeSub);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeSub.tv_sec         += SECONDS_SLEEP;
    nextnanosleeptimeSub.tv_nsec         = (__syscall_slong_t)(cycleTimeMsec * subWakeupPercentage * MILLI_SECONDS);
    nanoSecondFieldConversion(&nextnanosleeptimeSub);
    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSub, NULL);
        /* Read subscribed data from the SubscriberCounter variable */
        subCallback(server, currentReaderGroup);
        nextnanosleeptimeSub.tv_nsec += (__syscall_slong_t)(cycleTimeMsec * MILLI_SECONDS);
        nanoSecondFieldConversion(&nextnanosleeptimeSub);
    }

    UA_free(threadArgumentsSubscriber);

    return (void*)NULL;
}

/**
 * **UserApplication thread routine**
 *
 * The userapplication thread will wakeup at 30% of cycle time and handles the userdata in the Information Model.
 * This thread is used to write the counterdata that is subscribed by the Subscriber thread in a csv.
 */
void *userApplicationSub(void *arg) {
    struct timespec nextnanosleeptimeUserApplication;
    /* Get current time and compute the next nanosleeptime */
    clock_gettime(CLOCKID, &nextnanosleeptimeUserApplication);
    /* Variable to nano Sleep until 1ms before a 1 second boundary */
    nextnanosleeptimeUserApplication.tv_sec                      += SECONDS_SLEEP;
    nextnanosleeptimeUserApplication.tv_nsec                      = (__syscall_slong_t)(cycleTimeMsec * userAppWakeupPercentage * MILLI_SECONDS);
    nanoSecondFieldConversion(&nextnanosleeptimeUserApplication);

    while (running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeUserApplication, NULL);
        clock_gettime(CLOCKID, &dataReceiveTime);
        if ((fileWrite == UA_TRUE) || (consolePrint == UA_TRUE)) {
            if (*subCounterData > 0)
                updateMeasurementsSubscriber(dataReceiveTime, *subCounterData, *pressureData);
        }
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
static pthread_t threadCreation(UA_Int32 threadPriority, UA_Int32 coreAffinity, void *(*thread) (void *), \
                                char *applicationName, void *serverConfig){

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
 * ./bin/examples/pubsub_nodeset_rt_subscriber -h gives the list of options for running the application.
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
        " -P [num]      Publisher priority value (default %d)\n"
        " -U [num]      User application priority value (default %d)\n"
        " -c [num]      run on CPU for publisher'num'(default %d)\n"
        " -u [num]      run on CPU for userApplication'num'(default %d)\n"
        " -m [mac_addr] ToDO:dst MAC address\n"
        " -h            prints this message and exits\n"
        "\n",
        appname, DEFAULT_CYCLE_TIME, DEFAULT_SUB_SCHED_PRIORITY, \
        DEFAULT_USERAPPLICATION_SCHED_PRIORITY, DEFAULT_PUBSUB_CORE, DEFAULT_USER_APP_CORE);
}

/**
 * **Main Server code**
 *
 * The main function contains subscriber threads running
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

    /* Files namespace_example_subscriber_generated.h and namespace_example_subscriber_generated.c are created from
     * subDataModel.xml in the /src_generated directory by CMake */
    /* Loading the user created variables into the information model from the generated .c and .h files */
    if(namespace_example_subscriber_generated(server) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
        "Check previous output for any error.");
    }
    else
    {
        nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    }
    UA_NetworkAddressUrlDataType networkAddressUrlSub;
    /* For more information run ./bin/examples/pubsub_nodeset_rt_subscriber -h */
    /* Process the command line arguments */
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
                fpSubscriber = fopen(fileName, "w");
                break;
            case 'p':
                consolePrint = UA_TRUE;
                break;
            case 'P':
                subPriority = atoi(optarg);
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
    networkAddressUrlSub.networkInterface = UA_STRING(interface);
    networkAddressUrlSub.url              = UA_STRING(SUBSCRIBING_MAC_ADDRESS);

    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());

    addPubSubConnectionSubscriber(server, &networkAddressUrlSub);
    addReaderGroup(server);
    addDataSetReader(server);
    UA_Server_freezeReaderGroupConfiguration(server, readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
    serverConfigStruct *serverConfig;
    serverConfig            = (serverConfigStruct*)UA_malloc(sizeof(serverConfigStruct));
    serverConfig->ServerRun = server;

    char threadNameUserApplication[22] = "UserApplicationSub";
    userThreadID                       = threadCreation(userAppPriority, userAppCore, userApplicationSub, threadNameUserApplication, serverConfig);

    retval |= UA_Server_run(server, &running);

    UA_Server_unfreezeReaderGroupConfiguration(server, readerGroupIdentifier);
    returnValue = pthread_join(subthreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for subscriber thread:%d\n", returnValue);
    }
    returnValue = pthread_join(userThreadID, NULL);
    if (returnValue != 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"\nPthread Join Failed for User thread:%d\n", returnValue);
    }
    if (fileWrite == UA_TRUE) {
        /* Write the subscribed data in the file */
        size_t subLoopVariable               = 0;
        for (subLoopVariable = 0; subLoopVariable < measurementsSubscriber;
             subLoopVariable++) {
             fprintf(fpSubscriber, "%lu,%ld.%09ld,%lf\n",
                     (long unsigned)subscribeCounterValue[subLoopVariable],
                     subscribeTimestamp[subLoopVariable].tv_sec,
                     subscribeTimestamp[subLoopVariable].tv_nsec,
                     pressureValues[subLoopVariable]);
        }
        fclose(fpSubscriber);
    }
    if (consolePrint == UA_TRUE) {
        size_t subLoopVariable               = 0;
        for (subLoopVariable = 0; subLoopVariable < measurementsSubscriber;
             subLoopVariable++) {
             fprintf(fpSubscriber, "%lu,%ld.%09ld,%lf\n",
                     (long unsigned)subscribeCounterValue[subLoopVariable],
                     subscribeTimestamp[subLoopVariable].tv_sec,
                     subscribeTimestamp[subLoopVariable].tv_nsec,
                     pressureValues[subLoopVariable]);
        }
    }
    UA_Server_delete(server);
    UA_free(serverConfig);
    UA_free(subCounterData);
    /* Free external data source */
    UA_free(subDataValueRT);
    UA_free(pressureData);
    /* Free external data source */
    UA_free(pressureValueRT);
    return (int)retval;
}

