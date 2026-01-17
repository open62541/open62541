/* FileType Method Implementations
 * Based on FILETYPE_IMPLEMENTATION_GUIDE.md
 * Refactored to use session-based context per reviewer feedback
 */

#include <../src/server/ua_server_internal.h>
#include <open62541/driver/ua_fileserver_driver.h>
#include <../arch/common/fileSystemOperations_common.h>
#include <filesystem/ua_filetypes.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper: Find session context for a given sessionId */
static FileSessionContext*
findSessionContext(FileContext *fileCtx, const UA_NodeId *sessionId) {
    FileSessionContext *current = fileCtx->sessions;
    while (current) {
        if (UA_NodeId_equal(&current->sessionId, sessionId)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Helper: Create new session context */
static FileSessionContext*
createSessionContext(FileContext *fileCtx, const UA_NodeId *sessionId) {
    FileSessionContext *sessionCtx = (FileSessionContext*)UA_calloc(1, sizeof(FileSessionContext));
    if (!sessionCtx) return NULL;
    
    UA_NodeId_copy(sessionId, &sessionCtx->sessionId);
    sessionCtx->fileHandle.handle = NULL;
    sessionCtx->fileHandle.position = 0;
    sessionCtx->fileHandle.openMode = 0;
    sessionCtx->next = fileCtx->sessions;
    fileCtx->sessions = sessionCtx;
    
    return sessionCtx;
}

/* Helper: Remove session context */
static void
removeSessionContext(FileContext *fileCtx, const UA_NodeId *sessionId) {
    FileSessionContext **current = &fileCtx->sessions;
    while (*current) {
        if (UA_NodeId_equal(&(*current)->sessionId, sessionId)) {
            FileSessionContext *toRemove = *current;
            *current = toRemove->next;
            UA_NodeId_clear(&toRemove->sessionId);
            UA_free(toRemove);
            return;
        }
        current = &(*current)->next;
    }
}

/* Open method - uses sessionId for per-client context */
static UA_StatusCode
openFileMethod(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {
    
    /* Get file context from node */
    FileContext *fileCtx = NULL;
    UA_Server_getNodeContext(server, *objectId, (void**)&fileCtx);
    
    if (!fileCtx) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Check if this session already has the file open */
    FileSessionContext *sessionCtx = findSessionContext(fileCtx, sessionId);
    if (sessionCtx && sessionCtx->fileHandle.handle != NULL) {
        return UA_STATUSCODE_BADINVALIDSTATE;  /* Already open for this session */
    }
    
    /* Get openMode from input (UA_Byte) */
    UA_Byte openMode = *(UA_Byte*)input[0].data;
    
    /* Open file */
    FILE *handle = NULL;
    UA_StatusCode res = openFile(fileCtx->path, openMode, &handle);
    if (res != UA_STATUSCODE_GOOD) {
        return res;
    }
    
    /* Create or get session context */
    if (!sessionCtx) {
        sessionCtx = createSessionContext(fileCtx, sessionId);
        if (!sessionCtx) {
            closeFile(handle);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    
    /* Store handle in session context */
    sessionCtx->fileHandle.handle = handle;
    sessionCtx->fileHandle.position = 0;
    sessionCtx->fileHandle.openMode = openMode;
    
    /* Return file handle as output (use pointer as unique handle ID) */
    output[0].type = &UA_TYPES[UA_TYPES_UINT32];
    output[0].data = UA_UInt32_new();
    *(UA_UInt32*)output[0].data = (UA_UInt32)(uintptr_t)handle;
    
    return UA_STATUSCODE_GOOD;
}

/* Close method - uses sessionId for per-client context */
static UA_StatusCode
closeFileMethod(UA_Server *server,
          const UA_NodeId *sessionId, void *sessionHandle,
          const UA_NodeId *methodId, void *methodContext,
          const UA_NodeId *objectId, void *objectContext,
          size_t inputSize, const UA_Variant *input,
          size_t outputSize, UA_Variant *output) {
    
    /* Get file context from node */
    FileContext *fileCtx = NULL;
    UA_Server_getNodeContext(server, *objectId, (void**)&fileCtx);
    
    if (!fileCtx) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Find session context for this client */
    FileSessionContext *sessionCtx = findSessionContext(fileCtx, sessionId);
    if (!sessionCtx || !sessionCtx->fileHandle.handle) {
        return UA_STATUSCODE_BADINVALIDSTATE;  /* Not open for this session */
    }
    
    /* Close file */
    closeFile(sessionCtx->fileHandle.handle);
    
    /* Remove session context */
    removeSessionContext(fileCtx, sessionId);
    
    return UA_STATUSCODE_GOOD;
}


/* Thread-safe wrapper for Open */
static UA_StatusCode
openFileAction(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = openFileMethod(server, sessionId, sessionContext,
                                 methodId, methodContext,
                                 objectId, objectContext,
                                 inputSize, input,
                                 outputSize, output);
    unlockServer(server);
    return res;
}

/* Thread-safe wrapper for Close */
static UA_StatusCode
closeFileAction(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = closeFileMethod(server, sessionId, sessionContext,
                                  methodId, methodContext,
                                  objectId, objectContext,
                                  inputSize, input,
                                  outputSize, output);
    unlockServer(server);
    return res;
}

/* Method registration - as shown in guide */
UA_StatusCode
initFileTypeOperations(UA_FileServerDriver *fileDriver) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN),
        (UA_MethodCallback)openFileAction);
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE),
        (UA_MethodCallback)closeFileAction);
    
    return retval;
}
