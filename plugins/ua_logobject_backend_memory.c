/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/logobject_backend_memory.h>

#ifdef UA_ENABLE_LOGOBJECT

#include <string.h>

/* Ring buffer of one LogObject. The records are kept in the order of their
 * Time, records with the same Time in the order they were added. The record i
 * (counted from the oldest) lives in slots[(head + i) % maxRecords]. Every
 * record gets a unique sequence number when it is added. The cursor handed
 * out to the core is the sequence number of the last record that was
 * examined, 0 starts with the oldest record. */
typedef struct {
    UA_NodeId logObjectId;
    UA_LogObjectSettings settings;
    UA_LogRecord *slots; /* settings.maxRecords entries */
    UA_UInt64 *seqs;     /* Sequence number of the record in the same slot */
    size_t head;         /* Index of the oldest record */
    size_t count;
    UA_UInt64 lastSeq;   /* Sequence number of the last added record */
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

/* The slot of the i-th record counted from the oldest */
static size_t
slotIndex(const UA_LogObjectMemoryStore *store, size_t i) {
    return (store->head + i) % store->settings.maxRecords;
}

static UA_LogRecord *
slotAt(UA_LogObjectMemoryStore *store, size_t i) {
    return &store->slots[slotIndex(store, i)];
}

static void
evictOldest(UA_LogObjectMemoryStore *store) {
    UA_LogRecord_clear(&store->slots[store->head]);
    store->seqs[store->head] = 0;
    store->head = (store->head + 1) % store->settings.maxRecords;
    store->count--;
}

/* Is the record older than maxStorageDuration? */
static UA_Boolean
isExpired(const UA_LogObjectMemoryStore *store, UA_DateTime time, UA_DateTime now) {
    if(store->settings.maxStorageDuration <= 0.0)
        return false;
    return time < now -
        (UA_DateTime)(store->settings.maxStorageDuration * UA_DATETIME_MSEC);
}

/* Drop the records that are older than maxStorageDuration */
static void
pruneExpired(UA_LogObjectMemoryStore *store, UA_DateTime now) {
    while(store->count > 0 && isExpired(store, store->slots[store->head].time, now))
        evictOldest(store);
}

static void
clearStore(UA_LogObjectMemoryStore *store) {
    UA_Array_delete(store->slots, store->settings.maxRecords,
                    &UA_TYPES[UA_TYPES_LOGRECORD]);
    UA_free(store->seqs);
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
    store->seqs = (UA_UInt64*)UA_calloc(settings->maxRecords, sizeof(UA_UInt64));
    UA_StatusCode res = (store->slots && store->seqs) ?
        UA_NodeId_copy(logObjectId, &store->logObjectId) : UA_STATUSCODE_BADOUTOFMEMORY;
    if(res != UA_STATUSCODE_GOOD) {
        UA_Array_delete(store->slots, settings->maxRecords,
                        &UA_TYPES[UA_TYPES_LOGRECORD]);
        UA_free(store->seqs);
        memset(store, 0, sizeof(UA_LogObjectMemoryStore));
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

    /* A record that has already expired is not stored */
    if(isExpired(store, record->time, now)) {
        UA_UNLOCK(&ctx->lock);
        return UA_STATUSCODE_GOOD;
    }

    /* Find the position in the order of the Time. Usually the record is the
     * newest and appended. Records with the same Time keep the order in which
     * they were added. */
    size_t pos = store->count;
    while(pos > 0 && slotAt(store, pos - 1)->time > record->time)
        pos--;

    /* The ring is full: discard the oldest record. That is the new record
     * itself if it is older than all stored records. */
    if(store->count == store->settings.maxRecords) {
        if(overflow)
            *overflow = true;
        if(pos == 0) {
            UA_UNLOCK(&ctx->lock);
            return UA_STATUSCODE_GOOD;
        }
        evictOldest(store);
        pos--;
    }

    /* Copy before the slots are moved */
    UA_LogRecord copy;
    UA_StatusCode res = UA_LogRecord_copy(record, &copy);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&ctx->lock);
        return res;
    }

    /* Move the newer records by one slot (the ownership moves along) and
     * insert the copy */
    for(size_t i = store->count; i > pos; i--) {
        size_t to = slotIndex(store, i);
        size_t from = slotIndex(store, i - 1);
        store->slots[to] = store->slots[from];
        store->seqs[to] = store->seqs[from];
    }
    size_t idx = slotIndex(store, pos);
    store->slots[idx] = copy;
    store->seqs[idx] = ++store->lastSeq;
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

    /* Continue after the record of the cursor. A cursor that was never handed
     * out continues at the end. If the record of the cursor was evicted or
     * expired, so were all records before it. Then start with the oldest. */
    size_t start = 0;
    if(cursor > store->lastSeq) {
        start = store->count;
    } else if(cursor > 0) {
        for(size_t i = 0; i < store->count; i++) {
            if(store->seqs[slotIndex(store, i)] == cursor) {
                start = i + 1;
                break;
            }
        }
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

    /* The last examined record */
    if(stop > start)
        *nextCursor = store->seqs[slotIndex(store, stop - 1)];
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
