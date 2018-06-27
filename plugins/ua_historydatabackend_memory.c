/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 */

#include "ua_historydatabackend_memory.h"
#include <limits.h>
#include <string.h>

typedef struct {
    UA_DateTime timestamp;
    UA_DataValue value;
} UA_DataValueMemoryStoreItem;

static void UA_DataValueMemoryStoreItem_deleteMembers(UA_DataValueMemoryStoreItem* item) {
    UA_DateTime_deleteMembers(&item->timestamp);
    UA_DataValue_deleteMembers(&item->value);
}

typedef struct {
    UA_NodeId nodeId;
    UA_DataValueMemoryStoreItem **dataStore;
    size_t storeEnd;
    size_t storeSize;
} UA_NodeIdStoreContextItem;

static void UA_NodeIdStoreContextItem_deleteMembers(UA_NodeIdStoreContextItem* item) {
    UA_NodeId_deleteMembers(&item->nodeId);
    for (size_t i = 0; i < item->storeEnd; ++i) {
        UA_DataValueMemoryStoreItem_deleteMembers(item->dataStore[i]);
        UA_free(item->dataStore[i]);
    }
    UA_free(item->dataStore);
}

typedef struct {
    UA_NodeIdStoreContextItem **nodeStore;
    size_t storeEnd;
    size_t storeSize;
    size_t initialStoreSize;
} UA_MemoryStoreContext;

static void UA_MemoryStoreContext_deleteMembers(UA_MemoryStoreContext* ctx) {
    for (size_t i = 0; i < ctx->storeEnd; ++i) {
        UA_NodeIdStoreContextItem_deleteMembers(ctx->nodeStore[i]);
        UA_free(ctx->nodeStore[i]);
    }
    UA_free(ctx->nodeStore);
}

static UA_NodeIdStoreContextItem*
getNodeIdStoreContextItem(UA_MemoryStoreContext *ctx, const UA_NodeId *node) {
    for (size_t i = 0; i < ctx->storeEnd; ++i) {
        if (UA_NodeId_equal(&ctx->nodeStore[i]->nodeId, node))
            return ctx->nodeStore[i];
    }
    // not found a node item
    if (ctx->storeEnd >= ctx->storeSize) {
        size_t newStoreSize = ctx->storeSize == 0 ? INITIAL_MEMORY_STORE_SIZE : ctx->storeSize * 2;
        ctx->nodeStore = (UA_NodeIdStoreContextItem **)UA_realloc(ctx->nodeStore,  (newStoreSize * sizeof(UA_NodeIdStoreContextItem*)));
        if (!ctx->nodeStore) {
            ctx->storeSize = 0;
            return NULL;
        }
        ctx->storeSize = newStoreSize;
    }
    UA_NodeIdStoreContextItem *item = (UA_NodeIdStoreContextItem *)UA_calloc(1, sizeof(UA_NodeIdStoreContextItem));
    if (!item)
        return NULL;
    UA_NodeId_copy(node, &item->nodeId);
    item->storeEnd = 0;
    item->storeSize = ctx->initialStoreSize;
    item->dataStore = (UA_DataValueMemoryStoreItem **)UA_calloc(ctx->initialStoreSize, sizeof(UA_DataValueMemoryStoreItem*));
    if (!item->dataStore) {
        UA_free(item);
        return NULL;
    }
    ctx->nodeStore[ctx->storeEnd] = item;
    ++ctx->storeEnd;

    return item;
}
typedef enum {
    MATCH_EQUAL,
    MATCH_AFTER,
    MATCH_EQUAL_OR_AFTER,
    MATCH_BEFORE,
    MATCH_EQUAL_OR_BEFORE
} MatchStrategy;

static UA_Boolean binarySearch(const UA_NodeIdStoreContextItem* item, const UA_DateTime timestamp, size_t *index) {
    if (item->storeEnd == 0) {
        *index = item->storeEnd;
        return false;
    }
    size_t min = 0;
    size_t max = item->storeEnd - 1;
    while (min <= max) {
        *index = (min + max) / 2;
        if (item->dataStore[*index]->timestamp == timestamp) {
            return true;
        } else if (item->dataStore[*index]->timestamp < timestamp) {
            if (*index == item->storeEnd - 1) {
                *index = item->storeEnd;
                return false;
            }
            min = *index + 1;
        } else {
            if (*index == 0)
                return false;
            max = *index - 1;
        }
    }
    *index = min;
    return false;

}
static size_t
getMatch(const UA_NodeIdStoreContextItem* item, const UA_DateTime timestamp, const MatchStrategy strategy ) {
    size_t current;
    UA_Boolean retval = binarySearch(item, timestamp, &current);

    if ((strategy == MATCH_EQUAL
         || strategy == MATCH_EQUAL_OR_AFTER
         || strategy == MATCH_EQUAL_OR_BEFORE)
            && retval)
        return current;
    switch (strategy) {
    case MATCH_AFTER:
        if (retval)
            return current+1;
        return current;
    case MATCH_EQUAL_OR_AFTER:
        return current;
    case MATCH_EQUAL_OR_BEFORE:
        // retval == true aka "equal" is handled before
        // Fall through if !retval
    case MATCH_BEFORE:
        if (current > 0)
            return current-1;
        else
            return item->storeEnd;
    default:
        break;
    }
    return item->storeEnd;
}

static void
UA_MemoryStoreContext_delete(UA_MemoryStoreContext* ctx) {
    UA_MemoryStoreContext_deleteMembers(ctx);
    UA_free(ctx);
}

static size_t
resultSize(const UA_NodeIdStoreContextItem* item,
           size_t startIndex,
           size_t endIndex) {
    if (item->storeEnd == 0
            || startIndex == item->storeEnd
            || endIndex == item->storeEnd)
        return 0;
    return endIndex - startIndex + 1;
}

static size_t
getResultSize(const UA_NodeIdStoreContextItem* item,
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
    *startIndex = item->storeEnd;
    *endIndex = item->storeEnd;
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
    if (item->storeEnd > 0) {
        if (equal) {
            if (returnBounds) {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == item->storeEnd) {
                    *startIndex = getMatch(item, start, MATCH_AFTER);
                    *addFirst = true;
                }
                *endIndex = getMatch(item, start, MATCH_AFTER);
                size = resultSize(item, *startIndex, *endIndex);
            } else {
                *startIndex = getMatch(item, start, MATCH_EQUAL);
                *endIndex = *startIndex;
                if (*startIndex == item->storeEnd)
                    size = 0;
                else
                    size = 1;
            }
        } else if (start == LLONG_MIN) {
            *endIndex = 0;
            if (returnBounds) {
                *addLast = true;
                *startIndex = getMatch(item, end, MATCH_EQUAL_OR_AFTER);
                if (*startIndex == item->storeEnd) {
                    *startIndex = getMatch(item, end, MATCH_EQUAL_OR_BEFORE);
                    *addFirst = true;
                }
            } else {
                *startIndex = getMatch(item, end, MATCH_EQUAL_OR_BEFORE);
            }
            size = resultSize(item, *endIndex, *startIndex);
        } else if (end == LLONG_MIN) {
            *endIndex = item->storeEnd - 1;
            if (returnBounds) {
                *addLast = true;
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == item->storeEnd) {
                    *startIndex = getMatch(item, start, MATCH_AFTER);
                    *addFirst = true;
                }
            } else {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_AFTER);
            }
            size = resultSize(item, *startIndex, *endIndex);
        } else if (*reverse) {
            if (returnBounds) {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_AFTER);
                if (*startIndex == item->storeEnd) {
                    *addFirst = true;
                    *startIndex = getMatch(item, start, MATCH_BEFORE);
                }
                *endIndex = getMatch(item, end, MATCH_EQUAL_OR_BEFORE);
                if (*endIndex == item->storeEnd) {
                    *addLast = true;
                    *endIndex = getMatch(item, end, MATCH_AFTER);
                }
            } else {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_BEFORE);
                *endIndex = getMatch(item, end, MATCH_AFTER);
            }
            size = resultSize(item, *endIndex, *startIndex);
        } else {
            if (returnBounds) {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_BEFORE);
                if (*startIndex == item->storeEnd) {
                    *addFirst = true;
                    *startIndex = getMatch(item, start, MATCH_AFTER);
                }
                *endIndex = getMatch(item, end, MATCH_EQUAL_OR_AFTER);
                if (*endIndex == item->storeEnd) {
                    *addLast = true;
                    *endIndex = getMatch(item, end, MATCH_BEFORE);
                }
            } else {
                *startIndex = getMatch(item, start, MATCH_EQUAL_OR_AFTER);
                *endIndex = getMatch(item, end, MATCH_BEFORE);
            }
            size = resultSize(item, *startIndex, *endIndex);
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
getHistoryDataMemory(void* context,
                     const UA_DateTime start,
                     const UA_DateTime end,
                     UA_NodeId* nodeId,
                     size_t skip,
                     size_t maxSize,
                     UA_UInt32 numValuesPerNode,
                     UA_Boolean returnBounds,
                     UA_DataValue ** result,
                     size_t *resultSize,
                     UA_Boolean *hasMoreData)
{
    UA_MemoryStoreContext *ctx = (UA_MemoryStoreContext*)context;
    UA_NodeIdStoreContextItem *item = getNodeIdStoreContextItem(ctx, nodeId);
    size_t startIndex;
    size_t endIndex;
    UA_Boolean addFirst;
    UA_Boolean addLast;
    UA_Boolean reverse;
    size_t _resultSize = getResultSize(item, start, end, numValuesPerNode, returnBounds,
                                       &startIndex, &endIndex, &addFirst, &addLast, &reverse);
    if (_resultSize <= skip) {
        *resultSize = 0;
        *hasMoreData = false;
        return UA_STATUSCODE_GOOD;
    }
    *resultSize = _resultSize - skip;
    if (*resultSize > maxSize) {
        *resultSize = maxSize;
    }
    *result = (UA_DataValue*)UA_Array_new(*resultSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if (!(*result)) {
        *resultSize = 0;
        *hasMoreData = false;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t index = startIndex;
    size_t counter = 0;
    size_t skipCounter = 0;
    if (addFirst) {
        if (skip == 0) {
            (*result)[counter].hasStatus = true;
            (*result)[counter].status = UA_STATUSCODE_BADBOUNDNOTFOUND;
            (*result)[counter].hasSourceTimestamp = true;
            if (start == LLONG_MIN) {
                (*result)[counter].sourceTimestamp = end;
            } else {
                (*result)[counter].sourceTimestamp = start;
            }
            ++counter;
        }
        ++skipCounter;
    }
    if (endIndex < item->storeEnd && startIndex < item->storeEnd) {
        if (reverse) {
            while (index >= endIndex && index < item->storeEnd && counter < *resultSize) {
                if (skipCounter++ >= skip) {
                    UA_DataValue_copy(&item->dataStore[index]->value, &(*result)[counter]);
                    ++counter;
                }
                --index;
            }
        } else {
            while (index <= endIndex && counter < *resultSize) {
                if (skipCounter++ >= skip) {
                    UA_DataValue_copy(&item->dataStore[index]->value, &(*result)[counter]);
                    ++counter;
                }
                ++index;
            }
        }
    }
    if (addLast && counter < *resultSize) {
        (*result)[counter].hasStatus = true;
        (*result)[counter].status = UA_STATUSCODE_BADBOUNDNOTFOUND;
        (*result)[counter].hasSourceTimestamp = true;
        if (start == LLONG_MIN && item->storeEnd > 0) {
            (*result)[counter].sourceTimestamp = item->dataStore[endIndex]->value.sourceTimestamp - UA_DATETIME_SEC;
        } else if (end == LLONG_MIN && item->storeEnd > 0) {
            (*result)[counter].sourceTimestamp = item->dataStore[endIndex]->value.sourceTimestamp + UA_DATETIME_SEC;
        } else {
            (*result)[counter].sourceTimestamp = end;
        }
    }
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode
addHistoryDataMemory(void* context,
                     UA_DataValue *value,
                     UA_NodeId *nodeId)
{
    UA_MemoryStoreContext *ctx = (UA_MemoryStoreContext*)context;
    UA_NodeIdStoreContextItem *item = getNodeIdStoreContextItem(ctx, nodeId);

    if (item->storeEnd >= item->storeSize) {
        size_t newStoreSize = item->storeSize == 0 ? INITIAL_MEMORY_STORE_SIZE : item->storeSize * 2;
        item->dataStore = (UA_DataValueMemoryStoreItem **)UA_realloc(item->dataStore,  (newStoreSize * sizeof(UA_DataValueMemoryStoreItem*)));
        if (!item->dataStore) {
            item->storeSize = 0;
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        item->storeSize = newStoreSize;
    }
    UA_DateTime timestamp = 0;
    if (value->hasSourceTimestamp) {
        timestamp = value->sourceTimestamp;
    } else if (value->hasServerTimestamp) {
        timestamp = value->serverTimestamp;
    } else {
        timestamp = UA_DateTime_now();
    }
    UA_DataValueMemoryStoreItem *newItem = (UA_DataValueMemoryStoreItem *)UA_calloc(1, sizeof(UA_DataValueMemoryStoreItem));
    newItem->timestamp = timestamp;
    UA_DataValue_copy(value, &newItem->value);
    size_t index = getMatch(item, timestamp, MATCH_EQUAL_OR_AFTER);
    if (item->storeEnd > 0 && index < item->storeEnd) {
        memmove(&item->dataStore[index+1], &item->dataStore[index], sizeof(UA_DataValueMemoryStoreItem*) * (item->storeEnd - index));
    }
    item->dataStore[index] = newItem;
    ++item->storeEnd;
    return UA_STATUSCODE_GOOD;
}

UA_HistoryDataBackend
UA_HistoryDataBackend_Memory(size_t initialNodeStoreSize, size_t initialDataStoreSize) {
    UA_HistoryDataBackend result;
    result.addHistoryData = NULL;
    result.getHistoryData = NULL;
    result.context = NULL;
    UA_MemoryStoreContext *ctx = (UA_MemoryStoreContext *)UA_calloc(1, sizeof(UA_MemoryStoreContext));
    if (!ctx)
        return result;
    ctx->storeEnd = 0;
    ctx->storeSize = initialNodeStoreSize;
    ctx->initialStoreSize = initialDataStoreSize;
    ctx->nodeStore = (UA_NodeIdStoreContextItem **)UA_calloc(initialNodeStoreSize, sizeof(UA_NodeIdStoreContextItem*));
    if (!ctx->nodeStore) {
        UA_free(ctx);
        return result;
    }
    result.addHistoryData = &addHistoryDataMemory;
    result.getHistoryData = &getHistoryDataMemory;
    result.context = ctx;
    return result;
}

void
UA_HistoryDataBackend_Memory_deleteMembers(UA_HistoryDataBackend *backend)
{
    UA_MemoryStoreContext *ctx = (UA_MemoryStoreContext*)backend->context;
    UA_MemoryStoreContext_delete(ctx);
    memset(backend, 0, sizeof(UA_HistoryDataBackend));
}
