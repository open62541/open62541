/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Combined Batch Read/Write Operations with Async API
 * --------------------------------------------------
 *
 * This example demonstrates how to implement application-side batching for both
 * read and write operations using the open62541 async API. It combines the
 * functionality of both batch read and batch write examples in a single server.
 *
 * Key features:
 * - Configurable batch timeout (default: 20ms)
 * - Configurable max batch size for both read and write
 * - Separate batch contexts for read and write operations
 * - Automatic timer reset on each operation
 * - Batch processing of accumulated requests
 * - Support for both read and write operations on the same variables
 */

#include <open62541/server.h>
#include <open62541/plugin/log.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/accesscontrol_default.h>

#include <stdlib.h>
#include <string.h>

/* Batch configuration */
#define BATCH_TIMEOUT_MS 20
#define MAX_BATCH_SIZE 10

/* Request structure for both read and write */
typedef struct {
    UA_DataValue *dataValue;  /* For both read and write operations */
    UA_NodeId nodeId;
    UA_UInt32 requestId;
    UA_Boolean isWrite;  /* true for write, false for read */
} BatchRequest;

/* Batch context structure */
typedef struct {
    UA_Server *server;
    UA_DateTime nextBatchTime;
    size_t pendingCount;
    BatchRequest *pendingRequests;
    UA_Boolean timerActive;
} BatchContext;

/* Separate contexts for read and write operations */
static BatchContext readBatchCtx = {0};
static BatchContext writeBatchCtx = {0};



/* Initialize batch context */
static void
initBatchContext(BatchContext *ctx, UA_Server *server) {
    ctx->server = server;
    ctx->pendingCount = 0;
    ctx->timerActive = false;
    ctx->pendingRequests = malloc(MAX_BATCH_SIZE * sizeof(BatchRequest));
}

/* Cleanup batch context */
static void
cleanupBatchContext(BatchContext *ctx) {
    if (ctx->pendingRequests) {
        free(ctx->pendingRequests);
        ctx->pendingRequests = NULL;
    }
}

/* Process batch of read requests */
static void
processReadBatch(UA_Server *server, void *data) {
    (void)data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Processing batch of %zu read requests", readBatchCtx.pendingCount);
    
    /* Process all pending read requests */
    for (size_t i = 0; i < readBatchCtx.pendingCount; i++) {
        BatchRequest *req = &readBatchCtx.pendingRequests[i];
        
        /* Extract variable number from name (e.g., "variable-1" -> 1) */
        const char *name = (const char*)req->nodeId.identifier.string.data;
        UA_UInt32 value = 0;
        
        if (strncmp(name, "variable-", 9) == 0) {
            value = (UA_UInt32)(name[9] - '0'); /* Extract the digit */
        } else {
            value = 999; /* Default value for unknown variables */
        }
        
        /* Set the value in the data value */
        UA_Variant_setScalarCopy(&req->dataValue->value, 
                                &value, &UA_TYPES[UA_TYPES_UINT32]);
        req->dataValue->hasValue = true;
        
        /* Complete the async read operation */
        UA_Server_setAsyncReadResult(server, req->dataValue);
    }
    
    /* Reset batch context */
    readBatchCtx.pendingCount = 0;
    readBatchCtx.timerActive = false;
}

/* Process batch of write requests */
static void
processWriteBatch(UA_Server *server, void *data) {
    (void)data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Processing batch of %zu write requests", writeBatchCtx.pendingCount);
    
    /* Process all pending write requests */
    for (size_t i = 0; i < writeBatchCtx.pendingCount; i++) {
        BatchRequest *req = &writeBatchCtx.pendingRequests[i];
        
        /* Log the write operation */
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Processing write to node %s", 
                    req->nodeId.identifier.string.data);
        
        /* In a real application, you would process the write here */
        /* For this example, we just complete the async operation */
        UA_Server_setAsyncWriteResult(server, req->dataValue, UA_STATUSCODE_GOOD);
    }
    
    /* Reset batch context */
    writeBatchCtx.pendingCount = 0;
    writeBatchCtx.timerActive = false;
}

/* Schedule batch processing */
static void
scheduleBatchProcessing(BatchContext *ctx, UA_Server *server, 
                       void (*processFunc)(UA_Server*, void*)) {
    if (ctx->timerActive) {
        return; /* Timer already active */
    }
    
    /* Calculate next batch time */
    ctx->nextBatchTime = UA_DateTime_nowMonotonic() + 
                        (BATCH_TIMEOUT_MS * UA_DATETIME_MSEC);
    
    /* Add timed callback for batch processing */
    UA_Server_addTimedCallback(server, processFunc, NULL, 
                              ctx->nextBatchTime, NULL);
    ctx->timerActive = true;
}

/* Read callback for variables */
static UA_StatusCode
readCallback_async(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeId,
                   void *nodeContext, UA_Boolean includeSourceTimeStamp,
                   const UA_NumericRange *range, UA_DataValue *value) {

    /* Check if we can add to current read batch */
    if (readBatchCtx.pendingCount >= MAX_BATCH_SIZE) {
        /* Process current batch immediately */
        processReadBatch(server, NULL);
    }
    
    /* Add read request to pending batch */
    BatchRequest *req = &readBatchCtx.pendingRequests[readBatchCtx.pendingCount];
    req->dataValue = value;
    req->nodeId = *nodeId;
    req->requestId = 0;
    req->isWrite = false;
    readBatchCtx.pendingCount++;
    
    /* Schedule batch processing if not already scheduled */
    if (!readBatchCtx.timerActive) {
        scheduleBatchProcessing(&readBatchCtx, server, processReadBatch);
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Read request queued for batching (total pending: %zu)", 
                readBatchCtx.pendingCount);
    
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

/* Write callback for variables */
static UA_StatusCode
writeCallback_async(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *nodeId,
                    void *nodeContext, const UA_NumericRange *range,
                    const UA_DataValue *dataValue) {

    
    /* Check if we can add to current write batch */
    if (writeBatchCtx.pendingCount >= MAX_BATCH_SIZE) {
        /* Process current batch immediately */
        processWriteBatch(server, NULL);
    }
    
    /* Add write request to pending batch */
    BatchRequest *req = &writeBatchCtx.pendingRequests[writeBatchCtx.pendingCount];
    req->dataValue = (UA_DataValue*)dataValue;
    req->nodeId = *nodeId;
    req->requestId = 0;
    req->isWrite = true;
    writeBatchCtx.pendingCount++;
    
    /* Schedule batch processing if not already scheduled */
    if (!writeBatchCtx.timerActive) {
        scheduleBatchProcessing(&writeBatchCtx, server, processWriteBatch);
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Write request queued for batching (total pending: %zu)", 
                writeBatchCtx.pendingCount);
    
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

/* Add a variable with async read/write callbacks */
static void
addAsyncVariable(UA_Server *server, const char *name, UA_UInt16 namespaceIndex) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    
    /* Set an initial value to avoid type-checking issues */
    UA_UInt32 initialValue = 0;
    UA_Variant_setScalarCopy(&attr.value, &initialValue, &UA_TYPES[UA_TYPES_UINT32]);
    
    UA_NodeId nodeId = UA_NODEID_STRING(namespaceIndex, name);
    UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(namespaceIndex, name);
    
    /* First add as a regular variable to avoid initialization reads */
    UA_StatusCode retval = UA_Server_addVariableNode(server, nodeId,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                    qualifiedName,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                    attr, NULL, NULL);
    
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Failed to add variable %s: %s", name, UA_StatusCode_name(retval));
        return;
    }
    
    /* Then set the data source callbacks */
    UA_DataSource dataSource;
    dataSource.read = readCallback_async;
    dataSource.write = writeCallback_async;
    
    retval = UA_Server_setVariableNode_callbackValueSource(server, nodeId, dataSource);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Failed to set data source for variable %s: %s", name, UA_StatusCode_name(retval));
    }
}

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    
    /* Enable async operations */
    config->asyncOperationTimeout = 5000.0; /* 5 seconds */
    
    /* Initialize batch contexts */
    initBatchContext(&readBatchCtx, server);
    initBatchContext(&writeBatchCtx, server);
    
    /* Add several variables that will use batch reading and writing */
    addAsyncVariable(server, "variable-1", 1);
    addAsyncVariable(server, "variable-2", 1);
    addAsyncVariable(server, "variable-3", 1);
    addAsyncVariable(server, "variable-4", 1);
    addAsyncVariable(server, "variable-5", 1);
    

    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Combined batch server started. Batch timeout: %dms, Max batch size: %d", 
                BATCH_TIMEOUT_MS, MAX_BATCH_SIZE);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Variables available: variable-1 through variable-5");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Both read and write operations are batched separately");
    
    UA_Server_runUntilInterrupt(server);
    
    /* Cleanup */
    cleanupBatchContext(&readBatchCtx);
    cleanupBatchContext(&writeBatchCtx);
    UA_Server_delete(server);
    return 0;
}
