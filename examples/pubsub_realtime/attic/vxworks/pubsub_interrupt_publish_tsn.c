/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *   Copyright (c) 2020, 2022 Wind River Systems, Inc.
 */

#include <vxWorks.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include <endCommon.h>
#include <tsnClkLib.h>
#include <endian.h>
#include <semLib.h>
#include <taskLib.h>

#include <tsnConfigLib.h>

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#define ETH_PUBLISH_ADDRESS     "opc.eth://01-00-5E-00-00-01"
#define MILLI_AS_NANO_SECONDS   (1000 * 1000)
#define SECONDS_AS_NANO_SECONDS (1000 * 1000 * 1000)

#define DATA_SET_WRITER_ID      62541
#define TSN_TIMER_NO            0
#define NS_PER_SEC              1000000000 /* nanoseconds per one second */
#define NS_PER_MS               1000000    /* nanoseconds per 1 millisecond */
#define TSN_TASK_PRIO           30
#define TSN_TASK_STACKSZ        8192

#define KEY_STREAM_NAME         "streamName"
#define KEY_STACK_IDX           "stackIdx"

static UA_NodeId seqNumNodeId;
static UA_NodeId cycleTriggerTimeNodeId;
static UA_NodeId taskBeginTimeNodeId;
static UA_NodeId taskEndTimeNodeId;
static UA_ServerCallback pubCallback = NULL; /* Sentinel if a timer is active */
static UA_Server *pubServer;
static UA_Boolean running = true;
static void *pubData;
static TASK_ID tsnTask = TASK_ID_NULL;
static TASK_ID serverTask = TASK_ID_NULL;
static TSN_STREAM_CFG *streamCfg = NULL;

/* The value to published */
static UA_UInt32 sequenceNumber = 0;
static UA_UInt64 cycleTriggerTime = 0;
static UA_UInt64 lastCycleTriggerTime = 0;
static UA_UInt64 lastTaskBeginTime = 0;
static UA_UInt64 lastTaskEndTime = 0;
static UA_DataValue *staticValueSeqNum = NULL;
static UA_DataValue *staticValueCycTrig = NULL;
static UA_DataValue *staticValueCycBegin = NULL;
static UA_DataValue *staticValueCycEnd = NULL;
static clockid_t tsnClockId = 0; /* TSN clock ID */
static char *ethName = NULL;
static int ethUnit = 0;
static char ethInterface[END_NAME_MAX];
static SEM_ID msgSendSem = SEM_ID_NULL;
static UA_String streamName = {0, NULL};
static uint32_t stackIndex = 0;
static UA_Double pubInterval = 0;
static bool withTSN = false;

static UA_UInt64 ieee1588TimeGet() {
    struct timespec ts;
    (void)tsnClockTimeGet(tsnClockId, &ts);
    return ((UA_UInt64)ts.tv_sec * NS_PER_SEC + (UA_UInt64)ts.tv_nsec);
}

/* Signal handler */
static void
publishInterrupt(_Vx_usr_arg_t arg) {
    cycleTriggerTime = ieee1588TimeGet();
    if(running) {
        (void)semGive(msgSendSem);
    }
}

/**
 * **initTsnTimer**
 *
 * This function initializes a TSN timer. It connects a user defined routine
 * to the interrupt handler and sets timer expiration rate.
 *
 * period is the period of the timer in nanoseconds.
 * RETURNS: Clock Id or 0 if anything fails*/
static clockid_t
initTsnTimer(uint32_t period) {
    clockid_t cid;
    uint32_t tickRate = 0;

    cid = tsnClockIdGet(ethName, ethUnit, TSN_TIMER_NO);
    if(cid != 0) {
        tickRate = NS_PER_SEC / period;
        if(tsnTimerAllocate(cid) == ERROR) {
            return 0;
        }
        if((tsnClockConnect(cid, (FUNCPTR)publishInterrupt, NULL) == ERROR) ||
           (tsnClockRateSet(cid, tickRate) == ERROR)) {
            (void)tsnTimerRelease(cid);
            return 0;
        }
    }

    /* Reroute TSN timer interrupt to a specific CPU core */
    if(tsnClockIntReroute(ethName, ethUnit, stackIndex) != OK) {
        cid = 0;
    }
    return cid;
}

/* The following three methods are originally defined in
 * /src/pubsub/ua_pubsub_manager.c. We provide a custom implementation here to
 * use system interrupts instead of time-triggered callbacks in the OPC UA
 * server control flow. */

static UA_StatusCode
addApplicationCallback(UA_Server *server, UA_NodeId identifier,
                       UA_ServerCallback callback,
                       void *data,
                       UA_Double interval_ms,
                       UA_UInt64 *callbackId) {
    if(pubCallback) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                       "At most one publisher can be registered for interrupt callbacks");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Adding a publisher with a cycle time of %lf milliseconds", interval_ms);

    /* Convert a double float value milliseconds into an integer value in nanoseconds */
    uint32_t interval = (uint32_t)(interval_ms * NS_PER_MS);
    tsnClockId = initTsnTimer(interval);
    if(tsnClockId == 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot allocate a TSN timer");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* Set the callback -- used as a sentinel to detect an operational publisher */
    pubServer = server;
    pubCallback = callback;
    pubData = data;

    if(tsnClockEnable (tsnClockId, NULL) == ERROR) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot enable a TSN timer");
        (void)tsnTimerRelease(tsnClockId);
        tsnClockId = 0;
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
changeApplicationCallbackInterval(UA_Server *server, UA_NodeId identifier,
                                  UA_UInt64 callbackId,
                                  UA_Double interval_ms) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Switching the publisher cycle to %lf milliseconds", interval_ms);

    /* We are not going to change the timer interval for this case */

    return UA_STATUSCODE_GOOD;
}

static void
removeApplicationPubSubCallback(UA_Server *server, UA_NodeId identifier, UA_UInt64 callbackId) {
    if(!pubCallback) {
        return;
    }

    /* Before release timer resource, wait for TSN task stopping first. */
    (void) semGive(msgSendSem);
    if(tsnTask != TASK_ID_NULL) {
        (void) taskWait(tsnTask, WAIT_FOREVER);
        tsnTask = TASK_ID_NULL;
    }

    /* It is safe to disable and release the timer first, then clear callback */
    if(tsnClockId != 0) {
        (void)tsnClockDisable(tsnClockId);
        (void)tsnTimerRelease(tsnClockId);
        tsnClockId = 0;
    }

    pubCallback = NULL;
    pubServer = NULL;
    pubData = NULL;
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
        {UA_STRING(ethInterface), UA_STRING(ETH_PUBLISH_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();

    UA_KeyValuePair connectionOptions[2];

    connectionOptions[0].key = UA_QUALIFIEDNAME(0, KEY_STREAM_NAME);
    UA_Variant_setScalar(&connectionOptions[0].value, &streamName, &UA_TYPES[UA_TYPES_STRING]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, KEY_STACK_IDX);
    UA_Variant_setScalar(&connectionOptions[1].value, &stackIndex, &UA_TYPES[UA_TYPES_UINT32]);

    connectionConfig.connectionPropertiesSize = 2;
    connectionConfig.connectionProperties = connectionOptions;

    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                  &publishedDataSetIdent);

    UA_DataSetFieldConfig dataSetFieldCfg;
    UA_NodeId f4;
    memset(&dataSetFieldCfg, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldCfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldCfg.field.variable.fieldNameAlias = UA_STRING ("Sequence Number");
    dataSetFieldCfg.field.variable.promotedField = UA_FALSE;
    dataSetFieldCfg.field.variable.publishParameters.publishedVariable = seqNumNodeId;
    dataSetFieldCfg.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    dataSetFieldCfg.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    staticValueSeqNum = UA_DataValue_new();
    UA_Variant_setScalar(&staticValueSeqNum->value, &sequenceNumber, &UA_TYPES[UA_TYPES_UINT32]);
    staticValueSeqNum->value.storageType = UA_VARIANT_DATA_NODELETE;
    dataSetFieldCfg.field.variable.rtValueSource.staticValueSource = &staticValueSeqNum;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldCfg, &f4);

    UA_NodeId f3;
    memset(&dataSetFieldCfg, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldCfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldCfg.field.variable.fieldNameAlias = UA_STRING ("Cycle Trigger Time");
    dataSetFieldCfg.field.variable.promotedField = UA_FALSE;
    dataSetFieldCfg.field.variable.publishParameters.publishedVariable = cycleTriggerTimeNodeId;
    dataSetFieldCfg.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    dataSetFieldCfg.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    staticValueCycTrig = UA_DataValue_new();
    UA_Variant_setScalar(&staticValueCycTrig->value, &lastCycleTriggerTime, &UA_TYPES[UA_TYPES_UINT64]);
    staticValueCycTrig->value.storageType = UA_VARIANT_DATA_NODELETE;
    dataSetFieldCfg.field.variable.rtValueSource.staticValueSource = &staticValueCycTrig;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldCfg, &f3);

    UA_NodeId f2;
    memset(&dataSetFieldCfg, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldCfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldCfg.field.variable.fieldNameAlias = UA_STRING ("Task Begin Time");
    dataSetFieldCfg.field.variable.promotedField = UA_FALSE;
    dataSetFieldCfg.field.variable.publishParameters.publishedVariable = taskBeginTimeNodeId;
    dataSetFieldCfg.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    dataSetFieldCfg.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    staticValueCycBegin = UA_DataValue_new();
    UA_Variant_setScalar(&staticValueCycBegin->value, &lastTaskBeginTime, &UA_TYPES[UA_TYPES_UINT64]);
    staticValueCycBegin->value.storageType = UA_VARIANT_DATA_NODELETE;
    dataSetFieldCfg.field.variable.rtValueSource.staticValueSource = &staticValueCycBegin;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldCfg, &f2);


    UA_NodeId f1;
    memset(&dataSetFieldCfg, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldCfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldCfg.field.variable.fieldNameAlias = UA_STRING ("Task End Time");
    dataSetFieldCfg.field.variable.promotedField = UA_FALSE;
    dataSetFieldCfg.field.variable.publishParameters.publishedVariable = taskEndTimeNodeId;
    dataSetFieldCfg.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    dataSetFieldCfg.field.variable.rtValueSource.rtFieldSourceEnabled = UA_TRUE;
    staticValueCycEnd = UA_DataValue_new();
    UA_Variant_setScalar(&staticValueCycEnd->value, &lastTaskEndTime, &UA_TYPES[UA_TYPES_UINT64]);
    staticValueCycEnd->value.storageType = UA_VARIANT_DATA_NODELETE;
    dataSetFieldCfg.field.variable.rtValueSource.staticValueSource = &staticValueCycEnd;

    UA_Server_addDataSetField(server, publishedDataSetIdent, &dataSetFieldCfg, &f1);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = pubInterval;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_FIXED_SIZE;
    writerGroupConfig.pubsubManagerCallback.addCustomCallback = addApplicationCallback;
    writerGroupConfig.pubsubManagerCallback.changeCustomCallback = changeApplicationCallbackInterval;
    writerGroupConfig.pubsubManagerCallback.removeCustomCallback = removeApplicationPubSubCallback;
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
    UA_UInt64 initVal64 = 0;
    UA_UInt32 initVal32 = 0;

    UA_NodeId folderId;
    UA_NodeId_init(&folderId);
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Publisher TSN");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Publisher TSN"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &folderId);

    UA_NodeId_init(&seqNumNodeId);
    seqNumNodeId = UA_NODEID_STRING(1, "sequence.number");
    UA_VariableAttributes publisherAttr = UA_VariableAttributes_default;
    publisherAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_Variant_setScalar(&publisherAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    publisherAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Sequence Number");
    publisherAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, seqNumNodeId,
                              folderId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Sequence Number"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              publisherAttr, NULL, NULL);
    UA_ValueBackend valueBackend;
    valueBackend.backendType = UA_VALUEBACKENDTYPE_EXTERNAL;
    valueBackend.backend.external.value = &staticValueSeqNum;
    UA_Server_setVariableNode_valueBackend(server, seqNumNodeId, valueBackend);

    UA_NodeId_init(&cycleTriggerTimeNodeId);
    cycleTriggerTimeNodeId = UA_NODEID_STRING(1, "cycle.trigger.time");
    publisherAttr = UA_VariableAttributes_default;
    publisherAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    UA_Variant_setScalar(&publisherAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Cycle Trigger Time");
    publisherAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, cycleTriggerTimeNodeId,
                              folderId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Cycle Trigger Time"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              publisherAttr, NULL, NULL);
    valueBackend.backend.external.value = &staticValueCycTrig;
    UA_Server_setVariableNode_valueBackend(server, cycleTriggerTimeNodeId, valueBackend);

    UA_NodeId_init(&taskBeginTimeNodeId);
    taskBeginTimeNodeId = UA_NODEID_STRING(1, "task.begin.time");
    publisherAttr = UA_VariableAttributes_default;
    publisherAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    UA_Variant_setScalar(&publisherAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Task Begin Time");
    publisherAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, taskBeginTimeNodeId,
                              folderId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Task Begin Time"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              publisherAttr, NULL, NULL);
    valueBackend.backend.external.value = &staticValueCycBegin;
    UA_Server_setVariableNode_valueBackend(server, taskBeginTimeNodeId, valueBackend);

    UA_NodeId_init(&taskEndTimeNodeId);
    taskEndTimeNodeId = UA_NODEID_STRING(1, "task.end.time");
    publisherAttr = UA_VariableAttributes_default;
    publisherAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    UA_Variant_setScalar(&publisherAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    publisherAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Task End Time");
    publisherAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, taskEndTimeNodeId,
                              folderId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "Task End Time"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              publisherAttr, NULL, NULL);
    valueBackend.backend.external.value = &staticValueCycEnd;
    UA_Server_setVariableNode_valueBackend(server, taskEndTimeNodeId, valueBackend);
}

static void open62541EthTSNTask(void) {
    uint64_t t = 0;
    while(running) {
        (void) semTake(msgSendSem, WAIT_FOREVER);
        if(!running) {
            break;
        }
        t = ieee1588TimeGet();

        /*
         * Because we cannot get the task end time of one packet before it is
         * sent, we let one packet take its previous packet's cycleTriggerTime,
         * taskBeginTime and taskEndTime.
         */
        if(sequenceNumber == 0) {
            lastCycleTriggerTime = 0;
            lastTaskBeginTime = 0;
            lastTaskEndTime = 0;
        }
        pubCallback(pubServer, pubData);

        sequenceNumber++;
        lastCycleTriggerTime = cycleTriggerTime;
        lastTaskBeginTime = t;
        lastTaskEndTime = ieee1588TimeGet();
    }
}

static void open62541ServerTask(void) {
    UA_Server *server = UA_Server_new();
    if(server == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot allocate a server object");
        goto serverCleanup;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    addServerNodes(server);
    addPubSubConfiguration(server);

    /* Run the server */
    (void) UA_Server_run(server, &running);

serverCleanup:
    if(server != NULL) {
        UA_Server_delete(server);
        server = NULL;
    }
}

static bool initTSNStream(char *eName, size_t eNameSize, int unit) {
    streamCfg = tsnConfigFind (eName, eNameSize, unit);
    if(streamCfg == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot find TSN configuration for %s%d", eName, unit);
        return false;
    }
    if(streamCfg->streamCount == 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot find stream defined for %s%d", eName, unit);
        return false;
    }
    else {
        if(withTSN) {
            streamName = UA_STRING(streamCfg->streamObjs[0].stream.name);
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "No TSN stream associated.");
            streamName = UA_STRING_NULL;
        }

    }
    pubInterval = (UA_Double)((streamCfg->schedule.cycleTime * 1.0) / NS_PER_MS);
    return true;
}

static bool initTSNTask() {
    tsnTask = taskSpawn ((char *)"tTsnPub", TSN_TASK_PRIO, 0, TSN_TASK_STACKSZ, (FUNCPTR)open62541EthTSNTask,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if(tsnTask == TASK_ID_ERROR) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot spawn a TSN task");
        return false;
    }

    /* Reroute TSN task to a specific CPU core */
#ifdef _WRS_CONFIG_SMP
    unsigned int ncpus = vxCpuConfiguredGet ();
    cpuset_t cpus;

    if(stackIndex < ncpus) {
        CPUSET_ZERO (cpus);
        CPUSET_SET (cpus, stackIndex);
        if(taskCpuAffinitySet (tsnTask, cpus) != OK) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot move TSN task to core %d", stackIndex);
            return false;
        }
    }
#endif
    return true;
}

static bool initServerTask() {
    serverTask = taskSpawn((char *)"tPubServer", TSN_TASK_PRIO + 5, 0, TSN_TASK_STACKSZ*2, (FUNCPTR)open62541ServerTask,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if(serverTask == TASK_ID_ERROR) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot spawn a server task");
        return false;
    }

    /* Reroute TSN task to a specific CPU core */
#ifdef _WRS_CONFIG_SMP
    unsigned int ncpus = vxCpuConfiguredGet();
    cpuset_t cpus;

    if(stackIndex < ncpus) {
        CPUSET_ZERO(cpus);
        CPUSET_SET(cpus, stackIndex);
        if(taskCpuAffinitySet(serverTask, cpus) != OK) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot move TSN task to core %d", stackIndex);
            return false;
        }
    }
#endif
    return true;
}

void open62541PubTSNStop() {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Stop TSN publisher");
    running = UA_FALSE;
    if(serverTask != TASK_ID_NULL) {
        (void) taskWait(serverTask, WAIT_FOREVER);
        serverTask = TASK_ID_NULL;
    }
    (void) semGive(msgSendSem);
    if(tsnTask != TASK_ID_NULL) {
        (void) taskWait(tsnTask, WAIT_FOREVER);
        tsnTask = TASK_ID_NULL;
    }
    if(msgSendSem != NULL) {
        (void)semDelete(msgSendSem);
        msgSendSem = SEM_ID_NULL;
    }

    ethName = NULL;
    ethUnit = 0;
    streamName = UA_STRING_NULL;
    stackIndex = 0;
    withTSN = false;
    pubInterval = 0;
    streamCfg = NULL;

    if(staticValueSeqNum != NULL) {
        UA_DataValue_delete(staticValueSeqNum);
        staticValueSeqNum = NULL;
    }
    if(staticValueCycTrig != NULL) {
        UA_DataValue_delete(staticValueCycTrig);
        staticValueCycTrig = NULL;
    }
    if(staticValueCycBegin != NULL) {
        UA_DataValue_delete(staticValueCycBegin);
        staticValueCycBegin = NULL;
    }
    if(staticValueCycEnd != NULL) {
        UA_DataValue_delete(staticValueCycEnd);
        staticValueCycEnd = NULL;
    }
    sequenceNumber = 0;
    cycleTriggerTime = 0;
    lastCycleTriggerTime = 0;
    lastTaskBeginTime = 0;
    lastTaskEndTime = 0;
}

/**
 * Create a publisher with/without TSN.
 * eName: Ethernet interface name like "gei", "gem", etc
 * eNameSize: The length of eName including "\0"
 * unit: Unit NO of the ethernet interface
 * stkIdx: Network stack index
 * withTsn: true: Enable TSN; false: Disable TSN
 *
 * @return OK on success, otherwise ERROR
 */
STATUS open62541PubTSNStart(char *eName, size_t eNameSize, int unit, uint32_t stkIdx, bool withTsn) {
    if((eName == NULL) || (eNameSize == 0)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Ethernet interface name is invalid");
        return ERROR;
    }

    ethName = eName;
    ethUnit = unit;
    stackIndex = stkIdx;
    withTSN = withTsn;
    snprintf(ethInterface, sizeof(ethInterface), "%s%d", eName, unit);

    if(!initTSNStream(eName, eNameSize, unit)) {
        goto startCleanup;
    }

    /* Create a binary semaphore which is used by the TSN timer to wake up the sender task */
    msgSendSem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
    if(msgSendSem == SEM_ID_NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Cannot create a semaphore");
        goto startCleanup;
    }
    running = true;
    if(!initTSNTask()) {
        goto startCleanup;
    }

    if(!initServerTask()) {
        goto startCleanup;
    }


    return OK;
startCleanup:
    open62541PubTSNStop();
    return ERROR;
}
