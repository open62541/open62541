/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018-2019 (c) Kalycito Infotech
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright (c) 2020 Wind River Systems, Inc.
 */

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_pubsub.h>
#include <open62541/plugin/pubsub_ethernet.h>

#define ETH_PUBLISH_ADDRESS      "opc.eth://0a-00-27-00-00-08"
#define ETH_INTERFACE            "enp0s8"
#define MAX_MEASUREMENTS         10000
#define MILLI_AS_NANO_SECONDS    (1000 * 1000)
#define SECONDS_AS_NANO_SECONDS  (1000 * 1000 * 1000)
#define CLOCKID                  CLOCK_MONOTONIC_RAW
#define SIG                      SIGUSR1
#define PUB_INTERVAL             0.25 /* Publish interval in milliseconds */
#define DATA_SET_WRITER_ID       62541
#define MEASUREMENT_OUTPUT       "publisher_measurement.csv"

/* The RT level of the publisher */
/* possible options: PUBSUB_CONFIG_FASTPATH_NONE, PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS, PUBSUB_CONFIG_FASTPATH_STATIC_VALUES */
#define PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS

UA_NodeId counterNodePublisher = {1, UA_NODEIDTYPE_NUMERIC, {1234}};

typedef struct {
    UA_Callback callback;
    void *application;
    void *context;
    timer_t pubEventTimer;
    struct sigevent pubEvent;
    struct sigaction signalAction;
    UA_Int64 intervalNs;
} UA_PubSubTimer;

#define UA_PUBSUBTIMER_MAX 16
UA_PubSubTimer timer[16];
UA_UInt16 timerCount; /* The timer identifier is the array index */

UA_Boolean running = true;

#if defined(PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS) || (PUBSUB_CONFIG_FASTPATH_STATIC_VALUES)
UA_DataValue *staticValueSource = NULL;
#endif

/* Arrays to store measurement data */
UA_Int32 currentPublishCycleTime[MAX_MEASUREMENTS+1];
struct timespec calculatedCycleStartTime[MAX_MEASUREMENTS+1];
struct timespec cycleStartDelay[MAX_MEASUREMENTS+1];
struct timespec cycleDuration[MAX_MEASUREMENTS+1];
size_t publisherMeasurementsCounter  = 0;

/* The value to published */
static UA_UInt64 publishValue = 62541;

static UA_StatusCode
readPublishValue(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
    UA_Variant_setScalarCopy(&dataValue->value, &publishValue,
                             &UA_TYPES[UA_TYPES_UINT64]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static void
timespec_diff(struct timespec *start, struct timespec *stop,
              struct timespec *result) {
    if((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
}

/* Used to adjust the nanosecond > 1s field value */
static void
nanoSecondFieldConversion(struct timespec *timeSpecValue) {
    while(timeSpecValue->tv_nsec > (SECONDS_AS_NANO_SECONDS - 1)) {
        timeSpecValue->tv_sec += 1;
        timeSpecValue->tv_nsec -= SECONDS_AS_NANO_SECONDS;
    }
}

/* Signal handler */
static void
publishInterrupt(int sig, siginfo_t* si, void* uc) {
    if((uintptr_t)si->si_value.sival_ptr < (uintptr_t)timer ||
       (uintptr_t)si->si_value.sival_ptr > (uintptr_t)&timer[UA_PUBSUBTIMER_MAX]) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "stray signal");
        return;
    }

    UA_PubSubTimer *t = (UA_PubSubTimer*)si->si_value.sival_ptr;

    /* Execute the publish callback in the interrupt */
    struct timespec begin, end;
    clock_gettime(CLOCKID, &begin);
    t->callback(t->application, t->context);
    clock_gettime(CLOCKID, &end);

    if(publisherMeasurementsCounter >= MAX_MEASUREMENTS)
        return;

    /* Save current configured publish interval */
    currentPublishCycleTime[publisherMeasurementsCounter] = (UA_Int32)t->intervalNs;

    /* Save the difference to the calculated time */
    timespec_diff(&calculatedCycleStartTime[publisherMeasurementsCounter],
                  &begin, &cycleStartDelay[publisherMeasurementsCounter]);

    /* Save the duration of the publish callback */
    timespec_diff(&begin, &end, &cycleDuration[publisherMeasurementsCounter]);

    publisherMeasurementsCounter++;

    /* Save the calculated starting time for the next cycle */
    calculatedCycleStartTime[publisherMeasurementsCounter].tv_nsec =
        calculatedCycleStartTime[publisherMeasurementsCounter - 1].tv_nsec + t->intervalNs;
    calculatedCycleStartTime[publisherMeasurementsCounter].tv_sec =
        calculatedCycleStartTime[publisherMeasurementsCounter - 1].tv_sec;
    nanoSecondFieldConversion(&calculatedCycleStartTime[publisherMeasurementsCounter]);

    /* Write the pubsub measurement data */
    if(publisherMeasurementsCounter == MAX_MEASUREMENTS) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Logging the measurements to %s", MEASUREMENT_OUTPUT);

        FILE *fpPublisher = fopen(MEASUREMENT_OUTPUT, "w");
        for(UA_UInt32 i = 0; i < publisherMeasurementsCounter; i++) {
            fprintf(fpPublisher, "%u, %u, %ld.%09ld, %ld.%09ld, %ld.%09ld\n",
                    i,
                    currentPublishCycleTime[i],
                    calculatedCycleStartTime[i].tv_sec,
                    calculatedCycleStartTime[i].tv_nsec,
                    cycleStartDelay[i].tv_sec,
                    cycleStartDelay[i].tv_nsec,
                    cycleDuration[i].tv_sec,
                    cycleDuration[i].tv_nsec);
        }
        fclose(fpPublisher);
    }
}

/* The following three methods are originally defined in
 * /src/pubsub/ua_pubsub_manager.c. We provide a custom implementation here to
 * use system interrupts instead if time-triggered callbacks in the OPC UA
 * server control flow. */

static UA_StatusCode
addPubSubTimerCallback(UA_EventLoop *el, UA_Callback cb, void *application,
                       void *data, UA_Double interval_ms, UA_DateTime *baseTime,
                       UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    if(timerCount >= UA_PUBSUBTIMER_MAX) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "At most one publisher can be registered for interrupt callbacks");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Adding a publisher with a cycle time of %lf milliseconds", interval_ms);

    UA_PubSubTimer *t = &timer[timerCount];
    timerCount++;

    /* Set global values for the publish callback */
    int resultTimerCreate = 0;
    t->intervalNs = (UA_Int64) (interval_ms * MILLI_AS_NANO_SECONDS);

    /* Handle the signal */
    memset(&t->signalAction, 0, sizeof(t->signalAction));
    t->signalAction.sa_flags = SA_SIGINFO;
    t->signalAction.sa_sigaction = publishInterrupt;
    sigemptyset(&t->signalAction.sa_mask);
    sigaction(SIG, &t->signalAction, NULL);

    /* Create the timer */
    memset(&t->pubEventTimer, 0, sizeof(t->pubEventTimer));
    t->pubEvent.sigev_notify = SIGEV_SIGNAL;
    t->pubEvent.sigev_signo = SIG;
    t->pubEvent.sigev_value.sival_ptr = &t->pubEventTimer;
    resultTimerCreate = timer_create(CLOCKID, &t->pubEvent, &t->pubEventTimer);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Failed to create a system event with code %s",
                       strerror(errno));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Arm the timer */
    struct itimerspec timerspec;
    timerspec.it_interval.tv_sec = (long int) (t->intervalNs / (SECONDS_AS_NANO_SECONDS));
    timerspec.it_interval.tv_nsec = (long int) (t->intervalNs % SECONDS_AS_NANO_SECONDS);
    timerspec.it_value.tv_sec = (long int) (t->intervalNs / (SECONDS_AS_NANO_SECONDS));
    timerspec.it_value.tv_nsec = (long int) (t->intervalNs % SECONDS_AS_NANO_SECONDS);
    resultTimerCreate = timer_settime(t->pubEventTimer, 0, &timerspec, NULL);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Failed to arm the system timer with code %i", resultTimerCreate);
        timer_delete(t->pubEventTimer);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start taking measurements */
    publisherMeasurementsCounter = 0;
    clock_gettime(CLOCKID, &calculatedCycleStartTime[0]);
    calculatedCycleStartTime[0].tv_nsec += t->intervalNs;
    nanoSecondFieldConversion(&calculatedCycleStartTime[0]);

    /* Set the callback */
    t->callback = cb;
    t->application = application;
    t->context = data;

    *callbackId = timerCount; /* return timerCount + 1, as zero means no callback */

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
modifyPubSubTimerCallback(UA_EventLoop *el, UA_UInt64 callbackId,
                          UA_Double interval_ms, UA_DateTime *baseTime,
                          UA_TimerPolicy timerPolicy) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Switching the publisher cycle to %lf milliseconds", interval_ms);

    UA_PubSubTimer *t = &timer[callbackId-1];

    struct itimerspec timerspec;
    int resultTimerCreate = 0;
    t->intervalNs = (UA_Int64) (interval_ms * MILLI_AS_NANO_SECONDS);
    timerspec.it_interval.tv_sec = (long int) (t->intervalNs / SECONDS_AS_NANO_SECONDS);
    timerspec.it_interval.tv_nsec = (long int) (t->intervalNs % SECONDS_AS_NANO_SECONDS);
    timerspec.it_value.tv_sec = (long int) (t->intervalNs / (SECONDS_AS_NANO_SECONDS));
    timerspec.it_value.tv_nsec = (long int) (t->intervalNs % SECONDS_AS_NANO_SECONDS);
    resultTimerCreate = timer_settime(t->pubEventTimer, 0, &timerspec, NULL);
    if(resultTimerCreate != 0) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "Failed to arm the system timer");
        timer_delete(t->pubEventTimer);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    clock_gettime(CLOCKID, &calculatedCycleStartTime[publisherMeasurementsCounter]);
    calculatedCycleStartTime[publisherMeasurementsCounter].tv_nsec += t->intervalNs;
    nanoSecondFieldConversion(&calculatedCycleStartTime[publisherMeasurementsCounter]);

    return UA_STATUSCODE_GOOD;
}

static void
removePubSubTimerCallback(UA_EventLoop *el, UA_UInt64 callbackId) {
    UA_PubSubTimer *t = &timer[callbackId-1];
    timer_delete(t->pubEventTimer);
    /* TODO: Decrease the counter */
}

static void
addPubSubConfiguration(UA_Server* server) {
    UA_NodeId connectionIdent;
    UA_NodeId publishedDataSetIdent;
    UA_NodeId writerGroupIdent;

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    connectionConfig.enabled = true;
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING(ETH_INTERFACE), UA_STRING(ETH_PUBLISH_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConfig.publisherId.uint32 = UA_UInt32_random();

    /* Set a custom EventLoop */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_EventLoop *el = UA_EventLoop_new_POSIX(&config->logger);
    connectionConfig.eventLoop = el;
    UA_ConnectionManager *udpCM =
        UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp connection manager"));
    if(udpCM)
        el->registerEventSource(el, (UA_EventSource *)udpCM);
    el->addCyclicCallback = addPubSubTimerCallback;
    el->modifyCyclicCallback = modifyPubSubTimerCallback;
    el->removeCyclicCallback = removePubSubTimerCallback;

    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                  &publishedDataSetIdent);

    UA_NodeId dataSetFieldIdentCounter;
    UA_DataSetFieldConfig counterValue;
    memset(&counterValue, 0, sizeof(UA_DataSetFieldConfig));
    counterValue.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    counterValue.field.variable.fieldNameAlias = UA_STRING ("Counter Variable 1");
    counterValue.field.variable.promotedField = UA_FALSE;
    counterValue.field.variable.publishParameters.publishedVariable = counterNodePublisher;
    counterValue.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

#if defined(PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS) || (PUBSUB_CONFIG_FASTPATH_STATIC_VALUES)
    staticValueSource = UA_DataValue_new();
    UA_Variant_setScalar(&staticValueSource->value, &publishValue, &UA_TYPES[UA_TYPES_UINT64]);
    counterValue.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    counterValue.field.variable.rtValueSource.staticValueSource = &staticValueSource;
#endif
    UA_Server_addDataSetField(server, publishedDataSetIdent, &counterValue,
                              &dataSetFieldIdentCounter);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = PUB_INTERVAL;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    UA_Server_addWriterGroup(server, connectionIdent,
                             &writerGroupConfig, &writerGroupIdent);

    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATA_SET_WRITER_ID;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);

    UA_Server_freezeWriterGroupConfiguration(server, writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
}

static void
addServerNodes(UA_Server* server) {
    UA_UInt64 value = 0;
    UA_VariableAttributes publisherAttr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&publisherAttr.value, &value, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Publisher Counter");
    publisherAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dataSource;
    dataSource.read = readPublishValue;
    dataSource.write = NULL;
    UA_Server_addDataSourceVariableNode(server, counterNodePublisher,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "Publisher Counter"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        publisherAttr, dataSource, NULL, NULL);
}

/* Stop signal */
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = UA_FALSE;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());

    addServerNodes(server);
    addPubSubConfiguration(server);

    /* Run the server */
    UA_StatusCode retval = UA_Server_run(server, &running);
#if defined(PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS) || (PUBSUB_CONFIG_FASTPATH_STATIC_VALUES)
    if(staticValueSource != NULL) {
        UA_DataValue_delete(staticValueSource);
    }
#endif
    UA_Server_delete(server);

    return (int)retval;
}
