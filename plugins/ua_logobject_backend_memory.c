/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/logobject_backend_memory.h>

#ifdef UA_ENABLE_LOGOBJECT

#include <string.h>

/* Ring buffer of one LogObject. The record i (counted from the oldest) lives
 * in slots[(head + i) % maxRecords] and has the sequence number firstSeq + i.
 * The sequence number is the cursor handed out to the core. */
typedef struct {
    UA_NodeId logObjectId;
    UA_LogObjectSettings settings;
    UA_LogRecord *slots; /* settings.maxRecords entries */
    size_t head;         /* Index of the oldest record */
    size_t count;
    UA_UInt64 firstSeq;  /* Sequence number of the oldest record */
} UA_LogObjectMemoryStore;

typedef struct {
#if UA_MULTITHREADING >= 100
    UA_Lock lock;
#endif
    size_t storesSize;
    UA_LogObjectMemoryStore *stores;
} UA_LogObjectMemoryContext;

static UA_LogObjectMemoryStore *
findStore(UA_LogObjectMemoryContext *ctx, const UA_NodeId *logObjectId) {
    for(size_t i = 0; i < ctx->storesSize; i++) {
        if(UA_NodeId_equal(&ctx->stores[i].logObjectId, logObjectId))
            return &ctx->stores[i];
    }
    return NULL;
}

/* The i-th record counted from the oldest */
static UA_LogRecord *
slotAt(UA_LogObjectMemoryStore *store, size_t i) {
    return &store->slots[(store->head + i) % store->settings.maxRecords];
}

static void
evictOldest(UA_LogObjectMemoryStore *store) {
    UA_LogRecord_clear(&store->slots[store->head]);
    store->head = (store->head + 1) % store->settings.maxRecords;
    store->count--;
    store->firstSeq++;
}

/* Drop the records that are older than maxStorageDuration */
static void
pruneExpired(UA_LogObjectMemoryStore *store, UA_DateTime now) {
    if(store->settings.maxStorageDuration <= 0.0)
        return;
    UA_DateTime limit = now -
        (UA_DateTime)(store->settings.maxStorageDuration * UA_DATETIME_MSEC);
    while(store->count > 0 && store->slots[store->head].time < limit)
        evictOldest(store);
}

static void
clearStore(UA_LogObjectMemoryStore *store) {
    UA_Array_delete(store->slots, store->settings.maxRecords,
                    &UA_TYPES[UA_TYPES_LOGRECORD]);
    UA_NodeId_clear(&store->logObjectId);
    memset(store, 0, sizeof(UA_LogObjectMemoryStore));
}

/* Copy a record. Optional fields that are not requested are left out. */
static UA_StatusCode
copyRecordMasked(const UA_LogRecord *src, UA_LogRecord *dst,
                 UA_LogRecordMask mask) {
    UA_LogRecord_init(dst);
    dst->time = src->time;
    dst->severity = src->severity;
    UA_StatusCode res = UA_LocalizedText_copy(&src->message, &dst->message);
    if((mask & UA_LOGRECORDMASK_EVENTTYPE) && src->eventType) {
        dst->eventType = UA_NodeId_new();
        if(!dst->eventType)
            res |= UA_STATUSCODE_BADOUTOFMEMORY;
        else
            res |= UA_NodeId_copy(src->eventType, dst->eventType);
    }
    if((mask & UA_LOGRECORDMASK_SOURCENODE) && src->sourceNode) {
        dst->sourceNode = UA_NodeId_new();
        if(!dst->sourceNode)
            res |= UA_STATUSCODE_BADOUTOFMEMORY;
        else
            res |= UA_NodeId_copy(src->sourceNode, dst->sourceNode);
    }
    if((mask & UA_LOGRECORDMASK_SOURCENAME) && src->sourceName) {
        dst->sourceName = UA_String_new();
        if(!dst->sourceName)
            res |= UA_STATUSCODE_BADOUTOFMEMORY;
        else
            res |= UA_String_copy(src->sourceName, dst->sourceName);
    }
    if((mask & UA_LOGRECORDMASK_TRACECONTEXT) && src->traceContext) {
        dst->traceContext = UA_TraceContextDataType_new();
        if(!dst->traceContext)
            res |= UA_STATUSCODE_BADOUTOFMEMORY;
        else
            res |= UA_TraceContextDataType_copy(src->traceContext,
                                                dst->traceContext);
    }
    if((mask & UA_LOGRECORDMASK_ADDITIONALDATA) && src->additionalDataSize > 0) {
        res |= UA_Array_copy(src->additionalData, src->additionalDataSize,
                             (void**)&dst->additionalData,
                             &UA_TYPES[UA_TYPES_NAMEVALUEPAIR]);
        if(res == UA_STATUSCODE_GOOD)
            dst->additionalDataSize = src->additionalDataSize;
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_LogRecord_clear(dst);
    return res;
}

static UA_StatusCode
memory_registerLogObject(UA_Server *server, void *context,
                         const UA_NodeId *logObjectId,
                         const UA_LogObjectSettings *settings) {
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)context;
    if(!ctx || !logObjectId || !settings || settings->maxRecords == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&ctx->lock);
    if(findStore(ctx, logObjectId)) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    UA_LogObjectMemoryStore *stores = (UA_LogObjectMemoryStore*)
        UA_realloc(ctx->stores, (ctx->storesSize + 1) * sizeof(UA_LogObjectMemoryStore));
    if(!stores) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ctx->stores = stores;

    UA_LogObjectMemoryStore *store = &stores[ctx->storesSize];
    memset(store, 0, sizeof(UA_LogObjectMemoryStore));
    store->slots = (UA_LogRecord*)
        UA_Array_new(settings->maxRecords, &UA_TYPES[UA_TYPES_LOGRECORD]);
    if(!store->slots) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_StatusCode res = UA_NodeId_copy(logObjectId, &store->logObjectId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(store->slots, settings->maxRecords,
                        &UA_TYPES[UA_TYPES_LOGRECORD]);
        store->slots = NULL;
        UA_UNLOCK(&ctx->lock);
        return res;
    }
    store->settings = *settings;
    ctx->storesSize++;
    UA_UNLOCK(&ctx->lock);
    return UA_STATUSCODE_GOOD;
}

static void
memory_unregisterLogObject(UA_Server *server, void *context,
                           const UA_NodeId *logObjectId) {
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)context;
    if(!ctx || !logObjectId)
        return;
    UA_LOCK(&ctx->lock);
    UA_LogObjectMemoryStore *store = findStore(ctx, logObjectId);
    if(store) {
        clearStore(store);
        size_t idx = (size_t)(store - ctx->stores);
        size_t remaining = ctx->storesSize - idx - 1;
        if(remaining > 0)
            memmove(store, store + 1, remaining * sizeof(UA_LogObjectMemoryStore));
        ctx->storesSize--;
    }
    UA_UNLOCK(&ctx->lock);
}

static UA_StatusCode
memory_addRecord(UA_Server *server, void *context,
                 const UA_NodeId *logObjectId, const UA_LogRecord *record,
                 UA_DateTime now, UA_Boolean *overflow) {
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)context;
    if(overflow)
        *overflow = false;
    if(!ctx || !logObjectId || !record)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&ctx->lock);
    UA_LogObjectMemoryStore *store = findStore(ctx, logObjectId);
    if(!store) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    pruneExpired(store, now);

    /* Evict the oldest record if the ring is full */
    if(store->count == store->settings.maxRecords) {
        evictOldest(store);
        if(overflow)
            *overflow = true;
    }

    UA_LogRecord *slot = slotAt(store, store->count);
    UA_StatusCode res = UA_LogRecord_copy(record, slot);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&ctx->lock);
        return res;
    }

    /* Keep the insertion order chronological. GetRecords returns the records
     * in the order of their Time. */
    if(store->count > 0) {
        UA_DateTime last = slotAt(store, store->count - 1)->time;
        if(slot->time < last)
            slot->time = last;
    }
    store->count++;
    UA_UNLOCK(&ctx->lock);
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
recordMatches(const UA_LogRecord *r, UA_DateTime startTime,
              UA_DateTime endTime, UA_UInt16 minimumSeverity) {
    return r->time >= startTime && r->time <= endTime &&
        r->severity >= minimumSeverity;
}

static UA_StatusCode
memory_getRecords(UA_Server *server, void *context,
                  const UA_NodeId *logObjectId,
                  UA_DateTime startTime, UA_DateTime endTime,
                  UA_UInt16 minimumSeverity, UA_LogRecordMask requestMask,
                  UA_LogObjectCursor cursor, size_t maxRecords, UA_DateTime now,
                  size_t *recordsSize, UA_LogRecord **records,
                  UA_LogObjectCursor *nextCursor, UA_Boolean *moreAvailable) {
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)context;
    *recordsSize = 0;
    *records = NULL;
    *nextCursor = cursor;
    *moreAvailable = false;
    if(!ctx || !logObjectId || maxRecords == 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&ctx->lock);
    UA_LogObjectMemoryStore *store = findStore(ctx, logObjectId);
    if(!store) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    pruneExpired(store, now);

    /* Map the cursor to an index. Cursors before the oldest retained record
     * are clamped to the oldest record. */
    size_t start = 0;
    if(cursor > store->firstSeq) {
        UA_UInt64 offset = cursor - store->firstSeq;
        start = (offset > store->count) ? store->count : (size_t)offset;
    }

    /* Count the records to return. Stop at the first record after endTime,
     * the records are stored in chronological order. */
    size_t out = 0;
    size_t stop = start;
    UA_Boolean pastEnd = false;
    while(stop < store->count && out < maxRecords) {
        const UA_LogRecord *r = slotAt(store, stop);
        if(r->time > endTime) {
            pastEnd = true;
            break;
        }
        if(recordMatches(r, startTime, endTime, minimumSeverity))
            out++;
        stop++;
    }

    /* Copy the records */
    if(out > 0) {
        UA_LogRecord *result = (UA_LogRecord*)
            UA_Array_new(out, &UA_TYPES[UA_TYPES_LOGRECORD]);
        if(!result) {
            UA_UNLOCK(&ctx->lock);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        size_t k = 0;
        for(size_t i = start; i < stop; i++) {
            const UA_LogRecord *r = slotAt(store, i);
            if(!recordMatches(r, startTime, endTime, minimumSeverity))
                continue;
            UA_StatusCode res = copyRecordMasked(r, &result[k], requestMask);
            if(res != UA_STATUSCODE_GOOD) {
                UA_Array_delete(result, out, &UA_TYPES[UA_TYPES_LOGRECORD]);
                UA_UNLOCK(&ctx->lock);
                return res;
            }
            k++;
        }
        *records = result;
        *recordsSize = out;
    }

    /* Are there matching records after the returned ones? */
    if(!pastEnd) {
        for(size_t i = stop; i < store->count; i++) {
            const UA_LogRecord *r = slotAt(store, i);
            if(r->time > endTime)
                break;
            if(recordMatches(r, startTime, endTime, minimumSeverity)) {
                *moreAvailable = true;
                break;
            }
        }
    }

    *nextCursor = store->firstSeq + stop;
    UA_UNLOCK(&ctx->lock);
    return UA_STATUSCODE_GOOD;
}

static void
memory_clear(UA_LogObjectBackend *backend) {
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)backend->context;
    if(!ctx)
        return;
    for(size_t i = 0; i < ctx->storesSize; i++)
        clearStore(&ctx->stores[i]);
    UA_free(ctx->stores);
    UA_LOCK_DESTROY(&ctx->lock);
    UA_free(ctx);
    backend->context = NULL;
}

UA_LogObjectBackend
UA_LogObjectBackend_Memory(void) {
    UA_LogObjectBackend backend;
    memset(&backend, 0, sizeof(UA_LogObjectBackend));
    UA_LogObjectMemoryContext *ctx = (UA_LogObjectMemoryContext*)
        UA_calloc(1, sizeof(UA_LogObjectMemoryContext));
    if(!ctx)
        return backend;
    UA_LOCK_INIT(&ctx->lock);
    backend.context = ctx;
    backend.clear = memory_clear;
    backend.registerLogObject = memory_registerLogObject;
    backend.unregisterLogObject = memory_unregisterLogObject;
    backend.addRecord = memory_addRecord;
    backend.getRecords = memory_getRecords;
    return backend;
}

#endif /* UA_ENABLE_LOGOBJECT */
