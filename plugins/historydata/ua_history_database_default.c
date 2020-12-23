/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>

#include <limits.h>

typedef struct {
    UA_HistoryDataGathering gathering;
} UA_HistoryDatabaseContext_default;

static size_t
getResultSize_service_default(const UA_HistoryDataBackend* backend,
                              UA_Server *server,
                              const UA_NodeId *sessionId,
                              void* sessionContext,
                              const UA_NodeId *nodeId,
                              UA_DateTime start,
                              UA_DateTime end,
                              UA_UInt32 numValuesPerNode,
                              UA_Boolean returnBounds,
                              size_t *startIndex,
                              size_t *endIndex,
                              UA_Boolean *addFirst,
                              UA_Boolean *addLast,
                              UA_Boolean *reverse)
{
    size_t storeEnd = backend->getEnd(server, backend->context, sessionId, sessionContext, nodeId);
    size_t firstIndex = backend->firstIndex(server, backend->context, sessionId, sessionContext, nodeId);
    size_t lastIndex = backend->lastIndex(server, backend->context, sessionId, sessionContext, nodeId);
    *startIndex = storeEnd;
    *endIndex = storeEnd;
    *addFirst = false;
    *addLast = false;
    if (end == LLONG_MIN) {
        *reverse = false;
    } else if (start == LLONG_MIN) {
        *reverse = true;
    } else {
        *reverse = end < start;
    }
    UA_Boolean equal = start == end;
    size_t size = 0;
    if (lastIndex != storeEnd) {
        if (equal) {
            if (returnBounds) {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == storeEnd) {
                    *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_AFTER);
                    *addFirst = true;
                }
                *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_AFTER);
                size = backend->resultSize(server, backend->context, sessionId, sessionContext, nodeId, *startIndex, *endIndex);
            } else {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL);
                *endIndex = *startIndex;
                if (*startIndex == storeEnd)
                    size = 0;
                else
                    size = 1;
            }
        } else if (start == LLONG_MIN) {
            *endIndex = firstIndex;
            if (returnBounds) {
                *addLast = true;
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_EQUAL_OR_AFTER);
                if (*startIndex == storeEnd) {
                    *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_EQUAL_OR_BEFORE);
                    *addFirst = true;
                }
            } else {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_EQUAL_OR_BEFORE);
            }
            size = backend->resultSize(server, backend->context, sessionId, sessionContext, nodeId, *endIndex, *startIndex);
        } else if (end == LLONG_MIN) {
            *endIndex = lastIndex;
            if (returnBounds) {
                *addLast = true;
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == storeEnd) {
                    *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_AFTER);
                    *addFirst = true;
                }
            } else {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_AFTER);
            }
            size = backend->resultSize(server, backend->context, sessionId, sessionContext, nodeId, *startIndex, *endIndex);
        } else if (*reverse) {
            if (returnBounds) {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_AFTER);
                if (*startIndex == storeEnd) {
                    *addFirst = true;
                    *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_BEFORE);
                }
                *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_EQUAL_OR_BEFORE);
                if (*endIndex == storeEnd) {
                    *addLast = true;
                    *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_AFTER);
                }
            } else {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_BEFORE);
                *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_AFTER);
            }
            size = backend->resultSize(server, backend->context, sessionId, sessionContext, nodeId, *endIndex, *startIndex);
        } else {
            if (returnBounds) {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == storeEnd) {
                    *addFirst = true;
                    *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_AFTER);
                }
                *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_EQUAL_OR_AFTER);
                if (*endIndex == storeEnd) {
                    *addLast = true;
                    *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_BEFORE);
                }
            } else {
                *startIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, start, MATCH_EQUAL_OR_AFTER);
                *endIndex = backend->getDateTimeMatch(server, backend->context, sessionId, sessionContext, nodeId, end, MATCH_BEFORE);
            }
            size = backend->resultSize(server, backend->context, sessionId, sessionContext, nodeId, *startIndex, *endIndex);
        }
    } else if (returnBounds) {
        *addLast = true;
        *addFirst = true;
    }

    if (*addLast)
        ++size;
    if (*addFirst)
        ++size;

    if (numValuesPerNode > 0 && size > numValuesPerNode) {
        size = numValuesPerNode;
        *addLast = false;
    }
    return size;
}

static UA_StatusCode
getHistoryData_service_default(const UA_HistoryDataBackend* backend,
                               const UA_DateTime start,
                               const UA_DateTime end,
                               UA_Server *server,
                               const UA_NodeId *sessionId,
                               void *sessionContext,
                               const UA_NodeId* nodeId,
                               size_t maxSize,
                               UA_UInt32 numValuesPerNode,
                               UA_Boolean returnBounds,
                               UA_TimestampsToReturn timestampsToReturn,
                               UA_NumericRange range,
                               UA_Boolean releaseContinuationPoints,
                               const UA_ByteString *continuationPoint,
                               UA_ByteString *outContinuationPoint,
                               size_t *resultSize,
                               UA_DataValue ** result)
{
    size_t skip = 0;
    UA_ByteString backendContinuationPoint;
    UA_ByteString_init(&backendContinuationPoint);
    if (continuationPoint->length > 0) {
        if (continuationPoint->length >= sizeof(size_t)) {
            skip = *((size_t*)(continuationPoint->data));
            if (continuationPoint->length > 0) {
                backendContinuationPoint.length = continuationPoint->length - sizeof(size_t);
                backendContinuationPoint.data = continuationPoint->data + sizeof(size_t);
            }
        } else {
            return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        }
    }

    size_t storeEnd = backend->getEnd(server, backend->context, sessionId, sessionContext, nodeId);
    size_t startIndex;
    size_t endIndex;
    UA_Boolean addFirst;
    UA_Boolean addLast;
    UA_Boolean reverse;
    size_t _resultSize = getResultSize_service_default(backend,
                                                       server,
                                                       sessionId,
                                                       sessionContext,
                                                       nodeId,
                                                       start,
                                                       end,
                                                       numValuesPerNode == 0 ? 0 : numValuesPerNode + (UA_UInt32)skip,
                                                       returnBounds,
                                                       &startIndex,
                                                       &endIndex,
                                                       &addFirst,
                                                       &addLast,
                                                       &reverse);
    *resultSize = _resultSize - skip;
    if (*resultSize > maxSize) {
        *resultSize = maxSize;
    }
    UA_DataValue *outResult= (UA_DataValue*)UA_Array_new(*resultSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if (!outResult) {
        *resultSize = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    *result = outResult;

    size_t counter = 0;
    if (addFirst) {
        if (skip == 0) {
            outResult[counter].hasStatus = true;
            outResult[counter].status = UA_STATUSCODE_BADBOUNDNOTFOUND;
            outResult[counter].hasSourceTimestamp = true;
            if (start == LLONG_MIN) {
                outResult[counter].sourceTimestamp = end;
            } else {
                outResult[counter].sourceTimestamp = start;
            }
            ++counter;
        }
    }
    UA_ByteString backendOutContinuationPoint;
    UA_ByteString_init(&backendOutContinuationPoint);
    if (endIndex != storeEnd && startIndex != storeEnd) {
        size_t retval = 0;

        size_t valueSize = *resultSize - counter;
        if (valueSize + skip > _resultSize - addFirst - addLast) {
            if (skip == 0) {
                valueSize = _resultSize - addFirst - addLast;
            } else {
                valueSize = _resultSize - skip - addLast;
            }

        }

        UA_StatusCode ret = UA_STATUSCODE_GOOD;
        if (valueSize > 0)
            ret = backend->copyDataValues(server,
                                          backend->context,
                                          sessionId,
                                          sessionContext,
                                          nodeId,
                                          startIndex,
                                          endIndex,
                                          reverse,
                                          valueSize,
                                          range,
                                          releaseContinuationPoints,
                                          &backendContinuationPoint,
                                          &backendOutContinuationPoint,
                                          &retval,
                                          &outResult[counter]);
        if (ret != UA_STATUSCODE_GOOD) {
            UA_Array_delete(outResult, *resultSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
            *result = NULL;
            *resultSize = 0;
            return ret;
        }
        counter += retval;
    }
    if (addLast && counter < *resultSize) {
        outResult[counter].hasStatus = true;
        outResult[counter].status = UA_STATUSCODE_BADBOUNDNOTFOUND;
        outResult[counter].hasSourceTimestamp = true;
        if (start == LLONG_MIN && storeEnd != backend->firstIndex(server, backend->context, sessionId, sessionContext, nodeId)) {
            outResult[counter].sourceTimestamp = backend->getDataValue(server, backend->context, sessionId, sessionContext, nodeId, endIndex)->sourceTimestamp - UA_DATETIME_SEC;
        } else if (end == LLONG_MIN && storeEnd != backend->firstIndex(server, backend->context, sessionId, sessionContext, nodeId)) {
            outResult[counter].sourceTimestamp = backend->getDataValue(server, backend->context, sessionId, sessionContext, nodeId, endIndex)->sourceTimestamp + UA_DATETIME_SEC;
        } else {
            outResult[counter].sourceTimestamp = end;
        }
    }
    // there are more values
    if (skip + *resultSize < _resultSize
            // there are not more values for this request, but there are more values in database
            || (backendOutContinuationPoint.length > 0
                && numValuesPerNode != 0)
            // we deliver just one value which is a FIRST/LAST value
            || (skip == 0
                && addFirst == true
                && *resultSize == 1)) {
        if(UA_ByteString_allocBuffer(outContinuationPoint, backendOutContinuationPoint.length + sizeof(size_t))
                != UA_STATUSCODE_GOOD) {
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        *((size_t*)(outContinuationPoint->data)) = skip + *resultSize;
        if(backendOutContinuationPoint.length > 0)
            memcpy(outContinuationPoint->data + sizeof(size_t), backendOutContinuationPoint.data, backendOutContinuationPoint.length);
    }
    UA_ByteString_deleteMembers(&backendOutContinuationPoint);
    return UA_STATUSCODE_GOOD;
}

static void
updateData_service_default(UA_Server *server,
                           void *hdbContext,
                           const UA_NodeId *sessionId,
                           void *sessionContext,
                           const UA_RequestHeader *requestHeader,
                           const UA_UpdateDataDetails *details,
                           UA_HistoryUpdateResult *result)
{
    UA_HistoryDatabaseContext_default *ctx = (UA_HistoryDatabaseContext_default*)hdbContext;
    UA_Byte accessLevel = 0;
    UA_Server_readAccessLevel(server,
                              details->nodeId,
                              &accessLevel);
    if (!(accessLevel & UA_ACCESSLEVELMASK_HISTORYWRITE)) {
        result->statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    UA_Boolean historizing = false;
    UA_Server_readHistorizing(server,
                              details->nodeId,
                              &historizing);
    if (!historizing) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
        return;
    }
    const UA_HistorizingNodeIdSettings *setting = ctx->gathering.getHistorizingSetting(
                server,
                ctx->gathering.context,
                &details->nodeId);

    if (!setting) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
        return;
    }

    result->operationResultsSize = details->updateValuesSize;
    result->operationResults = (UA_StatusCode*)UA_Array_new(result->operationResultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    for (size_t i = 0; i < details->updateValuesSize; ++i) {
        if (!UA_Server_AccessControl_allowHistoryUpdateUpdateData(server,
                                                                  sessionId,
                                                                  sessionContext,
                                                                  &details->nodeId,
                                                                  details->performInsertReplace,
                                                                  &details->updateValues[i])) {
            result->operationResults[i] = UA_STATUSCODE_BADUSERACCESSDENIED;
            continue;
        }
        switch (details->performInsertReplace) {
        case UA_PERFORMUPDATETYPE_INSERT:
            if (!setting->historizingBackend.insertDataValue) {
                result->operationResults[i] = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
                continue;
            }
            result->operationResults[i]
                    = setting->historizingBackend.insertDataValue(server,
                                                                  setting->historizingBackend.context,
                                                                  sessionId,
                                                                  sessionContext,
                                                                  &details->nodeId,
                                                                  &details->updateValues[i]);
            continue;
        case UA_PERFORMUPDATETYPE_REPLACE:
            if (!setting->historizingBackend.replaceDataValue) {
                result->operationResults[i] = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
                continue;
            }
            result->operationResults[i]
                    = setting->historizingBackend.replaceDataValue(server,
                                                                   setting->historizingBackend.context,
                                                                   sessionId,
                                                                   sessionContext,
                                                                   &details->nodeId,
                                                                   &details->updateValues[i]);
            continue;
        case UA_PERFORMUPDATETYPE_UPDATE:
            if (!setting->historizingBackend.updateDataValue) {
                result->operationResults[i] = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
                continue;
            }
            result->operationResults[i]
                    = setting->historizingBackend.updateDataValue(server,
                                                                  setting->historizingBackend.context,
                                                                  sessionId,
                                                                  sessionContext,
                                                                  &details->nodeId,
                                                                  &details->updateValues[i]);
            continue;
        default:
            result->operationResults[i] = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
            continue;
        }
    }
}


static void
deleteRawModified_service_default(UA_Server *server,
                                  void *hdbContext,
                                  const UA_NodeId *sessionId,
                                  void *sessionContext,
                                  const UA_RequestHeader *requestHeader,
                                  const UA_DeleteRawModifiedDetails *details,
                                  UA_HistoryUpdateResult *result)
{
    if (details->isDeleteModified) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        return;
    }
    UA_HistoryDatabaseContext_default *ctx = (UA_HistoryDatabaseContext_default*)hdbContext;
    UA_Byte accessLevel = 0;
    UA_Server_readAccessLevel(server,
                              details->nodeId,
                              &accessLevel);
    if (!(accessLevel & UA_ACCESSLEVELMASK_HISTORYWRITE)) {
        result->statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    UA_Boolean historizing = false;
    UA_Server_readHistorizing(server,
                              details->nodeId,
                              &historizing);
    if (!historizing) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
        return;
    }
    const UA_HistorizingNodeIdSettings *setting = ctx->gathering.getHistorizingSetting(
                server,
                ctx->gathering.context,
                &details->nodeId);

    if (!setting) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
        return;
    }
    if (!setting->historizingBackend.removeDataValue) {
        result->statusCode = UA_STATUSCODE_BADHISTORYOPERATIONUNSUPPORTED;
        return;
    }

    if (!UA_Server_AccessControl_allowHistoryUpdateDeleteRawModified(server,
                                                                     sessionId,
                                                                     sessionContext,
                                                                     &details->nodeId,
                                                                     details->startTime,
                                                                     details->endTime,
                                                                     details->isDeleteModified)) {
        result->statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    result->statusCode
            = setting->historizingBackend.removeDataValue(server,
                                                          setting->historizingBackend.context,
                                                          sessionId,
                                                          sessionContext,
                                                          &details->nodeId,
                                                          details->startTime,
                                                          details->endTime);
}

static void
readRaw_service_default(UA_Server *server,
                        void *context,
                        const UA_NodeId *sessionId,
                        void *sessionContext,
                        const UA_RequestHeader *requestHeader,
                        const UA_ReadRawModifiedDetails *historyReadDetails,
                        UA_TimestampsToReturn timestampsToReturn,
                        UA_Boolean releaseContinuationPoints,
                        size_t nodesToReadSize,
                        const UA_HistoryReadValueId *nodesToRead,
                        UA_HistoryReadResponse *response,
                        UA_HistoryData * const * const historyData)
{
    UA_HistoryDatabaseContext_default *ctx = (UA_HistoryDatabaseContext_default*)context;
    for (size_t i = 0; i < nodesToReadSize; ++i) {
        UA_Byte accessLevel = 0;
        UA_Server_readAccessLevel(server,
                                  nodesToRead[i].nodeId,
                                  &accessLevel);
        if (!(accessLevel & UA_ACCESSLEVELMASK_HISTORYREAD)) {
            response->results[i].statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
            continue;
        }

        UA_Boolean historizing = false;
        UA_Server_readHistorizing(server,
                                  nodesToRead[i].nodeId,
                                  &historizing);
        if (!historizing) {
            response->results[i].statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
            continue;
        }

        const UA_HistorizingNodeIdSettings *setting = ctx->gathering.getHistorizingSetting(
                    server,
                    ctx->gathering.context,
                    &nodesToRead[i].nodeId);

        if (!setting) {
            response->results[i].statusCode = UA_STATUSCODE_BADHISTORYOPERATIONINVALID;
            continue;
        }

        if (historyReadDetails->returnBounds && !setting->historizingBackend.boundSupported(
                    server,
                    setting->historizingBackend.context,
                    sessionId,
                    sessionContext,
                    &nodesToRead[i].nodeId)) {
            response->results[i].statusCode = UA_STATUSCODE_BADBOUNDNOTSUPPORTED;
            continue;
        }

        if (!setting->historizingBackend.timestampsToReturnSupported(
                    server,
                    setting->historizingBackend.context,
                    sessionId,
                    sessionContext,
                    &nodesToRead[i].nodeId,
                    timestampsToReturn)) {
            response->results[i].statusCode = UA_STATUSCODE_BADTIMESTAMPNOTSUPPORTED;
            continue;
        }

        UA_NumericRange range;
        range.dimensionsSize = 0;
        range.dimensions = NULL;
        if (nodesToRead[i].indexRange.length > 0) {
            UA_StatusCode rangeParseResult = UA_NumericRange_parseFromString(&range, &nodesToRead[i].indexRange);
            if (rangeParseResult != UA_STATUSCODE_GOOD) {
                response->results[i].statusCode = rangeParseResult;
                continue;
            }
        }

        UA_StatusCode getHistoryDataStatusCode;
        if (setting->historizingBackend.getHistoryData) {
            getHistoryDataStatusCode = setting->historizingBackend.getHistoryData(
                        server,
                        sessionId,
                        sessionContext,
                        &setting->historizingBackend,
                        historyReadDetails->startTime,
                        historyReadDetails->endTime,
                        &nodesToRead[i].nodeId,
                        setting->maxHistoryDataResponseSize,
                        historyReadDetails->numValuesPerNode,
                        historyReadDetails->returnBounds,
                        timestampsToReturn,
                        range,
                        releaseContinuationPoints,
                        &nodesToRead[i].continuationPoint,
                        &response->results[i].continuationPoint,
                        historyData[i]);
        } else {
            getHistoryDataStatusCode = getHistoryData_service_default(
                        &setting->historizingBackend,
                        historyReadDetails->startTime,
                        historyReadDetails->endTime,
                        server,
                        sessionId,
                        sessionContext,
                        &nodesToRead[i].nodeId,
                        setting->maxHistoryDataResponseSize,
                        historyReadDetails->numValuesPerNode,
                        historyReadDetails->returnBounds,
                        timestampsToReturn,
                        range,
                        releaseContinuationPoints,
                        &nodesToRead[i].continuationPoint,
                        &response->results[i].continuationPoint,
                        &historyData[i]->dataValuesSize,
                        &historyData[i]->dataValues);
        }
        if (getHistoryDataStatusCode != UA_STATUSCODE_GOOD) {
            response->results[i].statusCode = getHistoryDataStatusCode;
            continue;
        }
    }
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    return;
}

static void
setValue_service_default(UA_Server *server,
                         void *context,
                         const UA_NodeId *sessionId,
                         void *sessionContext,
                         const UA_NodeId *nodeId,
                         UA_Boolean historizing,
                         const UA_DataValue *value)
{
    UA_HistoryDatabaseContext_default *ctx = (UA_HistoryDatabaseContext_default*)context;
    if (ctx->gathering.setValue)
        ctx->gathering.setValue(server,
                                ctx->gathering.context,
                                sessionId,
                                sessionContext,
                                nodeId,
                                historizing,
                                value);
}

static void
deleteMembers_service_default(UA_HistoryDatabase *hdb)
{
    if (hdb == NULL || hdb->context == NULL)
        return;
    UA_HistoryDatabaseContext_default *ctx = (UA_HistoryDatabaseContext_default*)hdb->context;
    ctx->gathering.deleteMembers(&ctx->gathering);
    UA_free(ctx);
}

UA_HistoryDatabase
UA_HistoryDatabase_default(UA_HistoryDataGathering gathering)
{
    UA_HistoryDatabase hdb;
    UA_HistoryDatabaseContext_default *context =
            (UA_HistoryDatabaseContext_default*)
            UA_calloc(1, sizeof(UA_HistoryDatabaseContext_default));
    context->gathering = gathering;
    hdb.context = context;
    hdb.readRaw = &readRaw_service_default;
    hdb.setValue = &setValue_service_default;
    hdb.updateData = &updateData_service_default;
    hdb.deleteRawModified = &deleteRawModified_service_default;
    hdb.deleteMembers = &deleteMembers_service_default;
    return hdb;
}
