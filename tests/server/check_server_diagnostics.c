/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Server diagnostics and service dispatch tests */

#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

/* --- Server Diagnostics --- */

START_TEST(readServerDiagnosticsSummary) {
    /* Read the ServerDiagnosticsSummary node if diagnostics is enabled */
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY),
        &value);
    /* May succeed or fail depending on UA_ENABLE_DIAGNOSTICS */
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerDiagnosticsCurrentSessionCount) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSESSIONCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD) {
        if(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]))
            ck_assert_uint_eq(*(UA_UInt32*)value.data, 0);
        UA_Variant_clear(&value);
    }
} END_TEST

START_TEST(readServerDiagnosticsCurrentSubscriptionCount) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_CURRENTSUBSCRIPTIONCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD) {
        if(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]))
            ck_assert_uint_eq(*(UA_UInt32*)value.data, 0);
        UA_Variant_clear(&value);
    }
} END_TEST

START_TEST(readServerDiagnosticsPublishingIntervalCount) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_PUBLISHINGINTERVALCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerDiagnosticsSecurityRejected) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SECURITYREJECTEDSESSIONCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD) {
        if(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]))
            ck_assert_uint_eq(*(UA_UInt32*)value.data, 0);
        UA_Variant_clear(&value);
    }
} END_TEST

START_TEST(readServerDiagnosticsRejectedSessionCount) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_REJECTEDSESSIONCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerDiagnosticsSessionAbortCount) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY_SESSIONABORTCOUNT),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(checkSessionDiagArray) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONDIAGNOSTICSARRAY),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(checkSessionSecurityDiagArray) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SESSIONSDIAGNOSTICSSUMMARY_SESSIONSECURITYDIAGNOSTICSARRAY),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

START_TEST(checkSubscriptionDiagArray) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY),
        &value);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&value);
} END_TEST

/* --- Server Status and Info Nodes --- */

START_TEST(readServerStatus) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO
START_TEST(readServerStatusShutdownReasonSnapshot) {
    const UA_NodeId reasonId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SHUTDOWNREASON);
    const UA_NodeId statusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_UInt32 seconds = 5;
    UA_Variant value;
    UA_Variant_setScalar(&value, &seconds, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode retval = UA_Server_writeValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText reasons[3];
    reasons[0] = UA_LOCALIZEDTEXT("en", "Scheduled maintenance");
    reasons[1] = UA_LOCALIZEDTEXT("de", "Wartung");
    reasons[2] = UA_LOCALIZEDTEXT("", "");
    for(size_t i = 0; i < sizeof(reasons) / sizeof(reasons[0]); i++) {
        UA_Variant_setScalar(&value, &reasons[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_writeValue(server, reasonId, value);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_Variant statusValue;
        retval = UA_Server_readValue(server, statusId, &statusValue);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_hasScalarType(&statusValue, &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE]));
        UA_ServerStatusDataType *status = (UA_ServerStatusDataType*)statusValue.data;
        ck_assert(UA_LocalizedText_equal(&status->shutdownReason, &reasons[i]));
        ck_assert_int_eq(status->state, UA_SERVERSTATE_SHUTDOWN);
        ck_assert_uint_eq(status->secondsTillShutdown, seconds);
        ck_assert(UA_BuildInfo_equal(&status->buildInfo, &UA_Server_getConfig(server)->buildInfo));

        /* A status snapshot owns its reason even after the node is overwritten. */
        UA_LocalizedText empty = UA_LOCALIZEDTEXT("", "");
        UA_Variant_setScalar(&value, &empty, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_writeValue(server, reasonId, value);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(UA_LocalizedText_equal(&status->shutdownReason, &reasons[i]));
        UA_Variant_clear(&statusValue);
    }
} END_TEST
#endif

START_TEST(readServerStatusState) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    if(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_SERVERSTATE])) {
        UA_ServerState state = *(UA_ServerState*)value.data;
        ck_assert_int_eq(state, UA_SERVERSTATE_RUNNING);
    }
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerStatusBuildInfo) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerStatusStartTime) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerStatusCurrentTime) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerStatusSecondsTillShutdown) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO
/* Initiates the shutdown like UA_Server_runUntilInterrupt does on SIGINT */
static void
initiateShutdown(UA_UInt32 seconds) {
    UA_Variant value;
    UA_Variant_setScalar(&value, &seconds, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode res = UA_Server_writeValue(server,
        UA_NS0ID(SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

/* Part 5, 12.10: Once the shutdown is initiated, the State is SHUTDOWN_4 and
 * SecondsTillShutdown counts down until the actual shutdown */
START_TEST(readServerStatusDuringShutdown) {
    initiateShutdown(10);
    UA_fakeSleep(4000);

    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NS0ID(SERVER_SERVERSTATUS_STATE), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_SERVERSTATE]));
    ck_assert_int_eq(*(UA_ServerState*)value.data, UA_SERVERSTATE_SHUTDOWN);
    UA_Variant_clear(&value);

    retval = UA_Server_readValue(server,
        UA_NS0ID(SERVER_SERVERSTATUS_SECONDSTILLSHUTDOWN), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 6);
    UA_Variant_clear(&value);

    retval = UA_Server_readValue(server, UA_NS0ID(SERVER_SERVERSTATUS), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                       &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE]));
    UA_ServerStatusDataType *status = (UA_ServerStatusDataType*)value.data;
    ck_assert_int_eq(status->state, UA_SERVERSTATE_SHUTDOWN);
    ck_assert_uint_eq(status->secondsTillShutdown, 6);
    UA_Variant_clear(&value);
} END_TEST

/* Part 5, 12.10: The ServerStatus structure has the ShutdownReason that the
 * application set in the ShutdownReason Variable */
START_TEST(readServerStatusShutdownReason) {
    initiateShutdown(10);
    UA_LocalizedText reason = UA_LOCALIZEDTEXT("en-US", "Maintenance");
    UA_Variant value;
    UA_Variant_setScalar(&value, &reason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_StatusCode retval = UA_Server_writeValue(server,
        UA_NS0ID(SERVER_SERVERSTATUS_SHUTDOWNREASON), value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_readValue(server,
        UA_NS0ID(SERVER_SERVERSTATUS_SHUTDOWNREASON), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    ck_assert(UA_equal(value.data, &reason, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    UA_Variant_clear(&value);

    retval = UA_Server_readValue(server, UA_NS0ID(SERVER_SERVERSTATUS), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                       &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE]));
    UA_ServerStatusDataType *status = (UA_ServerStatusDataType*)value.data;
    ck_assert_int_eq(status->state, UA_SERVERSTATE_SHUTDOWN);
    ck_assert(UA_equal(&status->shutdownReason, &reason,
                       &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    UA_Variant_clear(&value);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_ServerState lastServerState;
static size_t serverStateNotifications;

static void
serverStateChanged(UA_Server *s, UA_UInt32 monId, void *monContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_UInt32 attributeId, const UA_DataValue *value) {
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_SERVERSTATE]))
        lastServerState = *(UA_ServerState*)value->value.data;
    serverStateNotifications++;
}

/* A MonitoredItem on the State reports the transition to SHUTDOWN_4 */
START_TEST(monitorServerStateDuringShutdown) {
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    serverStateNotifications = 0;
    lastServerState = UA_SERVERSTATE_UNKNOWN;
    UA_MonitoredItemCreateRequest item =
        UA_MonitoredItemCreateRequest_default(UA_NS0ID(SERVER_SERVERSTATUS_STATE));
    item.requestedParameters.samplingInterval = 100.0;
    UA_MonitoredItemCreateResult res =
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_NEITHER,
                                                item, NULL, serverStateChanged);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);

    UA_fakeSleep(200);
    UA_Server_run_iterate(server, false);
    ck_assert_int_eq(lastServerState, UA_SERVERSTATE_RUNNING);
    size_t runningNotifications = serverStateNotifications;

    initiateShutdown(10);
    UA_fakeSleep(200);
    UA_Server_run_iterate(server, false);
    ck_assert_int_eq(lastServerState, UA_SERVERSTATE_SHUTDOWN);
    ck_assert_uint_gt(serverStateNotifications, runningNotifications);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */
#endif /* UA_GENERATED_NAMESPACE_ZERO */

START_TEST(readServerCapabilities) {
    /* MaxBrowseContinuationPoints */
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    /* MaxQueryContinuationPoints */
    retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXQUERYCONTINUATIONPOINTS),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    /* MaxHistoryContinuationPoints */
    retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0,
            UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXHISTORYCONTINUATIONPOINTS),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readNamespaceArray) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(value.type == &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_gt(value.arrayLength, 0);
    UA_Variant_clear(&value);
} END_TEST

START_TEST(readServerArray) {
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY),
        &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

/* --- Service-level read via ReadValueId --- */

START_TEST(readViaReadValueId) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert_uint_eq(resp.status, UA_STATUSCODE_GOOD);
    ck_assert(resp.hasValue);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(readViaReadValueIdTimestampSource) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_SOURCE);
    ck_assert_uint_eq(resp.status, UA_STATUSCODE_GOOD);
    UA_DataValue_clear(&resp);

    resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_SERVER);
    ck_assert_uint_eq(resp.status, UA_STATUSCODE_GOOD);
    UA_DataValue_clear(&resp);

    resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(resp.status, UA_STATUSCODE_GOOD);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(readBadNodeId) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, 99999);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert_uint_ne(resp.status, UA_STATUSCODE_GOOD);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(readBadAttributeId) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE; /* Server is an Object, no value */

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert_uint_ne(resp.status, UA_STATUSCODE_GOOD);
    UA_DataValue_clear(&resp);
} END_TEST

/* --- Write via WriteValue --- */

START_TEST(writeViaWriteValue) {
    /* Add a writable variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 10;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myId = UA_NODEID_STRING(1, "diag.test.writevalue");
    UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DiagWriteVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = myId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasValue = true;
    UA_Int32 newVal = 99;
    UA_Variant_setScalar(&wv.value.value, &newVal, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode retval = UA_Server_write(server, &wv);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)readVal.data, 99);
    UA_Variant_clear(&readVal);
} END_TEST

int main(void) {
    Suite *s = suite_create("server_diagnostics");

    TCase *tc_diag = tcase_create("Diagnostics");
    tcase_add_checked_fixture(tc_diag, setup, teardown);
    tcase_add_test(tc_diag, readServerDiagnosticsSummary);
    tcase_add_test(tc_diag, readServerDiagnosticsCurrentSessionCount);
    tcase_add_test(tc_diag, readServerDiagnosticsCurrentSubscriptionCount);
    tcase_add_test(tc_diag, readServerDiagnosticsPublishingIntervalCount);
    tcase_add_test(tc_diag, readServerDiagnosticsSecurityRejected);
    tcase_add_test(tc_diag, readServerDiagnosticsRejectedSessionCount);
    tcase_add_test(tc_diag, readServerDiagnosticsSessionAbortCount);
    tcase_add_test(tc_diag, checkSessionDiagArray);
    tcase_add_test(tc_diag, checkSessionSecurityDiagArray);
    tcase_add_test(tc_diag, checkSubscriptionDiagArray);
    suite_add_tcase(s, tc_diag);

    TCase *tc_status = tcase_create("ServerStatus");
    tcase_add_checked_fixture(tc_status, setup, teardown);
    tcase_add_test(tc_status, readServerStatus);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    tcase_add_test(tc_status, readServerStatusShutdownReasonSnapshot);
#endif
    tcase_add_test(tc_status, readServerStatusState);
    tcase_add_test(tc_status, readServerStatusBuildInfo);
    tcase_add_test(tc_status, readServerStatusStartTime);
    tcase_add_test(tc_status, readServerStatusCurrentTime);
    tcase_add_test(tc_status, readServerStatusSecondsTillShutdown);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    tcase_add_test(tc_status, readServerStatusDuringShutdown);
    tcase_add_test(tc_status, readServerStatusShutdownReason);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_status, monitorServerStateDuringShutdown);
#endif
#endif
    tcase_add_test(tc_status, readServerCapabilities);
    tcase_add_test(tc_status, readNamespaceArray);
    tcase_add_test(tc_status, readServerArray);
    suite_add_tcase(s, tc_status);

    TCase *tc_rw = tcase_create("ReadWrite");
    tcase_add_checked_fixture(tc_rw, setup, teardown);
    tcase_add_test(tc_rw, readViaReadValueId);
    tcase_add_test(tc_rw, readViaReadValueIdTimestampSource);
    tcase_add_test(tc_rw, readBadNodeId);
    tcase_add_test(tc_rw, readBadAttributeId);
    tcase_add_test(tc_rw, writeViaWriteValue);
    suite_add_tcase(s, tc_rw);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
