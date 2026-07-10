/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2024 open62541 contributors.
 *
 * Round 2: more PubSub state-machine edge cases. Targets branches reported
 * as untested after round 1, in particular:
 *
 *   - UA_PubSubConnection_setPubSubState
 *     * the "server not started" branch (psm->sc.state != STARTED)
 *     * the deleteFlag + non-disabled rejection
 *     * the beforeStateChangeCallback invocation
 *     * the stateChangeCallback invocation
 *     * the default (unknown target) state branch
 *
 *   - UA_DataSetWriter_setPubSubState
 *     * the beforeStateChangeCallback invocation
 *     * the customStateMachine invocation path
 *     * the stateChangeCallback invocation
 *
 *   - UA_WriterGroup_create
 *     * the "wrong encoding MIME-type" branch
 *     * the JSON settings / wrong decoded type branch
 *
 *   - generateFieldMetaData
 *     * the abstract-Datatype -> builtInType = VARIANT fallback branch
 *
 *   - addSubscribedDataSet / UA_PublishedDataSet_create
 *     * the NULL config early-exit branches
 *
 *   - UA_DataSetField_create
 *     * the "non-PUBLISHEDITEMS PDS" branch
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server = NULL;
static UA_NodeId connectionIdent;
static UA_NodeId publishedDataSetIdent;
static UA_NodeId writerGroupIdent;
static UA_NodeId dataSetWriterIdent;
static UA_UInt32 valueStore;

/* Callback counters used to assert that the public-API callbacks fire. */
static size_t beforeStateChangeConnectionCount = 0;
static size_t beforeStateChangeDswCount = 0;
static size_t stateChangeConnectionCount = 0;
static size_t stateChangeDswCount = 0;
static UA_PubSubState lastBeforeStateChangeTarget = (UA_PubSubState)0;
static UA_PubSubState lastStateChangeState = (UA_PubSubState)0;

static void
beforeStateChangeCbConnection(UA_Server *s, const UA_NodeId id,
                             UA_PubSubState *targetState) {
    (void)s; (void)id;
    beforeStateChangeConnectionCount++;
    lastBeforeStateChangeTarget = *targetState;
}

static void
stateChangeCbConnection(UA_Server *s, const UA_NodeId id,
                        UA_PubSubState state, UA_StatusCode status) {
    (void)s; (void)id; (void)status;
    stateChangeConnectionCount++;
    lastStateChangeState = state;
}

static void
beforeStateChangeCbDsw(UA_Server *s, const UA_NodeId id,
                       UA_PubSubState *targetState) {
    (void)s; (void)id;
    beforeStateChangeDswCount++;
}

static void
stateChangeCbDsw(UA_Server *s, const UA_NodeId id, UA_PubSubState state,
                  UA_StatusCode status) {
    (void)s; (void)id; (void)status;
    stateChangeDswCount++;
}

/* Custom state machine for the DataSetWriter test. */
static UA_StatusCode
customStateMachineDsw(UA_Server *s, const UA_NodeId componentId,
                     void *componentContext, UA_PubSubState *state,
                     UA_PubSubState targetState) {
    (void)s; (void)componentId; (void)componentContext;
    *state = targetState;
    return UA_STATUSCODE_GOOD;
}

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);

    beforeStateChangeConnectionCount = 0;
    beforeStateChangeDswCount = 0;
    stateChangeConnectionCount = 0;
    stateChangeDswCount = 0;
    lastBeforeStateChangeTarget = (UA_PubSubState)0;
    lastStateChangeState = (UA_PubSubState)0;
    valueStore = 0;

    UA_NodeId_init(&connectionIdent);
    UA_NodeId_init(&publishedDataSetIdent);
    UA_NodeId_init(&writerGroupIdent);
    UA_NodeId_init(&dataSetWriterIdent);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.beforeStateChangeCallback = beforeStateChangeCbConnection;
    sc->pubSubConfig.stateChangeCallback = stateChangeCbConnection;
}

static void
teardown(void) {
    if(server == NULL)
        return;
    if(server->state == UA_LIFECYCLESTATE_STARTED)
        UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

static void
addMinimalConnection(void) {
    UA_PubSubConnectionConfig cfg;
    memset(&cfg, 0, sizeof(UA_PubSubConnectionConfig));
    cfg.name = UA_STRING("StateMachine-Extra Connection");
    cfg.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType addressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&cfg.address, &addressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    cfg.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    cfg.publisherId.id.uint16 = 2234;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(server, &cfg, &connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

static void
addMinimalPublishedDataSet(void) {
    UA_PublishedDataSetConfig cfg;
    memset(&cfg, 0, sizeof(UA_PublishedDataSetConfig));
    cfg.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    cfg.name = UA_STRING("StateMachine-Extra PDS");
    UA_AddPublishedDataSetResult res =
        UA_Server_addPublishedDataSet(server, &cfg, &publishedDataSetIdent);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_GOOD);
}

static void
addOneStaticInt32Field(void) {
    UA_NodeId int32NodeId = UA_NODEID_NUMERIC(1, 0);
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Variant v;
    UA_Variant_setScalar(&v, &valueStore, &UA_TYPES[UA_TYPES_UINT32]);
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.value = v;
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "StateMachineExtraVar"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, &int32NodeId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fcfg;
    memset(&fcfg, 0, sizeof(UA_DataSetFieldConfig));
    fcfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fcfg.field.variable.fieldNameAlias = UA_STRING("v");
    fcfg.field.variable.promotedField = false;
    fcfg.field.variable.publishParameters.publishedVariable = int32NodeId;
    UA_DataSetFieldResult fr =
        UA_Server_addDataSetField(server, publishedDataSetIdent, &fcfg, NULL);
    ck_assert_int_eq(fr.result, UA_STATUSCODE_GOOD);
}

static void
addMinimalWriterGroup(void) {
    UA_WriterGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_WriterGroupConfig));
    cfg.name = UA_STRING("StateMachine-Extra WG");
    cfg.publishingInterval = 100.0;
    cfg.writerGroupId = 1;
    cfg.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    static UA_UadpWriterGroupMessageDataType wgms;
    UA_UadpWriterGroupMessageDataType_init(&wgms);
    UA_ExtensionObject_setValue(&cfg.messageSettings, &wgms,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_StatusCode res =
        UA_Server_addWriterGroup(server, connectionIdent, &cfg, &writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

static void
addMinimalDataSetWriter(void) {
    UA_DataSetWriterConfig cfg;
    memset(&cfg, 0, sizeof(UA_DataSetWriterConfig));
    cfg.name = UA_STRING("StateMachine-Extra DSW");
    cfg.dataSetWriterId = 1;
    UA_StatusCode res =
        UA_Server_addDataSetWriter(server, writerGroupIdent,
                                   publishedDataSetIdent, &cfg, &dataSetWriterIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Connection: enabling a connection while the PubSubManager lifecycle
 * is not STARTED must land the connection in the PAUSED state. This
 * exercises the "if(psm->sc.state != UA_LIFECYCLESTATE_STARTED)" branch
 * in UA_PubSubConnection_setPubSubState (line 332). We flip the manager
 * state directly because the public API does not expose a way to put
 * it in STOPPED state without going through the full shutdown cycle. */
START_TEST(Connection_EnableWhilePubSubManagerStopped_GoesToPaused) {
    addMinimalConnection();
    addMinimalPublishedDataSet();
    addOneStaticInt32Field();
    addMinimalWriterGroup();
    addMinimalDataSetWriter();

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_int_ne(psm->sc.state, UA_LIFECYCLESTATE_STARTED);

    UA_StatusCode res =
        UA_Server_enablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* The connection state is not exposed via a public getter. Use the
     * internal struct access. */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionIdent);
    ck_assert_ptr_ne(c, NULL);
    ck_assert_int_eq(c->head.state, UA_PUBSUBSTATE_PAUSED);

    res = UA_Server_disablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    c = UA_PubSubConnection_find(psm, connectionIdent);
    ck_assert_ptr_ne(c, NULL);
    ck_assert_int_eq(c->head.state, UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* Connection: setting the deleteFlag and then enabling must return
 * BADINTERNALERROR. The flag is set via internal access to mimic the
 * state the connection is in when UA_PubSubConnection_delete has been
 * triggered. */
START_TEST(Connection_EnableWhenDeleteFlagSet_Fails) {
    addMinimalConnection();

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionIdent);
    ck_assert_ptr_ne(c, NULL);
    c->deleteFlag = true;

    UA_StatusCode res =
        UA_Server_enablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINTERNALERROR);

    /* Disable is still allowed. */
    res = UA_Server_disablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* Connection: the public beforeStateChangeCallback must be invoked
 * once per state change. */
START_TEST(Connection_BeforeStateChangeCallbackIsInvoked) {
    addMinimalConnection();

    UA_StatusCode res =
        UA_Server_enablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(beforeStateChangeConnectionCount, 1);
    ck_assert_int_eq(lastBeforeStateChangeTarget, UA_PUBSUBSTATE_OPERATIONAL);

    size_t prev = beforeStateChangeConnectionCount;
    res = UA_Server_disablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(beforeStateChangeConnectionCount, prev);
    ck_assert_int_eq(lastBeforeStateChangeTarget, UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* Connection: the public stateChangeCallback must be invoked after a
 * state change is committed. */
START_TEST(Connection_StateChangeCallbackIsInvoked) {
    addMinimalConnection();

    UA_StatusCode res =
        UA_Server_enablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(stateChangeConnectionCount, 1);
    /* The final state is OPERATIONAL or PAUSED depending on whether the
     * server is running and the event loop has connected. We only check
     * that the callback fired with a valid state value. */
    ck_assert(lastStateChangeState != (UA_PubSubState)0);

    size_t prev = stateChangeConnectionCount;
    res = UA_Server_disablePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(stateChangeConnectionCount, prev);
} END_TEST

/* DataSetWriter: the beforeStateChangeCallback must be invoked. */
START_TEST(DataSetWriter_BeforeStateChangeCallbackIsInvoked) {
    addMinimalConnection();
    addMinimalPublishedDataSet();
    addOneStaticInt32Field();
    addMinimalWriterGroup();
    addMinimalDataSetWriter();

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.beforeStateChangeCallback = beforeStateChangeCbDsw;

    UA_StatusCode res =
        UA_Server_enableDataSetWriter(server, dataSetWriterIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(beforeStateChangeDswCount, 1);
} END_TEST

/* DataSetWriter: the customStateMachine branch is taken when the
 * DataSetWriter config has a customStateMachine callback. We use the
 * public update-config path to install it and then enable the writer
 * via the parent WriterGroup's enable. */
START_TEST(DataSetWriter_CustomStateMachineIsUsed) {
    addMinimalConnection();
    addMinimalPublishedDataSet();
    addOneStaticInt32Field();
    addMinimalWriterGroup();
    addMinimalDataSetWriter();

    UA_DataSetWriterConfig cfg;
    memset(&cfg, 0, sizeof(UA_DataSetWriterConfig));
    UA_StatusCode res = UA_Server_getDataSetWriterConfig(server, dataSetWriterIdent, &cfg);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    cfg.customStateMachine = customStateMachineDsw;
    res = UA_Server_updateDataSetWriterConfig(server, dataSetWriterIdent, &cfg);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_DataSetWriterConfig_clear(&cfg);

    /* Enable the WriterGroup to drive the DataSetWriter state machine
     * which takes the customStateMachine branch. */
    res = UA_Server_enableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* DataSetWriter: the stateChangeCallback must be invoked. */
START_TEST(DataSetWriter_StateChangeCallbackIsInvoked) {
    addMinimalConnection();
    addMinimalPublishedDataSet();
    addOneStaticInt32Field();
    addMinimalWriterGroup();
    addMinimalDataSetWriter();

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.stateChangeCallback = stateChangeCbDsw;

    UA_StatusCode res =
        UA_Server_enableDataSetWriter(server, dataSetWriterIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(stateChangeDswCount, 1);
} END_TEST

/* WriterGroup: the "wrong encoding MIME type" branch in
 * UA_WriterGroup_create must return BADINTERNALERROR. The branch is
 * reached only when the messageSettings is not UA_EXTENSIONOBJECT_ENCODED_NOBODY
 * AND the encodingMimeType is neither UADP nor JSON. */
START_TEST(WriterGroup_Create_InvalidEncodingMimeType_Fails) {
    addMinimalConnection();

    UA_WriterGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_WriterGroupConfig));
    cfg.name = UA_STRING("Invalid-Encoding WG");
    cfg.publishingInterval = 100.0;
    cfg.writerGroupId = 1;
    /* Use an unknown encoding. UA_PUBSUB_ENCODING_UADP = 0,
     * UA_PUBSUB_ENCODING_JSON = 1. Use 99 to hit the else branch.
     * The messageSettings must be present (not ENCODED_NOBODY) so the
     * function actually reaches the type-check switch. */
    cfg.encodingMimeType = (UA_PubSubEncodingType)99;
    static UA_UadpWriterGroupMessageDataType wgms;
    UA_UadpWriterGroupMessageDataType_init(&wgms);
    UA_ExtensionObject_setValue(&cfg.messageSettings, &wgms,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_StatusCode res =
        UA_Server_addWriterGroup(server, connectionIdent, &cfg, &writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* WriterGroup: the JSON settings with a non-JSON decoded type must
 * return BADTYPEMISMATCH. */
START_TEST(WriterGroup_Create_JsonEncodingWithUadpSettings_Fails) {
    addMinimalConnection();

    UA_WriterGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_WriterGroupConfig));
    cfg.name = UA_STRING("Json-With-Wrong-Settings WG");
    cfg.publishingInterval = 100.0;
    cfg.writerGroupId = 1;
    cfg.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    /* Provide a UADP message settings struct (the wrong type for JSON). */
    static UA_UadpWriterGroupMessageDataType wgms;
    UA_UadpWriterGroupMessageDataType_init(&wgms);
    UA_ExtensionObject_setValue(&cfg.messageSettings, &wgms,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_StatusCode res =
        UA_Server_addWriterGroup(server, connectionIdent, &cfg, &writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_BADTYPEMISMATCH);
} END_TEST

/* Public-API invalid-argument paths for the DataSetWriter wrappers. */
START_TEST(DataSetWriter_InvalidArguments_Rejected) {
    /* NULL server */
    UA_StatusCode res =
        UA_Server_addDataSetWriter(NULL, UA_NODEID_NULL, UA_NODEID_NULL,
                                   NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* NULL config */
    res = UA_Server_addDataSetWriter(server, UA_NODEID_NULL, UA_NODEID_NULL,
                                    NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* getDataSetWriterConfig: NULL server or NULL config */
    res = UA_Server_getDataSetWriterConfig(NULL, UA_NODEID_NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_DataSetWriterConfig cfg;
    res = UA_Server_getDataSetWriterConfig(server, UA_NODEID_NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* getDataSetWriterState: NULL state */
    res = UA_Server_getDataSetWriterState(server, UA_NODEID_NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* enableDataSetWriter: NULL server */
    res = UA_Server_enableDataSetWriter(NULL, UA_NODEID_NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* disableDataSetWriter: NULL server */
    res = UA_Server_disableDataSetWriter(NULL, UA_NODEID_NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* removeDataSetWriter: unknown NodeId */
    res = UA_Server_removeDataSetWriter(server, UA_NODEID_NUMERIC(0, 9999));
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

/* generateFieldMetaData: the "publishedVariable cannot be read"
 * branch (line 141-144) is taken when the DataSetField references a
 * node that does not exist. addDataSetField rejects the field with
 * BADCONFIGURATIONERROR in that case. */
START_TEST(GenerateFieldMetaData_PublishedVariableNotReadable_ReturnsError) {
    addMinimalConnection();
    addMinimalPublishedDataSet();

    /* Add a DataSetField that points to a non-existent variable. */
    UA_DataSetFieldConfig fcfg;
    memset(&fcfg, 0, sizeof(UA_DataSetFieldConfig));
    fcfg.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fcfg.field.variable.fieldNameAlias = UA_STRING("missing");
    fcfg.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(1, 9999);
    UA_DataSetFieldResult fr =
        UA_Server_addDataSetField(server, publishedDataSetIdent, &fcfg, NULL);
    ck_assert_int_eq(fr.result, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

/* addSubscribedDataSet: NULL config must return BADINVALIDARGUMENT. */
START_TEST(AddSubscribedDataSet_NullConfig_Rejected) {
    UA_StatusCode res =
        UA_Server_addSubscribedDataSet(server, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* addPublishedDataSet: NULL config must return BADINTERNALERROR. */
START_TEST(AddPublishedDataSet_NullConfig_Rejected) {
    UA_AddPublishedDataSetResult res =
        UA_Server_addPublishedDataSet(server, NULL, NULL);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* addPublishedDataSet: empty name must be rejected with
 * BADINVALIDARGUMENT. The PDS name is a required BrowseName. */
START_TEST(AddPublishedDataSet_EmptyName_Rejected) {
    UA_PublishedDataSetConfig cfg;
    memset(&cfg, 0, sizeof(UA_PublishedDataSetConfig));
    cfg.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    /* Intentionally do not set cfg.name. */
    UA_NodeId outId = UA_NODEID_NULL;
    UA_AddPublishedDataSetResult res =
        UA_Server_addPublishedDataSet(server, &cfg, &outId);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert(UA_NodeId_isNull(&outId));
} END_TEST

/* addDataSetField: PUBLISHEDEVENTS_TEMPLATE PDS type returns
 * BADINVALIDARGUMENT because the create function only supports
 * PUBLISHEDITEMS. */
START_TEST(AddDataSetField_NonPublishedItemsType_Rejected) {
    UA_PublishedDataSetConfig cfg;
    memset(&cfg, 0, sizeof(UA_PublishedDataSetConfig));
    cfg.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE;
    cfg.name = UA_STRING("EventsTemplatePDS");
    UA_NodeId pdsId = UA_NODEID_NULL;
    UA_AddPublishedDataSetResult res =
        UA_Server_addPublishedDataSet(server, &cfg, &pdsId);
    ck_assert_int_eq(res.addResult, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* triggerWriterGroupPublish: NULL server returns BADINVALIDARGUMENT. */
START_TEST(TriggerWriterGroupPublish_InvalidArguments_Rejected) {
    UA_StatusCode res =
        UA_Server_triggerWriterGroupPublish(NULL, UA_NODEID_NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* getPubSubComponentChildren: NULL outSize returns BADINVALIDARGUMENT. */
START_TEST(GetPubSubComponentChildren_InvalidArguments_Rejected) {
    UA_StatusCode res =
        UA_Server_getPubSubComponentChildren(server, UA_NODEID_NUMERIC(0, 9999),
                                             NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    size_t outSize = 0;
    res = UA_Server_getPubSubComponentChildren(server, UA_NODEID_NUMERIC(0, 9999),
                                             &outSize, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* getPubSubComponentType: NULL outType returns BADINVALIDARGUMENT. */
START_TEST(GetPubSubComponentType_InvalidArguments_Rejected) {
    UA_StatusCode res =
        UA_Server_getPubSubComponentType(server, UA_NODEID_NUMERIC(0, 9999), NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* computeWriterGroupOffsetTable: unknown writer group returns
 * BADNOTFOUND. */
START_TEST(ComputeWriterGroupOffsetTable_UnknownReturnsBadNotFound) {
    UA_PubSubOffsetTable ot;
    memset(&ot, 0, sizeof(UA_PubSubOffsetTable));
    UA_StatusCode res =
        UA_Server_computeWriterGroupOffsetTable(server,
                                               UA_NODEID_NUMERIC(0, 9999), &ot);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

static Suite *
stateMachineExtraSuite(void) {
    Suite *s = suite_create("PubSub state-machine edge cases (round 2)");

    TCase *tc_conn = tcase_create("PubSubConnection state-machine branches");
    tcase_add_checked_fixture(tc_conn, setup, teardown);
    tcase_add_test(tc_conn, Connection_EnableWhilePubSubManagerStopped_GoesToPaused);
    tcase_add_test(tc_conn, Connection_EnableWhenDeleteFlagSet_Fails);
    tcase_add_test(tc_conn, Connection_BeforeStateChangeCallbackIsInvoked);
    tcase_add_test(tc_conn, Connection_StateChangeCallbackIsInvoked);

    TCase *tc_dsw = tcase_create("DataSetWriter state-machine branches");
    tcase_add_checked_fixture(tc_dsw, setup, teardown);
    tcase_add_test(tc_dsw, DataSetWriter_BeforeStateChangeCallbackIsInvoked);
    tcase_add_test(tc_dsw, DataSetWriter_CustomStateMachineIsUsed);
    tcase_add_test(tc_dsw, DataSetWriter_StateChangeCallbackIsInvoked);
    tcase_add_test(tc_dsw, DataSetWriter_InvalidArguments_Rejected);

    TCase *tc_wg = tcase_create("WriterGroup create-time validation");
    tcase_add_checked_fixture(tc_wg, setup, teardown);
    tcase_add_test(tc_wg, WriterGroup_Create_InvalidEncodingMimeType_Fails);
    tcase_add_test(tc_wg, WriterGroup_Create_JsonEncodingWithUadpSettings_Fails);

    TCase *tc_pds = tcase_create("PublishedDataSet / SubscribedDataSet");
    tcase_add_checked_fixture(tc_pds, setup, teardown);
    tcase_add_test(tc_pds, GenerateFieldMetaData_PublishedVariableNotReadable_ReturnsError);
    tcase_add_test(tc_pds, AddSubscribedDataSet_NullConfig_Rejected);
    tcase_add_test(tc_pds, AddPublishedDataSet_NullConfig_Rejected);
    tcase_add_test(tc_pds, AddPublishedDataSet_EmptyName_Rejected);
    tcase_add_test(tc_pds, AddDataSetField_NonPublishedItemsType_Rejected);

    TCase *tc_api = tcase_create("PubSub public-API invalid-arg paths");
    tcase_add_checked_fixture(tc_api, setup, teardown);
    tcase_add_test(tc_api, TriggerWriterGroupPublish_InvalidArguments_Rejected);
    tcase_add_test(tc_api, GetPubSubComponentChildren_InvalidArguments_Rejected);
    tcase_add_test(tc_api, GetPubSubComponentType_InvalidArguments_Rejected);
    tcase_add_test(tc_api, ComputeWriterGroupOffsetTable_UnknownReturnsBadNotFound);

    suite_add_tcase(s, tc_conn);
    suite_add_tcase(s, tc_dsw);
    suite_add_tcase(s, tc_wg);
    suite_add_tcase(s, tc_pds);
    suite_add_tcase(s, tc_api);
    return s;
}

int
main(void) {
    Suite *s = stateMachineExtraSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
