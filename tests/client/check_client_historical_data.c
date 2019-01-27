/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include "ua_types.h"
#include "ua_server.h"
#include "server/ua_server_internal.h"
#include "ua_client.h"
#include "client/ua_client_internal.h"
#include "ua_client_highlevel.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"

#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"
#include "ua_plugin_historydatabase.h"
#include "ua_historydatabase_default.h"
#include "ua_plugin_history_data_gathering.h"
#include "ua_historydatabackend_memory.h"
#include "ua_historydatagathering_default.h"
#ifdef UA_ENABLE_HISTORIZING
#include "historical_read_test_data.h"
#endif
#include <stddef.h>


static UA_Server *server;
static UA_ServerConfig *config;
#ifdef UA_ENABLE_HISTORIZING
static UA_HistoryDataGathering *gathering;
#endif
static UA_Boolean running;
static THREAD_HANDLE server_thread;

static UA_Client *client;
static UA_NodeId parentNodeId;
static UA_NodeId parentReferenceNodeId;
static UA_NodeId outNodeId;
#ifdef UA_ENABLE_HISTORIZING
static UA_HistoryDataBackend serverBackend;

// same size as the test data
static const size_t testDataSize = sizeof(testData) / sizeof(testData[0]);
static UA_DateTime receivedTestData[sizeof(testData) / sizeof(testData[0])];
static size_t receivedTestDataPos;
#endif

THREAD_CALLBACK(serverloop)
{
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}
#ifdef UA_ENABLE_HISTORIZING
static UA_Boolean
fillHistoricalDataBackend(UA_HistoryDataBackend backend);
#endif
static void
setup(void)
{
    running = true;
    config = UA_ServerConfig_new_default();
#ifdef UA_ENABLE_HISTORIZING
    receivedTestDataPos = 0;
    memset(receivedTestData, 0, sizeof(receivedTestData));

    gathering = (UA_HistoryDataGathering*)UA_calloc(1, sizeof(UA_HistoryDataGathering));
    *gathering = UA_HistoryDataGathering_Default(1);
    config->historyDatabase = UA_HistoryDatabase_default(*gathering);
#endif
    server = UA_Server_new(config);
    UA_StatusCode retval = UA_Server_run_startup(server);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    THREAD_CREATE(server_thread, serverloop);
    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
    attr.historizing = true;

    /* Add the variable node to the information model */
    UA_NodeId uint32NodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName uint32Name = UA_QUALIFIEDNAME(1, "the answer");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId_init(&outNodeId);
    ck_assert_uint_eq(UA_Server_addVariableNode(server,
                                                uint32NodeId,
                                                parentNodeId,
                                                parentReferenceNodeId,
                                                uint32Name,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                attr,
                                                NULL,
                                                &outNodeId)
                      , UA_STATUSCODE_GOOD);

#ifdef UA_ENABLE_HISTORIZING
    UA_HistorizingNodeIdSettings setting;
    serverBackend = UA_HistoryDataBackend_Memory(1, 100);
    setting.historizingBackend = serverBackend;
    setting.maxHistoryDataResponseSize = 100;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(fillHistoricalDataBackend(setting.historizingBackend));
#endif
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;
}

static void
teardown(void)
{
    /* cleanup */
#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryDataBackend_Memory_deleteMembers(&serverBackend);
#endif
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_NodeId_deleteMembers(&parentNodeId);
    UA_NodeId_deleteMembers(&parentReferenceNodeId);
    UA_NodeId_deleteMembers(&outNodeId);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
#ifdef UA_ENABLE_HISTORIZING
    UA_free(gathering);
#endif
}

#ifdef UA_ENABLE_HISTORIZING

#include <stdio.h>
#include "ua_session.h"

static UA_Boolean
fillHistoricalDataBackend(UA_HistoryDataBackend backend)
{
    fprintf(stderr, "Adding to historical data backend: ");
    for (size_t i = 0; i < testDataSize; ++i) {
        fprintf(stderr, "%lld, ", testData[i] / UA_DATETIME_SEC);
        UA_DataValue value;
        UA_DataValue_init(&value);
        value.hasValue = true;
        UA_Int64 d = testData[i];
        UA_Variant_setScalarCopy(&value.value, &d, &UA_TYPES[UA_TYPES_INT64]);
        value.hasSourceTimestamp = true;
        value.sourceTimestamp = testData[i];
        value.hasServerTimestamp = true;
        value.serverTimestamp = testData[i];
        value.hasStatus = true;
        value.status = UA_STATUSCODE_GOOD;
        if (backend.serverSetHistoryData(server, backend.context, NULL, NULL, &outNodeId, UA_FALSE, &value) != UA_STATUSCODE_GOOD) {
            fprintf(stderr, "\n");
            return false;
        }
        UA_DataValue_deleteMembers(&value);
    }
    fprintf(stderr, "\n");
    return true;
}

static UA_Boolean checkTestData(UA_Boolean inverse) {
    for (size_t i = 0; i < testDataSize; ++i) {
        if (testDataSize != receivedTestDataPos)
            return false;
        if (!inverse && testData[i] != receivedTestData[i])
            return false;
        if (inverse && testData[i] != receivedTestData[testDataSize-i-1])
            return false;
    }
    return true;
}
static UA_Boolean
receiveCallback(UA_Client *clt,
                const UA_NodeId *nodeId,
                UA_Boolean moreDataAvailable,
                const UA_ExtensionObject *_data,
                void *callbackContext) {
    UA_HistoryData *data = (UA_HistoryData*)_data->content.decoded.data;
    fprintf(stderr, "Received %lu at pos %lu. moreDataAvailable %d\n", data->dataValuesSize, receivedTestDataPos, moreDataAvailable);
    if (receivedTestDataPos + data->dataValuesSize > testDataSize)
        return false;
    for (size_t i = 0; i < data->dataValuesSize; ++i) {
        receivedTestData[i+receivedTestDataPos] = *((UA_Int64*)data->dataValues[i].value.data);
    }
    receivedTestDataPos += data->dataValuesSize;
    return true;
}

START_TEST(Client_HistorizingReadRawAll)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_START_TIME,
                              TESTDATA_STOP_TIME,
                              UA_STRING_NULL,
                              false,
                              100,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)false);
    ck_assert(checkTestData(false));
}
END_TEST

START_TEST(Client_HistorizingReadRawOne)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_START_TIME,
                              TESTDATA_STOP_TIME,
                              UA_STRING_NULL,
                              false,
                              1,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)false);
    ck_assert(checkTestData(false));
}
END_TEST

START_TEST(Client_HistorizingReadRawTwo)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_START_TIME,
                              TESTDATA_STOP_TIME,
                              UA_STRING_NULL,
                              false,
                              2,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)false);
    ck_assert(checkTestData(false));
}
END_TEST
START_TEST(Client_HistorizingReadRawAllInv)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_STOP_TIME,
                              TESTDATA_START_TIME,
                              UA_STRING_NULL,
                              false,
                              100,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)true);
    ck_assert(checkTestData(true));
}
END_TEST

START_TEST(Client_HistorizingReadRawOneInv)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_STOP_TIME,
                              TESTDATA_START_TIME,
                              UA_STRING_NULL,
                              false,
                              1,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)true);
    ck_assert(checkTestData(true));
}
END_TEST

START_TEST(Client_HistorizingReadRawTwoInv)
{
    UA_Client_HistoryRead_raw(client,
                              &outNodeId,
                              receiveCallback,
                              TESTDATA_STOP_TIME,
                              TESTDATA_START_TIME,
                              UA_STRING_NULL,
                              false,
                              2,
                              UA_TIMESTAMPSTORETURN_BOTH,
                              (void*)true);
    ck_assert(checkTestData(true));
}
END_TEST

#endif /*UA_ENABLE_HISTORIZING*/

static Suite* testSuite_Client(void)
{
    Suite *s = suite_create("Client Historical Data");
    TCase *tc_client = tcase_create("Client Historical Data read_raw");
    tcase_add_checked_fixture(tc_client, setup, teardown);
#ifdef UA_ENABLE_HISTORIZING
    tcase_add_test(tc_client, Client_HistorizingReadRawAll);
    tcase_add_test(tc_client, Client_HistorizingReadRawOne);
    tcase_add_test(tc_client, Client_HistorizingReadRawTwo);
    tcase_add_test(tc_client, Client_HistorizingReadRawAllInv);
    tcase_add_test(tc_client, Client_HistorizingReadRawOneInv);
    tcase_add_test(tc_client, Client_HistorizingReadRawTwoInv);
#endif /* UA_ENABLE_HISTORIZING */
    suite_add_tcase(s, tc_client);

    return s;
}

int main(void)
{
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
