
#include <../src/server/ua_server_internal.h>
#include <filesystem/ua_fileserver_driver.h>
#include <directoryArch/common/fileSystemOperations_common.h>
#include <filesystem/ua_filetypes.h>
#include <open62541/plugin/log_stdout.h>
#include <stdio.h>

#if defined(UA_FILESYSTEM)

// #ifdef UA_ENABLE_FILESYSTEM
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
    char name[folderName.length + 1];
    memcpy(name, folderName.data, folderName.length);
    name[folderName.length] = '\0';
    snprintf(temp, sizeof(temp), "%s%s", fullPath, name);
    strncpy(fullPath, temp, 2048);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "FullPath: %s\n",
                fullPath);

    /* Perform filesystem delete */
    UA_FileDriverContext *driverCtx = NULL;
    FileDirectoryContext *ctx = NULL;

    /* Get the FileDirectoryContext stored on the node */
    UA_StatusCode res = UA_Server_getNodeContext(server, *objectId, (void**)&ctx);
    if(!ctx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDirectoryContext found for node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
    if(!driver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Node has no driver");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Retrieve the driver context */
    res |= UA_Server_getNodeContext(server, driver->base.nodeId, (void**)&driverCtx);

    if(!driverCtx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDriverContext found for driver node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    // Implementation to create a directory in the filesystem
    res |= driverCtx->makeDirectory(fullPath);
    UA_free(driverCtx);
    if (res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Failed to create directory: %s",
                     fullPath);
        return res;
    }

    output[0].type = &UA_TYPES[UA_TYPES_NODEID];
    output[0].data = UA_NodeId_new();      // SAFE allocation
    UA_NodeId_init((UA_NodeId*)output[0].data);

    UA_FileServerDriver_addFileDirectory(NULL, server, objectId, (const char*)folderName.data, (UA_NodeId*)output[0].data, NULL);

    return res;
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
    int written = snprintf(temp, sizeof(temp), "%s%s", fullPath, name);
    if(written < 0 || written >= (int)sizeof(temp)) {
        return UA_STATUSCODE_BADOUTOFMEMORY;

    }

    strncpy(fullPath, temp, 2048);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                "FullPath: %s\n",
                fullPath);


    
    /* Perform filesystem delete */
    UA_FileDriverContext *driverCtx = NULL;
    FileDirectoryContext *ctx = NULL;

    /* Get the FileDirectoryContext stored on the node */
    UA_StatusCode res = UA_Server_getNodeContext(server, *objectId, (void**)&ctx);
    if(!ctx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDirectoryContext found for node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
    if(!driver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Node has no driver");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Retrieve the driver context */
    res |= UA_Server_getNodeContext(server, driver->base.nodeId, (void**)&driverCtx);

    if(!driverCtx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDriverContext found for driver node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    // Implementation to create a directory in the filesystem
    res = driverCtx->makeFile(fullPath, fileHandleBool, (UA_Int32*)output); // TODO: implement driver dependant file handling
    UA_free(driverCtx);
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

/* Recursively delete all child nodes of a given node */
static UA_StatusCode
deleteSubtree(UA_Server *server, const UA_NodeId *nodeId, bool deleteFiles) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *nodeId;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    if(br.statusCode != UA_STATUSCODE_GOOD)
        return br.statusCode;

    /* Iterate over all references */
    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_ReferenceDescription *ref = &br.references[i];

        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, 
                     "Found Node %d, %d", &ref->referenceTypeId.identifier.numeric, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES).identifier.numeric);

        /* Only follow Organizes */
        UA_NodeId org = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        if(!UA_NodeId_equal(&ref->referenceTypeId, &org))
            continue;

        /* Only follow nodes in our namespace */
        if(ref->nodeId.nodeId.namespaceIndex != nodeId->namespaceIndex)
            continue;

        /* Only follow FileType or DirectoryType */
        UA_NodeId ft = UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE);
        UA_NodeId dt = UA_NODEID_NUMERIC(0, UA_NS0ID_FILEDIRECTORYTYPE);
        if(!UA_NodeId_equal(&ref->typeDefinition.nodeId, &ft) &&
           !UA_NodeId_equal(&ref->typeDefinition.nodeId, &dt))
            continue;

        UA_NodeId childId = UA_NODEID_NULL;
        UA_NodeId_copy(&ref->nodeId.nodeId, &childId);

        /* Recursively delete children first */
        deleteSubtree(server, &childId, deleteFiles);

        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER, 
                     "Deleting Node %d", childId.identifier.numeric);

        char fullPath[2048];
        getFullPath(server, &childId, fullPath, 2048);
        fullPath[strlen(fullPath)-1] = '\0';

        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                    "FullPath for deletion: %s", fullPath);

        if (deleteFiles) {
            
            /* Perform filesystem delete */
            UA_FileDriverContext *driverCtx = NULL;
            FileDirectoryContext *ctx = NULL;

            /* Get the FileDirectoryContext stored on the node */
            UA_StatusCode res = UA_Server_getNodeContext(server, *nodeId, (void**)&ctx);
            if(!ctx) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                            "No FileDirectoryContext found for node");
                res |= UA_STATUSCODE_BADINTERNALERROR;
            }

            UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
            if(!driver) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                            "Node has no driver");
                res |= UA_STATUSCODE_BADINTERNALERROR;
            }

            /* Retrieve the driver context */
            res |= UA_Server_getNodeContext(server, driver->base.nodeId, (void**)&driverCtx);

            if(!driverCtx) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                            "No FileDriverContext found for driver node");
                res |= UA_STATUSCODE_BADINTERNALERROR;
            }

            /* Perform filesystem delete */
            driverCtx->deleteDirOrFile(fullPath); // TODO: implement driver dependant file handling
            UA_free(driverCtx);
        }

        /* Delete the child node itself */
        FileDirectoryContext *nodeContext;
        UA_Server_getNodeContext(server, childId, (void**)&nodeContext);
        UA_free(nodeContext->path);
        UA_free(nodeContext);
        UA_Server_deleteNode(server, childId, true);
        UA_NodeId_clear(&childId);
    }

    UA_BrowseResult_clear(&br);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
deleteItem(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    /* Validate input count */
    if(inputSize != 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Extract path */
    UA_NodeId *nodeId = (UA_NodeId*)input[0].data;
    if(!nodeId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    deleteSubtree(server, nodeId, true);

    char fullPath[2048];
    getFullPath(server, nodeId, fullPath, 2048);
    fullPath[strlen(fullPath)-1] = '\0';

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                 "FullPath for deletion: %s", fullPath);

    /* Perform filesystem delete */
    UA_FileDriverContext *driverCtx = NULL;
    FileDirectoryContext *ctx = NULL;

    /* Get the FileDirectoryContext stored on the node */
    UA_StatusCode res = UA_Server_getNodeContext(server, *nodeId, (void**)&ctx);
    if(!ctx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDirectoryContext found for node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
    if(!driver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Node has no driver");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Retrieve the driver context */
    res |= UA_Server_getNodeContext(server, driver->base.nodeId, (void**)&driverCtx);

    if(!driverCtx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDriverContext found for driver node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    if (res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                        "Failed to get driver context for deletion");
        return res;
    }

    res = driverCtx->deleteDirOrFile(fullPath);
    UA_free(driverCtx);
    if (res == UA_STATUSCODE_GOOD) {
        res = UA_Server_deleteNode(server, *nodeId, true);           
        UA_NodeId_clear(nodeId);
    }

    return res;
}

static UA_StatusCode
moveOrCopy(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
                    
    /* Validate input count */
    if(inputSize != 4 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADTYPEMISMATCH;
        
    /* Validate input count */
    if(!UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_NODEID]))
        return UA_STATUSCODE_BADTYPEMISMATCH;
        
    /* Validate input count */
    if(!UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_BOOLEAN]))
        return UA_STATUSCODE_BADTYPEMISMATCH;
        
    /* Validate input count */
    if(!UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_STRING]))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId *nodeIdSrc = (UA_NodeId*)input[0].data;
    UA_NodeId *nodeIdDst = (UA_NodeId*)input[1].data;
    UA_Boolean *createCopy = (UA_Boolean*)input[2].data;
    UA_String *newName = (UA_String*)input[3].data;

    char srcPath[MAX_PATH], dstPath[MAX_PATH], temp[MAX_PATH];
    getFullPath(server, nodeIdSrc, srcPath, MAX_PATH);
    srcPath[strlen(srcPath)-1] = '\0';

    getFullPath(server, nodeIdDst, temp, MAX_PATH);
    char name[MAX_PATH];
    int written = snprintf(name, sizeof(name), "%.*s",
            (int)newName->length,
            newName->data);
    if(written < 0 || written >= (int)sizeof(name)) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    char tmp[MAX_PATH];
    strncpy(tmp, temp, sizeof(tmp));

    written = snprintf(temp, sizeof(temp), "%s%s", tmp, name);
    if(written < 0 || written >= (int)sizeof(temp)) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    strncpy(dstPath, temp, MAX_PATH);

    /* Perform filesystem delete */
    UA_FileDriverContext *driverCtx = NULL;
    FileDirectoryContext *ctx = NULL;

    /* Get the FileDirectoryContext stored on the node */
    UA_StatusCode res = UA_Server_getNodeContext(server, *objectId, (void**)&ctx);
    if(!ctx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDirectoryContext found for node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_FileServerDriver *driver = (UA_FileServerDriver*)ctx->driver;
    if(!driver) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "Node has no driver");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Retrieve the driver context */
    res |= UA_Server_getNodeContext(server, driver->base.nodeId, (void**)&driverCtx);

    if(!driverCtx) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_DRIVER,
                     "No FileDriverContext found for driver node");
        res |= UA_STATUSCODE_BADINTERNALERROR;
    }

    bool isDir = driverCtx->isDirectory(srcPath);
    res = driverCtx->moveOrCopyItem(srcPath, dstPath, *createCopy);
    UA_free(driverCtx);
    if (res == UA_STATUSCODE_GOOD) {
        if (!*createCopy) {
            res |= deleteSubtree(server, nodeIdSrc, false);
            if (res != UA_STATUSCODE_GOOD)
                return res;

            res |= UA_Server_deleteNode(server, *nodeIdSrc, true);           
            UA_NodeId_clear(nodeIdSrc);
        }
        if (isDir) {
            res |= UA_FileServerDriver_addFileDirectory(NULL, server, nodeIdDst, name, (UA_NodeId*)output, dstPath);
        } else {
            res |= UA_FileServerDriver_addFile(server, nodeIdDst, name, (UA_NodeId*)output);
        }
    }

    return res;
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
    UA_StatusCode res = deleteItem(server, sessionId, sessionHandle,
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
__directoryOperation(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output,
                  DirectoryOperationType opType) {
    // This function can be used to implement thread-safe wrappers for directory operations if needed
    return UA_STATUSCODE_GOOD;
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

#endif // UA_FILESYSTEM