/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "test_helpers.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

typedef enum {
    READ_RAW, READ_MODIFIED, READ_EVENT, READ_PROCESSED, READ_ATTIME
} ReadKind;

static UA_Server *server;
static struct {
    ReadKind kind;
    UA_HistoryReadRequest *request;
    UA_HistoryReadResult results[2];
    unsigned calls;
} readProbe;

static const UA_DataType *
resultType(ReadKind kind) {
    if(kind == READ_MODIFIED)
        return &UA_TYPES[UA_TYPES_HISTORYMODIFIEDDATA];
    if(kind == READ_EVENT)
        return &UA_TYPES[UA_TYPES_HISTORYEVENT];
    return &UA_TYPES[UA_TYPES_HISTORYDATA];
}

static void
checkRead(ReadKind kind, UA_Server *s, void *context,
          const UA_NodeId *sessionId, void *sessionContext,
          const UA_RequestHeader *header, const void *details,
          UA_TimestampsToReturn timestamps, UA_Boolean release,
          size_t nodesSize, const UA_HistoryReadValueId *nodes,
          UA_HistoryReadResponse *response, void *first, void *second) {
    ck_assert_ptr_eq(s, server);
    ck_assert_ptr_eq(context, &readProbe);
    ck_assert_uint_eq(kind, readProbe.kind);
    ck_assert_uint_eq(readProbe.calls++, 0);
    ck_assert(UA_NodeId_equal(sessionId, &server->adminSession.sessionId));
    ck_assert_ptr_eq(sessionContext, server->adminSession.context);
    ck_assert_ptr_eq(header, &readProbe.request->requestHeader);
    ck_assert_ptr_eq(details, readProbe.request->historyReadDetails.content.decoded.data);
    ck_assert_uint_eq(timestamps, readProbe.request->timestampsToReturn);
    ck_assert_uint_eq(release, readProbe.request->releaseContinuationPoints);
    ck_assert_uint_eq(nodesSize, 2);
    ck_assert_ptr_eq(nodes, readProbe.request->nodesToRead);
    ck_assert_uint_eq(response->resultsSize, nodesSize);
    ck_assert_ptr_nonnull(first);
    ck_assert_ptr_nonnull(second);
    ck_assert_ptr_ne(first, second);
    ck_assert_ptr_eq(first, response->results[0].historyData.content.decoded.data);
    ck_assert_ptr_eq(second, response->results[1].historyData.content.decoded.data);
    for(size_t i = 0; i < nodesSize; i++) {
        UA_HistoryReadResult *result = &response->results[i];
        ck_assert_uint_eq(result->historyData.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert_ptr_eq(result->historyData.content.decoded.type, resultType(kind));
        ck_assert_uint_eq(UA_copy(readProbe.results[i].historyData.content.decoded.data,
            result->historyData.content.decoded.data, resultType(kind)), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_ByteString_copy(&readProbe.results[i].continuationPoint,
            &result->continuationPoint), UA_STATUSCODE_GOOD);
        result->statusCode = readProbe.results[i].statusCode;
    }
}

static void
readRaw(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
        const UA_RequestHeader *header, const UA_ReadRawModifiedDetails *details,
        UA_TimestampsToReturn timestamps, UA_Boolean release, size_t size,
        const UA_HistoryReadValueId *nodes, UA_HistoryReadResponse *response,
        UA_HistoryData * const * const data) {
    checkRead(READ_RAW, s, context, sessionId, sessionContext, header, details,
              timestamps, release, size, nodes, response, data[0], data[1]);
}

static void
readModified(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
             const UA_RequestHeader *header, const UA_ReadRawModifiedDetails *details,
             UA_TimestampsToReturn timestamps, UA_Boolean release, size_t size,
             const UA_HistoryReadValueId *nodes, UA_HistoryReadResponse *response,
             UA_HistoryModifiedData * const * const data) {
    checkRead(READ_MODIFIED, s, context, sessionId, sessionContext, header, details,
              timestamps, release, size, nodes, response, data[0], data[1]);
}

static void
readEvent(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
          const UA_RequestHeader *header, const UA_ReadEventDetails *details,
          UA_TimestampsToReturn timestamps, UA_Boolean release, size_t size,
          const UA_HistoryReadValueId *nodes, UA_HistoryReadResponse *response,
          UA_HistoryEvent * const * const data) {
    checkRead(READ_EVENT, s, context, sessionId, sessionContext, header, details,
              timestamps, release, size, nodes, response, data[0], data[1]);
}

static void
readProcessed(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
              const UA_RequestHeader *header, const UA_ReadProcessedDetails *details,
              UA_TimestampsToReturn timestamps, UA_Boolean release, size_t size,
              const UA_HistoryReadValueId *nodes, UA_HistoryReadResponse *response,
              UA_HistoryData * const * const data) {
    checkRead(READ_PROCESSED, s, context, sessionId, sessionContext, header, details,
              timestamps, release, size, nodes, response, data[0], data[1]);
}

static void
readAtTime(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
           const UA_RequestHeader *header, const UA_ReadAtTimeDetails *details,
           UA_TimestampsToReturn timestamps, UA_Boolean release, size_t size,
           const UA_HistoryReadValueId *nodes, UA_HistoryReadResponse *response,
           UA_HistoryData * const * const data) {
    checkRead(READ_ATTIME, s, context, sessionId, sessionContext, header, details,
              timestamps, release, size, nodes, response, data[0], data[1]);
}

static void
setup(void) {
    memset(&readProbe, 0, sizeof(readProbe));
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    UA_HistoryDatabase *db = &UA_Server_getConfig(server)->historyDatabase;
    ck_assert_ptr_null(db->context);
    db->context = &readProbe;
    db->readRaw = readRaw;
    db->readModified = readModified;
    db->readEvent = readEvent;
    db->readProcessed = readProcessed;
    db->readAtTime = readAtTime;
}

static void
teardown(void) {
    for(size_t i = 0; i < 2; i++)
        UA_HistoryReadResult_clear(&readProbe.results[i]);
    UA_Server_delete(server);
}

static void
makeReadRequest(UA_HistoryReadRequest *request, ReadKind kind) {
    static const size_t detailTypes[] = {
        UA_TYPES_READRAWMODIFIEDDETAILS, UA_TYPES_READRAWMODIFIEDDETAILS,
        UA_TYPES_READEVENTDETAILS, UA_TYPES_READPROCESSEDDETAILS, UA_TYPES_READATTIMEDETAILS
    };
    UA_HistoryReadRequest_init(request);
    const UA_DataType *type = &UA_TYPES[detailTypes[kind]];
    void *details = UA_new(type);
    ck_assert_ptr_nonnull(details);
    UA_ExtensionObject_setValue(&request->historyReadDetails, details, type);
    if(kind == READ_MODIFIED)
        ((UA_ReadRawModifiedDetails *)details)->isReadModified = true;
    request->requestHeader.requestHandle = 73;
    request->timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    request->nodesToRead = (UA_HistoryReadValueId *)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_HISTORYREADVALUEID]);
    ck_assert_ptr_nonnull(request->nodesToRead);
    request->nodesToReadSize = 2;
    for(size_t i = 0; i < 2; i++) {
        request->nodesToRead[i].nodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)(60000 + i));
        request->nodesToRead[i].indexRange = UA_STRING_ALLOC("1:3");
        request->nodesToRead[i].continuationPoint = UA_BYTESTRING_ALLOC("previous page");
    }
    readProbe.kind = kind;
    readProbe.request = request;
}

static UA_HistoryReadResponse
callRead(UA_HistoryReadRequest *request) {
    UA_HistoryReadResponse response;
    UA_HistoryReadResponse_init(&response);
    lockServer(server);
    UA_Boolean done = Service_HistoryRead(server, &server->adminSession, request, &response);
    unlockServer(server);
    ck_assert(done);
    return response;
}

static void
makeExpectedResults(ReadKind kind, UA_Boolean release) {
    for(size_t i = 0; i < 2; i++) {
        void *data = UA_new(resultType(kind));
        ck_assert_ptr_nonnull(data);
        UA_ExtensionObject_setValue(&readProbe.results[i].historyData, data, resultType(kind));
    }
    if(release)
        return;
    readProbe.results[0].continuationPoint = UA_BYTESTRING_ALLOC("next page");
    readProbe.results[1].statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
    void *data = readProbe.results[0].historyData.content.decoded.data;
    UA_Int32 number = 42;
    if(kind == READ_EVENT) {
        UA_HistoryEvent *events = (UA_HistoryEvent *)data;
        events->events = UA_HistoryEventFieldList_new();
        ck_assert_ptr_nonnull(events->events);
        events->eventsSize = 1;
        events->events[0].eventFields = UA_Variant_new();
        ck_assert_ptr_nonnull(events->events[0].eventFields);
        events->events[0].eventFieldsSize = 1;
        ck_assert_uint_eq(UA_Variant_setScalarCopy(events->events[0].eventFields,
            &number, &UA_TYPES[UA_TYPES_INT32]), UA_STATUSCODE_GOOD);
        return;
    }
    UA_DataValue *value = UA_DataValue_new();
    ck_assert_ptr_nonnull(value);
    value->hasValue = true;
    ck_assert_uint_eq(UA_Variant_setScalarCopy(&value->value, &number,
        &UA_TYPES[UA_TYPES_INT32]), UA_STATUSCODE_GOOD);
    value->hasSourceTimestamp = true;
    value->sourceTimestamp = UA_DATETIME_UNIX_EPOCH + UA_DATETIME_SEC;
    if(kind == READ_MODIFIED) {
        UA_HistoryModifiedData *modified = (UA_HistoryModifiedData *)data;
        modified->dataValuesSize = 1;
        modified->dataValues = value;
        modified->modificationInfos = UA_ModificationInfo_new();
        ck_assert_ptr_nonnull(modified->modificationInfos);
        modified->modificationInfosSize = 1;
        modified->modificationInfos[0].modificationTime = value->sourceTimestamp;
        modified->modificationInfos[0].updateType = UA_HISTORYUPDATETYPE_REPLACE;
        modified->modificationInfos[0].userName = UA_STRING_ALLOC("operator");
    } else {
        UA_HistoryData *history = (UA_HistoryData *)data;
        history->dataValuesSize = 1;
        history->dataValues = value;
    }
}

START_TEST(historyReadDispatchesTypedResults) {
    ReadKind kind = (ReadKind)(_i % 5);
    UA_HistoryReadRequest request;
    makeReadRequest(&request, kind);
    request.releaseContinuationPoints = _i >= 5;
    if(request.releaseContinuationPoints)
        request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    makeExpectedResults(kind, request.releaseContinuationPoints);
    /* The operation limit is inclusive. */
    UA_Server_getConfig(server)->maxNodesPerRead = 2;
    UA_HistoryReadResponse response = callRead(&request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readProbe.calls, 1);
    ck_assert_uint_eq(response.resultsSize, 2);
    for(size_t i = 0; i < 2; i++) {
        ck_assert(UA_HistoryReadResult_equal(&response.results[i], &readProbe.results[i]));
        ck_assert_ptr_ne(response.results[i].historyData.content.decoded.data,
                         readProbe.results[i].historyData.content.decoded.data);
    }
    UA_HistoryReadRequest_clear(&request);
    UA_HistoryReadResponse_clear(&response);
} END_TEST

START_TEST(historyReadRejectsMissingBackend) {
    UA_HistoryReadRequest request;
    makeReadRequest(&request, (ReadKind)_i);
    UA_HistoryDatabase *db = &UA_Server_getConfig(server)->historyDatabase;
    switch((ReadKind)_i) {
    case READ_RAW: db->readRaw = NULL; break;
    case READ_MODIFIED: db->readModified = NULL; break;
    case READ_EVENT: db->readEvent = NULL; break;
    case READ_PROCESSED: db->readProcessed = NULL; break;
    case READ_ATTIME: db->readAtTime = NULL; break;
    }
    UA_HistoryReadResponse response = callRead(&request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert_uint_eq(readProbe.calls, 0);
    ck_assert_uint_eq(response.resultsSize, 0);
    ck_assert_ptr_null(response.results);
    UA_HistoryReadRequest_clear(&request);
    UA_HistoryReadResponse_clear(&response);
} END_TEST

START_TEST(historyReadRejectsInvalidRequest) {
    UA_HistoryReadRequest request;
    makeReadRequest(&request, READ_RAW);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode expected = UA_STATUSCODE_BADNOTSUPPORTED;
    switch(_i) {
    case 0:
        config->historyDatabase.context = NULL;
        break;
    case 1:
        UA_ExtensionObject_clear(&request.historyReadDetails);
        break;
    case 2:
        UA_ExtensionObject_clear(&request.historyReadDetails);
        UA_ExtensionObject_setValue(&request.historyReadDetails, UA_DateTime_new(),
                                    &UA_TYPES[UA_TYPES_DATETIME]);
        expected = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        break;
    case 3:
        UA_Array_delete(request.nodesToRead, request.nodesToReadSize,
                        &UA_TYPES[UA_TYPES_HISTORYREADVALUEID]);
        request.nodesToRead = NULL;
        request.nodesToReadSize = 0;
        expected = UA_STATUSCODE_BADNOTHINGTODO;
        break;
    case 4:
        config->maxNodesPerRead = 1;
        expected = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        break;
    }
    UA_HistoryReadResponse response = callRead(&request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, expected);
    ck_assert_uint_eq(readProbe.calls, 0);
    ck_assert_uint_eq(response.resultsSize, 0);
    ck_assert_ptr_null(response.results);
    UA_HistoryReadRequest_clear(&request);
    UA_HistoryReadResponse_clear(&response);
} END_TEST

static struct {
    UA_HistoryUpdateRequest *request;
    UA_HistoryUpdateResponse *response;
    unsigned calls[6];
} updateProbe;

static void
checkUpdate(UA_Server *s, void *context, const UA_NodeId *sessionId,
            void *sessionContext, const UA_RequestHeader *header,
            const void *details, const UA_DataType *type,
            const UA_NodeId *nodeId, UA_HistoryUpdateResult *result) {
    ck_assert_ptr_eq(s, server);
    ck_assert_ptr_eq(context, &readProbe);
    ck_assert(UA_NodeId_equal(sessionId, &server->adminSession.sessionId));
    ck_assert_ptr_eq(sessionContext, server->adminSession.context);
    ck_assert_ptr_eq(header, &updateProbe.request->requestHeader);
    size_t index;
    for(index = 0; index < updateProbe.request->historyUpdateDetailsSize; index++) {
        if(updateProbe.request->historyUpdateDetails[index].encoding == UA_EXTENSIONOBJECT_DECODED &&
           updateProbe.request->historyUpdateDetails[index].content.decoded.data == details)
            break;
    }
    ck_assert_uint_lt(index, updateProbe.request->historyUpdateDetailsSize);
    ck_assert_ptr_eq(type, updateProbe.request->historyUpdateDetails[index].content.decoded.type);
    UA_NodeId expectedId = UA_NODEID_NUMERIC(1, (UA_UInt32)(60000 + index));
    ck_assert(UA_NodeId_equal(nodeId, &expectedId));
    ck_assert_uint_eq(updateProbe.calls[index]++, 0);
    ck_assert_ptr_eq(result, &updateProbe.response->results[index]);
    result->operationResults = (UA_StatusCode *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STATUSCODE]);
    ck_assert_ptr_nonnull(result->operationResults);
    result->operationResultsSize = 2;
    result->operationResults[0] = UA_STATUSCODE_GOOD;
    result->operationResults[1] = UA_STATUSCODE_BADOUTOFRANGE;
}

static void
updateData(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
           const UA_RequestHeader *header, const UA_UpdateDataDetails *details,
           UA_HistoryUpdateResult *result) {
    ck_assert_uint_eq(details->performInsertReplace, UA_PERFORMUPDATETYPE_UPDATE);
    checkUpdate(s, context, sessionId, sessionContext, header, details,
                &UA_TYPES[UA_TYPES_UPDATEDATADETAILS], &details->nodeId, result);
}

static void
deleteRawModified(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
                  const UA_RequestHeader *header, const UA_DeleteRawModifiedDetails *details,
                  UA_HistoryUpdateResult *result) {
    ck_assert(details->isDeleteModified);
    checkUpdate(s, context, sessionId, sessionContext, header, details,
                &UA_TYPES[UA_TYPES_DELETERAWMODIFIEDDETAILS], &details->nodeId, result);
}

static void
deleteEvent(UA_Server *s, void *context, const UA_NodeId *sessionId, void *sessionContext,
            const UA_RequestHeader *header, const UA_DeleteEventDetails *details,
            UA_HistoryUpdateResult *result) {
    ck_assert_uint_eq(details->eventIdsSize, 1);
    UA_ByteString eventId = UA_BYTESTRING("event to delete");
    ck_assert(UA_ByteString_equal(&details->eventIds[0], &eventId));
    checkUpdate(s, context, sessionId, sessionContext, header, details,
                &UA_TYPES[UA_TYPES_DELETEEVENTDETAILS], &details->nodeId, result);
}

START_TEST(historyUpdateKeepsMixedResultsInOrder) {
    memset(&updateProbe, 0, sizeof(updateProbe));
    UA_HistoryDatabase *db = &UA_Server_getConfig(server)->historyDatabase;
    db->updateData = (_i & 1) ? updateData : NULL;
    db->deleteRawModified = (_i & 2) ? deleteRawModified : NULL;
    db->deleteEvent = (_i & 4) ? deleteEvent : NULL;
    UA_HistoryUpdateRequest request;
    UA_HistoryUpdateRequest_init(&request);
    request.requestHeader.requestHandle = 81;
    request.historyUpdateDetails = (UA_ExtensionObject *)
        UA_Array_new(6, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_ptr_nonnull(request.historyUpdateDetails);
    request.historyUpdateDetailsSize = 6;
    for(size_t i = 0; i < 6; i++) {
        UA_NodeId id = UA_NODEID_NUMERIC(1, (UA_UInt32)(60000 + i));
        if(i == 0 || i == 5) {
            UA_UpdateDataDetails *details = UA_UpdateDataDetails_new();
            ck_assert_ptr_nonnull(details);
            details->nodeId = id;
            details->performInsertReplace = UA_PERFORMUPDATETYPE_UPDATE;
            UA_ExtensionObject_setValue(&request.historyUpdateDetails[i], details,
                                        &UA_TYPES[UA_TYPES_UPDATEDATADETAILS]);
        } else if(i == 1) {
            UA_DateTime *details = UA_DateTime_new();
            ck_assert_ptr_nonnull(details);
            UA_ExtensionObject_setValue(&request.historyUpdateDetails[i], details,
                                        &UA_TYPES[UA_TYPES_DATETIME]);
        } else if(i == 2) {
            UA_DeleteRawModifiedDetails *details = UA_DeleteRawModifiedDetails_new();
            ck_assert_ptr_nonnull(details);
            details->nodeId = id;
            details->isDeleteModified = true;
            UA_ExtensionObject_setValue(&request.historyUpdateDetails[i], details,
                                        &UA_TYPES[UA_TYPES_DELETERAWMODIFIEDDETAILS]);
        } else if(i == 4) {
            UA_DeleteEventDetails *details = UA_DeleteEventDetails_new();
            ck_assert_ptr_nonnull(details);
            details->nodeId = id;
            details->eventIds = UA_ByteString_new();
            ck_assert_ptr_nonnull(details->eventIds);
            details->eventIdsSize = 1;
            details->eventIds[0] = UA_BYTESTRING_ALLOC("event to delete");
            UA_ExtensionObject_setValue(&request.historyUpdateDetails[i], details,
                                        &UA_TYPES[UA_TYPES_DELETEEVENTDETAILS]);
        }
        /* Entry 3 deliberately remains an undecoded ExtensionObject. The
         * successful entry after it must still reach the configured backend. */
    }
    UA_HistoryUpdateResponse response;
    UA_HistoryUpdateResponse_init(&response);
    updateProbe.request = &request;
    updateProbe.response = &response;
    lockServer(server);
    UA_Boolean done = Service_HistoryUpdate(server, &server->adminSession, &request, &response);
    unlockServer(server);
    ck_assert(done);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 6);
    for(size_t i = 0; i < 6; i++) {
        unsigned required = (i == 0 || i == 5) ? 1 : (i == 2 ? 2 : (i == 4 ? 4 : 0));
        UA_Boolean supported = ((unsigned)_i & required) != 0;
        UA_HistoryUpdateResult *result = &response.results[i];
        ck_assert_uint_eq(updateProbe.calls[i], supported ? 1 : 0);
        ck_assert_uint_eq(result->statusCode, supported ? UA_STATUSCODE_GOOD :
                          UA_STATUSCODE_BADNOTSUPPORTED);
        ck_assert_uint_eq(result->operationResultsSize, supported ? 2 : 0);
        if(supported) {
            ck_assert_uint_eq(result->operationResults[0], UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(result->operationResults[1], UA_STATUSCODE_BADOUTOFRANGE);
        } else {
            ck_assert_ptr_null(result->operationResults);
        }
    }
    UA_HistoryUpdateResponse_clear(&response);
    UA_HistoryUpdateRequest_clear(&request);
} END_TEST

int main(void) {
    Suite *suite = suite_create("History services");
    TCase *tc = tcase_create("History read dispatch");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, historyReadDispatchesTypedResults, 0, 10);
    tcase_add_loop_test(tc, historyReadRejectsMissingBackend, 0, 5);
    tcase_add_loop_test(tc, historyReadRejectsInvalidRequest, 0, 5);
    suite_add_tcase(suite, tc);
    tc = tcase_create("History update dispatch");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, historyUpdateKeepsMixedResultsInOrder, 0, 8);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
