/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) luibass92 <luibass92@live.it> (Author: Luigi Bassetta)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/historydata/history_data_backend.h>
#include <open62541/plugin/historydata/history_data_backend_memory.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>
#include <open62541/plugin/historydatabase.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "test_helpers.h"
#include "server/ua_server_internal.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_HistoryDataGathering *gathering;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

static UA_Client *client;
static UA_NodeId parentNodeId;
static UA_NodeId parentReferenceNodeId;
static UA_NodeId outNodeId;

THREAD_CALLBACK(serverloop) {
    while(running) {
        UA_Server_run_iterate(server, false);
    }
    return 0;
}

static void setup(void) {
    running = true;

    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    gathering = (UA_HistoryDataGathering*)UA_calloc(1, sizeof(UA_HistoryDataGathering));
    *gathering = UA_HistoryDataGathering_Circular(1);
    config->historyDatabase = UA_HistoryDatabase_default(*gathering);

    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Error while calling Server_run_startup. %s\n", UA_StatusCode_name(retval));
        UA_Server_delete(server);
        exit(1);
    }

    THREAD_CREATE(server_thread, serverloop);
    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE |
        UA_ACCESSLEVELMASK_HISTORYREAD | UA_ACCESSLEVELMASK_HISTORYWRITE;
    attr.historizing = true;

    /* Add the variable node to the information model */
    UA_NodeId uint32NodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName uint32Name = UA_QUALIFIEDNAME(1, "the answer");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    retval = UA_Server_addVariableNode(server, uint32NodeId, parentNodeId,
                                       parentReferenceNodeId, uint32Name,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       attr, NULL, &outNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Error adding variable node. %s\n", UA_StatusCode_name(retval));
        UA_Server_delete(server);
        exit(1);
    }

    client = UA_Client_newForUnitTest();
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s\n",
                UA_StatusCode_name(retval));
        UA_Client_delete(client);
        UA_Server_delete(server);
        exit(1);
    }
}

static void teardown(void) {
    /* cleanup */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(server_thread);
    UA_NodeId_clear(&parentNodeId);
    UA_NodeId_clear(&parentReferenceNodeId);
    UA_NodeId_clear(&outNodeId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_free(gathering);
}

static UA_StatusCode
setUInt32(UA_Client *thisClient, UA_NodeId node, UA_UInt32 value) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_Variant_setScalar(&dv.value, &value, &UA_TYPES[UA_TYPES_UINT32]);
    dv.hasValue = true;
    dv.sourceTimestamp = UA_DateTime_now_fake(NULL);
    dv.hasSourceTimestamp = true;
    return UA_Client_writeValueAttributeEx(thisClient, node, &dv);
}

static void
requestHistory(UA_DateTime start,
               UA_DateTime end,
               UA_HistoryReadResponse * response,
               UA_UInt32 numValuesPerNode,
               UA_Boolean returnBounds,
               UA_ByteString *continuationPoint) {
    UA_ReadRawModifiedDetails *details = UA_ReadRawModifiedDetails_new();
    details->startTime = start;
    details->endTime = end;
    details->isReadModified = false;
    details->numValuesPerNode = numValuesPerNode;
    details->returnBounds = returnBounds;

    UA_HistoryReadValueId *valueId = UA_HistoryReadValueId_new();
    UA_NodeId_copy(&outNodeId, &valueId->nodeId);
    if(continuationPoint)
        UA_ByteString_copy(continuationPoint, &valueId->continuationPoint);

    UA_HistoryReadRequest request;
    UA_HistoryReadRequest_init(&request);
    request.historyReadDetails.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.historyReadDetails.content.decoded.type = &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS];
    request.historyReadDetails.content.decoded.data = details;

    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;

    request.nodesToReadSize = 1;
    request.nodesToRead = valueId;

    UA_LOCK(&server->serviceMutex);
    Service_HistoryRead(server, &server->adminSession, &request, response);
    UA_UNLOCK(&server->serviceMutex);
    UA_HistoryReadRequest_clear(&request);
}

START_TEST(Server_HistorizingStrategyValueSet) {
    // init to a defined value
    UA_StatusCode retval = setUInt32(client, outNodeId, 43);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // set a data backend
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = UA_HistoryDataBackend_Memory_Circular(3, 10);
    setting.maxHistoryDataResponseSize = 10;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_VALUESET;
    UA_LOCK(&server->serviceMutex);
    retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // Fill the data overcoming the buffer size and starting to write new values replacing the old ones.
    // The circular buffer size is 10, the number of elements historized is 15 (from 0 to 14). So the final buffer will be:
    //
    //                  | 10 | 11 | 12 | 13 | 14 | 5 | 6 | 7 | 8 | 9 |
    //
    UA_fakeSleep(100);
    UA_DateTime start = UA_DateTime_now_fake(NULL);
    UA_fakeSleep(100);
    for (UA_UInt32 i = 0; i < 15; ++i) {
        retval = setUInt32(client, outNodeId, i);
        ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        UA_fakeSleep(100);
    }
    UA_DateTime end = UA_DateTime_now_fake(NULL);

    // request
    UA_HistoryReadResponse response;
    UA_HistoryReadResponse_init(&response);
    requestHistory(start, end, &response, 0, false, NULL);

    // test the response
    ck_assert_str_eq(UA_StatusCode_name(response.responseHeader.serviceResult),
                     UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(response.resultsSize, 1);
    for (size_t i = 0; i < response.resultsSize; ++i) {
        ck_assert_str_eq(UA_StatusCode_name(response.results[i].statusCode),
                         UA_StatusCode_name(UA_STATUSCODE_GOOD));
        ck_assert_uint_eq(response.results[i].historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(response.results[i].historyData.content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]);
        UA_HistoryData * data = (UA_HistoryData *)response.results[i].historyData.content.decoded.data;
        ck_assert(data->dataValuesSize > 0);
        for (size_t j = 0; j < data->dataValuesSize; ++j) {
            ck_assert(data->dataValues[j].sourceTimestamp >= start && data->dataValues[j].sourceTimestamp < end);
            ck_assert_uint_eq(data->dataValues[j].hasSourceTimestamp, true);
            ck_assert_str_eq(UA_StatusCode_name(data->dataValues[j].status),
                             UA_StatusCode_name(UA_STATUSCODE_GOOD));
            ck_assert_uint_eq(data->dataValues[j].hasValue, true);
            ck_assert(data->dataValues[j].value.type == &UA_TYPES[UA_TYPES_UINT32]);
            UA_UInt32 * value = (UA_UInt32 *)data->dataValues[j].value.data;
            if(j >= 5)
                ck_assert_uint_eq(*value, j);
            else
                ck_assert_uint_eq(*value, j + 10);
        }
    }
    UA_HistoryReadResponse_clear(&response);
    UA_HistoryDataBackend_Memory_clear(&setting.historizingBackend);
}
END_TEST

static Suite *
testSuite_Client(void) {
    Suite *s = suite_create("Server Historical Data");
    TCase *tc_server = tcase_create("Server Historical Data Circular");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_HistorizingStrategyValueSet);
    suite_add_tcase(s, tc_server);

    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
