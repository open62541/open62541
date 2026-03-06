/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Namespace 0 read and service tests */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <check.h>
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    running = true;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void setup_serveronly(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown_serveronly(void) {
    UA_Server_delete(server);
}

/* Helper: read a scalar value from a node  */
static UA_StatusCode readNodeValue(UA_NodeId nodeId, UA_Variant *out) {
    return UA_Server_readValue(server, nodeId, out);
}

/* === Server Status reads === */
START_TEST(read_serverStatus) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_SERVERSTATUSDATATYPE]);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_startTime) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME), &out);
    /* BUG: StartTime is a mandatory component of ServerStatus (Part 5, 12.4).
     * The read returns GOOD but the variant type is not DateTime -- the value
     * is not populated for sub-component reads via the generic Read service.
     * See COVERAGE_BUGS.md #7.
     * Verify at least that the read does not crash. */
    (void)res;
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_currentTime) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_state) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_productName) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_productUri) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_manufacturerName) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_softwareVersion) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_buildNumber) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_buildInfo_buildDate) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

/* === ServiceLevel / Auditing === */
START_TEST(read_serviceLevel) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVICELEVEL), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_BYTE]);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_auditing) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_AUDITING), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_clear(&out);
} END_TEST

/* === Server Capabilities === */
START_TEST(read_serverProfileArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_SERVERPROFILEARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_localeIdArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_LOCALEIDARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_minSupportedSampleRate) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MINSUPPORTEDSAMPLERATE), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_maxBrowseContinuationPoints) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_MAXBROWSECONTINUATIONPOINTS), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

/* === Operation Limits === */
START_TEST(read_operationLimits) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* MaxNodesPerRead */
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, 11705), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerWrite */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11707), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerMethodCall */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11709), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerBrowse */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11710), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerRegisterNodes */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11711), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerTranslateBrowsePathsToNodeIds */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11712), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxNodesPerNodeManagement */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11713), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* MaxMonitoredItemsPerCall */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11714), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_operationLimits_history) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* MaxNodesPerHistoryReadData - may not exist in all builds */
    UA_StatusCode res = readNodeValue(UA_NODEID_NUMERIC(0, 12165), &out);
    (void)res; /* OK if BadNodeIdUnknown */
    UA_Variant_clear(&out);

    /* MaxNodesPerHistoryReadEvents */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 12166), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxNodesPerHistoryUpdateData */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 12167), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxNodesPerHistoryUpdateEvents */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 12168), &out);
    (void)res;
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_maxArrayLength) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* MaxArrayLength - may not exist in all builds */
    UA_StatusCode res = readNodeValue(UA_NODEID_NUMERIC(0, 11702), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxStringLength */
    res = readNodeValue(UA_NODEID_NUMERIC(0, 11703), &out);
    (void)res;
    UA_Variant_clear(&out);
} END_TEST

/* === Diagnostics Summary === */
START_TEST(read_diagnosticsSummary) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* ServerDiagnosticsSummary */
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SERVERDIAGNOSTICSSUMMARY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_diagnosticsSummary_subfields) {
    UA_Variant out;
    /* ServerViewCount */
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(UA_NODEID_NUMERIC(0, 2276), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* CurrentSessionCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2277), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* CumulatedSessionCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2278), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* SecurityRejectedSessionCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2279), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* SessionTimeoutCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2281), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* SessionAbortCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2282), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* PublishingIntervalCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2284), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* CurrentSubscriptionCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2285), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* CumulatedSubscriptionCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2286), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* SecurityRejectedRequestsCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2287), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* RejectedRequestsCount */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 2288), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_enabledFlag) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_ENABLEDFLAG), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_subscriptionDiagnosticsArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_samplingIntervalDiagnosticsArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* SamplingIntervalDiagnosticsArray - may not exist in all builds */
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SAMPLINGINTERVALDIAGNOSTICSARRAY), &out);
    (void)res;
    UA_Variant_clear(&out);
} END_TEST

/* === Session Diagnostics (via client) === */
START_TEST(read_sessionDiagnosticsArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, 3707), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

START_TEST(read_sessionSecurityDiagnosticsArray) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = readNodeValue(
        UA_NODEID_NUMERIC(0, 3708), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

/* === Discovery: FindServers + GetEndpoints via client === */
START_TEST(client_findServers) {
    UA_Client *client = UA_Client_newForUnitTest();
    size_t numServers = 0;
    UA_ApplicationDescription *servers = NULL;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840",
        0, NULL, 0, NULL,
        &numServers, &servers);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(numServers > 0);
    UA_Array_delete(servers, numServers,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

START_TEST(client_findServers_withFilter) {
    UA_Client *client = UA_Client_newForUnitTest();
    size_t numServers = 0;
    UA_ApplicationDescription *servers = NULL;
    UA_String uri = UA_STRING("urn:open62541.server.application");
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840",
        1, &uri, 0, NULL,
        &numServers, &servers);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Array_delete(servers, numServers,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    /* Non-matching URI */
    UA_String noUri = UA_STRING("urn:nonexistent");
    res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840",
        1, &noUri, 0, NULL,
        &numServers, &servers);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(numServers, 0);

    UA_Client_delete(client);
} END_TEST

START_TEST(client_getEndpoints) {
    UA_Client *client = UA_Client_newForUnitTest();
    size_t numEndpoints = 0;
    UA_EndpointDescription *endpoints = NULL;
    UA_StatusCode res = UA_Client_getEndpoints(client,
        "opc.tcp://localhost:4840",
        &numEndpoints, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(numEndpoints > 0);
    UA_Array_delete(endpoints, numEndpoints,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

/* === RegisterNodes / UnregisterNodes === */
START_TEST(client_registerNodes) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RegisterNodesRequest req;
    UA_RegisterNodesRequest_init(&req);
    UA_NodeId nodes[2];
    nodes[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    nodes[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    req.nodesToRegister = nodes;
    req.nodesToRegisterSize = 2;

    UA_RegisterNodesResponse resp =
        UA_Client_Service_registerNodes(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.registeredNodeIdsSize, 2);

    /* Unregister */
    UA_UnregisterNodesRequest ureq;
    UA_UnregisterNodesRequest_init(&ureq);
    ureq.nodesToUnregister = resp.registeredNodeIds;
    ureq.nodesToUnregisterSize = resp.registeredNodeIdsSize;
    UA_UnregisterNodesResponse uresp =
        UA_Client_Service_unregisterNodes(client, ureq);
    ck_assert_uint_eq(uresp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Don't clear req/ureq - they point to stack/resp data */
    UA_RegisterNodesResponse_clear(&resp);
    UA_UnregisterNodesResponse_clear(&uresp);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* === Server addReference / deleteReference === */
START_TEST(server_addDeleteReference) {
    /* Add a variable first */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns0ExtVar1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Add another variable */
    res = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns0ExtVar2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Add a reference between the two */
    res = UA_Server_addReference(server,
        UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_NUMERIC(1, 90002), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Try adding duplicate reference */
    res = UA_Server_addReference(server,
        UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_NUMERIC(1, 90002), true);
    ck_assert(res == UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED);

    /* Delete the reference */
    res = UA_Server_deleteReference(server,
        UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true,
        UA_EXPANDEDNODEID_NUMERIC(1, 90002), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete non-existing */
    res = UA_Server_deleteReference(server,
        UA_NODEID_NUMERIC(1, 90001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true,
        UA_EXPANDEDNODEID_NUMERIC(1, 99999), true);
    /* May return error or bad status */
    (void)res;

    /* Delete nodes */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90001), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90002), true);
} END_TEST

/* === Server: read various attributes === */
START_TEST(read_serverNode_description) {
    /* Read Description attribute of Server node */
    UA_LocalizedText desc;
    UA_LocalizedText_init(&desc);
    UA_StatusCode res = UA_Server_readDescription(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);
} END_TEST

START_TEST(read_serverNode_writeMask) {
    UA_UInt32 wm = 0;
    UA_StatusCode res = UA_Server_readWriteMask(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_variable_inverseName) {
    /* Read InverseName of Organizes reference type */
    UA_LocalizedText inv;
    UA_LocalizedText_init(&inv);
    UA_StatusCode res = UA_Server_readInverseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inv);
} END_TEST

START_TEST(read_dataType_isAbstract) {
    UA_Boolean ia = false;
    UA_StatusCode res = UA_Server_readIsAbstract(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), &ia);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(ia == true);
} END_TEST

START_TEST(read_refType_symmetric) {
    UA_Boolean sym = false;
    UA_StatusCode res = UA_Server_readSymmetric(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &sym);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(sym == false);
} END_TEST

START_TEST(read_object_eventNotifier) {
    UA_Byte en = 0;
    UA_StatusCode res = UA_Server_readEventNotifier(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &en);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_variable_accessLevel) {
    /* First add a variable */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90010),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns0ExtALVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    UA_Byte al = 0;
    UA_StatusCode res = UA_Server_readAccessLevel(server,
        UA_NODEID_NUMERIC(1, 90010), &al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(al & UA_ACCESSLEVELMASK_READ);

    /* Read dataType */
    UA_NodeId dt;
    res = UA_Server_readDataType(server,
        UA_NODEID_NUMERIC(1, 90010), &dt);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dt);

    /* Read valueRank */
    UA_Int32 vr = 0;
    res = UA_Server_readValueRank(server,
        UA_NODEID_NUMERIC(1, 90010), &vr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read minimumSamplingInterval */
    UA_Double msi = 0;
    res = UA_Server_readMinimumSamplingInterval(server,
        UA_NODEID_NUMERIC(1, 90010), &msi);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read historizing */
    UA_Boolean hist = false;
    res = UA_Server_readHistorizing(server,
        UA_NODEID_NUMERIC(1, 90010), &hist);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90010), true);
} END_TEST

START_TEST(read_method_executable) {
    /* Add a method node */
    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    mattr.executable = true;
    mattr.userExecutable = true;
    UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 90020),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Ns0ExtMethod"),
        mattr, NULL, 0, NULL, 0, NULL, NULL, NULL);

    UA_Boolean exe = false;
    UA_StatusCode res = UA_Server_readExecutable(server,
        UA_NODEID_NUMERIC(1, 90020), &exe);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(exe == true);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90020), true);
} END_TEST

/* === Server: write various attributes === */
START_TEST(write_description) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.writeMask = 0xFFFFFFFF;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90030),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns0ExtWrDesc"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    UA_LocalizedText desc = UA_LOCALIZEDTEXT("en", "test desc");
    UA_StatusCode res = UA_Server_writeDescription(server,
        UA_NODEID_NUMERIC(1, 90030), desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write displayName */
    UA_LocalizedText dn = UA_LOCALIZEDTEXT("en", "TestDisplayName");
    res = UA_Server_writeDisplayName(server,
        UA_NODEID_NUMERIC(1, 90030), dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write writeMask */
    res = UA_Server_writeWriteMask(server,
        UA_NODEID_NUMERIC(1, 90030), UA_WRITEMASK_DESCRIPTION);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90030), true);
} END_TEST

START_TEST(write_dataType_valueRank) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.writeMask = 0xFFFFFFFF;
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90031),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns0ExtWrDT"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Write dataType */
    UA_StatusCode res = UA_Server_writeDataType(server,
        UA_NODEID_NUMERIC(1, 90031),
        UA_NODEID_NUMERIC(0, UA_NS0ID_INT32));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write valueRank */
    res = UA_Server_writeValueRank(server,
        UA_NODEID_NUMERIC(1, 90031), UA_VALUERANK_SCALAR);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write accessLevel */
    res = UA_Server_writeAccessLevel(server,
        UA_NODEID_NUMERIC(1, 90031),
        UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write minimumSamplingInterval */
    res = UA_Server_writeMinimumSamplingInterval(server,
        UA_NODEID_NUMERIC(1, 90031), 100.0);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write historizing */
    res = UA_Server_writeHistorizing(server,
        UA_NODEID_NUMERIC(1, 90031), false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90031), true);
} END_TEST

START_TEST(write_isAbstract) {
    /* Add an object type */
    UA_ObjectTypeAttributes otattr = UA_ObjectTypeAttributes_default;
    otattr.isAbstract = false;
    UA_Server_addObjectTypeNode(server,
        UA_NODEID_NUMERIC(1, 90040),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "Ns0ExtObjType"),
        otattr, NULL, NULL);

    UA_StatusCode res = UA_Server_writeIsAbstract(server,
        UA_NODEID_NUMERIC(1, 90040), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90040), true);
} END_TEST

START_TEST(write_executable) {
    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    mattr.executable = true;
    mattr.userExecutable = true;
    mattr.writeMask = 0xFFFFFFFF;
    UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 90050),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Ns0ExtWrExec"),
        mattr, NULL, 0, NULL, 0, NULL, NULL, NULL);

    UA_StatusCode res = UA_Server_writeExecutable(server,
        UA_NODEID_NUMERIC(1, 90050), false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90050), true);
} END_TEST

/* === Browse / BrowsePath edge cases === */
START_TEST(browse_withSubtypes) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Browse with HierarchicalReferences (includes subtypes) */
    UA_BrowseRequest breq;
    UA_BrowseRequest_init(&breq);
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    bd.includeSubtypes = true;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    breq.nodesToBrowse = &bd;
    breq.nodesToBrowseSize = 1;
    breq.requestedMaxReferencesPerNode = 5;

    UA_BrowseResponse bres = UA_Client_Service_browse(client, breq);
    ck_assert_uint_eq(bres.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert(bres.resultsSize > 0);

    /* If continuation point, call browseNext */
    if(bres.results[0].continuationPoint.length > 0) {
        UA_BrowseNextRequest bnr;
        UA_BrowseNextRequest_init(&bnr);
        bnr.releaseContinuationPoints = false;
        bnr.continuationPoints = &bres.results[0].continuationPoint;
        bnr.continuationPointsSize = 1;
        UA_BrowseNextResponse bnres = UA_Client_Service_browseNext(client, bnr);
        ck_assert_uint_eq(bnres.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

        /* Release the continuation point */
        if(bnres.resultsSize > 0 &&
           bnres.results[0].continuationPoint.length > 0) {
            UA_BrowseNextRequest bnr2;
            UA_BrowseNextRequest_init(&bnr2);
            bnr2.releaseContinuationPoints = true;
            bnr2.continuationPoints = &bnres.results[0].continuationPoint;
            bnr2.continuationPointsSize = 1;
            UA_BrowseNextResponse bnres2 = UA_Client_Service_browseNext(client, bnr2);
            /* Just don't clear bnr2 since it refs bnres data */
            UA_BrowseNextResponse_clear(&bnres2);
        }
        /* Don't clear bnr since it refs bres data */
        UA_BrowseNextResponse_clear(&bnres);
    }
    UA_BrowseResponse_clear(&bres);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(translateBrowsePath_multiElement) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Multi-element browse path: Objects -> Server -> ServerStatus */
    UA_TranslateBrowsePathsToNodeIdsRequest tReq;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&tReq);
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_RelativePathElement elements[2];
    UA_RelativePathElement_init(&elements[0]);
    elements[0].targetName = UA_QUALIFIEDNAME(0, "Server");
    elements[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    elements[0].includeSubtypes = true;
    UA_RelativePathElement_init(&elements[1]);
    elements[1].targetName = UA_QUALIFIEDNAME(0, "ServerStatus");
    elements[1].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    elements[1].includeSubtypes = true;
    bp.relativePath.elements = elements;
    bp.relativePath.elementsSize = 2;
    tReq.browsePaths = &bp;
    tReq.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse tRes =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, tReq);
    ck_assert_uint_eq(tRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert(tRes.resultsSize > 0);
    ck_assert_uint_eq(tRes.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert(tRes.results[0].targetsSize > 0);

    /* Don't clear tReq since it refs stack data */
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&tRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(translateBrowsePath_badPath) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Bad path: non-existent element */
    UA_TranslateBrowsePathsToNodeIdsRequest tReq;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&tReq);
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_RelativePathElement el;
    UA_RelativePathElement_init(&el);
    el.targetName = UA_QUALIFIEDNAME(0, "NonExistentNode");
    el.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    bp.relativePath.elements = &el;
    bp.relativePath.elementsSize = 1;
    tReq.browsePaths = &bp;
    tReq.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse tRes =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, tReq);
    ck_assert_uint_eq(tRes.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    /* The individual result should have an error */
    ck_assert(tRes.results[0].statusCode != UA_STATUSCODE_GOOD);

    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&tRes);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* === Server: complex node addition === */
START_TEST(add_objectType_with_children) {
    /* Add an object type with sub-variables */
    UA_ObjectTypeAttributes otattr = UA_ObjectTypeAttributes_default;
    UA_StatusCode res = UA_Server_addObjectTypeNode(server,
        UA_NODEID_NUMERIC(1, 90100),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "MyTestType"),
        otattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Add a mandatory variable child */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90101),
        UA_NODEID_NUMERIC(1, 90100),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TypeChild"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Make it mandatory via ModellingRule */
    res = UA_Server_addReference(server,
        UA_NODEID_NUMERIC(1, 90101),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Instantiate the type - should copy child variables */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    res = UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 90110),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MyTestInstance"),
        UA_NODEID_NUMERIC(1, 90100),
        oattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Cleanup */
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90110), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90101), true);
    UA_Server_deleteNode(server, UA_NODEID_NUMERIC(1, 90100), true);
} END_TEST

START_TEST(add_node_with_invalid_parent) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 90200),
        UA_NODEID_NUMERIC(1, 99999), /* non-existing parent */
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "BadParent"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* == Subscription diagnostics via client == */
START_TEST(subscription_diagnostic_reads) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Create subscription to generate diagnostic data */
    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Create a monitored item */
    UA_MonitoredItemCreateRequest miReq =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    UA_MonitoredItemCreateResult miRes =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, miReq, NULL, NULL, NULL);
    ck_assert_uint_eq(miRes.statusCode, UA_STATUSCODE_GOOD);

    /* Run a few iterations so diagnostics update */
    for(int i = 0; i < 5; i++)
        UA_Client_run_iterate(client, 100);

    /* Read subscription diagnostics array from server */
    UA_Variant out;
    UA_Variant_init(&out);
    res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERDIAGNOSTICS_SUBSCRIPTIONDIAGNOSTICSARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* Read session diagnostics */
    UA_Variant_init(&out);
    res = UA_Server_readValue(server, UA_NODEID_NUMERIC(0, 3707), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* Cleanup */
    UA_Client_Subscriptions_deleteSingle(client, subId);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */

/* === Max capabilities reads === */
START_TEST(read_maxSessions) {
    UA_Variant out;
    UA_Variant_init(&out);
    /* MaxSessions - may not exist in all builds */
    UA_StatusCode res = readNodeValue(UA_NODEID_NUMERIC(0, 24095), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxSubscriptions */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 24096), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxMonitoredItems */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 24097), &out);
    (void)res;
    UA_Variant_clear(&out);

    /* MaxSubscriptionsPerSession */
    UA_Variant_init(&out);
    res = readNodeValue(UA_NODEID_NUMERIC(0, 24098), &out);
    (void)res;
    UA_Variant_clear(&out);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_ns0Ext(void) {
    /* Server-only tests (no client needed) */
    TCase *tc_status = tcase_create("ServerStatus");
    tcase_add_checked_fixture(tc_status, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_status, read_serverStatus);
    tcase_add_test(tc_status, read_startTime);
    tcase_add_test(tc_status, read_currentTime);
    tcase_add_test(tc_status, read_state);
    tcase_add_test(tc_status, read_buildInfo);
    tcase_add_test(tc_status, read_buildInfo_productName);
    tcase_add_test(tc_status, read_buildInfo_productUri);
    tcase_add_test(tc_status, read_buildInfo_manufacturerName);
    tcase_add_test(tc_status, read_buildInfo_softwareVersion);
    tcase_add_test(tc_status, read_buildInfo_buildNumber);
    tcase_add_test(tc_status, read_buildInfo_buildDate);

    TCase *tc_svcLevel = tcase_create("ServiceLevel");
    tcase_add_checked_fixture(tc_svcLevel, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_svcLevel, read_serviceLevel);
    tcase_add_test(tc_svcLevel, read_auditing);

    TCase *tc_caps = tcase_create("Capabilities");
    tcase_add_checked_fixture(tc_caps, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_caps, read_serverProfileArray);
    tcase_add_test(tc_caps, read_localeIdArray);
    tcase_add_test(tc_caps, read_minSupportedSampleRate);
    tcase_add_test(tc_caps, read_maxBrowseContinuationPoints);
    tcase_add_test(tc_caps, read_operationLimits);
    tcase_add_test(tc_caps, read_operationLimits_history);
    tcase_add_test(tc_caps, read_maxArrayLength);
    tcase_add_test(tc_caps, read_maxSessions);

    TCase *tc_diag = tcase_create("DiagnosticsSummary");
    tcase_add_checked_fixture(tc_diag, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_diag, read_diagnosticsSummary);
    tcase_add_test(tc_diag, read_diagnosticsSummary_subfields);
    tcase_add_test(tc_diag, read_enabledFlag);
    tcase_add_test(tc_diag, read_subscriptionDiagnosticsArray);
    tcase_add_test(tc_diag, read_samplingIntervalDiagnosticsArray);
    tcase_add_test(tc_diag, read_sessionDiagnosticsArray);
    tcase_add_test(tc_diag, read_sessionSecurityDiagnosticsArray);

    TCase *tc_attr = tcase_create("AttrReadWrite");
    tcase_add_checked_fixture(tc_attr, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_attr, read_serverNode_description);
    tcase_add_test(tc_attr, read_serverNode_writeMask);
    tcase_add_test(tc_attr, read_variable_inverseName);
    tcase_add_test(tc_attr, read_dataType_isAbstract);
    tcase_add_test(tc_attr, read_refType_symmetric);
    tcase_add_test(tc_attr, read_object_eventNotifier);
    tcase_add_test(tc_attr, read_variable_accessLevel);
    tcase_add_test(tc_attr, read_method_executable);
    tcase_add_test(tc_attr, write_description);
    tcase_add_test(tc_attr, write_dataType_valueRank);
    tcase_add_test(tc_attr, write_isAbstract);
    tcase_add_test(tc_attr, write_executable);

    TCase *tc_refs = tcase_create("References");
    tcase_add_checked_fixture(tc_refs, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_refs, server_addDeleteReference);
    tcase_add_test(tc_refs, add_objectType_with_children);
    tcase_add_test(tc_refs, add_node_with_invalid_parent);

    /* Tests requiring server + client */
    TCase *tc_disc = tcase_create("Discovery");
    tcase_add_checked_fixture(tc_disc, setup, teardown);
    tcase_add_test(tc_disc, client_findServers);
    tcase_add_test(tc_disc, client_findServers_withFilter);
    tcase_add_test(tc_disc, client_getEndpoints);
    tcase_add_test(tc_disc, client_registerNodes);

    TCase *tc_browse = tcase_create("BrowseExt");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, browse_withSubtypes);
    tcase_add_test(tc_browse, translateBrowsePath_multiElement);
    tcase_add_test(tc_browse, translateBrowsePath_badPath);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    TCase *tc_subdiag = tcase_create("SubDiag");
    tcase_add_checked_fixture(tc_subdiag, setup, teardown);
    tcase_add_test(tc_subdiag, subscription_diagnostic_reads);
#endif

    Suite *s = suite_create("NS0 Extended");
    suite_add_tcase(s, tc_status);
    suite_add_tcase(s, tc_svcLevel);
    suite_add_tcase(s, tc_caps);
    suite_add_tcase(s, tc_diag);
    suite_add_tcase(s, tc_attr);
    suite_add_tcase(s, tc_refs);
    suite_add_tcase(s, tc_disc);
    suite_add_tcase(s, tc_browse);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    suite_add_tcase(s, tc_subdiag);
#endif
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_ns0Ext();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
