#ifndef BACKEND_H
#define BACKEND_H

#include <open62541/plugin/historydata/history_data_backend.h>

struct tupel {
    size_t index;
    UA_DataValue value;
};

// null terminated array
struct context_randomindextest {
    // null terminated array
    struct tupel *tupels;
    size_t tupelSize;
};

static size_t
indexByIndex_randomindextest(struct context_randomindextest *context, size_t index) {
    for(size_t i = 0; i < context->tupelSize; ++i) {
        if(context->tupels[i].index == index)
            return i;
    }
    return context->tupelSize;
}

// last value should be NULL, all values must be sorted
static struct context_randomindextest *
generateTestContext_randomindextest(const UA_DateTime *data) {
    size_t count = 0;
    while(data[count++]);
    struct context_randomindextest* ret = (struct context_randomindextest*)UA_calloc(1, sizeof(struct context_randomindextest));
    if(!ret)
        return NULL;

    ret->tupels = (struct tupel*)UA_calloc(count, sizeof(struct tupel));
    if(!ret->tupels) {
        UA_free(ret);
        return NULL;
    }
    ret->tupelSize = count;

    UA_DateTime *sortData;
    UA_StatusCode retval = UA_Array_copy(data, count, (void**)&sortData, &UA_TYPES[UA_TYPES_DATETIME]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ret->tupels);
        UA_free(ret);
        return NULL;
    }

    for(size_t i = 0; i < count; ++i) {
        size_t current = 0;
        for(size_t j = 1; j < count-1; ++j){
            UA_DateTime currentDate = sortData[current];
            UA_DateTime jDate = sortData[j];
            if(currentDate > jDate)
                current = j;
        }
        UA_DateTime nextValue = i == count-1 ? 0 : sortData[current];
        sortData[current] = LLONG_MAX;
        bool unique;
        do {
            unique = true;
            ret->tupels[i].index = UA_UInt32_random();
            for(size_t j = 0; j < i; ++j)
                if(i != j && ret->tupels[i].index == ret->tupels[j].index)
                    unique = false;
        } while (!unique);
        UA_DataValue_init(&ret->tupels[i].value);
        ret->tupels[i].value.hasValue = true;
        UA_Variant_setScalarCopy(&ret->tupels[i].value.value, &nextValue, &UA_TYPES[UA_TYPES_INT64]);
        ret->tupels[i].value.hasStatus = true;
        ret->tupels[i].value.status = UA_STATUSCODE_GOOD;
        ret->tupels[i].value.hasServerTimestamp = true;
        ret->tupels[i].value.serverTimestamp = nextValue;
        ret->tupels[i].value.hasSourceTimestamp = true;
        ret->tupels[i].value.sourceTimestamp = nextValue;
    }
    UA_free(sortData);
    return ret;
}

static void
deleteMembers_randomindextest(UA_HistoryDataBackend *backend) {
    struct context_randomindextest* context = (struct context_randomindextest*)backend->context;
    if(!context)
        return;
    for(size_t i = 0; i < context->tupelSize; ++i) {
        UA_DataValue_clear(&context->tupels[i].value);
    }
    UA_free(context->tupels);
    UA_free(context);
}

static UA_StatusCode
serverSetHistoryData_randomindextest(UA_Server *server,
                                     void *hdbContext,
                                     const UA_NodeId *sessionId,
                                     void *sessionContext,
                                     const UA_NodeId *nodeId,
                                     UA_Boolean historizing,
                                     const UA_DataValue *value)
{
    // we do not add data to the test data
    return UA_STATUSCODE_GOOD;
}

static size_t
getEnd_private(struct context_randomindextest* context)
{
    return context->tupels[context->tupelSize-1].index;
}

static size_t
getEnd_randomindextest(UA_Server *server,
                       void *hdbContext,
                       const UA_NodeId *sessionId,
                       void *sessionContext,
                       const UA_NodeId *nodeId)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    return getEnd_private(context);
}

static size_t
lastIndex_randomindextest(UA_Server *server,
                          void *hdbContext,
                          const UA_NodeId *sessionId,
                          void *sessionContext,
                          const UA_NodeId *nodeId)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    return context->tupels[context->tupelSize-2].index;
}

static size_t
firstIndex_randomindextest(UA_Server *server,
                           void *hdbContext,
                           const UA_NodeId *sessionId,
                           void *sessionContext,
                           const UA_NodeId *nodeId)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    return context->tupels[0].index;
}

static UA_Boolean
search_randomindextest(struct context_randomindextest* ctx,
                            const UA_DateTime timestamp,
                            size_t *index) {
    for (size_t i = 0; i < ctx->tupelSize; ++i) {
        if (ctx->tupels[i].value.sourceTimestamp == timestamp) {
            *index = i;
            return true;
        }
        if (ctx->tupels[i].value.sourceTimestamp > timestamp) {
            *index = i;
            return false;
        }
    }
    *index = ctx->tupelSize-1;
    return false;
}

static size_t
getDateTimeMatch_randomindextest(UA_Server *server,
                                 void *hdbContext,
                                 const UA_NodeId *sessionId,
                                 void *sessionContext,
                                 const UA_NodeId *nodeId,
                                 const UA_DateTime timestamp,
                                 const MatchStrategy strategy)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    size_t current;

    UA_Boolean retval = search_randomindextest(context, timestamp, &current);

    if ((strategy == MATCH_EQUAL
         || strategy == MATCH_EQUAL_OR_AFTER
         || strategy == MATCH_EQUAL_OR_BEFORE)
            && retval)
        return context->tupels[current].index;
    switch (strategy) {
    case MATCH_AFTER:
        if (retval)
            return context->tupels[current+1].index;
        return context->tupels[current].index;
    case MATCH_EQUAL_OR_AFTER:
        return context->tupels[current].index;
    case MATCH_EQUAL_OR_BEFORE:
        // retval == true aka "equal" is handled before
        // Fall through if !retval
    case MATCH_BEFORE:
        if (current > 0)
            return context->tupels[current-1].index;
        else
            return context->tupels[context->tupelSize-1].index;
    default:
        break;
    }
    return context->tupels[context->tupelSize-1].index;
}


#define MYABSSUB(a,b) ((a)<(b)?((b)-(a)):(a)-(b))
static size_t
resultSize_randomindextest(UA_Server *server,
                           void *hdbContext,
                           const UA_NodeId *sessionId,
                           void *sessionContext,
                           const UA_NodeId *nodeId,
                           size_t startIndex,
                           size_t endIndex)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    if (startIndex == getEnd_private(context)
            || endIndex == getEnd_private(context))
        return 0;
    size_t realEndIndex = indexByIndex_randomindextest(context, endIndex);
    size_t realStartIndex = indexByIndex_randomindextest(context, startIndex);
    size_t result = MYABSSUB(realEndIndex,realStartIndex);
    return result+1;
}

static UA_StatusCode
copyDataValues_randomindextest(UA_Server *server,
                               void *hdbContext,
                               const UA_NodeId *sessionId,
                               void *sessionContext,
                               const UA_NodeId *nodeId,
                               size_t startIndex,
                               size_t endIndex,
                               UA_Boolean reverse,
                               size_t maxValues,
                               UA_NumericRange range,
                               UA_Boolean releaseContinuationPoints,
                               const UA_ByteString *continuationPoint,
                               UA_ByteString *outContinuationPoint,
                               size_t *providedValues,
                               UA_DataValue *values)
{
    size_t skip = 0;
    if (continuationPoint->length > 0) {
        if (continuationPoint->length == sizeof(size_t)) {
            skip = *((size_t*)(continuationPoint->data));
        } else {
            return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        }
    }
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    size_t index = indexByIndex_randomindextest(context,startIndex);
    size_t counter = 0;
    size_t skipedValues = 0;
    if (reverse) {
        while (index >= indexByIndex_randomindextest(context,endIndex)
               && index < context->tupelSize-1
               && counter < maxValues) {
            if (skipedValues++ >= skip) {
                UA_DataValue_copy(&context->tupels[index].value, &values[counter]);
                ++counter;
            }
            --index;
        }
    } else {
        while (index <= indexByIndex_randomindextest(context,endIndex) && counter < maxValues) {
            if (skipedValues++ >= skip) {
                UA_DataValue_copy(&context->tupels[index].value, &values[counter]);
                ++counter;
            }
            ++index;
        }
    }

    if (providedValues)
        *providedValues = counter;

    if ((!reverse && (indexByIndex_randomindextest(context,endIndex)-indexByIndex_randomindextest(context,startIndex)-skip+1) > counter)
            || (reverse && (indexByIndex_randomindextest(context,startIndex)-indexByIndex_randomindextest(context,endIndex)-skip+1) > counter)) {
        outContinuationPoint->length = sizeof(size_t);
        size_t t = sizeof(size_t);
        outContinuationPoint->data = (UA_Byte*)UA_malloc(t);
        *((size_t*)(outContinuationPoint->data)) = skip + counter;
    }

    return UA_STATUSCODE_GOOD;
}

static const UA_DataValue*
getDataValue_randomindextest(UA_Server *server,
                             void *hdbContext,
                             const UA_NodeId *sessionId,
                             void *sessionContext,
                             const UA_NodeId *nodeId,
                             size_t index)
{
    struct context_randomindextest* context = (struct context_randomindextest*)hdbContext;
    size_t realIndex = indexByIndex_randomindextest(context, index);
    return &context->tupels[realIndex].value;
}

static UA_Boolean
boundSupported_randomindextest(UA_Server *server,
                               void *hdbContext,
                               const UA_NodeId *sessionId,
                               void *sessionContext,
                               const UA_NodeId *nodeId)
{
    return true;
}

static UA_Boolean
timestampsToReturnSupported_randomindextest(UA_Server *server,
                                            void *hdbContext,
                                            const UA_NodeId *sessionId,
                                            void *sessionContext,
                                            const UA_NodeId *nodeId,
                                            const UA_TimestampsToReturn timestampsToReturn)
{
    return true;
}

UA_HistoryDataBackend
UA_HistoryDataBackend_randomindextest(UA_DateTime *data);

UA_HistoryDataBackend
UA_HistoryDataBackend_randomindextest(UA_DateTime *data)
{
    UA_HistoryDataBackend result;
    memset(&result, 0, sizeof(UA_HistoryDataBackend));
    result.serverSetHistoryData = &serverSetHistoryData_randomindextest;
    result.resultSize = &resultSize_randomindextest;
    result.getEnd = &getEnd_randomindextest;
    result.lastIndex = &lastIndex_randomindextest;
    result.firstIndex = &firstIndex_randomindextest;
    result.getDateTimeMatch = &getDateTimeMatch_randomindextest;
    result.copyDataValues = &copyDataValues_randomindextest;
    result.getDataValue = &getDataValue_randomindextest;
    result.boundSupported = &boundSupported_randomindextest;
    result.timestampsToReturnSupported = &timestampsToReturnSupported_randomindextest;
    result.deleteMembers = &deleteMembers_randomindextest;
    result.getHistoryData = NULL;
    result.context = generateTestContext_randomindextest(data);
    return result;
}

void
UA_HistoryDataBackend_randomindextest_clear(UA_HistoryDataBackend *backend);

void
UA_HistoryDataBackend_randomindextest_clear(UA_HistoryDataBackend *backend)
{
    deleteMembers_randomindextest(backend);
}

#endif // BACKEND_H
