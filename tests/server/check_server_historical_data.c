/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
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

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"

#include <check.h>

#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"
#ifdef UA_ENABLE_HISTORIZING
#include "historical_read_test_data.h"
#include "randomindextest_backend.h"
#endif
#include <stddef.h>

static UA_Server *server;
#ifdef UA_ENABLE_HISTORIZING
static UA_HistoryDataGathering *gathering;
#endif
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static MUTEX_HANDLE serverMutex;

static UA_Client *client;
static UA_NodeId parentNodeId;
static UA_NodeId parentReferenceNodeId;
static UA_NodeId outNodeId;

static UA_DateTime *testDataSorted;

static void serverMutexLock(void) {
    if (!(MUTEX_LOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be locked.\n");
        exit(1);
    }
}

static void serverMutexUnlock(void) {
    if (!(MUTEX_UNLOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be unlocked.\n");
        exit(1);
    }
}

THREAD_CALLBACK(serverloop) {
    while(running) {
        serverMutexLock();
        UA_Server_run_iterate(server, false);
        serverMutexUnlock();
    }
    return 0;
}

static void setup(void) {
    if (!(MUTEX_INIT(serverMutex))) {
        fprintf(stderr, "Server mutex was not created correctly.\n");
        exit(1);
    }
    running = true;

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

#ifdef UA_ENABLE_HISTORIZING
    gathering = (UA_HistoryDataGathering*)UA_calloc(1, sizeof(UA_HistoryDataGathering));
    *gathering = UA_HistoryDataGathering_Default(1);
    config->historyDatabase = UA_HistoryDatabase_default(*gathering);
#endif

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
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD | UA_ACCESSLEVELMASK_HISTORYWRITE;
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
    if (retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Error adding variable node. %s\n", UA_StatusCode_name(retval));
        UA_Server_delete(server);
        exit(1);
    }

    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if (retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s\n", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        UA_Server_delete(server);
        exit(1);
    }

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;
}

static void teardown(void) {
    /* cleanup */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(server_thread);
    UA_NodeId_deleteMembers(&parentNodeId);
    UA_NodeId_deleteMembers(&parentReferenceNodeId);
    UA_NodeId_deleteMembers(&outNodeId);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
#ifdef UA_ENABLE_HISTORIZING
    UA_free(gathering);
#endif
    if (!MUTEX_DESTROY(serverMutex)) {
        fprintf(stderr, "Server mutex was not destroyed correctly.\n");
        exit(1);
    }
}

#ifdef UA_ENABLE_HISTORIZING

#include <stdio.h>
#include "ua_session.h"

static UA_StatusCode
setUInt32(UA_Client *thisClient, UA_NodeId node, UA_UInt32 value)
{
    UA_Variant variant;
    UA_Variant_setScalar(&variant, &value, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_Client_writeValueAttribute(thisClient, node, &variant);
}

static UA_DateTime* sortDateTimes(UA_DateTime *data) {
    size_t count = 0;
    while(data[count++]);
    UA_DateTime* ret;
    if (UA_Array_copy(data, count, (void**)&ret, &UA_TYPES[UA_TYPES_DATETIME]) != UA_STATUSCODE_GOOD)
        return NULL;
    --count;
    // sort it
    for (size_t i = 1; i < count; i++) {
       for (size_t j = 0; j < count - i; j++) {
           if (ret[j] > ret[j+1]) {
               UA_DateTime tmp = ret[j];
               ret[j] = ret[j+1];
               ret[j+1] = tmp;
           }
       }
    }
    return ret;
}

static void
printTimestamp(UA_DateTime timestamp)
{
    if (timestamp == TIMESTAMP_FIRST) {
        fprintf(stderr, "FIRST,");
    } else if (timestamp == TIMESTAMP_LAST) {
        fprintf(stderr, "LAST,");
    } else {
        fprintf(stderr, "%3lld,", timestamp / UA_DATETIME_SEC);
    }
}

static void
printResult(UA_DataValue * value)
{
    if (value->status != UA_STATUSCODE_GOOD)
        fprintf(stderr, "%s:", UA_StatusCode_name(value->status));
    printTimestamp(value->sourceTimestamp);
}

static UA_Boolean
resultIsEqual(const UA_DataValue * result, const testTuple * tuple, size_t index)
{
    switch (tuple->result[index]) {
    case TIMESTAMP_FIRST:
        if (result->status != UA_STATUSCODE_BADBOUNDNOTFOUND
                || !UA_Variant_isEmpty(&result->value))
            return false;
        /* we do not test timestamp if TIMESTAMP_UNSPECIFIED is given for start.
         * See OPC UA Part 11, Version 1.03, Page 5-6, Table 1, Mark b for details.*/
        if (tuple->start != TIMESTAMP_UNSPECIFIED
                && tuple->start != result->sourceTimestamp)
            return false;
        break;
    case TIMESTAMP_LAST:
        if (result->status != UA_STATUSCODE_BADBOUNDNOTFOUND
                || !UA_Variant_isEmpty(&result->value))
            return false;
        /* we do not test timestamp if TIMESTAMP_UNSPECIFIED is given for end.
         * See OPC UA Part 11, Version 1.03, Page 5-6, Table 1, Mark a for details.*/
        if (tuple->end != TIMESTAMP_UNSPECIFIED
                && tuple->end != result->sourceTimestamp)
            return false;
        break;
    default:
        if (result->sourceTimestamp != tuple->result[index]
                || result->value.type != &UA_TYPES[UA_TYPES_INT64]
                || *((UA_Int64*)result->value.data) != tuple->result[index])
            return false;
    }
    return true;
}

static UA_Boolean
fillHistoricalDataBackend(UA_HistoryDataBackend backend)
{
    int i = 0;
    UA_DateTime currentDateTime = testData[i];
    fprintf(stderr, "Adding to historical data backend: ");
    while (currentDateTime) {
        fprintf(stderr, "%lld, ", currentDateTime / UA_DATETIME_SEC);
        UA_DataValue value;
        UA_DataValue_init(&value);
        value.hasValue = true;
        UA_Int64 d = currentDateTime;
        UA_Variant_setScalarCopy(&value.value, &d, &UA_TYPES[UA_TYPES_INT64]);
        value.hasSourceTimestamp = true;
        value.sourceTimestamp = currentDateTime;
        value.hasServerTimestamp = true;
        value.serverTimestamp = currentDateTime;
        value.hasStatus = true;
        value.status = UA_STATUSCODE_GOOD;
        if (backend.serverSetHistoryData(server, backend.context, NULL, NULL, &outNodeId, UA_FALSE, &value) != UA_STATUSCODE_GOOD) {
            fprintf(stderr, "\n");
            return false;
        }
        UA_DataValue_deleteMembers(&value);
        currentDateTime = testData[++i];
    }
    fprintf(stderr, "\n");
    return true;
}

void
Service_HistoryRead(UA_Server *server, UA_Session *session,
                    const UA_HistoryReadRequest *request,
                    UA_HistoryReadResponse *response);

static void
requestHistory(UA_DateTime start,
               UA_DateTime end,
               UA_HistoryReadResponse * response,
               UA_UInt32 numValuesPerNode,
               UA_Boolean returnBounds,
               UA_ByteString *continuationPoint)
{
    UA_ReadRawModifiedDetails *details = UA_ReadRawModifiedDetails_new();
    details->startTime = start;
    details->endTime = end;
    details->isReadModified = false;
    details->numValuesPerNode = numValuesPerNode;
    details->returnBounds = returnBounds;

    UA_HistoryReadValueId *valueId = UA_HistoryReadValueId_new();
    UA_NodeId_copy(&outNodeId, &valueId->nodeId);
    if (continuationPoint)
        UA_ByteString_copy(continuationPoint, &valueId->continuationPoint);

    UA_HistoryReadRequest request;
    UA_HistoryReadRequest_init(&request);
    request.historyReadDetails.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.historyReadDetails.content.decoded.type = &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS];
    request.historyReadDetails.content.decoded.data = details;

    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;

    request.nodesToReadSize = 1;
    request.nodesToRead = valueId;

    Service_HistoryRead(server, &server->adminSession, &request, response);
    UA_HistoryReadRequest_deleteMembers(&request);
}

static UA_UInt32
testHistoricalDataBackend(size_t maxResponseSize)
{
    const UA_HistorizingNodeIdSettings* setting = gathering->getHistorizingSetting(server, gathering->context, &outNodeId);
    UA_HistorizingNodeIdSettings newSetting = *setting;
    newSetting.maxHistoryDataResponseSize = maxResponseSize;
    gathering->updateNodeIdSetting(server, gathering->context, &outNodeId, newSetting);

    UA_UInt32 retval = 0;
    size_t i = 0;
    testTuple *current = &testRequests[i];
    fprintf(stderr, "Testing with maxResponseSize of %lu\n", maxResponseSize);
    fprintf(stderr, "Start | End  | numValuesPerNode | returnBounds |ContPoint| {Expected}{Result} Result\n");
    fprintf(stderr, "------+------+------------------+--------------+---------+----------------\n");
    size_t j;
    while (current->start || current->end) {
        j = 0;
        if (current->start == TIMESTAMP_UNSPECIFIED) {
            fprintf(stderr, "UNSPEC|");
        } else {
            fprintf(stderr, "  %3lld |", current->start / UA_DATETIME_SEC);
        }
        if (current->end == TIMESTAMP_UNSPECIFIED) {
            fprintf(stderr, "UNSPEC|");
        } else {
            fprintf(stderr, "  %3lld |", current->end / UA_DATETIME_SEC);
        }
        fprintf(stderr, "               %2u |          %s |     %s | {", current->numValuesPerNode, (current->returnBounds ? "Yes" : " No"), (current->returnContinuationPoint ? "Yes" : " No"));
        while (current->result[j]) {
            printTimestamp(current->result[j]);
            ++j;
        }
        fprintf(stderr, "}");

        UA_DataValue *result = NULL;
        size_t resultSize = 0;
        UA_ByteString continuous;
        UA_ByteString_init(&continuous);
        UA_Boolean readOk = true;
        size_t reseivedValues = 0;
        fprintf(stderr, "{");
        size_t counter = 0;
        do {
            UA_HistoryReadResponse response;
            UA_HistoryReadResponse_init(&response);
            UA_UInt32 numValuesPerNode = current->numValuesPerNode;
            if (numValuesPerNode > 0 && numValuesPerNode + (UA_UInt32)reseivedValues > current->numValuesPerNode)
                numValuesPerNode = current->numValuesPerNode - (UA_UInt32)reseivedValues;

            requestHistory(current->start,
                           current->end,
                           &response,
                           numValuesPerNode,
                           current->returnBounds,
                           &continuous);
            ++counter;

            if(response.resultsSize != 1) {
                fprintf(stderr, "ResultError:Size %lu %s", response.resultsSize, UA_StatusCode_name(response.responseHeader.serviceResult));
                readOk = false;
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }

            UA_StatusCode stat = response.results[0].statusCode;
            if (stat == UA_STATUSCODE_BADBOUNDNOTSUPPORTED && current->returnBounds) {
                fprintf(stderr, "%s", UA_StatusCode_name(stat));
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }

            if(response.results[0].historyData.encoding != UA_EXTENSIONOBJECT_DECODED
                    || response.results[0].historyData.content.decoded.type != &UA_TYPES[UA_TYPES_HISTORYDATA]) {
                fprintf(stderr, "ResultError:HistoryData");
                readOk = false;
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }

            UA_HistoryData * data = (UA_HistoryData *)response.results[0].historyData.content.decoded.data;
            resultSize = data->dataValuesSize;
            result = data->dataValues;

            if (resultSize == 0 && continuous.length > 0) {
                fprintf(stderr, "continuousResultEmpty");
                readOk = false;
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }

            if (resultSize > maxResponseSize) {
                fprintf(stderr, "resultToBig");
                readOk = false;
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }

            if (stat != UA_STATUSCODE_GOOD) {
                fprintf(stderr, "%s", UA_StatusCode_name(stat));
            } else {
                for (size_t k = 0; k < resultSize; ++k)
                    printResult(&result[k]);
            }

            if (stat == UA_STATUSCODE_GOOD && j >= resultSize + reseivedValues) {
                for (size_t l = 0; l < resultSize; ++l) {
                    /* See OPC UA Part 11, Version 1.03, Page 5-6, Table 1, Mark a for details.*/
                    if (current->result[l + reseivedValues] == TIMESTAMP_LAST && current->end == TIMESTAMP_UNSPECIFIED) {
                        // This test will work on not continous read, only
                        if (reseivedValues == 0 && !(l > 0 && result[l].sourceTimestamp == result[l-1].sourceTimestamp + UA_DATETIME_SEC))
                            readOk = false;
                    }
                    /* See OPC UA Part 11, Version 1.03, Page 5-6, Table 1, Mark b for details.*/
                    if (current->result[l + reseivedValues] == TIMESTAMP_FIRST && current->start == TIMESTAMP_UNSPECIFIED) {
                        // This test will work on not continous read, only
                        if (reseivedValues == 0 && !(l > 0 && result[l].sourceTimestamp == result[l-1].sourceTimestamp - UA_DATETIME_SEC))
                            readOk = false;
                    }
                    if (!resultIsEqual(&result[l], current, l + reseivedValues))
                        readOk = false;
                }
                if (response.results[0].continuationPoint.length > 0)
                    fprintf(stderr, "C,");
                reseivedValues += resultSize;
                if (reseivedValues == j) {
                    if (current->returnContinuationPoint && response.results[0].continuationPoint.length == 0) {
                        readOk = false;
                        fprintf(stderr, "missingContinuationPoint");
                    }
                    if (!current->returnContinuationPoint && response.results[0].continuationPoint.length > 0) {
                        readOk = false;
                        fprintf(stderr, "unexpectedContinuationPoint");
                    }
                    UA_HistoryReadResponse_deleteMembers(&response);
                    break;
                }
                UA_ByteString_deleteMembers(&continuous);
                UA_ByteString_copy(&response.results[0].continuationPoint, &continuous);
            } else {
                readOk = false;
                UA_HistoryReadResponse_deleteMembers(&response);
                break;
            }
            UA_HistoryReadResponse_deleteMembers(&response);
        } while (continuous.length > 0);

        if (j != reseivedValues) {
            readOk = false;
        }
        UA_ByteString_deleteMembers(&continuous);
        if (!readOk) {
            fprintf(stderr, "} Fail (%lu requests)\n", counter);
            ++retval;
        } else {
            fprintf(stderr, "} OK (%lu requests)\n", counter);
        }
        current = &testRequests[++i];
    }
    return retval;
}

void
Service_HistoryUpdate(UA_Server *server, UA_Session *session,
                      const UA_HistoryUpdateRequest *request,
                      UA_HistoryUpdateResponse *response);

static UA_StatusCode
deleteHistory(UA_DateTime start,
              UA_DateTime end)
{
    UA_DeleteRawModifiedDetails *details = UA_DeleteRawModifiedDetails_new();
    details->startTime = start;
    details->endTime = end;
    details->isDeleteModified = false;
    UA_NodeId_copy(&outNodeId, &details->nodeId);

    UA_HistoryUpdateRequest request;
    UA_HistoryUpdateRequest_init(&request);
    request.historyUpdateDetailsSize = 1;
    request.historyUpdateDetails = UA_ExtensionObject_new();
    UA_ExtensionObject_init(request.historyUpdateDetails);

    request.historyUpdateDetails[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    request.historyUpdateDetails[0].content.decoded.type = &UA_TYPES[UA_TYPES_DELETERAWMODIFIEDDETAILS];
    request.historyUpdateDetails[0].content.decoded.data = details;

    UA_HistoryUpdateResponse response;
    UA_HistoryUpdateResponse_init(&response);
    Service_HistoryUpdate(server, &server->adminSession, &request, &response);
    UA_HistoryUpdateRequest_deleteMembers(&request);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        ret = response.responseHeader.serviceResult;
    else if (response.resultsSize != 1)
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;
    else if (response.results[0].statusCode != UA_STATUSCODE_GOOD)
        ret = response.results[0].statusCode;
    else if (response.results[0].operationResultsSize != 0)
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;

    UA_HistoryUpdateResponse_deleteMembers(&response);
    return ret;
}

static UA_StatusCode
updateHistory(UA_PerformUpdateType updateType, UA_DateTime *updateData, UA_StatusCode ** operationResults, size_t *operationResultsSize)
{
    UA_UpdateDataDetails *details = UA_UpdateDataDetails_new();
    details->performInsertReplace = updateType;
    UA_NodeId_copy(&outNodeId, &details->nodeId);
    int updateDataSize = -1;
    while(updateData[++updateDataSize]);
    fprintf(stderr, "updateHistory for %d values.\n", updateDataSize);
    details->updateValuesSize = (size_t)updateDataSize;
    details->updateValues = (UA_DataValue*)UA_Array_new(details->updateValuesSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    for (size_t i = 0; i < details->updateValuesSize; ++i) {
        UA_DataValue_init(&details->updateValues[i]);
        details->updateValues[i].hasValue = true;
        UA_Int64 d = updateType;
        UA_Variant_setScalarCopy(&details->updateValues[i].value, &d, &UA_TYPES[UA_TYPES_INT64]);
        details->updateValues[i].hasSourceTimestamp = true;
        details->updateValues[i].sourceTimestamp = updateData[i];
        details->updateValues[i].hasServerTimestamp = true;
        details->updateValues[i].serverTimestamp = updateData[i];
        details->updateValues[i].hasStatus = true;
        details->updateValues[i].status = UA_STATUSCODE_GOOD;
    }

    UA_HistoryUpdateRequest request;
    UA_HistoryUpdateRequest_init(&request);
    request.historyUpdateDetailsSize = 1;
    request.historyUpdateDetails = UA_ExtensionObject_new();
    UA_ExtensionObject_init(request.historyUpdateDetails);

    request.historyUpdateDetails[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    request.historyUpdateDetails[0].content.decoded.type = &UA_TYPES[UA_TYPES_UPDATEDATADETAILS];
    request.historyUpdateDetails[0].content.decoded.data = details;

    UA_HistoryUpdateResponse response;
    UA_HistoryUpdateResponse_init(&response);
    Service_HistoryUpdate(server, &server->adminSession, &request, &response);
    UA_HistoryUpdateRequest_deleteMembers(&request);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        ret = response.responseHeader.serviceResult;
    else if (response.resultsSize != 1)
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;
    else if (response.results[0].statusCode != UA_STATUSCODE_GOOD)
        ret = response.results[0].statusCode;
    else if (response.results[0].operationResultsSize != (size_t)updateDataSize)
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;
    else {
        if (operationResults) {
            *operationResultsSize = response.results[0].operationResultsSize;
            ret = UA_Array_copy(response.results[0].operationResults, *operationResultsSize, (void**)operationResults, &UA_TYPES[UA_TYPES_STATUSCODE]);
        } else {
            for (size_t i = 0; i < response.results[0].operationResultsSize; ++i) {
                if (response.results[0].operationResults[i] != UA_STATUSCODE_GOOD) {
                    ret = response.results[0].operationResults[i];
                    break;
                }
            }
        }
    }
    UA_HistoryUpdateResponse_deleteMembers(&response);
    return ret;
}

static void
testResult(UA_DateTime *resultData, UA_HistoryData * historyData) {

    // request
    UA_HistoryReadResponse localResponse;
    UA_HistoryReadResponse_init(&localResponse);
    requestHistory(TIMESTAMP_FIRST, TIMESTAMP_LAST, &localResponse, 0, false, NULL);

    // test the response
    ck_assert_str_eq(UA_StatusCode_name(localResponse.responseHeader.serviceResult), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(localResponse.resultsSize, 1);
    ck_assert_str_eq(UA_StatusCode_name(localResponse.results[0].statusCode), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(localResponse.results[0].historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(localResponse.results[0].historyData.content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]);
    UA_HistoryData * data = (UA_HistoryData *)localResponse.results[0].historyData.content.decoded.data;
    if (historyData)
        UA_HistoryData_copy(data, historyData);
    for (size_t j = 0; j < data->dataValuesSize; ++j) {
        ck_assert(resultData[j] != 0);
        ck_assert_uint_eq(data->dataValues[j].hasSourceTimestamp, true);
        ck_assert_uint_eq(data->dataValues[j].sourceTimestamp, resultData[j]);
    }
    UA_HistoryReadResponse_deleteMembers(&localResponse);
}

START_TEST(Server_HistorizingUpdateDelete)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_Memory(1, 1);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill backend
    ck_assert_uint_eq(fillHistoricalDataBackend(backend), true);

    // delete some values
    ck_assert_str_eq(UA_StatusCode_name(deleteHistory(DELETE_START_TIME, DELETE_STOP_TIME)),
                     UA_StatusCode_name(UA_STATUSCODE_GOOD));

    testResult(testDataAfterDelete, NULL);

    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingUpdateInsert)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_Memory(1, 1);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill backend with insert
    ck_assert_str_eq(UA_StatusCode_name(updateHistory(UA_PERFORMUPDATETYPE_INSERT, testData, NULL, NULL))
                                        , UA_StatusCode_name(UA_STATUSCODE_GOOD));

    UA_HistoryData data;
    UA_HistoryData_init(&data);

    testResult(testDataSorted, &data);

    for (size_t i = 0; i < data.dataValuesSize; ++i) {
        ck_assert_uint_eq(data.dataValues[i].hasValue, true);
        ck_assert(data.dataValues[i].value.type == &UA_TYPES[UA_TYPES_INT64]);
        ck_assert_uint_eq(*((UA_Int64*)data.dataValues[i].value.data), UA_PERFORMUPDATETYPE_INSERT);
    }

    UA_HistoryData_deleteMembers(&data);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingUpdateReplace)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_Memory(1, 1);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill backend with insert
    ck_assert_str_eq(UA_StatusCode_name(updateHistory(UA_PERFORMUPDATETYPE_INSERT, testData, NULL, NULL))
                                        , UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // replace all
    ck_assert_str_eq(UA_StatusCode_name(updateHistory(UA_PERFORMUPDATETYPE_REPLACE, testData, NULL, NULL))
                                        , UA_StatusCode_name(UA_STATUSCODE_GOOD));

    UA_HistoryData data;
    UA_HistoryData_init(&data);

    testResult(testDataSorted, &data);

    for (size_t i = 0; i < data.dataValuesSize; ++i) {
        ck_assert_uint_eq(data.dataValues[i].hasValue, true);
        ck_assert(data.dataValues[i].value.type == &UA_TYPES[UA_TYPES_INT64]);
        ck_assert_uint_eq(*((UA_Int64*)data.dataValues[i].value.data), UA_PERFORMUPDATETYPE_REPLACE);
    }

    UA_HistoryData_deleteMembers(&data);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingUpdateUpdate)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_Memory(1, 1);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill backend with insert
    ck_assert_str_eq(UA_StatusCode_name(updateHistory(UA_PERFORMUPDATETYPE_INSERT, testData, NULL, NULL))
                                        , UA_StatusCode_name(UA_STATUSCODE_GOOD));

    testResult(testDataSorted, NULL);

    // delete some values
    ck_assert_str_eq(UA_StatusCode_name(deleteHistory(DELETE_START_TIME, DELETE_STOP_TIME)),
                     UA_StatusCode_name(UA_STATUSCODE_GOOD));

    testResult(testDataAfterDelete, NULL);

    // update all and insert some
    UA_StatusCode *result;
    size_t resultSize = 0;
    ck_assert_str_eq(UA_StatusCode_name(updateHistory(UA_PERFORMUPDATETYPE_UPDATE, testDataSorted, &result, &resultSize))
                                        , UA_StatusCode_name(UA_STATUSCODE_GOOD));

    for (size_t i = 0; i < resultSize; ++i) {
        ck_assert_str_eq(UA_StatusCode_name(result[i]), UA_StatusCode_name(testDataUpdateResult[i]));
    }
    UA_Array_delete(result, resultSize, &UA_TYPES[UA_TYPES_STATUSCODE]);

    UA_HistoryData data;
    UA_HistoryData_init(&data);

    testResult(testDataSorted, &data);

    for (size_t i = 0; i < data.dataValuesSize; ++i) {
        ck_assert_uint_eq(data.dataValues[i].hasValue, true);
        ck_assert(data.dataValues[i].value.type == &UA_TYPES[UA_TYPES_INT64]);
        ck_assert_uint_eq(*((UA_Int64*)data.dataValues[i].value.data), UA_PERFORMUPDATETYPE_UPDATE);
    }

    UA_HistoryData_deleteMembers(&data);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingStrategyUser)
{
    // set a data backend
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = UA_HistoryDataBackend_Memory(3, 100);
    setting.maxHistoryDataResponseSize = 100;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    UA_StatusCode retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill the data
    UA_DateTime start = UA_DateTime_now();
    UA_DateTime end = start + (10 * UA_DATETIME_SEC);
    for (UA_UInt32 i = 0; i < 10; ++i) {
        UA_DataValue value;
        UA_DataValue_init(&value);
        value.hasValue = true;
        value.hasStatus = true;
        value.status = UA_STATUSCODE_GOOD;
        UA_Variant_setScalarCopy(&value.value, &i, &UA_TYPES[UA_TYPES_UINT32]);
        value.hasSourceTimestamp = true;
        value.sourceTimestamp = start + (i * UA_DATETIME_SEC);
        value.hasServerTimestamp = true;
        value.serverTimestamp = value.sourceTimestamp;
        retval = setting.historizingBackend.serverSetHistoryData(server,
                                                                 setting.historizingBackend.context,
                                                                 NULL,
                                                                 NULL,
                                                                 &outNodeId,
                                                                 UA_FALSE,
                                                                 &value);
        ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        UA_DataValue_deleteMembers(&value);
    }

    // request
    UA_HistoryReadResponse response;
    UA_HistoryReadResponse_init(&response);
    requestHistory(start, end, &response, 0, false, NULL);

    // test the response
    ck_assert_str_eq(UA_StatusCode_name(response.responseHeader.serviceResult), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(response.resultsSize, 1);
    for (size_t i = 0; i < response.resultsSize; ++i) {
        ck_assert_str_eq(UA_StatusCode_name(response.results[i].statusCode), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        ck_assert_uint_eq(response.results[i].historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(response.results[i].historyData.content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]);
        UA_HistoryData * data = (UA_HistoryData *)response.results[i].historyData.content.decoded.data;
        ck_assert_uint_eq(data->dataValuesSize, 10);
        for (size_t j = 0; j < data->dataValuesSize; ++j) {
            ck_assert_uint_eq(data->dataValues[j].hasSourceTimestamp, true);
            ck_assert_uint_eq(data->dataValues[j].sourceTimestamp, start + (j * UA_DATETIME_SEC));
            ck_assert_uint_eq(data->dataValues[j].hasStatus, true);
            ck_assert_str_eq(UA_StatusCode_name(data->dataValues[j].status), UA_StatusCode_name(UA_STATUSCODE_GOOD));
            ck_assert_uint_eq(data->dataValues[j].hasValue, true);
            ck_assert(data->dataValues[j].value.type == &UA_TYPES[UA_TYPES_UINT32]);
            UA_UInt32 * value = (UA_UInt32 *)data->dataValues[j].value.data;
            ck_assert_uint_eq(*value, j);
        }
    }
    UA_HistoryReadResponse_deleteMembers(&response);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingStrategyPoll)
{
    // init to a defined value
    UA_StatusCode retval = setUInt32(client, outNodeId, 43);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // set a data backend
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = UA_HistoryDataBackend_Memory(3, 100);
    setting.maxHistoryDataResponseSize = 100;
    setting.pollingInterval = 100;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_POLL;
    serverMutexLock();
    retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill the data
    UA_DateTime start = UA_DateTime_now();
    serverMutexLock();
    retval = gathering->startPoll(server, gathering->context, &outNodeId);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    for (size_t k = 0; k < 10; ++k) {
        UA_fakeSleep(50);
        UA_realSleep(50);
        if (k == 5) {
            serverMutexLock();
            gathering->stopPoll(server, gathering->context, &outNodeId);
            serverMutexUnlock();
        }
        setUInt32(client, outNodeId, (unsigned int)k);
    }

    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    UA_DateTime end = UA_DateTime_now();

    // request
    UA_HistoryReadResponse response;
    UA_HistoryReadResponse_init(&response);
    requestHistory(start, end, &response, 0, false, NULL);

    // test the response
    ck_assert_str_eq(UA_StatusCode_name(response.responseHeader.serviceResult), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(response.resultsSize, 1);
    for (size_t i = 0; i < response.resultsSize; ++i) {
        ck_assert_str_eq(UA_StatusCode_name(response.results[i].statusCode), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        ck_assert_uint_eq(response.results[i].historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(response.results[i].historyData.content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]);
        UA_HistoryData * data = (UA_HistoryData *)response.results[i].historyData.content.decoded.data;
        ck_assert(data->dataValuesSize > 1);
        for (size_t j = 0; j < data->dataValuesSize; ++j) {
            ck_assert_uint_eq(data->dataValues[j].hasSourceTimestamp, true);
            ck_assert(data->dataValues[j].sourceTimestamp >= start);
            ck_assert(data->dataValues[j].sourceTimestamp < end);
            ck_assert_uint_eq(data->dataValues[j].hasValue, true);
            ck_assert(data->dataValues[j].value.type == &UA_TYPES[UA_TYPES_UINT32]);
            UA_UInt32 * value = (UA_UInt32 *)data->dataValues[j].value.data;
            // first need to be 43
            if (j == 0) {
                ck_assert(*value == 43);
            } else {
                ck_assert(*value < 5);
            }
        }
    }
    UA_HistoryReadResponse_deleteMembers(&response);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingStrategyValueSet)
{
    // init to a defined value
    UA_StatusCode retval = setUInt32(client, outNodeId, 43);
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // set a data backend
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = UA_HistoryDataBackend_Memory(3, 100);
    setting.maxHistoryDataResponseSize = 100;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_VALUESET;
    serverMutexLock();
    retval = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // fill the data
    UA_fakeSleep(100);
    UA_DateTime start = UA_DateTime_now();
    UA_fakeSleep(100);
    for (UA_UInt32 i = 0; i < 10; ++i) {
        retval = setUInt32(client, outNodeId, i);
        ck_assert_str_eq(UA_StatusCode_name(retval), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        UA_fakeSleep(100);
    }
    UA_DateTime end = UA_DateTime_now();

    // request
    UA_HistoryReadResponse response;
    UA_HistoryReadResponse_init(&response);
    requestHistory(start, end, &response, 0, false, NULL);

    // test the response
    ck_assert_str_eq(UA_StatusCode_name(response.responseHeader.serviceResult), UA_StatusCode_name(UA_STATUSCODE_GOOD));
    ck_assert_uint_eq(response.resultsSize, 1);
    for (size_t i = 0; i < response.resultsSize; ++i) {
        ck_assert_str_eq(UA_StatusCode_name(response.results[i].statusCode), UA_StatusCode_name(UA_STATUSCODE_GOOD));
        ck_assert_uint_eq(response.results[i].historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(response.results[i].historyData.content.decoded.type == &UA_TYPES[UA_TYPES_HISTORYDATA]);
        UA_HistoryData * data = (UA_HistoryData *)response.results[i].historyData.content.decoded.data;
        ck_assert(data->dataValuesSize > 0);
        for (size_t j = 0; j < data->dataValuesSize; ++j) {
            ck_assert(data->dataValues[j].sourceTimestamp >= start && data->dataValues[j].sourceTimestamp < end);
            ck_assert_uint_eq(data->dataValues[j].hasSourceTimestamp, true);
            ck_assert_str_eq(UA_StatusCode_name(data->dataValues[j].status), UA_StatusCode_name(UA_STATUSCODE_GOOD));
            ck_assert_uint_eq(data->dataValues[j].hasValue, true);
            ck_assert(data->dataValues[j].value.type == &UA_TYPES[UA_TYPES_UINT32]);
            UA_UInt32 * value = (UA_UInt32 *)data->dataValues[j].value.data;
            ck_assert_uint_eq(*value, j);
        }
    }
    UA_HistoryReadResponse_deleteMembers(&response);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingBackendMemory)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_Memory(1, 1);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // empty backend should not crash
    UA_UInt32 retval = testHistoricalDataBackend(100);
    fprintf(stderr, "%d tests expected failed.\n", retval);

    // fill backend
    ck_assert_uint_eq(fillHistoricalDataBackend(backend), true);

    // read all in one
    retval = testHistoricalDataBackend(100);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);

    // read continuous one at one request
    retval = testHistoricalDataBackend(1);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);

    // read continuous two at one request
    retval = testHistoricalDataBackend(2);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);
    UA_HistoryDataBackend_Memory_deleteMembers(&setting.historizingBackend);
}
END_TEST

START_TEST(Server_HistorizingRandomIndexBackend)
{
    UA_HistoryDataBackend backend = UA_HistoryDataBackend_randomindextest(testData);
    UA_HistorizingNodeIdSettings setting;
    setting.historizingBackend = backend;
    setting.maxHistoryDataResponseSize = 1000;
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
    serverMutexLock();
    UA_StatusCode ret = gathering->registerNodeId(server, gathering->context, &outNodeId, setting);
    serverMutexUnlock();
    ck_assert_str_eq(UA_StatusCode_name(ret), UA_StatusCode_name(UA_STATUSCODE_GOOD));

    // read all in one
    UA_UInt32 retval = testHistoricalDataBackend(100);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);

    // read continuous one at one request
    retval = testHistoricalDataBackend(1);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);

    // read continuous two at one request
    retval = testHistoricalDataBackend(2);
    fprintf(stderr, "%d tests failed.\n", retval);
    ck_assert_uint_eq(retval, 0);
    UA_HistoryDataBackend_randomindextest_deleteMembers(&backend);
}
END_TEST

#endif /*UA_ENABLE_HISTORIZING*/

static Suite* testSuite_Client(void)
{
    Suite *s = suite_create("Server Historical Data");
    TCase *tc_server = tcase_create("Server Historical Data Basic");
    tcase_add_checked_fixture(tc_server, setup, teardown);
#ifdef UA_ENABLE_HISTORIZING
    tcase_add_test(tc_server, Server_HistorizingStrategyPoll);
    tcase_add_test(tc_server, Server_HistorizingStrategyUser);
    tcase_add_test(tc_server, Server_HistorizingStrategyValueSet);
    tcase_add_test(tc_server, Server_HistorizingBackendMemory);
    tcase_add_test(tc_server, Server_HistorizingRandomIndexBackend);
    tcase_add_test(tc_server, Server_HistorizingUpdateDelete);
    tcase_add_test(tc_server, Server_HistorizingUpdateInsert);
    tcase_add_test(tc_server, Server_HistorizingUpdateReplace);
    tcase_add_test(tc_server, Server_HistorizingUpdateUpdate);
#endif /* UA_ENABLE_HISTORIZING */
    suite_add_tcase(s, tc_server);

    return s;
}

int main(void)
{
#ifdef UA_ENABLE_HISTORIZING
    testDataSorted = sortDateTimes(testData);
#endif /* UA_ENABLE_HISTORIZING */
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
#ifdef UA_ENABLE_HISTORIZING
    UA_free(testDataSorted);
#endif /* UA_ENABLE_HISTORIZING */
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
