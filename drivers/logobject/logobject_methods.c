/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <string.h>

/* The GetRecords and ReleaseContinuationPoint Methods of Part 26 (5.3, 5.4).
 * The Method callbacks run under the server lock, which includes the driver
 * lock. Access is checked by the AccessControl plugin (Part 26, 5.11). */

#define UA_LOGRECORDMASK_ALL                                            \
    (UA_LOGRECORDMASK_EVENTTYPE | UA_LOGRECORDMASK_SOURCENODE |         \
     UA_LOGRECORDMASK_SOURCENAME | UA_LOGRECORDMASK_TRACECONTEXT |      \
     UA_LOGRECORDMASK_ADDITIONALDATA)

static UA_StatusCode
checkOutputArguments(size_t outputSize, size_t expected) {
    return (outputSize == expected) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

static UA_Boolean
scalarInput(const UA_Variant *v, const UA_DataType *type) {
    return v->data != NULL && UA_Variant_hasScalarType(v, type);
}

/***********************/
/* Continuation points */
/***********************/

static void
deleteContinuationPoint(LogObjectContinuationPoint *cp) {
    LIST_REMOVE(cp, pointers);
    UA_NodeId_clear(&cp->sessionId);
    UA_NodeId_clear(&cp->logObjectId);
    UA_free(cp);
}

void
releaseContinuationPoints(LogObjectDriver *lod, const UA_NodeId *sessionId,
                          const UA_NodeId *logObjectId) {
    LogObjectContinuationPoint *cp, *tmp;
    LIST_FOREACH_SAFE(cp, &lod->continuationPoints, pointers, tmp) {
        if(sessionId && !UA_NodeId_equal(&cp->sessionId, sessionId))
            continue;
        if(logObjectId && !UA_NodeId_equal(&cp->logObjectId, logObjectId))
            continue;
        deleteContinuationPoint(cp);
    }
}

static LogObjectContinuationPoint *
findContinuationPoint(LogObjectDriver *lod, const UA_NodeId *sessionId,
                      const UA_ByteString *identifier) {
    if(identifier->length != sizeof(UA_Guid))
        return NULL;
    LogObjectContinuationPoint *cp;
    LIST_FOREACH(cp, &lod->continuationPoints, pointers) {
        if(UA_NodeId_equal(&cp->sessionId, sessionId) &&
           memcmp(identifier->data, &cp->identifier, sizeof(UA_Guid)) == 0)
            return cp;
    }
    return NULL;
}

static size_t
countContinuationPoints(LogObjectDriver *lod, const UA_NodeId *sessionId) {
    size_t count = 0;
    LogObjectContinuationPoint *cp;
    LIST_FOREACH(cp, &lod->continuationPoints, pointers) {
        if(UA_NodeId_equal(&cp->sessionId, sessionId))
            count++;
    }
    return count;
}

/* Remove the optional fields that were not requested (Part 26, 5.8) */
static void
applyRequestMask(UA_LogRecord *r, UA_LogRecordMask mask) {
    if(!(mask & UA_LOGRECORDMASK_EVENTTYPE) && r->eventType) {
        UA_NodeId_delete(r->eventType);
        r->eventType = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_SOURCENODE) && r->sourceNode) {
        UA_NodeId_delete(r->sourceNode);
        r->sourceNode = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_SOURCENAME) && r->sourceName) {
        UA_String_delete(r->sourceName);
        r->sourceName = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_TRACECONTEXT) && r->traceContext) {
        UA_TraceContextDataType_delete(r->traceContext);
        r->traceContext = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_ADDITIONALDATA) && r->additionalData) {
        UA_Array_delete(r->additionalData, r->additionalDataSize,
                        &UA_TYPES[UA_TYPES_NAMEVALUEPAIR]);
        r->additionalData = NULL;
        r->additionalDataSize = 0;
    }
}

/**************/
/* GetRecords */
/**************/

UA_StatusCode
logObjectGetRecordsMethod(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output) {
    UA_StatusCode res = checkOutputArguments(outputSize, 2);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(inputSize < 6)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* The Call service checked the argument types. The RequestMask arrives as
     * LogRecordMask or UInt32, both are backed by UInt32. */
    if(!scalarInput(&input[0], &UA_TYPES[UA_TYPES_DATETIME]) ||
       !scalarInput(&input[1], &UA_TYPES[UA_TYPES_DATETIME]) ||
       !scalarInput(&input[2], &UA_TYPES[UA_TYPES_UINT32]) ||
       !scalarInput(&input[3], &UA_TYPES[UA_TYPES_UINT16]) ||
       !input[4].data || !UA_Variant_isScalar(&input[4]) ||
       input[4].type->typeKind != UA_DATATYPEKIND_UINT32)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DateTime startTime = *(UA_DateTime*)input[0].data;
    UA_DateTime endTime = *(UA_DateTime*)input[1].data;
    UA_UInt32 maxReturnRecords = *(UA_UInt32*)input[2].data;
    UA_UInt16 minimumSeverity = *(UA_UInt16*)input[3].data;
    UA_LogRecordMask requestMask = *(UA_UInt32*)input[4].data;
    const UA_ByteString *cpIn = NULL;
    if(input[5].data && UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_BYTESTRING]))
        cpIn = (const UA_ByteString*)input[5].data;
    UA_Boolean continuation = (cpIn && cpIn->length > 0);

    /* Validate the arguments of a new request (Part 26, Table 3). A
     * continuation uses the arguments of the original call. */
    if(!continuation) {
        if(minimumSeverity < 1 || minimumSeverity > 1000)
            return UA_STATUSCODE_BADOUTOFRANGE;
        if(startTime != 0 && endTime != 0 && startTime > endTime)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        if(requestMask & ~(UA_LogRecordMask)UA_LOGRECORDMASK_ALL)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        /* Zero denotes an unbounded time range */
        if(startTime == 0)
            startTime = UA_INT64_MIN;
        if(endTime == 0)
            endTime = UA_INT64_MAX;
    }

    LogObjectDriver *lod = findLogObjectDriver(server);
    if(!lod || lod->driver.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectEntry *entry = findLogObjectEntry(lod, objectId);
    if(!entry)
        return UA_STATUSCODE_BADNODEIDINVALID;

    /* Continue a previous call */
    UA_LogObjectCursor cursor = 0;
    if(continuation) {
        LogObjectContinuationPoint *cp = findContinuationPoint(lod, sessionId, cpIn);
        if(!cp || !UA_NodeId_equal(&cp->logObjectId, objectId))
            return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        startTime = cp->startTime;
        endTime = cp->endTime;
        minimumSeverity = cp->minimumSeverity;
        requestMask = cp->requestMask;
        maxReturnRecords = cp->maxReturnRecords;
        cursor = cp->cursor;
        deleteContinuationPoint(cp);
    }

    /* The server can impose a limit below MaxReturnRecords (Part 26, 5.3) */
    size_t limit = maxReturnRecords;
    UA_UInt32 cap = lod->maxRecordsPerCall;
    if(cap > 0 && (limit == 0 || limit > cap))
        limit = cap;
    if(limit == 0)
        limit = entry->settings.maxRecords;

    /* Read the records from the backend */
    size_t recordsSize = 0;
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor nextCursor = cursor;
    UA_Boolean moreAvailable = false;
    res = lod->backend.getRecords(server, lod->backend.context, &entry->nodeId,
                                  startTime, endTime, minimumSeverity,
                                  cursor, limit, logObjectNow(lod), &recordsSize,
                                  &records, &nextCursor, &moreAvailable);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Remove the optional fields that were not requested */
    for(size_t i = 0; i < recordsSize; i++)
        applyRequestMask(&records[i], requestMask);

    /* Allocate the output arguments */
    UA_LogRecordsDataType *results = UA_LogRecordsDataType_new();
    UA_ByteString *cpOut = UA_ByteString_new();
    if(!results || !cpOut) {
        UA_LogRecordsDataType_delete(results);
        UA_ByteString_delete(cpOut);
        UA_Array_delete(records, recordsSize, &UA_TYPES[UA_TYPES_LOGRECORD]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Keep a continuation point for the remaining records. The Session limit
     * is enforced without evicting older points (Part 26, Table 3). */
    if(moreAvailable) {
        LogObjectContinuationPoint *cp = NULL;
        if(lod->maxContinuationPoints > 0 &&
           countContinuationPoints(lod, sessionId) >= lod->maxContinuationPoints) {
            res = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
        } else {
            cp = (LogObjectContinuationPoint*)
                UA_calloc(1, sizeof(LogObjectContinuationPoint));
            if(!cp)
                res = UA_STATUSCODE_BADOUTOFMEMORY;
        }
        if(res == UA_STATUSCODE_GOOD)
            res = UA_NodeId_copy(objectId, &cp->logObjectId);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_NodeId_copy(sessionId, &cp->sessionId);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_ByteString_allocBuffer(cpOut, sizeof(UA_Guid));
        if(res != UA_STATUSCODE_GOOD) {
            if(cp) {
                UA_NodeId_clear(&cp->logObjectId);
                UA_NodeId_clear(&cp->sessionId);
                UA_free(cp);
            }
            UA_LogRecordsDataType_delete(results);
            UA_ByteString_delete(cpOut);
            UA_Array_delete(records, recordsSize, &UA_TYPES[UA_TYPES_LOGRECORD]);
            return res;
        }
        cp->identifier = UA_Guid_random();
        memcpy(cpOut->data, &cp->identifier, sizeof(UA_Guid));
        cp->startTime = startTime;
        cp->endTime = endTime;
        cp->minimumSeverity = minimumSeverity;
        cp->requestMask = requestMask;
        cp->maxReturnRecords = maxReturnRecords;
        cp->cursor = nextCursor;
        LIST_INSERT_HEAD(&lod->continuationPoints, cp, pointers);
    }

    /* Set the output arguments. An empty result is an empty array. */
    results->logRecordArraySize = recordsSize;
    results->logRecordArray = records;
    if(!records)
        results->logRecordArray = (UA_LogRecord*)UA_EMPTY_ARRAY_SENTINEL;
    UA_Variant_setScalar(&output[0], results, &UA_TYPES[UA_TYPES_LOGRECORDSDATATYPE]);
    UA_Variant_setScalar(&output[1], cpOut, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return UA_STATUSCODE_GOOD;
}

/****************************/
/* ReleaseContinuationPoint */
/****************************/

UA_StatusCode
logObjectReleaseContinuationPointMethod(UA_Server *server, const UA_NodeId *sessionId,
                                        void *sessionContext, const UA_NodeId *methodId,
                                        void *methodContext, const UA_NodeId *objectId,
                                        void *objectContext, size_t inputSize,
                                        const UA_Variant *input, size_t outputSize,
                                        UA_Variant *output) {
    UA_StatusCode res = checkOutputArguments(outputSize, 0);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(inputSize < 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(!scalarInput(&input[0], &UA_TYPES[UA_TYPES_BYTESTRING]))
        return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
    const UA_ByteString *identifier = (const UA_ByteString*)input[0].data;

    LogObjectDriver *lod = findLogObjectDriver(server);
    if(!lod || lod->driver.state != UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    LogObjectContinuationPoint *cp = findContinuationPoint(lod, sessionId, identifier);
    if(!cp || !UA_NodeId_equal(&cp->logObjectId, objectId))
        return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
    deleteContinuationPoint(cp);
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
