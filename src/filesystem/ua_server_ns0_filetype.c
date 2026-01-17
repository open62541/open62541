/* FileType Method Implementations
 * Based on FILETYPE_IMPLEMENTATION_GUIDE.md
 */

#include <../src/server/ua_server_internal.h>
#include <open62541/driver/ua_fileserver_driver.h>
#include <../arch/common/fileSystemOperations_common.h>
#include <filesystem/ua_filetypes.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Open method - exactly as shown in guide */
static UA_StatusCode
openFileMethod(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {
    
    // Get file context
    FileContext *fileCtx = NULL;
    UA_Server_getNodeContext(server, *objectId, (void**)&fileCtx);
    
    if (!fileCtx) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    // Check if already open
    if (fileCtx->fileHandle != NULL) {
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    
    // Get writable flag from input
    UA_Boolean writable = *(UA_Boolean*)input[0].data;
    
    // Open file
    FileHandle handle;
    UA_StatusCode res = openFile(fileCtx->path, writable, &handle);
    if (res != UA_STATUSCODE_GOOD) {
        return res;
    }
    
    // Store handle in context
    fileCtx->fileHandle = handle.handle;
    fileCtx->currentPosition = 0;
    fileCtx->writable = writable;
    
    // Return file handle as output
    output[0].type = &UA_TYPES[UA_TYPES_UINT32];
    output[0].data = UA_UInt32_new();
    *(UA_UInt32*)output[0].data = (UA_UInt32)(uintptr_t)fileCtx->fileHandle;
    
    return UA_STATUSCODE_GOOD;
}

/* Close method - exactly as shown in guide */
static UA_StatusCode
closeFileMethod(UA_Server *server,
          const UA_NodeId *sessionId, void *sessionHandle,
          const UA_NodeId *methodId, void *methodContext,
          const UA_NodeId *objectId, void *objectContext,
          size_t inputSize, const UA_Variant *input,
          size_t outputSize, UA_Variant *output) {
    
    FileContext *fileCtx = NULL;
    UA_Server_getNodeContext(server, *objectId, (void**)&fileCtx);
    
    if (!fileCtx || !fileCtx->fileHandle) {
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    
    // Close file
    FileHandle handle;
    handle.handle = fileCtx->fileHandle;
    closeFile(&handle);
    
    // Clear context
    fileCtx->fileHandle = NULL;
    fileCtx->currentPosition = 0;
    
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
