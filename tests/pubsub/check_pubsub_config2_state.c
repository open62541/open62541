/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* Interaction of the configuration update with the PubSub state machine */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "pubsub_config_fixtures.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;
static UA_NodeId pubVarId, subVarId;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* SConn with SWG + SDSW (SPDS) */
static UA_NodeId sConnId, sWgId, sDswId, sPdsId;

static void
buildBaseConfig(void) {
    UA_PubSubTestBase base;
    UA_PubSubTest_addBaseConfig(server, "S", 52001, 50.0, false, &base);
    pubVarId = base.varId;
    sConnId = base.connId;
    sWgId = base.wgId;
    sDswId = base.dswId;
    sPdsId = base.pdsId;
}

/* Update file with a single WriterGroup under the named connection */
static UA_PubSubConnectionDataType fileConn;
static UA_WriterGroupDataType fileWg;

static void
buildAddWgFile(UA_PubSubConfiguration2DataType *cfg, const char *connName,
               const char *wgName, UA_Boolean enabled) {
    UA_PubSubConfiguration2DataType_init(cfg);
    UA_PubSubConnectionDataType_init(&fileConn);
    fileConn.name = UA_STRING((char*)(uintptr_t)connName);
    UA_WriterGroupDataType_init(&fileWg);
    fileWg.name = UA_STRING((char*)(uintptr_t)wgName);
    fileWg.writerGroupId = 150;
    fileWg.publishingInterval = 50.0;
    fileWg.keepAliveTime = 5000.0;
    fileWg.enabled = enabled;
    fileConn.writerGroups = &fileWg;
    fileConn.writerGroupsSize = 1;
    cfg->connections = &fileConn;
    cfg->connectionsSize = 1;
}

/* A component added under a disabled parent stays Paused and cascades to
 * Operational when the parent is enabled */
START_TEST(AddUnderDisabledParent) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "PausedWG", true);
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_NodeId newWgId = result.configurationObjects[0];

    UA_WriterGroup *wg = UA_WriterGroup_find(psm, newWgId);
    ck_assert(wg != NULL);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_PAUSED);

    res = UA_Server_enablePubSubConnection(server, sConnId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);

    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* A modified running WriterGroup stays Operational and keeps publishing */
START_TEST(ModifyRunningKeepsPublishing) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroup *wg = UA_WriterGroup_find(psm, sWgId);
    ck_assert(wg != NULL);
    for(size_t i = 0; i < 20 && wg->sequenceNumber < 3; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    UA_UInt16 seqBefore = wg->sequenceNumber;
    ck_assert_uint_ge(seqBefore, 3);

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "SWG", true);
    fileWg.writerGroupId = 100;
    fileWg.publishingInterval = 25.0;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert(wg->config.publishingInterval == 25.0);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);

    for(size_t i = 0; i < 20 && wg->sequenceNumber < seqBefore + 3; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_ge(wg->sequenceNumber, seqBefore + 3);
} END_TEST

/* The componentLifecycleCallback runs on add and remove and can veto adds */
static size_t lifecycleAddCount;
static size_t lifecycleRemoveCount;
static UA_StatusCode lifecycleReturn;

static UA_StatusCode
countingLifecycleCallback(UA_Server *s, const UA_NodeId id,
                          const UA_PubSubComponentType componentType,
                          UA_Boolean remove) {
    /* Do not veto removals: that would block the cleanup of a vetoed add */
    if(remove) {
        lifecycleRemoveCount++;
        return UA_STATUSCODE_GOOD;
    }
    lifecycleAddCount++;
    return lifecycleReturn;
}

START_TEST(LifecycleCallbackAndVeto) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    lifecycleAddCount = 0;
    lifecycleRemoveCount = 0;
    lifecycleReturn = UA_STATUSCODE_GOOD;
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        countingLifecycleCallback;

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "CallbackWG", false);
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(lifecycleAddCount, 1);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(lifecycleRemoveCount, 1);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* A bad return code vetoes the add */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, sConnId);
    lifecycleReturn = UA_STATUSCODE_BADUSERACCESSDENIED;
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0],
                     UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Check by name: removed groups linger until the EventLoop frees them */
    for(size_t i = 0; i < 5; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    UA_String vetoedName = UA_STRING("CallbackWG");
    UA_WriterGroup *iterWg;
    LIST_FOREACH(iterWg, &c->writerGroups, listEntry) {
        ck_assert(!UA_String_equal(&iterWg->config.name, &vetoedName));
    }

    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
} END_TEST

/* A modify of a running component fires the state-change callbacks */
static size_t beforeStateChangeCount;
static size_t stateChangeCount;

static void
countingBeforeStateChange(UA_Server *s, const UA_NodeId id,
                          UA_PubSubState *targetState) {
    beforeStateChangeCount++;
}

static void
countingStateChange(UA_Server *s, const UA_NodeId id,
                    UA_PubSubState state, UA_StatusCode status) {
    stateChangeCount++;
}

START_TEST(StateCallbacksOnModify) {
    buildBaseConfig();
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.beforeStateChangeCallback = countingBeforeStateChange;
    sc->pubSubConfig.stateChangeCallback = countingStateChange;
    beforeStateChangeCount = 0;
    stateChangeCount = 0;

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "SWG", true);
    fileWg.writerGroupId = 100;
    fileWg.publishingInterval = 30.0;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* At least disable and restore to Operational */
    ck_assert_uint_ge(beforeStateChangeCount, 2);
    ck_assert_uint_ge(stateChangeCount, 2);

    sc->pubSubConfig.beforeStateChangeCallback = NULL;
    sc->pubSubConfig.stateChangeCallback = NULL;
} END_TEST

/* Element operations keep the state of a custom connection state machine */
static UA_StatusCode
connectionStateMachine(UA_Server *s, const UA_NodeId componentId,
                       void *componentContext, UA_PubSubState *state,
                       UA_PubSubState targetState) {
    /* No sockets are opened. Move directly to the target state. */
    if(targetState == UA_PUBSUBSTATE_OPERATIONAL ||
       targetState == UA_PUBSUBSTATE_PREOPERATIONAL)
        *state = UA_PUBSUBSTATE_OPERATIONAL;
    else
        *state = targetState;
    return UA_STATUSCODE_GOOD;
}

START_TEST(CustomStateMachineSurvivesOps) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConnectionConfig cc;
    UA_StatusCode res = UA_Server_getPubSubConnectionConfig(server, sConnId, &cc);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    cc.customStateMachine = connectionStateMachine;
    res = UA_Server_updatePubSubConnectionConfig(server, sConnId, &cc);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubConnectionConfig_clear(&cc);

    res = UA_Server_enablePubSubConnection(server, sConnId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, sConnId);
    ck_assert_int_eq((int)c->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("SConn");
    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("CustomRG");
    conn.readerGroups = &rg;
    conn.readerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);
    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_int_eq((int)c->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_uint_eq(c->readerGroupsSize, 1);
} END_TEST

/* A reader loaded from a file receives the data of a loopback publisher */
START_TEST(FileLoadedReaderReceives) {
    pubVarId = UA_PubSubTest_addVariable(server, 52001, 42);
    subVarId = UA_PubSubTest_addVariable(server, 52002, 0);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    cfg.enabled = true;

    UA_PublishedDataSetDataType pds;
    UA_PublishedDataSetDataType_init(&pds);
    pds.name = UA_STRING("LoopPDS");
    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    fmd.name = UA_STRING("Field1");
    fmd.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    fmd.builtInType = UA_NS0ID_UINT32;
    fmd.valueRank = -1;
    pds.dataSetMetaData.name = UA_STRING("LoopPDS");
    pds.dataSetMetaData.fields = &fmd;
    pds.dataSetMetaData.fieldsSize = 1;
    UA_PublishedDataItemsDataType pdi;
    UA_PublishedDataItemsDataType_init(&pdi);
    UA_PublishedVariableDataType pv;
    UA_PublishedVariableDataType_init(&pv);
    pv.publishedVariable = pubVarId;
    pv.attributeId = UA_ATTRIBUTEID_VALUE;
    pdi.publishedData = &pv;
    pdi.publishedDataSize = 1;
    pds.dataSetSource.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    pds.dataSetSource.content.decoded.type =
        &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE];
    pds.dataSetSource.content.decoded.data = &pdi;
    cfg.publishedDataSets = &pds;
    cfg.publishedDataSetsSize = 1;

    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("LoopConn");
    conn.enabled = true;
    conn.transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_NetworkAddressUrlDataType addr;
    UA_PubSubTest_initNetworkAddressUrl(&addr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    conn.address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    conn.address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    conn.address.content.decoded.data = &addr;
    UA_UInt16 publisherId = 2234;
    UA_Variant_setScalar(&conn.publisherId, &publisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);

    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("LoopWG");
    wg.enabled = true;
    wg.writerGroupId = 100;
    wg.publishingInterval = 50.0;
    wg.keepAliveTime = 5000.0;
    UA_DataSetWriterDataType dsw;
    UA_DataSetWriterDataType_init(&dsw);
    dsw.name = UA_STRING("LoopDSW");
    dsw.enabled = true;
    dsw.dataSetWriterId = 200;
    dsw.dataSetName = UA_STRING("LoopPDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;

    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("LoopRG");
    rg.enabled = true;
    UA_DataSetReaderDataType dsr;
    UA_DataSetReaderDataType_init(&dsr);
    dsr.name = UA_STRING("LoopDSR");
    dsr.enabled = true;
    dsr.writerGroupId = 100;
    dsr.dataSetWriterId = 200;
    UA_Variant_setScalar(&dsr.publisherId, &publisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    dsr.dataSetMetaData.fields = &fmd;
    dsr.dataSetMetaData.fieldsSize = 1;
    UA_TargetVariablesDataType targets;
    UA_TargetVariablesDataType_init(&targets);
    UA_FieldTargetDataType target;
    UA_FieldTargetDataType_init(&target);
    target.attributeId = UA_ATTRIBUTEID_VALUE;
    target.targetNodeId = subVarId;
    targets.targetVariables = &target;
    targets.targetVariablesSize = 1;
    dsr.subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    dsr.subscribedDataSet.content.decoded.type =
        &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE];
    dsr.subscribedDataSet.content.decoded.data = &targets;
    rg.dataSetReaders = &dsr;
    rg.dataSetReadersSize = 1;
    conn.readerGroups = &rg;
    conn.readerGroupsSize = 1;

    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    /* Add all elements and run until the subscriber received the published
     * value */
    const UA_UInt32 add = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD;
    UA_PubSubConfigurationRefDataType refs[6] = {
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0),
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0),
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0),
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0),
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0),
        UA_PubSubTest_ref(add | UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, 0, 0, 0)};
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 6, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < result.referencesResultsSize; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    UA_Boolean received = false;
    for(size_t i = 0; i < 100 && !received; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);

        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.nodeId = subVarId;
        rvi.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_DataValue dv = UA_Server_read(server, &rvi,
                                         UA_TIMESTAMPSTORETURN_NEITHER);
        if(dv.hasValue && dv.value.type == &UA_TYPES[UA_TYPES_UINT32] &&
           *(UA_UInt32*)dv.value.data == 42)
            received = true;
        UA_DataValue_clear(&dv);
    }
    ck_assert(received);

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *c = TAILQ_FIRST(&psm->connections);
    UA_ReaderGroup *liveRg = LIST_FIRST(&c->readerGroups);
    UA_DataSetReader *liveDsr = LIST_FIRST(&liveRg->readers);
    ck_assert_int_eq((int)liveDsr->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
} END_TEST


/* Removing the PDS of a publishing writer removes the writer as well
 * (Part 14 9.1.3.7.2). The running WriterGroup is restored. */
START_TEST(RemovePdsWithRunningWriter) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_WriterGroup *wg = UA_WriterGroup_find(psm, sWgId);
    ck_assert(wg != NULL);
    for(size_t i = 0; i < 20 && wg->sequenceNumber < 3; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PublishedDataSetDataType pds;
    UA_PublishedDataSetDataType_init(&pds);
    pds.name = UA_STRING("SPDS");
    cfg.publishedDataSets = &pds;
    cfg.publishedDataSetsSize = 1;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_ptr_null(UA_PublishedDataSet_find(psm, sPdsId));
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, sDswId));
    wg = UA_WriterGroup_find(psm, sWgId);
    ck_assert(wg != NULL);
    ck_assert_uint_eq(wg->writersCount, 0);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);

    for(size_t i = 0; i < 5; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
} END_TEST


/* Removal notifications per component type */
static size_t removeNotifications[8];

static UA_StatusCode
removeCountingCallback(UA_Server *s, const UA_NodeId id,
                       const UA_PubSubComponentType componentType,
                       UA_Boolean remove) {
    if(remove && (size_t)componentType < 8)
        removeNotifications[componentType]++;
    return UA_STATUSCODE_GOOD;
}

/* A deferred delete (open sockets) asks the application once per component */
START_TEST(DeferredDeleteNotifiesOnce) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }

    memset(removeNotifications, 0, sizeof(removeNotifications));
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        removeCountingCallback;

    res = UA_Server_removePubSubConnection(server, sConnId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 100 && psm->connectionsSize > 0; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(psm->connectionsSize, 0);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;

    ck_assert_uint_eq(removeNotifications[UA_PUBSUBCOMPONENT_CONNECTION], 1);
    ck_assert_uint_eq(removeNotifications[UA_PUBSUBCOMPONENT_WRITERGROUP], 1);
    ck_assert_uint_eq(removeNotifications[UA_PUBSUBCOMPONENT_DATASETWRITER], 1);
} END_TEST


/* WriterGroup state machine that records the last target and each disable */
static UA_PubSubState lastWgTarget;
static size_t wgDisableCount;

static UA_StatusCode
recordingStateMachine(UA_Server *s, const UA_NodeId id, void *ctx,
                      UA_PubSubState *state, UA_PubSubState target) {
    lastWgTarget = target;
    if(target == UA_PUBSUBSTATE_DISABLED && *state != UA_PUBSUBSTATE_DISABLED)
        wgDisableCount++;
    *state = target;
    return UA_STATUSCODE_GOOD;
}

static UA_WriterGroup *
addCustomStateWriterGroup(UA_PubSubState state) {
    UA_WriterGroupConfig wgConfig;
    memset(&wgConfig, 0, sizeof(wgConfig));
    wgConfig.name = UA_STRING("CustomWG");
    wgConfig.writerGroupId = 170;
    wgConfig.publishingInterval = 50.0;
    wgConfig.customStateMachine = recordingStateMachine;
    UA_NodeId wgId;
    UA_StatusCode res = UA_Server_addWriterGroup(server, sConnId, &wgConfig, &wgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    ck_assert(wg != NULL);
    lockServer(server);
    UA_WriterGroup_setPubSubState(psm, wg, state);
    unlockServer(server);
    ck_assert_int_eq((int)wg->head.state, (int)state);
    wgDisableCount = 0;
    return wg;
}

static UA_DataSetWriterDataType fileDsws[2];

/* Adding writers restores the exact prior group state (PreOperational) */
START_TEST(GroupStateRestoredExactly) {
    buildBaseConfig();
    UA_WriterGroup *wg = addCustomStateWriterGroup(UA_PUBSUBSTATE_PREOPERATIONAL);

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "CustomWG", false);
    UA_DataSetWriterDataType_init(&fileDsws[0]);
    fileDsws[0].name = UA_STRING("CustomDSW");
    fileDsws[0].dataSetWriterId = 270;
    fileDsws[0].dataSetName = UA_STRING("SPDS");
    fileWg.dataSetWriters = fileDsws;
    fileWg.dataSetWritersSize = 1;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res =
        UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_uint_eq(wg->writersCount, 1);
    ck_assert_uint_eq(wgDisableCount, 1);
    ck_assert_int_eq((int)lastWgTarget, (int)UA_PUBSUBSTATE_PREOPERATIONAL);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_PREOPERATIONAL);
} END_TEST

/* Several writer operations in one call disable the group only once */
START_TEST(WriterOpsDisableGroupOnce) {
    buildBaseConfig();
    UA_WriterGroup *wg = addCustomStateWriterGroup(UA_PUBSUBSTATE_OPERATIONAL);

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "CustomWG", false);
    for(size_t i = 0; i < 2; i++) {
        UA_DataSetWriterDataType_init(&fileDsws[i]);
        fileDsws[i].name = (i == 0) ? UA_STRING("CustomDSW1") : UA_STRING("CustomDSW2");
        fileDsws[i].dataSetWriterId = (UA_UInt16)(271 + i);
        fileDsws[i].dataSetName = UA_STRING("SPDS");
    }
    fileWg.dataSetWriters = fileDsws;
    fileWg.dataSetWritersSize = 2;
    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    refs[1] = refs[0];
    refs[1].elementIndex = 1;

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res =
        UA_PubSubTest_updateConfig(server, &cfg, 2, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_uint_eq(wg->writersCount, 2);
    ck_assert_uint_eq(wgDisableCount, 1);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
} END_TEST

/* A connection, group and writer added enabled in one call start publishing */
static UA_NetworkAddressUrlDataType subtreeAddr;
static UA_UInt16 subtreePublisherId = 3100;

START_TEST(AddRunningSubtreeInOneCall) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "NewConn", "NewWG", true);
    fileConn.transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_PubSubTest_initNetworkAddressUrl(&subtreeAddr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    fileConn.address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    fileConn.address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    fileConn.address.content.decoded.data = &subtreeAddr;
    UA_Variant_setScalar(&fileConn.publisherId, &subtreePublisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    fileConn.enabled = true;
    fileWg.writerGroupId = 160;
    UA_DataSetWriterDataType_init(&fileDsws[0]);
    fileDsws[0].name = UA_STRING("NewDSW");
    fileDsws[0].dataSetWriterId = 260;
    fileDsws[0].dataSetName = UA_STRING("SPDS");
    fileDsws[0].enabled = true;
    fileWg.dataSetWriters = fileDsws;
    fileWg.dataSetWritersSize = 1;

    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res =
        UA_PubSubTest_updateConfig(server, &cfg, 3, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 3; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    UA_DataSetWriter *dsw =
        UA_DataSetWriter_find(psm, result.configurationObjects[2]);
    UA_PubSubConfigurationUpdateResult_clear(&result);
    ck_assert(wg != NULL);
    ck_assert(dsw != NULL);
    ck_assert(wg->config.enabled);

    for(size_t i = 0; i < 40 && wg->sequenceNumber < 3; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_int_eq((int)wg->linkedConnection->head.state,
                     (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq((int)dsw->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_uint_ge(wg->sequenceNumber, 3);
} END_TEST

/* Remove a running connection and add it again by name in one call */
static UA_PubSubConnectionDataType reAddConns[2];
static UA_NetworkAddressUrlDataType reAddAddr;
static UA_UInt16 reAddPublisherId = 3200;

START_TEST(RemoveAndReAddRunningConnection) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }

    UA_PubSubConfiguration2DataType cfg;
    buildAddWgFile(&cfg, "SConn", "ReWG", true);
    fileWg.writerGroupId = 151;
    UA_DataSetWriterDataType_init(&fileDsws[0]);
    fileDsws[0].name = UA_STRING("ReDSW");
    fileDsws[0].dataSetWriterId = 251;
    fileDsws[0].dataSetName = UA_STRING("SPDS");
    fileDsws[0].enabled = true;
    fileWg.dataSetWriters = fileDsws;
    fileWg.dataSetWritersSize = 1;
    reAddConns[1] = fileConn; /* [1] the connection to add with the group */
    reAddConns[1].transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_PubSubTest_initNetworkAddressUrl(&reAddAddr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    reAddConns[1].address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    reAddConns[1].address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    reAddConns[1].address.content.decoded.data = &reAddAddr;
    UA_Variant_setScalar(&reAddConns[1].publisherId, &reAddPublisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    reAddConns[1].enabled = true;
    UA_PubSubConnectionDataType_init(&reAddConns[0]); /* [0] the remove */
    reAddConns[0].name = UA_STRING("SConn");
    cfg.connections = reAddConns;
    cfg.connectionsSize = 2;

    UA_PubSubConfigurationRefDataType refs[4];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    refs[0].connectionIndex = 1;
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[1].connectionIndex = 1;
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[2].connectionIndex = 1;
    refs[3] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 4, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 4; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    UA_PubSubConnection *newConn =
        UA_PubSubConnection_find(psm, result.configurationObjects[2]);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    UA_PubSubConfigurationUpdateResult_clear(&result);
    ck_assert_ptr_nonnull(newConn);
    ck_assert_ptr_nonnull(wg);
    ck_assert_ptr_eq(wg->linkedConnection, newConn);

    /* The old connection is deleted once its sockets are closed */
    ck_assert_uint_eq(psm->connectionsSize, 2);
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry)
        ck_assert(c == newConn || c->deleteFlag);
    for(size_t i = 0; i < 100 && (psm->connectionsSize > 1 || wg->sequenceNumber < 3); i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_ptr_eq(TAILQ_FIRST(&psm->connections), newConn);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_uint_ge(wg->sequenceNumber, 3);
} END_TEST

/* A connection pending deletion is removed once, within and across calls */
START_TEST(DoubleRemoveRunningConnection) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }

    memset(removeNotifications, 0, sizeof(removeNotifications));
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        removeCountingCallback;

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("SConn");
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = refs[0];

    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_BADNOMATCH);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* The delete is deferred. A later call does not find the connection. */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, sConnId);
    ck_assert_ptr_nonnull(c);
    ck_assert(c->deleteFlag);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADNOMATCH);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    for(size_t i = 0; i < 100 && psm->connectionsSize > 0; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(psm->connectionsSize, 0);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
    ck_assert_uint_eq(removeNotifications[UA_PUBSUBCOMPONENT_CONNECTION], 1);
    ck_assert_uint_eq(removeNotifications[UA_PUBSUBCOMPONENT_WRITERGROUP], 1);
} END_TEST

/* Vetoes the creation of "VetoConn" to force a rollback. With
 * vetoRemovesGroup it also removes SWG, so the writer cannot be restored. */
static UA_Boolean vetoRemovesGroup;

static UA_StatusCode
vetoByNameCallback(UA_Server *s, const UA_NodeId id,
                   const UA_PubSubComponentType componentType,
                   UA_Boolean remove) {
    if(remove) {
        lifecycleRemoveCount++;
        return UA_STATUSCODE_GOOD;
    }
    lifecycleAddCount++;
    if(componentType != UA_PUBSUBCOMPONENT_CONNECTION)
        return UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(s);
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, id);
    UA_String vetoName = UA_STRING("VetoConn");
    if(!c || !UA_String_equal(&c->config.name, &vetoName))
        return UA_STATUSCODE_GOOD;
    if(vetoRemovesGroup) {
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, sWgId);
        if(wg)
            UA_WriterGroup_remove(psm, wg);
    }
    return UA_STATUSCODE_BADUSERACCESSDENIED;
}

static UA_PubSubConnectionDataType vetoConns[2];
static UA_PublishedDataSetDataType vetoPds;
static UA_NetworkAddressUrlDataType vetoAddr;
static UA_UInt16 vetoPublisherId = 3300;

/* [0] the element removed by the reference, [1] the vetoed connection */
static void
buildVetoFile(UA_PubSubConfiguration2DataType *cfg,
              UA_PubSubConfigurationRefDataType *refs, UA_Boolean removePds) {
    UA_PubSubConfiguration2DataType_init(cfg);
    UA_PubSubConnectionDataType_init(&vetoConns[0]);
    vetoConns[0].name = UA_STRING("SConn");
    UA_PubSubConnectionDataType_init(&vetoConns[1]);
    vetoConns[1].name = UA_STRING("VetoConn");
    vetoConns[1].transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_PubSubTest_initNetworkAddressUrl(&vetoAddr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    vetoConns[1].address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    vetoConns[1].address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    vetoConns[1].address.content.decoded.data = &vetoAddr;
    UA_Variant_setScalar(&vetoConns[1].publisherId, &vetoPublisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    cfg->connections = vetoConns;
    cfg->connectionsSize = 2;
    UA_PublishedDataSetDataType_init(&vetoPds);
    vetoPds.name = UA_STRING("SPDS");
    cfg->publishedDataSets = &vetoPds;
    cfg->publishedDataSetsSize = 1;

    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      ((removePds) ? UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET :
                       UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION), 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1].connectionIndex = 1;
}

static void
runUntilPublished(UA_WriterGroup *wg, UA_UInt16 seq) {
    for(size_t i = 0; i < 100 && wg->sequenceNumber < seq; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
}

/* Rollback recreates a removed running connection with its groups and
 * writers, without asking the application. It publishes again. */
START_TEST(RollbackRunningConnectionRemove) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    runUntilPublished(UA_WriterGroup_find(psm, sWgId), 3);

    lifecycleAddCount = 0;
    lifecycleRemoveCount = 0;
    vetoRemovesGroup = false;
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        vetoByNameCallback;

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfigurationRefDataType refs[2];
    buildVetoFile(&cfg, refs, false);
    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(!result.changesApplied);
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[0]));
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Only the vetoed creation and the removal were notified */
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
    ck_assert_uint_eq(lifecycleAddCount, 1);
    ck_assert_uint_eq(lifecycleRemoveCount, 3);

    /* The connection is recreated with a new WriterGroup instance */
    UA_PubSubConnection *c = NULL, *iter;
    UA_String connName = UA_STRING("SConn");
    TAILQ_FOREACH(iter, &psm->connections, listEntry) {
        if(!iter->deleteFlag && UA_String_equal(&iter->config.name, &connName))
            c = iter;
    }
    ck_assert_ptr_nonnull(c);
    ck_assert_uint_eq(c->writerGroupsSize, 1);
    UA_WriterGroup *wg = LIST_FIRST(&c->writerGroups);
    ck_assert_uint_eq(wg->writersCount, 1);
    UA_DataSetWriter *dsw = LIST_FIRST(&wg->writers);
    ck_assert(UA_NodeId_equal(&dsw->connectedDataSet->head.identifier, &sPdsId));

    /* The removed connection is freed, the restored one publishes */
    runUntilPublished(wg, 3);
    for(size_t i = 0; i < 100 && psm->connectionsSize > 1; i++) {
        UA_fakeSleep(50);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq((int)c->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq((int)wg->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_int_eq((int)dsw->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_uint_ge(wg->sequenceNumber, 3);
} END_TEST

/* Rollback restores a removed PDS and its writer, the group publishes again */
START_TEST(RollbackRestoresPdsWithRunningWriter) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, sWgId);
    runUntilPublished(wg, 3);
    UA_PubSubState wgState = wg->head.state;

    lifecycleAddCount = 0;
    lifecycleRemoveCount = 0;
    vetoRemovesGroup = false;
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        vetoByNameCallback;

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfigurationRefDataType refs[2];
    buildVetoFile(&cfg, refs, true);
    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;

    UA_PublishedDataSet *pds = UA_PublishedDataSet_findByName(psm, UA_STRING("SPDS"));
    ck_assert_ptr_nonnull(pds);
    ck_assert_uint_eq(wg->writersCount, 1);
    UA_DataSetWriter *dsw = LIST_FIRST(&wg->writers);
    ck_assert_ptr_eq(dsw->connectedDataSet, pds);
    ck_assert_int_eq((int)wg->head.state, (int)wgState);

    UA_UInt16 seq = wg->sequenceNumber;
    runUntilPublished(wg, (UA_UInt16)(seq + 3));
    ck_assert_int_eq((int)dsw->head.state, (int)UA_PUBSUBSTATE_OPERATIONAL);
    ck_assert_uint_ge(wg->sequenceNumber, seq + 3);
} END_TEST

/* When a change cannot be undone, the result reports the applied changes */
START_TEST(RollbackFailureReportsChangesApplied) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_UInt32 versionBefore = psm->configurationVersion;

    vetoRemovesGroup = true;
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        vetoByNameCallback;

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfigurationRefDataType refs[2];
    buildVetoFile(&cfg, refs, true);
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res =
        UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
    vetoRemovesGroup = false;

    /* The PDS is restored, the writer has no group to return to */
    ck_assert_ptr_nonnull(UA_PublishedDataSet_findByName(psm, UA_STRING("SPDS")));
    ck_assert_ptr_null(UA_WriterGroup_find(psm, sWgId));
    ck_assert_uint_ne(psm->configurationVersion, versionBefore);
} END_TEST

int main(void) {
    TCase *tc_state = tcase_create("Config2 state machine interaction");
    tcase_add_checked_fixture(tc_state, setup, teardown);
    tcase_add_test(tc_state, AddUnderDisabledParent);
    tcase_add_test(tc_state, ModifyRunningKeepsPublishing);
    tcase_add_test(tc_state, LifecycleCallbackAndVeto);
    tcase_add_test(tc_state, StateCallbacksOnModify);
    tcase_add_test(tc_state, CustomStateMachineSurvivesOps);
    tcase_add_test(tc_state, FileLoadedReaderReceives);
    tcase_add_test(tc_state, RemovePdsWithRunningWriter);
    tcase_add_test(tc_state, DeferredDeleteNotifiesOnce);
    tcase_add_test(tc_state, GroupStateRestoredExactly);
    tcase_add_test(tc_state, WriterOpsDisableGroupOnce);
    tcase_add_test(tc_state, AddRunningSubtreeInOneCall);
    tcase_add_test(tc_state, RemoveAndReAddRunningConnection);
    tcase_add_test(tc_state, DoubleRemoveRunningConnection);
    tcase_add_test(tc_state, RollbackRunningConnectionRemove);
    tcase_add_test(tc_state, RollbackRestoresPdsWithRunningWriter);
    tcase_add_test(tc_state, RollbackFailureReportsChangesApplied);

    Suite *s = suite_create("PubSub Configuration2 state machine");
    suite_add_tcase(s, tc_state);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
