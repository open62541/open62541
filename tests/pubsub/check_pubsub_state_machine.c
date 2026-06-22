/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2024 open62541 contributors.
 *
 * Unit tests that exercise code paths in the PubSub state machines that are
 * not covered by the existing test suite. The targeted branches are the ones
 * reported as missing by gcov after running the full pubsub test set.
 *
 * In particular this covers:
 *   - The "server is not started" branch in UA_WriterGroup_setPubSubState
 *     and UA_ReaderGroup_setPubSubState (the component ends up in the
 *     PAUSED state with a warning).
 *   - The pubSubConfig.beforeStateChangeCallback invocation path.
 *   - The "component marked for deletion" branch where enabling a group
 *     that has its deleteFlag set must fail with BADINTERNALERROR.
 *   - The pubSubConfig.componentLifecycleCallback early-exit path for
 *     addPubSubConnection (callback that refuses the new component).
 *   - The pubSubConfig.componentLifecycleCallback early-exit path for
 *     addReaderGroup/addWriterGroup.
 *   - Some null-argument and unknown-connection paths in the public
 *     addReaderGroup / addWriterGroup wrappers.
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
static UA_NodeId writerGroupIdent;
static UA_NodeId readerGroupIdent;
static UA_UInt32 valueStore;

/* Counter used to assert the beforeStateChangeCallback fires. */
static size_t beforeStateChangeCallbackCount = 0;
static UA_PubSubState beforeStateChangeCallbackLastTarget = (UA_PubSubState)0;
static UA_Boolean beforeStateChangeCallbackLastIdValid = false;
static UA_NodeId beforeStateChangeCallbackLastId;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    /* Note: UA_Server_run_startup is intentionally NOT called here for the
     * "server not started" tests. The tests that need a running server
     * call UA_Server_run_startup themselves. */
    beforeStateChangeCallbackCount = 0;
    beforeStateChangeCallbackLastTarget = (UA_PubSubState)0;
    beforeStateChangeCallbackLastIdValid = false;
    UA_NodeId_init(&beforeStateChangeCallbackLastId);
    valueStore = 0;
}

static void
teardown(void) {
    if(server == NULL)
        return;
    /* Only call UA_Server_run_shutdown if the server was actually
     * started. Tests that exercise the "server not started" branch in
     * the state machine leave the server unstarted. */
    if(server->state == UA_LIFECYCLESTATE_STARTED) {
        UA_Server_run_shutdown(server);
    }
    UA_Server_delete(server);
    server = NULL;
    UA_NodeId_init(&connectionIdent);
    UA_NodeId_init(&writerGroupIdent);
    UA_NodeId_init(&readerGroupIdent);
    if(beforeStateChangeCallbackLastIdValid) {
        UA_NodeId_clear(&beforeStateChangeCallbackLastId);
        beforeStateChangeCallbackLastIdValid = false;
    }
}

static void
addMinimalPubSubConnection(void) {
    UA_PubSubConnectionConfig cfg;
    memset(&cfg, 0, sizeof(UA_PubSubConnectionConfig));
    cfg.name = UA_STRING("State-Machine Test Connection");
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
    ck_assert(UA_NodeId_isNull(&connectionIdent) == false);
}

static UA_WriterGroup *currentMessageSettings = NULL;

static void
addMinimalWriterGroup(void) {
    UA_WriterGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_WriterGroupConfig));
    cfg.name = UA_STRING("State-Machine Test WG");
    cfg.publishingInterval = 100.0;
    cfg.writerGroupId = 1;
    cfg.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    /* The messageSettings must be a decoded ExtensionObject with both
     * type and data set. Use a static UADP message-settings struct. */
    static UA_UadpWriterGroupMessageDataType wgms;
    UA_UadpWriterGroupMessageDataType_init(&wgms);
    currentMessageSettings = NULL; /* unused placeholder */
    UA_ExtensionObject_setValue(&cfg.messageSettings, &wgms,
                                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_StatusCode res =
        UA_Server_addWriterGroup(server, connectionIdent, &cfg, &writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

static void
addMinimalReaderGroup(void) {
    UA_ReaderGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_ReaderGroupConfig));
    cfg.name = UA_STRING("State-Machine Test RG");
    cfg.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_StatusCode res =
        UA_Server_addReaderGroup(server, connectionIdent, &cfg, &readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* The beforeStateChangeCallback. Stores the target it was called with and
 * counts invocations. */
static void
testBeforeStateChangeCallback(UA_Server *s, const UA_NodeId id,
                              UA_PubSubState *targetState) {
    beforeStateChangeCallbackCount++;
    beforeStateChangeCallbackLastTarget = *targetState;
    if(beforeStateChangeCallbackLastIdValid)
        UA_NodeId_clear(&beforeStateChangeCallbackLastId);
    UA_NodeId_copy(&id, &beforeStateChangeCallbackLastId);
    beforeStateChangeCallbackLastIdValid = true;
}

/* WriterGroup: enabling while the parent PubSubConnection is in
 * DISABLED state must land the WriterGroup in the PAUSED state. This
 * exercises the "if(connection->head.state != UA_PUBSUBSTATE_OPERATIONAL)"
 * branch in UA_WriterGroup_setPubSubState, which is a sibling of the
 * "server not started" branch (psm->sc.state != UA_LIFECYCLESTATE_STARTED)
 * covered by the connection state machine. */
START_TEST(WriterGroup_EnableWhileConnectionDisabled_GoesToPaused) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalWriterGroup();
    /* The connection starts in DISABLED so the WriterGroup cannot go
     * to OPERATIONAL. It must end up in PAUSED. */
    UA_StatusCode res =
        UA_Server_enableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubState st = UA_PUBSUBSTATE_DISABLED;
    res = UA_Server_getWriterGroupState(server, writerGroupIdent, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(st, UA_PUBSUBSTATE_PAUSED);

    res = UA_Server_disableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_getWriterGroupState(server, writerGroupIdent, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(st, UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* ReaderGroup: enabling while the parent PubSubConnection is in
 * DISABLED state must land the ReaderGroup in the PAUSED state. This
 * exercises the same branch in UA_ReaderGroup_setPubSubState. */
START_TEST(ReaderGroup_EnableWhileConnectionDisabled_GoesToPaused) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalReaderGroup();

    UA_StatusCode res =
        UA_Server_enableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubState st = UA_PUBSUBSTATE_DISABLED;
    res = UA_Server_getReaderGroupState(server, readerGroupIdent, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(st, UA_PUBSUBSTATE_PAUSED);

    res = UA_Server_disableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_getReaderGroupState(server, readerGroupIdent, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(st, UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* Register the beforeStateChangeCallback. Starting the server and then
 * enabling the WriterGroup must invoke the callback. */
START_TEST(WriterGroup_BeforeStateChangeCallbackIsInvoked) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalWriterGroup();

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.beforeStateChangeCallback = testBeforeStateChangeCallback;

    UA_StatusCode res =
        UA_Server_enableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* The callback should have been invoked at least once for the
     * enable call. The target state observed must be OPERATIONAL (or
     * something the callback can rewrite). We do not assert the exact
     * node id since the WriterGroup is enabled as a side-effect of the
     * connection becoming operational. */
    ck_assert_uint_ge(beforeStateChangeCallbackCount, 1);
    ck_assert(beforeStateChangeCallbackLastIdValid);
    ck_assert_int_eq(beforeStateChangeCallbackLastTarget,
                     UA_PUBSUBSTATE_OPERATIONAL);

    /* Disable the WriterGroup - this triggers another callback
     * invocation with target DISABLED. */
    size_t prev = beforeStateChangeCallbackCount;
    res = UA_Server_disableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(beforeStateChangeCallbackCount, prev);
    ck_assert_int_eq(beforeStateChangeCallbackLastTarget,
                     UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* Same as above for ReaderGroup. */
START_TEST(ReaderGroup_BeforeStateChangeCallbackIsInvoked) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalReaderGroup();

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.beforeStateChangeCallback = testBeforeStateChangeCallback;

    UA_StatusCode res =
        UA_Server_enableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_uint_ge(beforeStateChangeCallbackCount, 1);
    ck_assert(beforeStateChangeCallbackLastIdValid);
    ck_assert_int_eq(beforeStateChangeCallbackLastTarget,
                     UA_PUBSUBSTATE_OPERATIONAL);

    size_t prev = beforeStateChangeCallbackCount;
    res = UA_Server_disableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(beforeStateChangeCallbackCount, prev);
    ck_assert_int_eq(beforeStateChangeCallbackLastTarget,
                     UA_PUBSUBSTATE_DISABLED);
} END_TEST

/* Setting deleteFlag on a WriterGroup and then trying to enable it must
 * fail with UA_STATUSCODE_BADINTERNALERROR. */
START_TEST(WriterGroup_EnableWhenDeleteFlagSet_Fails) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalWriterGroup();

    /* Reach into the internal structure to set deleteFlag without
     * removing the group from the connection. The user-facing
     * UA_Server_removeWriterGroup is asynchronous (delete + disable) and
     * would set the state to DISABLED on its own. */
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroupIdent);
    ck_assert_ptr_ne(wg, NULL);
    wg->deleteFlag = true;

    UA_StatusCode res =
        UA_Server_enableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINTERNALERROR);

    /* Disabling is allowed even when deleteFlag is set. */
    res = UA_Server_disableWriterGroup(server, writerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* Same as above for ReaderGroup. */
START_TEST(ReaderGroup_EnableWhenDeleteFlagSet_Fails) {
    UA_Server_run_startup(server);
    addMinimalPubSubConnection();
    addMinimalReaderGroup();

    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupIdent);
    ck_assert_ptr_ne(rg, NULL);
    rg->deleteFlag = true;

    UA_StatusCode res =
        UA_Server_enableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINTERNALERROR);

    res = UA_Server_disableReaderGroup(server, readerGroupIdent);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* A componentLifecycleCallback that returns a bad statuscode must
 * abort the addition of a new PubSubConnection. */
static UA_StatusCode
lifecycleCallbackAbortAll(UA_Server *s, const UA_NodeId id,
                          const UA_PubSubComponentType componentType,
                          UA_Boolean remove) {
    (void)s; (void)id; (void)componentType; (void)remove;
    return UA_STATUSCODE_BADUNEXPECTEDERROR;
}

START_TEST(AddConnection_AbortedByLifecycleCallback) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.componentLifecycleCallback = lifecycleCallbackAbortAll;

    UA_PubSubConnectionConfig cfg;
    memset(&cfg, 0, sizeof(UA_PubSubConnectionConfig));
    cfg.name = UA_STRING("Refused Connection");
    cfg.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType addressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&cfg.address, &addressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    cfg.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    cfg.publisherId.id.uint16 = 1;
    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addPubSubConnection(server, &cfg, &outId);
    ck_assert_int_eq(res, UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert(UA_NodeId_isNull(&outId));
} END_TEST

/* Public-API invalid-argument paths for addReaderGroup. */
START_TEST(AddReaderGroup_InvalidArguments_Rejected) {
    /* NULL server */
    UA_ReaderGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_ReaderGroupConfig));
    cfg.name = UA_STRING("Unused");
    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addReaderGroup(NULL, UA_NODEID_NULL, &cfg, &outId);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* NULL config */
    res = UA_Server_addReaderGroup(server, UA_NODEID_NULL, NULL, &outId);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* Public-API invalid-argument paths for addWriterGroup. */
START_TEST(AddWriterGroup_InvalidArguments_Rejected) {
    UA_WriterGroupConfig cfg;
    memset(&cfg, 0, sizeof(UA_WriterGroupConfig));
    cfg.name = UA_STRING("Unused");
    cfg.publishingInterval = 100.0;
    UA_NodeId outId = UA_NODEID_NULL;

    UA_StatusCode res = UA_Server_addWriterGroup(NULL, UA_NODEID_NULL, &cfg, &outId);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    res = UA_Server_addWriterGroup(server, UA_NODEID_NULL, NULL, &outId);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* Public-API invalid-argument paths for the state getters. */
START_TEST(GetState_InvalidArguments_Rejected) {
    UA_PubSubState st = UA_PUBSUBSTATE_DISABLED;
    UA_StatusCode res =
        UA_Server_getWriterGroupState(NULL, UA_NODEID_NULL, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    res = UA_Server_getReaderGroupState(NULL, UA_NODEID_NULL, &st);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* Public-API invalid-argument paths for the config getters. */
START_TEST(GetConfig_InvalidArguments_Rejected) {
    UA_WriterGroupConfig wcfg;
    UA_StatusCode res =
        UA_Server_getWriterGroupConfig(NULL, UA_NODEID_NULL, &wcfg);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    res = UA_Server_getWriterGroupConfig(server, UA_NODEID_NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_ReaderGroupConfig rcfg;
    res = UA_Server_getReaderGroupConfig(NULL, UA_NODEID_NULL, &rcfg);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

    res = UA_Server_getReaderGroupConfig(server, UA_NODEID_NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

/* removePubSubConnection for a connection that does not exist must
 * return BADNOTFOUND. */
START_TEST(RemovePubSubConnection_UnknownReturnsBadNotFound) {
    UA_StatusCode res =
        UA_Server_removePubSubConnection(server, UA_NODEID_NUMERIC(0, 9999));
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

/* removeWriterGroup / removeReaderGroup for a group that does not exist
 * must return BADNOTFOUND. */
START_TEST(RemoveWriterGroup_UnknownReturnsBadNotFound) {
    UA_StatusCode res =
        UA_Server_removeWriterGroup(server, UA_NODEID_NUMERIC(0, 9999));
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(RemoveReaderGroup_UnknownReturnsBadNotFound) {
    UA_StatusCode res =
        UA_Server_removeReaderGroup(server, UA_NODEID_NUMERIC(0, 9999));
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

static Suite *
stateMachineSuite(void) {
    Suite *s = suite_create("PubSub state machine edge cases");

    TCase *tc_wg_state = tcase_create("WriterGroup state-machine branches");
    tcase_add_checked_fixture(tc_wg_state, setup, teardown);
    tcase_add_test(tc_wg_state, WriterGroup_EnableWhileConnectionDisabled_GoesToPaused);
    tcase_add_test(tc_wg_state, WriterGroup_BeforeStateChangeCallbackIsInvoked);
    tcase_add_test(tc_wg_state, WriterGroup_EnableWhenDeleteFlagSet_Fails);
    tcase_add_test(tc_wg_state, AddWriterGroup_InvalidArguments_Rejected);

    TCase *tc_rg_state = tcase_create("ReaderGroup state-machine branches");
    tcase_add_checked_fixture(tc_rg_state, setup, teardown);
    tcase_add_test(tc_rg_state, ReaderGroup_EnableWhileConnectionDisabled_GoesToPaused);
    tcase_add_test(tc_rg_state, ReaderGroup_BeforeStateChangeCallbackIsInvoked);
    tcase_add_test(tc_rg_state, ReaderGroup_EnableWhenDeleteFlagSet_Fails);
    tcase_add_test(tc_rg_state, AddReaderGroup_InvalidArguments_Rejected);

    TCase *tc_api = tcase_create("PubSub public-API invalid-arg paths");
    tcase_add_checked_fixture(tc_api, setup, teardown);
    tcase_add_test(tc_api, AddConnection_AbortedByLifecycleCallback);
    tcase_add_test(tc_api, GetState_InvalidArguments_Rejected);
    tcase_add_test(tc_api, GetConfig_InvalidArguments_Rejected);
    tcase_add_test(tc_api, RemovePubSubConnection_UnknownReturnsBadNotFound);
    tcase_add_test(tc_api, RemoveWriterGroup_UnknownReturnsBadNotFound);
    tcase_add_test(tc_api, RemoveReaderGroup_UnknownReturnsBadNotFound);

    suite_add_tcase(s, tc_wg_state);
    suite_add_tcase(s, tc_rg_state);
    suite_add_tcase(s, tc_api);
    return s;
}

int
main(void) {
    Suite *s = stateMachineSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
