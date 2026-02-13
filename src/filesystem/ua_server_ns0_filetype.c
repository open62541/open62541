/* FileType Method Implementations
 * Based on FILETYPE_IMPLEMENTATION_GUIDE.md
 * Refactored to use session-based context per reviewer feedback
 */
#if defined(UA_FILESYSTEM)

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
    char fullPath[MAX_PATH];
    getFullPath(server, objectId, fullPath, MAX_PATH);
    fullPath[strlen(fullPath)-1] = '\0';
    FILE *handle = NULL;
    UA_StatusCode res = openFile(fullPath, openMode, &handle);
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

/* Read method - uses sessionId for per-client context */
static UA_StatusCode
readFileMethod(UA_Server *server,
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
    
    /* Check read permission (Bit 0) */
    if (!(sessionCtx->fileHandle.openMode & 0x01)) {
        return UA_STATUSCODE_BADNOTREADABLE;
    }
    
    /* Get length from input (UA_Int32) */
    UA_Int32 length = *(UA_Int32*)input[0].data;
    
    /* Read from file */
    UA_ByteString data;
    UA_ByteString_init(&data);
    
    UA_StatusCode res = readFile(sessionCtx->fileHandle.handle, length, &data);
    if (res != UA_STATUSCODE_GOOD) {
        return res;
    }
    
    /* Update position */
    sessionCtx->fileHandle.position += data.length;
    
    /* Return data */
    UA_Variant_setScalarCopy(&output[0], &data, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_clear(&data);
    
    return UA_STATUSCODE_GOOD;
}

/* Write method - uses sessionId for per-client context */
static UA_StatusCode
writeFileMethod(UA_Server *server,
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
    
    /* Check write permission (Bit 1) */
    if (!(sessionCtx->fileHandle.openMode & 0x02)) {
        return UA_STATUSCODE_BADNOTWRITABLE;
    }
    
    /* Get data from input (UA_ByteString) */
    UA_ByteString *data = (UA_ByteString*)input[0].data;
    
    /* Write to file */
    UA_StatusCode res = writeFile(sessionCtx->fileHandle.handle, data);
    if (res != UA_STATUSCODE_GOOD) {
        return res;
    }
    
    /* Update position */
    sessionCtx->fileHandle.position += data->length;
    
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

/* Thread-safe wrapper for Read */
static UA_StatusCode
readFileAction(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = readFileMethod(server, sessionId, sessionContext,
                                 methodId, methodContext,
                                 objectId, objectContext,
                                 inputSize, input,
                                 outputSize, output);
    unlockServer(server);
    return res;
}

/* Thread-safe wrapper for Write */
static UA_StatusCode
writeFileAction(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = writeFileMethod(server, sessionId, sessionContext,
                                  methodId, methodContext,
                                  objectId, objectContext,
                                  inputSize, input,
                                  outputSize, output);
    unlockServer(server);
    return res;
}

/* GetPosition method - uses sessionId for per-client context */
static UA_StatusCode
getPositionMethod(UA_Server *server,
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
    
    /* Return current position */
    UA_UInt64 position = sessionCtx->fileHandle.position;
    UA_Variant_setScalarCopy(&output[0], &position, &UA_TYPES[UA_TYPES_UINT64]);
    
    return UA_STATUSCODE_GOOD;
}

/* SetPosition method - uses sessionId for per-client context */
static UA_StatusCode
setPositionMethod(UA_Server *server,
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
    
    /* Get position from input (UA_UInt64) */
    UA_UInt64 position = *(UA_UInt64*)input[0].data;
    
    /* Seek to position */
    UA_StatusCode res = seekFile(sessionCtx->fileHandle.handle, position);
    if (res != UA_STATUSCODE_GOOD) {
        return res;
    }
    
    /* Update position in context */
    sessionCtx->fileHandle.position = position;
    
    return UA_STATUSCODE_GOOD;
}

/* Thread-safe wrapper for GetPosition */
static UA_StatusCode
getPositionAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = getPositionMethod(server, sessionId, sessionContext,
                                    methodId, methodContext,
                                    objectId, objectContext,
                                    inputSize, input,
                                    outputSize, output);
    unlockServer(server);
    return res;
}

/* Thread-safe wrapper for SetPosition */
static UA_StatusCode
setPositionAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = setPositionMethod(server, sessionId, sessionContext,
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
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ),
        (UA_MethodCallback)readFileAction);
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE),
        (UA_MethodCallback)writeFileAction);
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION),
        (UA_MethodCallback)getPositionAction);
    
    retval |= UA_Server_setMethodNodeCallback(
        fileDriver->base.context->server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION),
        (UA_MethodCallback)setPositionAction);
    
    return retval;
}

#endif /* UA_FILESYSTEM */
