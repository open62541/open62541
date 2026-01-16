
#include <../src/server/ua_server_internal.h>
#include <open62541/driver/ua_fileserver_driver.h>
#include <../arch/common/fileSystemOperations_common.h>
#include <filesystem/ua_filetypes.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>

// #ifdef UA_ENABLE_FILESYSTEM

static UA_StatusCode
getFullPath(UA_Server *server,
            const UA_NodeId *startNode,
            char *buffer,
            size_t bufferSize)
{
    buffer[0] = '\0';

    UA_NodeId current = *startNode;

    while(true) {
        /* Get the context of the current node */
        FileDirectoryContext *ctx = NULL;
        UA_Server_getNodeContext(server, current, (void**)&ctx);

        if(!ctx || !ctx->path) {
            /* No context means we reached the root */
            break;
        }

        /* Prepend this path segment */
        char temp[2048];
        snprintf(temp, sizeof(temp), "%s/%s", ctx->path, buffer);
        strncpy(buffer, temp, bufferSize);

        /* Browse inverse to find the parent */
        UA_BrowseResult br = UA_Server_browse(server, 10,
            &(UA_BrowseDescription){
                .nodeId = current,
                .browseDirection = UA_BROWSEDIRECTION_INVERSE,
                .includeSubtypes = true,
                .referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                .nodeClassMask = UA_NODECLASS_OBJECT
            });

        if(br.referencesSize == 0) {
            UA_BrowseResult_clear(&br);
            break;
        }

        /* Move to parent */
        current = br.references[0].nodeId.nodeId;

        UA_BrowseResult_clear(&br);
    }

    return UA_STATUSCODE_GOOD;
}



static UA_StatusCode
createDirectory(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    if (!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING])) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Input argument is not of type String");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    UA_String folderName = *(UA_String*)input[0].data;

    char fullPath[2048];
    getFullPath(server, objectId, fullPath, 2048);

    char temp[2048];
    snprintf(temp, sizeof(temp), "%s%s", fullPath, (const char*)folderName.data);
    strncpy(fullPath, temp, 2048);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "FullPath: %s\n",
                fullPath);

    // Implementation to create a directory in the filesystem
    UA_StatusCode res = makeDirectory(fullPath);
    if (res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Failed to create directory: %s",
                     fullPath);
        return res;
    }

    output[0].type = &UA_TYPES[UA_TYPES_NODEID];
    output[0].data = UA_NodeId_new();      // SAFE allocation
    UA_NodeId_init((UA_NodeId*)output[0].data);

    UA_FileServerDriver_addFileDirectory(NULL, server, objectId, (const char*)folderName.data, (UA_NodeId*)output[0].data);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createFile(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    if (!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING])) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Input argument 0 is not of type String");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    if (!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Input argument 1 is not of type Boolean");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_String fileName = *(UA_String*)input[0].data;
    UA_Boolean fileHandleBool = *(UA_Boolean*)input[1].data;

    char fullPath[2048];
    getFullPath(server, objectId, fullPath, 2048);

    char name[512];
    snprintf(name, sizeof(name), "%.*s",
            (int)fileName.length,
            fileName.data);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "FileName: %s\n",
                name);

    char temp[2048];
    snprintf(temp, sizeof(temp), "%s%s", fullPath, name);
    strncpy(fullPath, temp, 2048);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "FullPath: %s\n",
                fullPath);

    // Implementation to create a directory in the filesystem
    UA_StatusCode res = makeFile(fullPath);
    if (res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Failed to create file: %s",
                     fullPath);
        return res;
    }

    output[0].type = &UA_TYPES[UA_TYPES_NODEID];
    output[0].data = UA_NodeId_new();      // SAFE allocation
    UA_NodeId_init((UA_NodeId*)output[0].data);

    UA_FileServerDriver_addFile(server, objectId, name, (UA_NodeId*)output[0].data);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
deleteDirectory(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    // Implementation to remove a directory from the filesystem
    // This is a placeholder and should contain actual filesystem operations
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
moveOrCopy(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    // Implementation to move or copy a file/directory in the filesystem
    // This is a placeholder and should contain actual filesystem operations
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
createDirectoryAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = createDirectory(server, sessionId, sessionHandle,
                           methodId, methodContext,
                           objectId, objectContext,
                           inputSize, input,
                           outputSize, output);
    unlockServer(server);
    return res;
}

static UA_StatusCode
createFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = createFile(server, sessionId, sessionHandle,
                           methodId, methodContext,
                           objectId, objectContext,
                           inputSize, input,
                           outputSize, output);
    unlockServer(server);
    return res;
}

static UA_StatusCode
deleteAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = deleteDirectory(server, sessionId, sessionHandle,
                           methodId, methodContext,
                           objectId, objectContext,
                           inputSize, input,
                           outputSize, output);
    unlockServer(server);
    return res;
}

static UA_StatusCode
moveOrCopyAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    lockServer(server);
    UA_StatusCode res = moveOrCopy(server, sessionId, sessionHandle,
                           methodId, methodContext,
                           objectId, objectContext,
                           inputSize, input,
                           outputSize, output);
    unlockServer(server);
    return res;
}

UA_StatusCode
initFileSystemManagement(UA_FileServerDriver *fileDriver) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    retval |= UA_Server_setMethodNodeCallback(fileDriver->base.context->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEDIRECTORY), (UA_MethodCallback)createDirectoryAction);

    retval |= UA_Server_setMethodNodeCallback(fileDriver->base.context->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_CREATEFILE), (UA_MethodCallback)createFileAction);

    retval |= UA_Server_setMethodNodeCallback(fileDriver->base.context->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_DELETEFILESYSTEMOBJECT), (UA_MethodCallback)deleteAction);

    retval |= UA_Server_setMethodNodeCallback(fileDriver->base.context->server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE_MOVEORCOPY), (UA_MethodCallback)moveOrCopyAction);

    return retval;
}

// #endif // UA_ENABLE_FILESYSTEM