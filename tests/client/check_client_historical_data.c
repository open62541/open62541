/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/historydata/history_data_backend.h>
#include <open62541/plugin/historydata/history_data_backend_memory.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>
#include <open62541/plugin/historydatabase.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"
#include "historical_read_test_data.h"

static UA_Server *server;
static UA_HistoryDataGathering *gathering;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

static UA_Client *client;
static UA_NodeId parentNodeId;
static UA_NodeId parentReferenceNodeId;
static UA_NodeId outNodeId;
static UA_HistoryDataBackend serverBackend;

// to receive data after we inserted data, we need in datavalue more space
struct ReceiveTupel {
    UA_DateTime timestamp;
    UA_Int64 value;
};

static const size_t receivedDataSize = (sizeof(testData) / sizeof(testData[0])) + 10;
static struct ReceiveTupel receivedTestData[(sizeof(testData) / sizeof(testData[0])) + 10];
static size_t receivedTestDataPos;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_Boolean
fillHistoricalDataBackend(UA_HistoryDataBackend backend);
static void resetReceiveBuffer(void) {
    receivedTestDataPos = 0;
    memset(receivedTestData, 0, sizeof(receivedTestData));
}

static void fillInt64DataValue(UA_DateTime timestamp, UA_Int64 value,
                               UA_DataValue *dataValue) {
    UA_DataValue_init(dataValue);
    dataValue->hasValue = true;
    UA_Int64 d = value;
    UA_Variant_setScalarCopy(&dataValue->value, &d, &UA_TYPES[UA_TYPES_INT64]);
    dataValue->hasSourceTimestamp = true;
    dataValue->sourceTimestamp = timestamp;
    dataValue->hasServerTimestamp = true;
    dataValue->serverTimestamp = timestamp;
    dataValue->hasStatus = true;
    dataValue->status = UA_STATUSCODE_GOOD;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

#ifdef UA_ENABLE_HISTORIZING
    resetReceiveBuffer();
    gathering = (UA_HistoryDataGathering*)UA_calloc(1, sizeof(UA_HistoryDataGathering));
    *gathering = UA_HistoryDataGathering_Default(1);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->historyDatabase = UA_HistoryDatabase_default(*gathering);
#endif

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
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD | UA_ACCESSLEVELMASK_HISTORYWRITE;
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

    UA_HistorizingNodeIdSettings setting;
    serverBackend = UA_HistoryDataBackend_Memory(1, 100);
    setting.historizingBackend = serverBackend;
    setting.maxHistoryDataResponseSize = 100;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert(fillHistoricalDataBackend(setting.historizingBackend));

    client = UA_Client_newForUnitTest();
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
}

static void
teardown(void) {
    /* cleanup */
    UA_HistoryDataBackend_Memory_clear(&serverBackend);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_NodeId_clear(&parentNodeId);
    UA_NodeId_clear(&parentReferenceNodeId);
    UA_NodeId_clear(&outNodeId);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_free(gathering);
}

static UA_Boolean
fillHistoricalDataBackend(UA_HistoryDataBackend backend) {
    fprintf(stderr, "Adding to historical data backend: ");
    for (size_t i = 0; i < testDataSize; ++i) {
        fprintf(stderr, "%lld, ", (long long)testData[i]);
        UA_DataValue value;
        fillInt64DataValue(testData[i], testData[i], &value);
        if (backend.serverSetHistoryData(server, backend.context, NULL, NULL,
                                         &outNodeId, UA_FALSE, &value) != UA_STATUSCODE_GOOD) {
            fprintf(stderr, "\n");
            UA_DataValue_clear(&value);
            return false;
        }
        UA_DataValue_clear(&value);
    }
    fprintf(stderr, "\n");
    return true;
}

static UA_Boolean
checkTestData(UA_Boolean inverse, UA_DateTime *dataA,
              struct ReceiveTupel *dataB, size_t dataSize) {
    for (size_t i = 0; i < dataSize; ++i) {
        if (!inverse && dataA[i] != dataB[i].timestamp)
            return false;
        if (inverse && dataA[i] != dataB[dataSize-i-1].timestamp)
            return false;
    }
    return true;
}

static UA_Boolean
checkModifiedData(UA_DateTime *dataA, size_t dataASize,
                  struct ReceiveTupel *dataB, size_t dataBSize) {
    for (size_t i = 0; i < dataBSize; ++i) {
        UA_Boolean found = UA_FALSE;
        for (size_t j = 0; j < dataASize; ++j) {
            if (dataA[j] == dataB[i].timestamp)
                found = UA_TRUE;
        }
        if (found && dataB[i].timestamp == dataB[i].value) {
            return false;
        }
        if (!found && dataB[i].timestamp != dataB[i].value) {
            return false;
        }
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
    fprintf(stderr, "Received %lu at pos %lu. moreDataAvailable %d. Data: ", (unsigned long)data->dataValuesSize, (unsigned long)receivedTestDataPos, moreDataAvailable);
    if (receivedTestDataPos + data->dataValuesSize > receivedDataSize)
        return false;
    for (size_t i = 0; i < data->dataValuesSize; ++i) {
        receivedTestData[i+receivedTestDataPos].timestamp = data->dataValues[i].sourceTimestamp;
        receivedTestData[i+receivedTestDataPos].value = *((UA_Int64*)data->dataValues[i].value.data);
        fprintf(stderr, "%lld/%lld, ",
                (long long)receivedTestData[i+receivedTestDataPos].timestamp,
                (long long)receivedTestData[i+receivedTestDataPos].value);
    }
    fprintf(stderr, "\n");
    receivedTestDataPos += data->dataValuesSize;
    return true;
}

START_TEST(Client_HistorizingReadRawAll) {
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
}
END_TEST

START_TEST(Client_HistorizingReadRawOne) {
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  1,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
}
END_TEST

START_TEST(Client_HistorizingReadRawTwo)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  2,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
}
END_TEST
START_TEST(Client_HistorizingReadRawAllInv)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_STOP_TIME,
                                                  TESTDATA_START_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)true);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(true, testData, receivedTestData, testDataSize));
}
END_TEST

START_TEST(Client_HistorizingReadRawOneInv)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_STOP_TIME,
                                                  TESTDATA_START_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  1,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)true);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(true, testData, receivedTestData, testDataSize));
}
END_TEST

START_TEST(Client_HistorizingReadRawTwoInv)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_STOP_TIME,
                                                  TESTDATA_START_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  2,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)true);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(testDataSize, receivedTestDataPos);
    ck_assert(checkTestData(true, testData, receivedTestData, testDataSize));
}
END_TEST

START_TEST(Client_HistorizingInsertRawSuccess)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
    resetReceiveBuffer();
    // insert values to the database
    for (size_t i = 0; i < testInsertDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testInsertDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_insert(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        UA_DataValue_clear(&value);
    }
    // check result
    ret = UA_Client_HistoryRead_raw(client,
                                    &outNodeId,
                                    receiveCallback,
                                    TESTDATA_START_TIME,
                                    TESTDATA_STOP_TIME,
                                    UA_STRING_NULL,
                                    false,
                                    100,
                                    UA_TIMESTAMPSTORETURN_BOTH,
                                    (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(receivedTestDataPos, testInsertResultDataSize);
    ck_assert(checkTestData(false, testInsertResultData, receivedTestData, receivedTestDataPos));
    ck_assert(checkModifiedData(testInsertDataSuccess, testInsertDataSuccessSize, receivedTestData, receivedTestDataPos));
}
END_TEST

START_TEST(Client_HistorizingReplaceRawSuccess)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
    resetReceiveBuffer();
    // replace values to the database
    for (size_t i = 0; i < testReplaceDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testReplaceDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_replace(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        UA_DataValue_clear(&value);
    }
    // check result
    ret = UA_Client_HistoryRead_raw(client,
                                    &outNodeId,
                                    receiveCallback,
                                    TESTDATA_START_TIME,
                                    TESTDATA_STOP_TIME,
                                    UA_STRING_NULL,
                                    false,
                                    100,
                                    UA_TIMESTAMPSTORETURN_BOTH,
                                    (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(receivedTestDataPos, testDataSize);
    ck_assert(checkTestData(false, testData, receivedTestData, receivedTestDataPos));
    ck_assert(checkModifiedData(testReplaceDataSuccess, testReplaceDataSuccessSize, receivedTestData, receivedTestDataPos));
}
END_TEST

START_TEST(Client_HistorizingUpdateRawSuccess)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
    resetReceiveBuffer();
    // insert values to the database
    for (size_t i = 0; i < testInsertDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testInsertDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_update(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOODENTRYINSERTED));
        UA_DataValue_clear(&value);
    }
    // replace values to the database
    for (size_t i = 0; i < testReplaceDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testReplaceDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_update(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOODENTRYREPLACED));
        UA_DataValue_clear(&value);
    }
    // check result
    ret = UA_Client_HistoryRead_raw(client,
                                    &outNodeId,
                                    receiveCallback,
                                    TESTDATA_START_TIME,
                                    TESTDATA_STOP_TIME,
                                    UA_STRING_NULL,
                                    false,
                                    100,
                                    UA_TIMESTAMPSTORETURN_BOTH,
                                    (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(receivedTestDataPos, testInsertResultDataSize);
    ck_assert(checkTestData(false, testInsertResultData, receivedTestData, receivedTestDataPos));
}
END_TEST

START_TEST(Client_HistorizingDeleteRaw)
{
    for (size_t i = 0; i < testDeleteRangeDataSize; ++i) {
        fprintf(stderr, "Client_HistorizingDeleteRaw: Testing %lu: {%lld, %lld, %lu, %s}\n",
                (unsigned long)i,
                (long long)testDeleteRangeData[i].start,
                (long long)testDeleteRangeData[i].end,
                (unsigned long)testDeleteRangeData[i].historySize,
                UA_StatusCode_name(testDeleteRangeData[i].statusCode));
        resetReceiveBuffer();
        serverBackend.removeDataValue(server,
                                      serverBackend.context,
                                      NULL,
                                      NULL,
                                      &outNodeId,
                                      TESTDATA_START_TIME,
                                      TESTDATA_STOP_TIME);
        fillHistoricalDataBackend(serverBackend);
        // check result
        UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                        &outNodeId,
                                        receiveCallback,
                                        TESTDATA_START_TIME,
                                        TESTDATA_STOP_TIME,
                                        UA_STRING_NULL,
                                        false,
                                        100,
                                        UA_TIMESTAMPSTORETURN_BOTH,
                                        (void*)false);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        ck_assert(checkTestData(false, testData, receivedTestData, receivedTestDataPos));
        resetReceiveBuffer();

        ret = UA_Client_HistoryUpdate_deleteRaw(client,
                                                &outNodeId,
                                                testDeleteRangeData[i].start,
                                                testDeleteRangeData[i].end);
        if (ret != testDeleteRangeData[i].statusCode)
                fprintf(stderr, "Error: ret %s != statusCode%s\n", UA_StatusCode_name(ret), UA_StatusCode_name(testDeleteRangeData[i].statusCode));
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(testDeleteRangeData[i].statusCode));

        // check result
        ret = UA_Client_HistoryRead_raw(client,
                                        &outNodeId,
                                        receiveCallback,
                                        TESTDATA_START_TIME,
                                        TESTDATA_STOP_TIME,
                                        UA_STRING_NULL,
                                        false,
                                        100,
                                        UA_TIMESTAMPSTORETURN_BOTH,
                                        (void*)false);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        if (receivedTestDataPos != testDeleteRangeData[i].historySize)
                fprintf(stderr, "Error: receivedTestDataPos != testDeleteRangeData[i].historySize\n");
        ck_assert_uint_eq(receivedTestDataPos, testDeleteRangeData[i].historySize);
    }
}
END_TEST

START_TEST(Client_HistorizingInsertRawFail)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
    resetReceiveBuffer();
    // insert values to the database
    for (size_t i = 0; i < testReplaceDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testReplaceDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_insert(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_BADENTRYEXISTS));
        UA_DataValue_clear(&value);
    }
    // check result
    ret = UA_Client_HistoryRead_raw(client,
                                    &outNodeId,
                                    receiveCallback,
                                    TESTDATA_START_TIME,
                                    TESTDATA_STOP_TIME,
                                    UA_STRING_NULL,
                                    false,
                                    100,
                                    UA_TIMESTAMPSTORETURN_BOTH,
                                    (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(receivedTestDataPos, testDataSize);
    ck_assert(checkTestData(false, testData, receivedTestData, receivedTestDataPos));
    ck_assert(checkModifiedData(NULL, 0, receivedTestData, receivedTestDataPos));
}
END_TEST

START_TEST(Client_HistorizingReplaceRawFail)
{
    UA_StatusCode ret = UA_Client_HistoryRead_raw(client,
                                                  &outNodeId,
                                                  receiveCallback,
                                                  TESTDATA_START_TIME,
                                                  TESTDATA_STOP_TIME,
                                                  UA_STRING_NULL,
                                                  false,
                                                  100,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert(checkTestData(false, testData, receivedTestData, testDataSize));
    resetReceiveBuffer();
    // replace values to the database
    for (size_t i = 0; i < testInsertDataSuccessSize; ++i) {
        UA_DataValue value;
        fillInt64DataValue(testInsertDataSuccess[i], 0, &value);
        ret = UA_Client_HistoryUpdate_replace(client, &outNodeId, &value);
        ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_BADNOENTRYEXISTS));
        UA_DataValue_clear(&value);
    }
    // check result
    ret = UA_Client_HistoryRead_raw(client,
                                    &outNodeId,
                                    receiveCallback,
                                    TESTDATA_START_TIME,
                                    TESTDATA_STOP_TIME,
                                    UA_STRING_NULL,
                                    false,
                                    100,
                                    UA_TIMESTAMPSTORETURN_BOTH,
                                    (void*)false);
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_uint_eq(receivedTestDataPos, testDataSize);
    ck_assert(checkTestData(false, testData, receivedTestData, receivedTestDataPos));
    ck_assert(checkModifiedData(NULL, 0, receivedTestData, receivedTestDataPos));
}
END_TEST

static Suite *
testSuite_Client(void) {
    Suite *s = suite_create("Client Historical Data");
    TCase *tc_client = tcase_create("Client Historical Data read_raw");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_HistorizingReadRawAll);
    tcase_add_test(tc_client, Client_HistorizingReadRawOne);
    tcase_add_test(tc_client, Client_HistorizingReadRawTwo);
    tcase_add_test(tc_client, Client_HistorizingReadRawAllInv);
    tcase_add_test(tc_client, Client_HistorizingReadRawOneInv);
    tcase_add_test(tc_client, Client_HistorizingReadRawTwoInv);
    tcase_add_test(tc_client, Client_HistorizingInsertRawSuccess);
    tcase_add_test(tc_client, Client_HistorizingReplaceRawSuccess);
    tcase_add_test(tc_client, Client_HistorizingUpdateRawSuccess);
    tcase_add_test(tc_client, Client_HistorizingDeleteRaw);
    tcase_add_test(tc_client, Client_HistorizingInsertRawFail);
    tcase_add_test(tc_client, Client_HistorizingReplaceRawFail);
    suite_add_tcase(s, tc_client);
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
